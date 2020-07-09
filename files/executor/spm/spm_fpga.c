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
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <getopt.h>

#include "config.h"
#include "spm.h"
#include "spm_fpga.h"
#include "../../protocol/resetful/data_frame.h"

extern uint8_t * akt_assamble_data_frame_header_data(uint32_t *len, void *config);
static int spm_stream_stop(enum stream_type type);

#define DEFAULT_IQ_SEND_BYTE 512
#define DEFAULT_AGC_REF_VAL  0x5e8

size_t pagesize = 0;
size_t iq_send_unit_byte = DEFAULT_IQ_SEND_BYTE;    /* IQ发送长度 */
int32_t agc_ref_val_0dbm = DEFAULT_AGC_REF_VAL;     /* 信号为0DBm时对应的FPGA幅度值 */


FILE *_file_fd;
static  void init_write_file(char *filename)
{
    _file_fd = fopen(filename, "w+b");
    if(!_file_fd){
        printf("Open file error!\n");
    }
}

static inline int write_file_ascii(int16_t *pdata)
{
    char _file_buffer[32] = {0};
    sprintf(_file_buffer, "%d ", *pdata);
    fwrite((void *)_file_buffer,strlen(_file_buffer), 1, _file_fd);

    return 0;
}

static inline int write_lf(void)
{
    char lf = '\n';
    fwrite((void *)&lf,1, 1, _file_fd);

    return 0;
}

/* Allocate zeroed out memory */
static inline void *zalloc(size_t size)
{
	return calloc(1, size);
}

#define RUN_MAX_TIMER 2
static struct timeval *runtimer[RUN_MAX_TIMER];
static int32_t _init_run_timer(uint8_t ch)
{
    int i;
    for(i = 0; i< RUN_MAX_TIMER ; i++){
        runtimer[i] = (struct timeval *)malloc(sizeof(struct timeval)*ch);
        memset(runtimer[i] , 0, sizeof(struct timeval)*ch);
    }
    
    return 0;
}

static int32_t _get_run_timer(uint8_t index, uint8_t ch)
{
    struct timeval *oldTime; 
    struct timeval newTime; 
    int32_t _t_us, ntime_us; 
    if(ch >= MAX_RADIO_CHANNEL_NUM){
        return -1;
    }
    if(index >= RUN_MAX_TIMER){
        return -1;
    }
    oldTime = runtimer[index]+ch;
     if(oldTime->tv_sec == 0 && oldTime->tv_usec == 0){
        gettimeofday(oldTime, NULL);
        return 0;
    }
    gettimeofday(&newTime, NULL);
    _t_us = (newTime.tv_sec - oldTime->tv_sec)*1000000 + (newTime.tv_usec - oldTime->tv_usec); 
    if(_t_us > 0)
        ntime_us = _t_us; 
    else 
        ntime_us = 0; 
    
    return ntime_us;
}

static int32_t _reset_run_timer(uint8_t index, uint8_t ch)
{
    struct timeval newTime; 
    struct timeval *oldTime; 
    if(ch >= MAX_RADIO_CHANNEL_NUM){
        return -1;
    }
    if(index >= RUN_MAX_TIMER){
        return -1;
    }
    gettimeofday(&newTime, NULL);
    oldTime = runtimer[index]+ch;
    memcpy(oldTime, &newTime, sizeof(struct timeval)); 
    return 0;
}


static inline const char * get_str_by_code(const char *const *list, int max, int code)
{
    int i;

    for (i = 0; i < max; i++){
        if(list[i] == NULL)
            continue;
        if (i == code)
            return list[i];
    }

    return -1;
}


static struct _spm_stream spm_stream[] = {
        {DMA_IQ_DEV,  -1, NULL, DMA_BUFFER_SIZE, "IQ Stream"},
        {DMA_FFT_DEV, -1, NULL, DMA_BUFFER_SIZE, "FFT Stream"},
        {DMA_ADC_DEV, -1, NULL, DMA_BUFFER_SIZE, "ADC Stream"},
};

static const char *const dma_status_array[] = {
    [DMA_STATUS_IDLE] = "idle",
    [DMA_STATUS_BUSY] = "busy",
    [DMA_STATUS_INITIALIZING]  = "initializing"
};

static inline const char * dma_str_status(dma_status status)
{
    return get_str_by_code(dma_status_array, ARRAY_SIZE(dma_status_array), status);
}

static int spm_create(void)
{
    struct _spm_stream *pstream;
    ioctl_dma_status status;
    pstream = spm_stream;
    status.dir = DMA_S2MM;
    int i = 0;
    printf_note("SPM init.%d\n", ARRAY_SIZE(spm_stream));
    
    /* create stream */
    for(i = 0; i< ARRAY_SIZE(spm_stream) ; i++){
        pstream[i].id = open(pstream[i].devname, O_RDWR);
        if( pstream[i].id < 0){
            fprintf(stderr, "[%d]open:%s, %s\n", i, pstream[i].devname, strerror(errno));
            continue;
        }
        /* first of all, stop stream */
        spm_stream_stop(i);
        ioctl(pstream[i].id, IOCTL_DMA_INIT_BUFFER, &pstream[i].len);
        pstream[i].ptr = mmap(NULL, pstream[i].len, PROT_READ | PROT_WRITE,MAP_SHARED, pstream[i].id, 0);
        if (pstream[i].ptr == (void*) -1) {
            fprintf(stderr, "mmap: %s\n", strerror(errno));
            exit(-1);
        }
        printf_note("create stream[%s]: dev:%s, ptr=%p, len=%d\n", 
            pstream[i].name, pstream[i].devname, pstream[i].ptr, pstream[i].len);
        ioctl(pstream[i].id, IOCTL_DMA_GET_STATUS, &status);
        if(status.status != DMA_STATUS_IDLE){
            printf_err("DMA status error: %s[%d], exit!!\n", dma_str_status(status.status), status.status);
            exit(-1);
        }
        printf_note("Create %d[%s] OK, status:%s[%d]\n", i, pstream[i].name,dma_str_status(status.status), status.status);
    }
    
    return 0;
}

static ssize_t spm_stream_read(enum stream_type type, volatile void **data)
{
    struct _spm_stream *pstream;
    ioctl_dma_status status;
    status.dir = DMA_S2MM;
    pstream = spm_stream;
    read_info info;
    size_t readn = 0;
    memset(&info, 0, sizeof(read_info));
    if(pstream[type].id < 0){
        printf_warn("stream node:%s not found\n", pstream[type].name);
        return -1;
    }
    do{
        ioctl(pstream[type].id, IOCTL_DMA_GET_ASYN_READ_INFO, &info);
        printf_debug("read status:%d, block_num:%d\n", info.status, info.block_num);
        if(info.status == READ_BUFFER_STATUS_FAIL){
            printf_err("read data error\n");
            exit(-1);
        }else if(info.status == READ_BUFFER_STATUS_PENDING){
            ioctl(pstream[type].id, IOCTL_DMA_GET_STATUS, &status);
           // printf_note("DMA get [%s] status:%s[%d]\n", pstream[type].name, dma_str_status(status.status), status.status);
            if(status.status == DMA_STATUS_IDLE){
                printf_debug("[%s]DMA idle!\n", pstream[type].name);
                return -1;
            }
            usleep(100);
            printf_debug("[%s]no data, waitting\n", pstream[type].name);
        }else if(info.status == READ_BUFFER_STATUS_OVERRUN){
            printf_warn("[%s]data is overrun\n", pstream[type].name);
        }
    }while(info.status == READ_BUFFER_STATUS_PENDING);
    readn = info.blocks[0].length;
    *data = pstream[type].ptr + info.blocks[0].offset;
    printf_debug("[%p, %p, offset=0x%x]%s, readn:%u\n", *data, pstream[type].ptr, info.blocks[0].offset,  pstream[type].name, readn);

    return readn;
}

static ssize_t spm_read_iq_data(void **data)
{
    return spm_stream_read(STREAM_IQ, data);
}

static ssize_t spm_read_fft_data(void **data)
{
    return spm_stream_read(STREAM_FFT, data);
}

static ssize_t spm_read_adc_data(void **data)
{
    return spm_stream_read(STREAM_ADC, data);
}


static int spm_read_adc_over_deal(void *arg)
{
    unsigned int nwrite_byte;
    nwrite_byte = *(unsigned int *)arg;
    
    struct _spm_stream *pstream = spm_stream;
    if(pstream){
        ioctl(pstream[STREAM_ADC].id, IOCTL_DMA_SET_ASYN_READ_INFO, &nwrite_byte);
    }
        
}

static  float get_side_band_rate(uint32_t bandwidth)
{
    #define DEFAULT_SIDE_BAND_RATE  (1.462857)//(1.28)  256/175
    float side_rate = 0.0;
     /* 根据带宽获取边带率 */
    if(config_read_by_cmd(EX_CTRL_CMD, EX_CTRL_SIDEBAND,0, &side_rate, bandwidth) == -1){
        printf_note("!!!!!!!!!!!!!SideRate Is Not Set In Config File[bandwidth=%u]!!!!!!!!!!!!!\n", bandwidth);
        return DEFAULT_SIDE_BAND_RATE;
    }
    return side_rate;
}

/* 频谱数据整理 */
static fft_t *spm_data_order(volatile fft_t *fft_data, 
                                size_t fft_len,  
                                size_t *order_fft_len,
                                void *arg)
{
    struct spm_run_parm *run_args;
    float sigle_side_rate, side_rate;
    uint32_t single_sideband_size;
    uint32_t offset = 0;
    uint64_t start_freq_hz = 0;
    int i;
    size_t order_len = 0;

    if(fft_data == NULL || fft_len == 0){
        printf_note("null data\n");
        return NULL;
    }
        
    run_args = (struct spm_run_parm *)arg;
    /* 获取边带率 */
    side_rate  =  get_side_band_rate(run_args->scan_bw);
    /* 去边带后FFT长度 */
    order_len = (size_t)((float)(fft_len) / side_rate);
    printf_debug("side_rate = %f[fft_len:%u, order_len=%u], scan_bw=%u\n", side_rate, fft_len, order_len, run_args->scan_bw);
    // printf_warn("run_args->fft_ptr=%p, fft_data=%p, order_len=%u, fft_len=%u, side_rate=%f\n", run_args->fft_ptr, fft_data, order_len,fft_len, side_rate);
    /* 信号倒谱 */
    /*
       原始信号（注意去除中间边带）：==>真实输出信号；
       __                     __                             ___
         \                   /              ==》             /   \
          \_______  ________/                     _________/     \_________
                  \/                                                      
                 |边带  |
    */
   // void *psrc = (void *)(fft_data+fft_len -(order_len/2));
   #if 1
    fft_t *pdst = calloc(1, 32*1024*2);
    memcpy((uint8_t *)pdst,                 (uint8_t *)(fft_data) , fft_len*2);
    memcpy((uint8_t *)run_args->fft_ptr,    (uint8_t *)(pdst+ fft_len -order_len/2 ), order_len*2);
    memcpy((uint8_t *)(run_args->fft_ptr+order_len),    (uint8_t *)pdst , order_len*2);
    free(pdst);
    #else
    memcpy((uint8_t *)run_args->fft_ptr,                (uint8_t *)(fft_data+fft_len -order_len/2) , order_len);
    memcpy((uint8_t *)(run_args->fft_ptr+order_len),    (uint8_t *)fft_data , order_len);
    #endif
    //printf_warn(">>>side_rate = %f, scan_bw = %u, bandwidth = %u\n", side_rate, run_args->scan_bw, run_args->bandwidth);
    if(run_args->scan_bw > run_args->bandwidth){
        order_len = (order_len * run_args->bandwidth)/run_args->scan_bw;
        start_freq_hz = run_args->s_freq_offset - (run_args->m_freq_s - run_args->scan_bw/2);
        offset = (order_len * start_freq_hz)/run_args->scan_bw;
    }
    *order_fft_len = order_len;
    printf_debug("order_len=%u, offset = %u, start_freq_hz=%llu, s_freq_offset=%llu, m_freq=%llu, scan_bw=%u\n", 
        order_len, offset, start_freq_hz, run_args->s_freq_offset, run_args->m_freq_s, run_args->scan_bw);
    return (fft_t *)run_args->fft_ptr + offset;
}

static int spm_send_fft_data(void *data, size_t fft_len, void *arg)
{
    uint8_t *ptr_header = NULL;
    uint32_t header_len = 0;
    size_t data_byte_size = 0;

    if(data == NULL || fft_len == 0 || arg == NULL)
        return -1;
    data_byte_size = fft_len * sizeof(fft_t);
#if (defined SUPPORT_DATA_PROTOCAL_AKT)
    struct spm_run_parm *hparam;
    hparam = (struct spm_run_parm *)arg;
    hparam->data_len = data_byte_size; 
    hparam->type = SPECTRUM_DATUM_FLOAT;
    hparam->ex_type = SPECTRUM_DATUM;
    ptr_header = akt_assamble_data_frame_header_data(&header_len, arg);
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
#if 0
    ptr = (uint8_t *)safe_malloc(header_len+ data_byte_size+2);
    if (!ptr){
        printf_err("malloc failed\n");
        return -1;
    }
    memcpy(ptr, ptr_header, header_len);
    memcpy(ptr+header_len, data, data_byte_size);
    udp_send_data(ptr, header_len + data_byte_size);
    safe_free(ptr);
#else
    struct iovec iov[2];

    iov[0].iov_base = ptr_header;
    iov[0].iov_len = header_len;
    iov[1].iov_base = data;
    iov[1].iov_len = data_byte_size;

    udp_send_vec_data(iov, 2);
#endif
#if (defined SUPPORT_DATA_PROTOCAL_XW)
    safe_free(ptr_header);
#endif
    return (header_len + data_byte_size);
}

static int spm_send_iq_data(void *data, size_t len, void *arg)
{
    uint8_t *ptr = NULL, *ptr_header = NULL;
    uint32_t header_len = 0;
    struct _spm_stream *pstream = spm_stream;
    size_t _send_byte = (iq_send_unit_byte > 0 ? iq_send_unit_byte : DEFAULT_IQ_SEND_BYTE);
    
    if(data == NULL || len == 0 || arg == NULL)
        return -1;

    if(len < _send_byte)
        return -1;
#ifdef SUPPORT_DATA_PROTOCAL_AKT
    struct spm_run_parm *hparam;
    hparam = (struct spm_run_parm *)arg;
    hparam->data_len = _send_byte;
    hparam->type = BASEBAND_DATUM_IQ;
    hparam->ex_type = DEMODULATE_DATUM;
    ptr_header = akt_assamble_data_frame_header_data(&header_len, arg);
#elif defined(SUPPORT_DATA_PROTOCAL_XW)
    struct spm_run_parm *hparam;
    hparam = (struct spm_run_parm *)arg;
    hparam->data_len = len; 
    hparam->type = DEFH_DTYPE_BB_IQ;
    hparam->ex_type = DFH_EX_TYPE_DEMO;
    ptr_header = xw_assamble_frame_data(&header_len, arg);
    if(ptr_header == NULL)
        return -1;
#endif
#if 1
    #if 1
    int i, index,sbyte;
    uint8_t *pdata;
    struct iovec iov[2];
    iov[0].iov_base = ptr_header;
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
    #else
    int i, index,sbyte;
    uint8_t *pdata;
    struct iovec iov[1];
    index = len / _send_byte;
    sbyte = index * _send_byte;
    pdata = (uint8_t *)data;
    for(i = 0; i<index; i++){
        iov[0].iov_base = pdata;
        iov[0].iov_len = _send_byte;
        udp_send_vec_data_to_taget_addr(iov, 1);
        pdata += _send_byte;
    }
    #endif
#else
    int i, index,sbyte;
    uint8_t *pdata;
    ptr = (uint8_t *)safe_malloc(header_len+ _send_byte);
    if (!ptr){
        printf_err("malloc failed\n");
        return -1;
    }
    index = len / _send_byte;
    sbyte = index * _send_byte;
    pdata = data;
    memcpy(ptr, ptr_header, header_len);
    for(i = 0; i<index; i++){
        memcpy(ptr+header_len, pdata, _send_byte);
        udp_send_data(ptr, header_len + _send_byte);
        pdata += _send_byte;
    }
    safe_free(ptr);
#endif
    
    ioctl(pstream[STREAM_IQ].id, IOCTL_DMA_SET_ASYN_READ_INFO, &sbyte);
    return (header_len + len);
}

static int spm_send_cmd(void *cmd, void *data, size_t data_len)
{
#ifdef SUPPORT_PROTOCAL_AKT
    poal_send_active_to_all_client((uint8_t *)data, data_len);
#endif
}

static int spm_convet_iq_to_fft(void *iq, void *fft, size_t fft_len)
{
    if(iq == NULL || fft == NULL || fft_len == 0)
        return -1;
    //fft_spectrum_iq_to_fft_handle((short *)iq, fft_len, fft_len*2, (float *)fft);
    return 0;
}

static int spm_save_data(void *data, size_t len)
{
    return 0;
}

static int spm_backtrace_data(void *data, size_t len)
{
    return 0;
}

static int spm_set_psd_analysis_enable(bool enable)
{
    return 0;
}

#include <math.h>

static int spm_get_psd_analysis_result(void *data, size_t len)
{
    return 0;
}

static int spm_scan(uint64_t *s_freq_offset, uint64_t *e_freq, uint32_t *scan_bw, uint32_t *bw, uint64_t *m_freq)
{
    uint64_t _m_freq;
    uint64_t _s_freq, _e_freq;
    uint32_t _scan_bw, _bw;
    #define DIRECT_FREQ_THR (200000000)
   

    _s_freq = *s_freq_offset;
    _e_freq = *e_freq;
    _scan_bw = *scan_bw;
#if defined(SUPPORT_DIRECT_SAMPLE)
    #define DIRECT_MID_FREQ_HZ (128000000)
    if(_s_freq < DIRECT_FREQ_THR ){
        if(_e_freq < DIRECT_FREQ_THR){
            _bw = _e_freq - _s_freq;
            *s_freq_offset = _e_freq;
        }else{
            _bw = DIRECT_FREQ_THR - _s_freq;
            *s_freq_offset = DIRECT_FREQ_THR;
        }
        *scan_bw = DIRECT_FREQ_THR;
        *bw = _bw;
        _m_freq = DIRECT_MID_FREQ_HZ;
    }else
#endif
    {
        _scan_bw = 175000000;
        if((_e_freq - _s_freq)/_scan_bw > 0){
            _bw = _scan_bw;
            *s_freq_offset = _s_freq + _scan_bw;
        }else{
            _bw = _e_freq - _s_freq;
            *s_freq_offset = _e_freq;
        }
        *scan_bw = _scan_bw;
        _m_freq = _s_freq + _scan_bw/2;
        *bw = _bw;
    }
    *m_freq = _m_freq;

    return 0;
}



/* 自动增益控制  : 该函数在频谱处理线程中，循环调用。通过不断控制，逐渐逼近设置幅度值 */
static int spm_agc_ctrl(int ch, struct spm_context *ctx)
{
    /* 
    通过FPGA寄存器参数获取当前信号幅度值；再根据设定值，对信号幅度值做增减 。
    幅度值数据公式：
        db_val=20lg(agc/agcref) 
        agc:为FPGA读取的当前信号幅度值；
        agcref: 为信号为0DBm时对应的FPGA幅度值,需要标定；AGC_REF_VAL
        db_val:为当前信号DBm值

    */
    
    #define AGC_CTRL_PRECISION      2       /* 控制精度+-2dbm */
    //#define AGC_REF_VAL             0x5e8 /* 信号为0DBm时对应的FPGA幅度值 */
    #define AGC_MODE                1       /* 自动增益模式 */
    #define MGC_MODE                0       /* 手动增益模式 */
    #define RF_GAIN_THRE            30      /* 增益到达该阀值，开启射频增益/衰减 */
    #define MID_GAIN_THRE           60      /* 增益到达该阀值，开启中频增益 */

    uint8_t gain_ctrl_method = ctx->pdata->rf_para[ch].gain_ctrl_method;
    uint16_t agc_ctrl_time= ctx->pdata->rf_para[ch].agc_ctrl_time;
    int8_t agc_ctrl_dbm = ctx->pdata->rf_para[ch].agc_mid_freq_out_level;
    int16_t agc_val = 0;
    int8_t dbm_val = 0;
    int ret = -1;
    /* 各通道初始dbm为0 */
    static uint8_t cur_dbm[MAX_RADIO_CHANNEL_NUM] = {0};
    static int8_t ctrl_method[MAX_RADIO_CHANNEL_NUM]={-1};
    uint8_t is_cur_dbm_change = false;
    
    printf_info("gain_ctrl_method:%d agc_ctrl_time:%d agc_ctrl_dbm:%d\n", gain_ctrl_method,agc_ctrl_time,agc_ctrl_dbm);

    /* 当模式变化时， 需要通知内核更新模式信息 */
    if(ctrl_method[ch] != gain_ctrl_method){
        ctrl_method[ch] = gain_ctrl_method;
        if(gain_ctrl_method == MGC_MODE){
            goto exit_mode;
        }
    }

    /* 非自动模式退出控制 */
    if(agc_ctrl_time == 0 ||gain_ctrl_method != AGC_MODE){
        return -1;
    }
    if(_get_run_timer(0, ch) < agc_ctrl_time){
        return -1;
    }
    _reset_run_timer(0, ch);
    if((agc_val = io_get_agc_thresh_val(ch)) < 0){
        return -1;
    }
    printf_info("read agc value=%d\n", agc_val);
    dbm_val = (int32_t)(20 * log10((double)agc_val / ((double)agc_ref_val_0dbm)));
    /* 判断读取的幅度值是否>设置的输出幅度+控制精度 */
    if(dbm_val > (agc_ctrl_dbm+AGC_CTRL_PRECISION)){
        if(cur_dbm[ch] > 0){
            cur_dbm[ch] --;
            is_cur_dbm_change = true;
        }
            
    /* 判断读取的幅度值是否<设置的输出幅度-控制精度 */
    }else if(dbm_val < (agc_ctrl_dbm-AGC_CTRL_PRECISION)){
        cur_dbm[ch] ++;
        is_cur_dbm_change = true;
    }
    /* 若当前幅度值需要修改；则按档次开始设置 ：
        
    */
    if(is_cur_dbm_change == true){
        int8_t rf_gain = 0, mid_gain = 0;
        if(cur_dbm[ch] <= RF_GAIN_THRE){
            rf_gain = cur_dbm[ch];
            mid_gain = 0;
        }else if((cur_dbm[ch] > RF_GAIN_THRE)&&(cur_dbm[ch] <= MID_GAIN_THRE)){
            rf_gain = RF_GAIN_THRE;
            mid_gain = cur_dbm[ch] - RF_GAIN_THRE;
        }else{  /* cur_dbm[ch] > MID_GAIN_THRE */
            rf_gain = RF_GAIN_THRE;
            mid_gain = RF_GAIN_THRE;
        }
        ret = executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, ch, &rf_gain);
        ret = executor_set_command(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, ch, &mid_gain);
    }
exit_mode:
    #if defined(SUPPORT_SPECTRUM_KERNEL) 
    ret = executor_set_command(EX_MID_FREQ_CMD, EX_FILL_RF_PARAM, ch, &cur_dbm[ch], gain_ctrl_method);
    #endif
    return 0;
}

static int spm_sample_ctrl(uint64_t freq_hz)
{
    uint8_t val = 0;
    static int8_t val_dup = -1;
    #define _200MHZ 200000000
    if(freq_hz < _200MHZ){
        val = 1;    /* 直采开启 */
    }else{
        val = 0;    /* 直采关闭 */
    }
    if(val_dup == val){
        return 0;
    }else{
        val_dup = val;
    }
    printf_note("samle ctrl:freq_hz =%lluHz, direct %d\n", freq_hz, val);
    executor_set_command(EX_MID_FREQ_CMD, EX_SAMPLE_CTRL,    0, &val);
    executor_set_command(EX_RF_FREQ_CMD,  EX_RF_SAMPLE_CTRL, 0, &val);
    
    return 0;
}

/*Signal Residency strategy*/
static long signal_residency_policy(int ch, int policy, bool is_signal)
{
    long residence_time = -1; /* 驻留时间:秒， -1为永久驻留 */
    #define POLICY1_SWITCH      0       /* 策略1：有信号驻留；无信号等3S无信号切换下一个点 */
    #define POLICY2_PENDING     -1      /* 策略2：有信号，永久驻留，无信号驻留1秒切换      */
    #define POLICY3_WAIT_TIME   1       /* 策略3： policy>0 有信号按驻留时间驻留切换，无信号驻留NO_SIGAL_WAIT_TIME毫秒立即切换 */

    #define NO_SIGAL_WAIT_TIME  20 /* 无信号驻留时间 */
    
    switch(policy){
        case POLICY2_PENDING:
            if(is_signal){
                residence_time = -1; /* 有信号驻留,永久驻留 */
            }else{
                residence_time = NO_SIGAL_WAIT_TIME;  /* 无信号驻留20毫秒 */
            }
            break;
        case POLICY1_SWITCH:
            if(is_signal){
                residence_time = -1; /* 有信号驻留,永久驻留 */
            }else{
                residence_time = 3000;  /* 无信号驻留3秒 */
            }
            break;
        default: 
            if(is_signal){
                if(policy > 0)  /* 有信号按驻留时间驻留切换  */
                    residence_time = policy*1000;
                else
                    printf("error policy: %d\n", policy);
            }else{
                residence_time = NO_SIGAL_WAIT_TIME;  /* 无信号驻留20毫秒 */
            }
    }
    return residence_time;
}

bool is_sigal_residency_time_arrived(uint8_t ch, int policy, bool is_signal)
{
    long residency_time = 0;
    residency_time = signal_residency_policy(ch, policy, is_signal);
    if(_get_run_timer(1, ch) < residency_time){
        return false;
    }
    _reset_run_timer(1, ch);
    return true;
}

/* 获取信号门限标定值 */
int32_t _get_singal_threshold(uint8_t ch)
{
    int32_t singal_threshold;
    int8_t ret;
    ret = config_read_by_cmd(EX_CTRL_CMD, EX_CTRL_CALI_SIGAL_THRE, ch, &singal_threshold);
    if(ret != 0){
        printf_err("Read signal threshold error!\n");
        return -1;
    }
        
    return singal_threshold;
}

/* 获取信号门限 */
static int32_t _get_signal_threshold_by_amp(uint8_t ch, int32_t *sigal_thred)
{
    int8_t rf_attenuation, m_attenuation;
    uint16_t threshold_0db, threshold = 0;
    int ret = 0;

    ret = config_read_by_cmd(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, ch, &m_attenuation);
    if(ret != 0){
        printf_err("Read MGC Gain error\n");
        return -1;
    }
    ret = config_read_by_cmd(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, ch, &rf_attenuation);
    if(ret != 0){
        printf_err("Read RF attenuation error\n");
        return -1;
    }
    ret = _get_singal_threshold(ch);
    if(ret == -1)
        return -1;
    threshold_0db = ret;
    printf_note("rf:%d,mf:%d,0db_th:%d\n",rf_attenuation,m_attenuation,threshold_0db);
    threshold = threshold_0db * pow(10.0f,(double)(rf_attenuation+m_attenuation)/20);
    printf_note("threshold = %d\n", threshold);
    *sigal_thred = threshold;
    
    return 0;
}

/* 判断对应通道是否有信号:      true: 有信号; false:无信号*/
static int32_t  spm_get_signal_strength(uint8_t ch, bool *is_singal, uint16_t *strength)
{
    uint16_t sig_amp = 0;
    int32_t sigal_thred = 0;
    int32_t ret;
    
    sig_amp = io_get_signal_strength(ch);
    if(strength != NULL)
        *strength = sig_amp;
    ret = _get_signal_threshold_by_amp(ch, &sigal_thred);
    if(ret == -1)
        return -1;
    
    if(sig_amp > sigal_thred) {
        *is_singal = true;
    }else{
        *is_singal = false;
    }
    
    return 0;
}

static int spm_stream_back_start(uint32_t len,uint8_t continuous, enum stream_type type)
{
    struct _spm_stream *pstream = spm_stream;
    IOCTL_DMA_START_PARA para;
    
    printf_note("stream type:%d, back start, continuous[%d], len=%u\n", type, continuous, len);

    if(continuous)
        para.mode = DMA_MODE_CONTINUOUS;
    else{
        para.mode = DMA_MODE_ONCE;
        para.trans_len = len;
    }
    ioctl(pstream[type].id, IOCTL_DMA_ASYN_WRITE_START, &para);
    return 0;
}

static int spm_stream_back_stop(enum stream_type type)
{
    struct _spm_stream *pstream = spm_stream;
    ioctl(pstream[type].id, IOCTL_DMA_ASYN_WRITE_STOP, NULL);
    printf_note("stream back stop: %d\n", type);
    sync();
    return 0;
}

static int spm_stream_back_running_file(enum stream_type type, int fd)
{
    void *user_mem = NULL, *w_addr = NULL;
    int i, rc, ret = 0;
    ssize_t total_Byte = 0;
    unsigned int size;
    write_info info;
    int isDone = 0;
    int file_fd;

    struct _spm_stream *pstream = spm_stream;

    if(fd <= 0)
        return -1;
    file_fd = fd;
    ioctl(pstream[type].id, IOCTL_DMA_GET_ASYN_WRITE_INFO, &info);
    if(info.status != 0){   /* NOT OK */
        printf_debug("write status:%d, block_num:%d\n", info.status, info.block_num);
        if(info.status == WRITE_BUFFER_STATUS_FAIL){
            printf_debug("read error!\n");
            exit(1);
        }else if (info.status == WRITE_BUFFER_STATUS_EMPTY){
            printf_debug("write buffer empty\n");
        }else if (info.status == WRITE_BUFFER_STATUS_FULL){
            printf_debug("write buffer full\n");
            usleep(100);
        }else{
            printf_debug("unknown status[0x%x]\n", info.status);
        }
    }

    for(i = 0; i < info.block_num; i++){
        size = info.blocks[i].length;
        w_addr = pstream[type].ptr + info.blocks[i].offset;
       // printf_debug("file_fd=%d,ptr=%p, w_addr=%p, info.blocks[%d].offset=%x, size=%u\n",file_fd,pstream[type].ptr,  w_addr, i, info.blocks[i].offset,  size);
        rc = read(file_fd, w_addr, size);
        if (rc < 0){
            perror("read file");
            ret = -1;
            break;
        }
        else if(rc == 0){
            printf_debug("read file over.\n");
            return 0;
        }       
        ioctl(pstream[type].id, IOCTL_DMA_SET_ASYN_WRITE_INFO, &rc);
        total_Byte += rc;
        ret = total_Byte;
    }
    return ret;
}

static int spm_stream_start(uint32_t len,uint8_t continuous, enum stream_type type)
{
    struct _spm_stream *pstream = spm_stream;
    IOCTL_DMA_START_PARA para;
    
    printf_debug("stream type:%d, start, continuous[%d], len=%u\n", type, continuous, len);

    if(continuous)
        para.mode = DMA_MODE_CONTINUOUS;
    else{
        para.mode = DMA_MODE_ONCE;
        para.trans_len = len;
    }
    ioctl(pstream[type].id, IOCTL_DMA_ASYN_READ_START, &para);
    return 0;
}

static int spm_stream_stop(enum stream_type type)
{
    struct _spm_stream *pstream = spm_stream;
    ioctl(pstream[type].id, IOCTL_DMA_ASYN_READ_STOP, NULL);
    printf_debug("stream_stop: %d\n", type);
    return 0;
}

static int _spm_close(struct spm_context *ctx)
{
    struct _spm_stream *pstream = spm_stream;
    int i;

    for(i = 0; i< ARRAY_SIZE(spm_stream) ; i++){
        spm_stream_stop(i);
        close(pstream[i].id);
    }
    safe_free(ctx->run_args->fft_ptr);
    safe_free(ctx->run_args);
    printf_note("close..\n");
    return 0;
}


static const struct spm_backend_ops spm_ops = {
    .create = spm_create,
    .read_iq_data = spm_read_iq_data,
    .read_fft_data = spm_read_fft_data,
    .read_adc_data = spm_read_adc_data,
    .read_adc_over_deal = spm_read_adc_over_deal,
    .data_order = spm_data_order,
    .send_fft_data = spm_send_fft_data,
    .send_iq_data = spm_send_iq_data,
    .send_cmd = spm_send_cmd,
    .convet_iq_to_fft = spm_convet_iq_to_fft,
    .set_psd_analysis_enable = spm_set_psd_analysis_enable,
    .get_psd_analysis_result = spm_get_psd_analysis_result,
    .save_data = spm_save_data,
    .backtrace_data = spm_backtrace_data,
    .agc_ctrl = spm_agc_ctrl,
    .residency_time_arrived = is_sigal_residency_time_arrived,
    .signal_strength = spm_get_signal_strength,
    .back_running_file = spm_stream_back_running_file,
    .stream_back_start = spm_stream_back_start,
    .stream_back_stop = spm_stream_back_stop,
    .stream_start = spm_stream_start,
    .stream_stop = spm_stream_stop,
    .sample_ctrl = spm_sample_ctrl,
    .spm_scan = spm_scan,
    .close = _spm_close,
};

struct spm_context * spm_create_fpga_context(void)
{
    int ret = -ENOMEM;
    unsigned int len;
    struct spm_context *ctx = zalloc(sizeof(*ctx));
    if (!ctx)
        goto err_set_errno;
    pagesize = getpagesize();
    if(config_read_by_cmd(EX_CTRL_CMD, EX_CTRL_IQ_DATA_LENGTH, 0, &iq_send_unit_byte) == -1){
            printf_note("read iq_send_unit_byte Error, use default[%u]\n", DEFAULT_IQ_SEND_BYTE);
    }
    if(iq_send_unit_byte == 0){
        iq_send_unit_byte = DEFAULT_IQ_SEND_BYTE;
    }
    printf_note("iq send unit byte:%u\n", iq_send_unit_byte);
    ctx->ops = &spm_ops;
    ctx->pdata = &config_get_config()->oal_config;
    
    if( config_get_config()->oal_config.ctrl_para.agc_ref_val_0dbm != 0)
        agc_ref_val_0dbm = config_get_config()->oal_config.ctrl_para.agc_ref_val_0dbm;
    
    _init_run_timer(MAX_RADIO_CHANNEL_NUM);
err_set_errno:
    errno = -ret;
    return ctx;

}
