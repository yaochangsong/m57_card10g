/******************************************************************************
*  Copyright 2019, Showay Technology Dev Co.,Ltd.
*  ---------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Showay Technology Dev Co.,Ltd. (C) 2019
******************************************************************************/
/*****************************************************************************     
*  Rev 1.0   21 Feb. 2020   yaochangsong
*  Initial revision.
******************************************************************************/
#include <math.h>
#include "config.h"
#include "../spm.h"
#include "spm_chip.h"
#include "../../../protocol/resetful/data_frame.h"

#ifdef CONFIG_BSP_SSA_MONITOR
static int64_t division_point_array[] = {GHZ(7.5)};

int64_t *get_division_point_array(int ch, int *array_len)
{
    *array_len = ARRAY_SIZE(division_point_array);
    return division_point_array;
}
#endif

struct _vec_fft{
    fft_t *data;
    size_t len;
};

/* Allocate zeroed out memory */
static inline void *zalloc(size_t size)
{
	return calloc(1, size);
}

static int spm_chip_create(void)
{
#ifdef CONFIG_SPM_FFT_IQ_CONVERT
    fft_init();
    /* set default smooth count */
    fft_set_smoothcount(2);
#endif
#ifdef CONFIG_SPM_FFT_SIGNAL_ANALYSIS
    spm_analysis_init();
#endif
	return 0;
}

volatile int32_t _smooth_time = 0;
static void spm_chip_set_smooth_time(int32_t count)
{
#ifdef CONFIG_SPM_FFT_IQ_CONVERT
    fft_set_smoothcount(count);
#endif
    _smooth_time = count;
}

static inline int32_t spm_chip_get_smooth_time(void)
{
    return _smooth_time;
}


volatile int32_t _calibration_value = 0;
static int32_t spm_set_calibration_value(int32_t value)
{
    _calibration_value = value;
    return _calibration_value;
}

static int32_t spm_get_calibration_value(void)
{
    return _calibration_value;
}


bool _flush_trigger = false;
int spm_chip_set_flush_trigger(bool action)
{
    _flush_trigger = action;
    return _flush_trigger;
}

static int spm_chip_get_flush_trigger(void)
{
    return _flush_trigger;
}

static void spm_fft_calibration(fft_t *data, size_t fft_len)
{
    int32_t cal_value = 0;
    cal_value = spm_get_calibration_value();
    SSDATA_CUT_OFFSET(data, cal_value, fft_len);
}

iq_t *_iqdata_once  = NULL;

static ssize_t spm_chip_read_iq_data(void **data)
{
    ssize_t r = 0;
    iq_t *iqdata = NULL;
    
    iqdata = (iq_t *)specturm_rx0_read_data(&r);
    *data = iqdata;
    return r;
}

static ssize_t spm_chip_read_fft_data(void **data, void *args)
{
    float *floatdata = NULL;
    static fft_t *fftdata = NULL;
    static size_t fft_size_dup = 0;
    iq_t *iqdata = NULL;
    ssize_t r = 0;
    size_t fft_size;
    struct spm_run_parm *run_args;
    
    run_args = (struct spm_run_parm *)args;
    if(run_args == NULL)
        goto err;
    
    fft_size = run_args->fft_size;
    if(fft_size == 0)
        goto err;
    iqdata = (iq_t *)specturm_rx0_read_data(&r);
#ifdef CONFIG_SPM_FFT_SIGNAL_ANALYSIS
    spm_analysis_start(0, iqdata, r, args);
#else
    iqdata = iqdata; /* for warning */
#endif
    floatdata = zalloc(fft_size*4);
    if(floatdata == NULL)
        goto err;
    fft_spectrum_iq_to_fft_handle((short *)iqdata, fft_size, fft_size*2, floatdata);
    if(fft_size_dup < fft_size){
        fft_size_dup = fft_size;
        fftdata = realloc(fftdata, fft_size_dup*4);
        if(fftdata == NULL){
            free(floatdata);
            floatdata = NULL;
            goto err;
        }
    }
    memset(fftdata, 0, fft_size_dup*4);
    FDATA_MUL_OFFSET(floatdata, 10, fft_size);
    FLOAT_TO_SHORT(floatdata, fftdata, fft_size);
    free(floatdata);
    floatdata = NULL;
    *data = fftdata;
    return (fft_size*sizeof(fft_t));
err:
    *data = NULL;
    return -1;
}

static fft_t calc_avg(fft_t *ptr, int count)
{
    int64_t sum = 0;
    fft_t avg = 0;
    for(int i = 0; i< count; i++)
        sum += *ptr++;
    avg = sum / count;
    return avg;
}
static void print_fft(fft_t *ptr, int count)
{
    for(int i = 0; i< count; i++)
        printfn("[%d]%d ",i, *ptr++);
    printfn("\n");
}

static void print_fft_middle(fft_t *ptr, size_t fftsize, int count)
{
    size_t mid_size = fftsize / 2;

    ptr= ptr+mid_size - count/2;
    for(int i = 0; i<count; i++)
        printfi("[%d]%d ",i, *ptr++);
    printfi("\n");
}

static inline ssize_t _vector_avg(struct _vec_fft *vdata, ssize_t y, void **data)
{
    int i, j;
    struct _vec_fft *_vfft = vdata, *_first = NULL;
    uint64_t sum = 0;
    ssize_t x = 0; 
    uint32_t count = 0;

    /* find first not null data */
    _vfft = vdata;
    for(i = 0; i < y; i++){
        if(_vfft->data != NULL){
            _first = _vfft;
            break;
        }
        _vfft++;
    }
    if(_first == NULL)
        return -1;

    /* find min fft size */
    _vfft = vdata;
    x = _first->len;
    for(i = 0; i < y; i++){
        if(_vfft->len > 0)
            x = min(x, _vfft->len);
        _vfft++;
    }
    if(x == 0){
        printf_err("fft size is null\n");
        return -1;
    }

    /* calculate the average value */
    for(j = 0; j < x; j++){
        sum = 0; 
        count = 0;
        _vfft = vdata;
        for(i = 0; i < y; i++){
            if(_vfft->data){
                if(_vfft->data[j] > 0){
                    sum += _vfft->data[j];
                    count++;
                }
            }
            _vfft++;
        }
        /* the average array is stored in the first non empty array */
        if(count > 0)
            _first->data[j] = sum / count;
    }
    *data = _first->data;
    return x;
}



static inline void _shift_add_tail(struct _vec_fft *vdata, size_t array_num, struct _vec_fft *new_node)
{
    int i;
    struct _vec_fft *_vfft = vdata, *_next;
    
    if(array_num <= 0)
        return;
    
    _vfft = vdata;
    for(i = 0; i< array_num - 1; i++){
        if(i == 0){
            safe_free(_vfft->data);
            _vfft->len = 0;
        }
        _next = _vfft+1;
        _vfft->data = _next->data;
        _vfft->len = _next->len;
        _vfft++;
    }
    _vfft->data = new_node->data;
    _vfft->len = new_node->len;
}

static inline ssize_t _read_arrary_fft_data(struct _vec_fft *vdata, void *args, size_t array_len)
{
    ssize_t r, i, j;
    fft_t  *rawdata, *newdata = NULL;
    struct _vec_fft *_vfft = vdata;

    for(i = 0; i < array_len; i++){
        r = spm_chip_read_fft_data((void **)&rawdata, args);
        if(r > 0){
            newdata = zalloc(r);
            if(newdata == NULL)
                return -1;
            _vfft->data = newdata;
            memcpy(_vfft->data, rawdata, r);
            _vfft->len = r/sizeof(fft_t);
            _vfft ++;
        }
        else
            return -1;
    }
    return 0;
}

static ssize_t spm_chip_read_fft_data_smooth(int ch, void **data, void *args)
{
    #define _MAX_SMOOTH_NUM (256)
    #define _MAX_BW_NUM (256)
    #define _MAX_FREGMENT_NUM (10)
    ssize_t smooth = spm_chip_get_smooth_time();
    int i, j, f;
    ssize_t r = -1, fft_byte_size = 0;
    fft_t *data_dup;
    static struct _vec_fft vfft[_MAX_FREGMENT_NUM][_MAX_BW_NUM][_MAX_SMOOTH_NUM] = {NULL};

    static int is_initialized = 0;
    struct _vec_fft *vfft_dup, new_data;
    struct spm_run_parm *r_args;
    r_args = (struct spm_run_parm *)args;
    uint32_t index = r_args->fft_sn;
    uint32_t freg_num = r_args->fregment_num;

    ch = ch;
    if(smooth <= 1){
        r = spm_chip_read_fft_data((void **)&data_dup, args);
        *data = data_dup;
        return r;
    }
    if(is_initialized == 0 || spm_chip_get_flush_trigger()){
        is_initialized = 1;
        spm_chip_set_flush_trigger(false);
        for(f = 0; f < _MAX_FREGMENT_NUM; f++){
            for(i = 0; i < _MAX_BW_NUM; i++){
                for(j = 0; j < _MAX_SMOOTH_NUM; j++){
                    memset(&vfft[f][i][j], 0, sizeof(struct _vec_fft));
                }
            }
        }

        printf_info("smooth buffer flush\n");
    }

    if(index >= _MAX_BW_NUM){
        printf_err("fft sn[%u] is too large!\n", index);
        goto exit;
    }
    if(smooth >= _MAX_SMOOTH_NUM){
        printf_err("[NOT SUPPORT]smooth[%zd] time is bigger than max:%d!\n", smooth, _MAX_SMOOTH_NUM);
        goto exit;
    }
    if(freg_num >= _MAX_FREGMENT_NUM){
        printf_err("[NOT SUPPORT]fregment[%d] is bigger than max:%d!\n", freg_num, _MAX_FREGMENT_NUM);
        freg_num = _MAX_FREGMENT_NUM -1;
    }

    memset(&new_data, 0, sizeof(struct _vec_fft));
    r = _read_arrary_fft_data(&new_data, args, 1);
    if(r < 0)
        goto exit;
    _shift_add_tail(vfft[freg_num][index], smooth, &new_data);
    #if 0
    struct _vec_fft * vfft_tmp = vfft[freg_num][index];
    printf_note("[%u]after add:\n", index);
    for(i = 0; i< smooth; i++){
        if(vfft_tmp->data)
            print_fft(vfft_tmp->data, 16);
        vfft_tmp++;
    }
    #endif

    r = _vector_avg(vfft[freg_num][index], smooth, data);
    if(r < 0)
        goto exit;

    fft_byte_size = r*sizeof(fft_t);
    return fft_byte_size;
    
exit:
    return -1;
}


static  float get_side_band_rate(uint32_t bandwidth)
{
    #define DEFAULT_SIDE_BAND_RATE  (1.28)
    float side_rate = 0.0;
     /* 根据带宽获取边带率 */
    if(config_read_by_cmd(EX_CTRL_CMD, EX_CTRL_SIDEBAND,0, &side_rate, bandwidth) == -1){
        printf_info("!!!!!!!!!!!!!SideRate Is Not Set In Config File[bandwidth=%u]!!!!!!!!!!!!!\n", bandwidth);
        return DEFAULT_SIDE_BAND_RATE;
    }
    return side_rate;
}

static void spm_middle_freq_leakage_calibration(fft_t *data , size_t  fft_len,unsigned int fft_size)
{
    #define LEAKAGE_FFT_POINTS 4
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    size_t half_fft_len = fft_len / 2, renew_data_len = 0, global_copy_points = 0;
    fft_t avg1 = 0, avg2 = 0, avg3 = 0;
    ssize_t threshold = 0, half_fft = 0;
    int i;
    for(i = 0; i< sizeof(poal_config->cal_level.lo_leakage.fft_size)/sizeof(uint32_t); i++){
        if(fft_size == poal_config->cal_level.lo_leakage.fft_size[i]){
            threshold = poal_config->cal_level.lo_leakage.threshold[i];
            renew_data_len = poal_config->cal_level.lo_leakage.renew_data_len[i];
            break;
        }
    }
    threshold += poal_config->cal_level.lo_leakage.global_threshold;
    renew_data_len += poal_config->cal_level.lo_leakage.global_renew_data_len;
    global_copy_points = poal_config->cal_level.lo_leakage.global_copy_points;
    if(renew_data_len == 0)
        renew_data_len = LEAKAGE_FFT_POINTS;
    half_fft = renew_data_len / 2;
    if(global_copy_points == 0)
        global_copy_points = 80;
    avg1 = calc_avg(data + half_fft_len - half_fft, renew_data_len);
    avg2 = calc_avg(data + half_fft_len - global_copy_points, renew_data_len);
    avg3 = calc_avg(data + half_fft_len + global_copy_points, renew_data_len);
    if(avg2 < avg3){
        if((avg1 > avg2) && (avg1 < threshold)){
            memcpy(data + half_fft_len - half_fft, data + half_fft_len - global_copy_points, renew_data_len*sizeof(fft_t));
        }
    } else{
        if((avg1 > avg3) && (avg1 < threshold)){
            memcpy(data + half_fft_len - half_fft, data + half_fft_len + global_copy_points, renew_data_len*sizeof(fft_t));
        }
    }
    printf_info("avg1=%d, avg2=%d, avg3=%d, threshold=%zd, renew_data_len=%zd,global_copy_points=%zd\n", avg1, avg2, avg3, threshold, renew_data_len, global_copy_points);
}

static int _spm_extract_half_point(void *data, int len, void *outbuf)
{
    fft_t *pdata = (fft_t *)data;
    for(int i = 0; i < len; i++){
        *(uint8_t *)outbuf++ = *pdata & 0x0ff;
        pdata++;
    }
    return len/2;
}


/* 频谱数据整理 */
static fft_t *spm_chip_data_order(fft_t *fft_data, 
                                size_t fft_len,  
                                size_t *order_fft_len,
                                void *arg)
{
    struct spm_run_parm *run_args;
    float sigle_side_rate, side_rate;
    uint32_t single_sideband_size;
    size_t order_len = 0;

    fft_t *out_data;
    if(fft_data == NULL || fft_len == 0)
        return NULL;

    run_args = (struct spm_run_parm *)arg;
    /* 1：去边带 */
    /* 获取边带率 */
    side_rate  = get_side_band_rate(run_args->scan_bw);
    sigle_side_rate = (1.0-1.0/side_rate)/2.0;
    single_sideband_size = fft_len*sigle_side_rate;

    order_len = fft_len - 2 * single_sideband_size;
    /*双字节对齐*/
    order_len = order_len&0x0fffffffe; 
    
    /* 2：频谱带宽<射频扫描带宽;去除多余频谱数据 */
    uint64_t start_freq_hz = 0;
    uint32_t offset = 0;
    if(run_args->scan_bw > run_args->bandwidth){
        start_freq_hz = run_args->s_freq_offset - (run_args->m_freq_s - run_args->scan_bw/2);
        offset = ((uint64_t)order_len * start_freq_hz)/(uint64_t)run_args->scan_bw;
        order_len = ((uint64_t)order_len * (uint64_t)run_args->bandwidth)/(uint64_t)run_args->scan_bw;
    }
    *order_fft_len = order_len;
    out_data = fft_data + single_sideband_size + offset;
#if defined(CONFIG_BSP_SSA_MONITOR)
    uint64_t freq_hz = run_args->m_freq_s;
    if (freq_hz >= LVTTL_FREQ_START2 && freq_hz <= LVTTL_FREQ_end3) {
        for (size_t i = 0; i < fft_len; i++) {
            *((fft_t *)run_args->fft_ptr + i) = *(fft_data + fft_len - i -1);
        }
        out_data = (fft_t *)run_args->fft_ptr + single_sideband_size + offset;
    }
#endif
#if defined(CONFIG_SPM_FFT_EXTRACT_POINT)
    order_len =  _spm_extract_half_point(out_data, order_len, run_args->fft_ptr_swap);
    out_data = run_args->fft_ptr_swap;
#endif
    spm_middle_freq_leakage_calibration(out_data, order_len, run_args->fft_size);
    spm_fft_calibration(out_data, order_len);
    
    return out_data;
}

static int spm_chip_send_fft_data(void *data, size_t fft_len, void *arg)
{
    #define HEAD_BUFFER_LEN  512 
    uint8_t *ptr_header = NULL;
    uint32_t header_len = 0;
    size_t data_byte_size = 0;
    struct spm_run_parm *hparam;
    hparam = (struct spm_run_parm *)arg;

    if(data == NULL || fft_len == 0 || arg == NULL)
        return -1;
    
    data_byte_size = fft_len * sizeof(fft_t);
#if (defined CONFIG_PROTOCOL_ACT)
    hparam->data_len = data_byte_size; 
    hparam->type = SPECTRUM_DATUM_FLOAT;
    hparam->ex_type = SPECTRUM_DATUM;
    if((ptr_header = akt_assamble_data_frame_header_data(&header_len, arg)) == NULL){
        return -1;
    }
#elif defined(CONFIG_PROTOCOL_XW)
    hparam->data_len = data_byte_size;
#if defined(CONFIG_SPM_FFT_EXTRACT_POINT)
    hparam->type = DEFH_DTYPE_CHAR;
#else
    hparam->type = DEFH_DTYPE_SHORT;
#endif
    hparam->ex_type = DFH_EX_TYPE_PSD;
    ptr_header = xw_assamble_frame_data(&header_len, arg);
    if(ptr_header == NULL)
        return -1;

#endif
    struct iovec iov[2];

    iov[0].iov_base = ptr_header;
    iov[0].iov_len = header_len;
    iov[1].iov_base = data;
    iov[1].iov_len = data_byte_size;
    int ch = hparam->ch;
    __lock_fft_send__(ch);
#if (defined CONFIG_PROTOCOL_DATA_TCP)
    tcp_send_vec_data(iov, 2, NET_DATA_TYPE_FFT);
#else
    udp_send_vec_data(iov, 2, NET_DATA_TYPE_FFT);
#endif
    __unlock_fft_send__(ch);
    safe_free(ptr_header);
    return (header_len + data_byte_size);

}

/*********************************************************
    功能:组装IQ数据协议头
    输入参数：
        @data_len: IQ数据长度
        @arg: 数据属性指针
    输出参数:
        @hlen: (IQ)数据头长度
    返回:
        NULL: 失败
        非空: 协议头指针; 使用后需要释放
*********************************************************/
static void *_assamble_iq_header(int ch, int subch, size_t *hlen, size_t data_len, void *arg, enum stream_iq_type type)
{
    uint8_t *ptr = NULL, *ptr_header = NULL;
    uint32_t header_len = 0;
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    struct sub_channel_freq_para_st *sub_channel_array;
    
    if(data_len == 0 || arg == NULL)
        return NULL;

    struct spm_run_parm *hparam;
    hparam = (struct spm_run_parm *)arg;
#ifdef CONFIG_PROTOCOL_ACT
    hparam->data_len = data_len;
    hparam->type = BASEBAND_DATUM_IQ;
    hparam->ex_type = DEMODULATE_DATUM;
    ch = hparam->ch;
    if(type == STREAM_NIQ_TYPE_AUDIO){
        int i;
        struct multi_freq_point_para_st  *points = &poal_config->channel[ch].multi_freq_point_param;
        i = executor_get_audio_point(ch);
        hparam->sub_ch_para.bandwidth_hz = points->points[i].d_bandwith;
        hparam->sub_ch_para.m_freq_hz =  points->points[i].center_freq;
        hparam->sub_ch_para.d_method = points->points[i].raw_d_method;
        hparam->sub_ch_para.sample_rate = 32000;    /* audio 32Khz */
    }else if(type == STREAM_NIQ_TYPE_RAW){
        float side_rate = 0.0;
        sub_channel_array = &poal_config->channel[ch].sub_channel_para;
        hparam->sub_ch_para.bandwidth_hz = sub_channel_array->sub_ch[subch].d_bandwith;
        hparam->sub_ch_para.m_freq_hz = sub_channel_array->sub_ch[subch].center_freq;
        hparam->sub_ch_para.d_method = sub_channel_array->sub_ch[subch].raw_d_method;
        config_read_by_cmd(EX_CTRL_CMD, EX_CTRL_SIDEBAND,ch, &side_rate, hparam->bandwidth);
        hparam->sub_ch_para.sample_rate = side_rate * hparam->sub_ch_para.bandwidth_hz; 
    }
   
    printf_debug("ch=%d, subch = %d, m_freq_hz=%"PRIu64", bandwidth:%uhz, sample_rate=%u, d_method=%d\n", 
        ch,
        subch,
        hparam->sub_ch_para.m_freq_hz,
        hparam->sub_ch_para.bandwidth_hz, 
        hparam->sub_ch_para.sample_rate,  
        hparam->sub_ch_para.d_method );
    
    if((ptr_header = akt_assamble_data_frame_header_data(&header_len, arg))== NULL){
        return NULL;
    }
    
#elif defined(CONFIG_PROTOCOL_XW)
    hparam->data_len = data_len; 
    if(type == STREAM_BIQ_TYPE_RAW){
        printf_debug("ch=%d,middle_freq=%"PRIu64",bandwidth=%"PRIu64"\n",ch,poal_config->channel[ch].multi_freq_point_param.ddc.middle_freq,
                        poal_config->channel[ch].multi_freq_point_param.ddc.bandwidth);
        hparam->type = DEFH_DTYPE_BB_IQ;
        hparam->ddc_m_freq = poal_config->channel[ch].multi_freq_point_param.ddc.middle_freq;
        hparam->ddc_bandwidth = poal_config->channel[ch].multi_freq_point_param.ddc.bandwidth;
    }
    else if(type == STREAM_NIQ_TYPE_RAW || STREAM_NIQ_TYPE_AUDIO){
        sub_channel_array = &poal_config->channel[ch].sub_channel_para;
        hparam->ddc_bandwidth = sub_channel_array->sub_ch[subch].d_bandwith;
        hparam->ddc_m_freq = sub_channel_array->sub_ch[subch].center_freq;
        hparam->d_method = sub_channel_array->sub_ch[subch].raw_d_method;
        hparam->type = (type ==  STREAM_NIQ_TYPE_RAW ? DEFH_DTYPE_CH_IQ : DEFH_DTYPE_AUDIO);
    }
    hparam->ex_type = DFH_EX_TYPE_DEMO;
    ptr_header = xw_assamble_frame_data(&header_len, arg);
    if(ptr_header == NULL)
        return NULL;

#endif
    *hlen = header_len;

    return ptr_header;

}

#define DEFAULT_IQ_SEND_BYTE 512
static size_t iq_send_unit_byte = DEFAULT_IQ_SEND_BYTE;    /* IQ发送长度 */
static int spm_chip_send_niq_data(void *data, size_t len, void *arg)
{
    size_t header_len = 0;
    size_t _send_byte = 32768;//(iq_send_unit_byte > 0 ? iq_send_unit_byte : DEFAULT_IQ_SEND_BYTE);
    uint8_t *hptr = NULL;
    
    if(data == NULL || len == 0 || arg == NULL)
        return -1;
    if(len < _send_byte)
        return -1;
    if((hptr = _assamble_iq_header(0, 0, &header_len, _send_byte, arg, STREAM_NIQ_TYPE_RAW)) == NULL){
        printf_err("assamble head error\n");
        return -1;
    }
    int i, index;
    uint8_t *pdata;
    struct iovec iov[2];
    iov[0].iov_base = hptr;
    iov[0].iov_len = header_len;
    index = len / _send_byte;
    pdata = (uint8_t *)data;
    __lock_iq_send__();
    for(i = 0; i<index; i++){
        iov[1].iov_base = pdata;
        iov[1].iov_len = _send_byte;
#if (defined CONFIG_PROTOCOL_DATA_TCP)
        tcp_send_vec_data(iov, 2, NET_DATA_TYPE_NIQ);
#else
        udp_send_vec_data(iov, 2, NET_DATA_TYPE_NIQ);
#endif
        pdata += _send_byte;
    }
    __unlock_iq_send__();
    
    safe_free(hptr);
    
    return (int)(header_len + len);
}



static int spm_scan(uint64_t *s_freq_offset, uint64_t *e_freq, uint32_t *scan_bw, uint32_t *bw, uint64_t *m_freq)
{
    //#define MAX_SCAN_FREQ_HZ (6000000000)
    uint64_t _m_freq;
    uint64_t _s_freq, _e_freq;
    uint32_t _scan_bw, _bw;
    
    _s_freq = *s_freq_offset;
    _e_freq = *e_freq;
    _scan_bw = *scan_bw;
    if((_e_freq - _s_freq)/_scan_bw > 0){
        _bw = _scan_bw;
        *s_freq_offset = _s_freq + _scan_bw;
    }else{
        _bw = _e_freq - _s_freq;
        *s_freq_offset = _e_freq;
    }
    *scan_bw = _scan_bw;
    _m_freq = _s_freq + _scan_bw/2;
#ifdef CONFIG_BSP_SSA_MONITOR
    if (_m_freq >= _e_freq) {
        _m_freq = _s_freq + _bw/2;
    }
#endif
    //fix bug:中频超6G无信号 wzq
    #if 0
    if (_m_freq > MAX_SCAN_FREQ_HZ){
        _m_freq = MAX_SCAN_FREQ_HZ;
    }
    #endif
    *bw = _bw;
    *m_freq = _m_freq;

    return 0;
}

static int spm_chip_convet_iq_to_fft(void *iq, void *fft, size_t fft_len)
{
    if(iq == NULL || fft == NULL || fft_len == 0)
        return -1;
    //fft_spectrum_iq_to_fft_handle((short *)iq, fft_len, fft_len*2, (float *)fft);
    return 0;
}

static int spm_chip_set_psd_analysis_enable(bool enable)
{
    return 0;
}


static int spm_chip_get_psd_analysis_result(void *data, size_t len)
{
    return 0;
}


static int spm_chip_close(void *args)
{
    return 0;
}

static int spm_sample_ctrl(void *args)
{
    if(misc_get()->freq_ctrl)
        misc_get()->freq_ctrl(args);
    return 0;
}

static const struct spm_backend_ops spm_ops = {
    .create = spm_chip_create,
    .read_niq_data = spm_chip_read_iq_data,
    .read_fft_data = spm_chip_read_fft_data_smooth,//spm_chip_read_fft_data,
    .data_order = spm_chip_data_order,
    .send_fft_data = spm_chip_send_fft_data,
    .send_niq_data = spm_chip_send_niq_data,
    .set_calibration_value = spm_set_calibration_value,
    .set_smooth_time = spm_chip_set_smooth_time,
    .sample_ctrl = spm_sample_ctrl,
    .spm_scan = spm_scan,
    .set_flush_trigger = spm_chip_set_flush_trigger,
    .close = spm_chip_close,
};

struct spm_context * spm_create_context(void)
{
    int ret = -ENOMEM;
    unsigned int len;
    struct spm_context *ctx = zalloc(sizeof(*ctx));
    if (!ctx)
        goto err_set_errno;
    
    ctx->ops = &spm_ops;
    ctx->pdata = &config_get_config()->oal_config;
    
    
err_set_errno:
    errno = -ret;
    return ctx;

}
