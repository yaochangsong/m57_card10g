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
#include "spm.h"
#include "spm_chip.h"
#include "../../protocol/resetful/data_frame.h"

static struct _vec_fft{
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
#ifdef SUPPORT_SPECTRUM_FFT
    fft_init();
    /* set default smooth count */
    fft_set_smoothcount(2);
#endif
#ifdef SUPPORT_SPECTRUM_ANALYSIS
    spm_analysis_init();
#endif
}

volatile int32_t _smooth_time = 0;
static void spm_chip_set_smooth_time(int32_t count)
{
#ifdef SUPPORT_SPECTRUM_FFT
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
    
    iqdata = specturm_rx0_read_data(&r);
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
    iqdata = specturm_rx0_read_data(&r);
#ifdef SUPPORT_SPECTRUM_ANALYSIS
    spm_analysis_start(0, iqdata, r, args);
#endif
    floatdata = zalloc(fft_size*4);
    if(floatdata == NULL)
        goto err;
    fft_spectrum_iq_to_fft_handle(iqdata, fft_size, fft_size*2, floatdata);
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
        r = spm_chip_read_fft_data(&rawdata, args);
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

static ssize_t spm_chip_read_fft_data_smooth(void **data, void *args)
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

    if(smooth <= 1){
        r = spm_chip_read_fft_data(&data_dup, args);
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
        printf_err("[NOT SUPPORT]smooth[%d] time is bigger than max:%d!\n", smooth, _MAX_SMOOTH_NUM);
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
    
    return  (fft_data + single_sideband_size + offset);
}

static int spm_chip_send_fft_data(void *data, size_t fft_len, void *arg)
{
    #define HEAD_BUFFER_LEN  512 
    uint8_t *ptr_header = NULL;
    uint32_t header_len = 0;
    size_t data_byte_size = 0;

    if(data == NULL || fft_len == 0 || arg == NULL)
        return -1;
    spm_fft_calibration(data, fft_len);
    data_byte_size = fft_len * sizeof(fft_t);
#if (defined SUPPORT_DATA_PROTOCAL_AKT)
    struct spm_run_parm *hparam;
    hparam = (struct spm_run_parm *)arg;
    hparam->data_len = data_byte_size; 
    hparam->type = SPECTRUM_DATUM_FLOAT;
    hparam->ex_type = SPECTRUM_DATUM;
    if((ptr_header = akt_assamble_data_frame_header_data(&header_len, arg)) == NULL){
        return -1;
    }
#elif defined(SUPPORT_DATA_PROTOCAL_XW)
    struct spm_run_parm *hparam;
    hparam = (struct spm_run_parm *)arg;
    hparam->data_len = data_byte_size; 
    hparam->type = DEFH_DTYPE_FLOAT;
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

    udp_send_vec_data(iov, 2);
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
static void *_assamble_iq_header(size_t subch, size_t *hlen, size_t data_len, void *arg)
{
    uint8_t *ptr = NULL, *ptr_header = NULL;
    uint32_t header_len = 0;
    
    if(data_len == 0 || arg == NULL)
        return NULL;
    
#ifdef SUPPORT_DATA_PROTOCAL_AKT
    struct spm_run_parm *hparam;
    hparam = (struct spm_run_parm *)arg;
    hparam->data_len = data_len;
    hparam->type = BASEBAND_DATUM_IQ;
    hparam->ex_type = DEMODULATE_DATUM;
    //hparam->sub_ch = subch;
    if((ptr_header = akt_assamble_data_frame_header_data(&header_len, arg))== NULL){
        return NULL;
    }
    
#elif defined(SUPPORT_DATA_PROTOCAL_XW)
    struct spm_run_parm *hparam;
    hparam = (struct spm_run_parm *)arg;
    hparam->data_len = data_len; 
    hparam->type = DEFH_DTYPE_BB_IQ;
    hparam->ex_type = DFH_EX_TYPE_DEMO;
   // hparam->sub_ch = subch;
    ptr_header = xw_assamble_frame_data(&header_len, arg);
    if(ptr_header == NULL)
        return NULL;
#endif
    *hlen = header_len;

    return ptr_header;

}

#define DEFAULT_IQ_SEND_BYTE 512
static size_t iq_send_unit_byte = DEFAULT_IQ_SEND_BYTE;    /* IQ发送长度 */
static int spm_chip_send_iq_data(void *data, size_t len, void *arg)
{
    uint32_t header_len = 0;
    size_t _send_byte = (iq_send_unit_byte > 0 ? iq_send_unit_byte : DEFAULT_IQ_SEND_BYTE);
    uint8_t *hptr = NULL;
    
    if(data == NULL || len == 0 || arg == NULL)
        return -1;

    if(len < _send_byte)
        return -1;
    if((hptr = _assamble_iq_header(1, &header_len, _send_byte, arg)) == NULL){
        printf_err("assamble head error\n");
        return -1;
    }
   
    int i, index,sbyte;
    uint8_t *pdata;
    struct iovec iov[2];
    iov[0].iov_base = hptr;
    iov[0].iov_len = header_len;
    index = len / _send_byte;
    sbyte = index * _send_byte;
    pdata = (uint8_t *)data;
    for(i = 0; i<index; i++){
        iov[1].iov_base = pdata;
        iov[1].iov_len = _send_byte;
        udp_send_vec_data(iov, 2);
        pdata += _send_byte;
    }
   
    safe_free(hptr);
    
    return (header_len + len);
}


static int spm_scan(uint64_t *s_freq_offset, uint64_t *e_freq, uint32_t *scan_bw, uint32_t *bw, uint64_t *m_freq)
{
    #define MAX_SCAN_FREQ_HZ (6000000000)
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
#ifdef SUPPORT_PROJECT_SSA_MONITOR
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


static int spm_chip_close(void)
{
    return 0;
}

static int spm_sample_ctrl(void *args)
{
    static int old_switch = -1;
    int tmp_switch = 0;
    struct spm_run_parm *r_args;
    r_args = (struct spm_run_parm *)args;
#if defined(SUPPORT_PROJECT_SSA_MONITOR)
    uint64_t freq_hz = r_args->m_freq_s;
    if (freq_hz >= LVTTL_FREQ_START1 && freq_hz <= LVTTL_FREQ_end1) {
        tmp_switch = 1;
        if (old_switch != tmp_switch) {
            old_switch = tmp_switch;
            printf_info("switch to 2-4G\n");
            SW_TO_2_4_G();
        }
    } else if (freq_hz >= LVTTL_FREQ_START2 && freq_hz < LVTTL_FREQ_end2) {
        tmp_switch = 2;
        if (old_switch != tmp_switch) {
            old_switch = tmp_switch;
            printf_info("switch to 7-7.5G\n");
            SW_TO_7_75_G();
        }
    } else if (freq_hz >= LVTTL_FREQ_START3 && freq_hz <= LVTTL_FREQ_end3) {
        tmp_switch = 3;
        if (old_switch != tmp_switch) {
            old_switch = tmp_switch;
            printf_info("switch to 7.5-9G\n");
            SW_TO_75_9_G();
        }
    } else {
        printf_info("rf unsupported freq:%llu hz\n", freq_hz);
    }
#endif
    return 0;
}

static const struct spm_backend_ops spm_ops = {
    .create = spm_chip_create,
    .read_iq_data = spm_chip_read_iq_data,
    .read_fft_data = spm_chip_read_fft_data_smooth,//spm_chip_read_fft_data,
    .data_order = spm_chip_data_order,
    .send_fft_data = spm_chip_send_fft_data,
    .send_iq_data = spm_chip_send_iq_data,
    .set_calibration_value = spm_set_calibration_value,
    .set_smooth_time = spm_chip_set_smooth_time,
    .sample_ctrl = spm_sample_ctrl,
    .spm_scan = spm_scan,
    .set_flush_trigger = spm_chip_set_flush_trigger,
    .close = spm_chip_close,
};

struct spm_context * spm_create_chip_context(void)
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
