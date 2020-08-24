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

extern int8_t akt_assamble_data_frame_header_data( uint8_t *head_buf,  int buf_len, uint32_t *len, void *config);
static int spm_stream_stop(enum stream_type type);
static int spm_stream_back_stop(enum stream_type type);


#define DIRECT_FREQ_THR (200000000) /* 直采截止频率 */
#define DIRECT_BANDWIDTH (256000000)
#define DEFAULT_IQ_SEND_BYTE 512
//#define DEFAULT_AGC_REF_VAL  0x5e8
//#define DEFAULT_AGC_REF_VAL  0xBE4
#define DEFAULT_AGC_REF_VAL  0x3000
#define DEFAULT_SUBCH_REF_VAL 0x3000

uint8_t cur_dbm[MAX_RADIO_CHANNEL_NUM] = {0};

size_t pagesize = 0;
size_t iq_send_unit_byte = DEFAULT_IQ_SEND_BYTE;    /* IQ发送长度 */
int32_t agc_ref_val_0dbm = DEFAULT_AGC_REF_VAL;     /* 信号为0DBm时对应的FPGA幅度值 */
int32_t subch_ref_val_0dbm  = DEFAULT_SUBCH_REF_VAL;



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
    //printf("newTime:%ld,%ld\n",newTime.tv_sec, newTime.tv_usec);
    //printf("oldTime:%ld,%ld\n",oldTime->tv_sec, oldTime->tv_usec);
    _t_us = (newTime.tv_sec - oldTime->tv_sec)*1000000 + (newTime.tv_usec - oldTime->tv_usec); 
    //printf("_t_us:%d\n",_t_us);
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
        {DMA_IQ_DEV,  -1, NULL, DMA_BUFFER_16M_SIZE, "IQ Stream", DMA_READ},
        {DMA_FFT_DEV, -1, NULL, DMA_BUFFER_16M_SIZE, "FFT Stream", DMA_READ},
        {DMA_ADC_TX_DEV, -1, NULL, DMA_BUFFER_128M_SIZE, "ADC Tx Stream", DMA_WRITE},
        {DMA_ADC_RX_DEV, -1, NULL, DMA_BUFFER_128M_SIZE, "ADC Rx Stream", DMA_READ},
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
            usleep(5);
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
    return spm_stream_read(STREAM_ADC_READ, data);
}


static int spm_read_adc_over_deal(void *arg)
{
    unsigned int nwrite_byte;
    nwrite_byte = *(unsigned int *)arg;
    
    struct _spm_stream *pstream = spm_stream;
    if(pstream){
        ioctl(pstream[STREAM_ADC_READ].id, IOCTL_DMA_SET_ASYN_READ_INFO, &nwrite_byte);
    }
        
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
    /*双字节对齐*/
    order_len = order_len&0x0fffffffe; 
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
    memcpy((uint8_t *)run_args->fft_ptr,    (uint8_t *)(pdst+ fft_len -order_len/2 ), order_len);
    memcpy((uint8_t *)(run_args->fft_ptr+order_len),    (uint8_t *)pdst , order_len);
    free(pdst);
    #else
    memcpy((uint8_t *)run_args->fft_ptr,                (uint8_t *)(fft_data+fft_len -order_len/2) , order_len);
    memcpy((uint8_t *)(run_args->fft_ptr+order_len),    (uint8_t *)fft_data , order_len);
    #endif
     if(run_args->mode == OAL_FAST_SCAN_MODE || run_args->mode ==OAL_MULTI_ZONE_SCAN_MODE){
#if defined(SUPPORT_DIRECT_SAMPLE)
        if((run_args->m_freq_s < DIRECT_FREQ_THR) && (run_args->m_freq_s > 0)){
            order_len = (fft_len * run_args->bandwidth)/DIRECT_BANDWIDTH;//run_args->scan_bw;
            offset = (run_args->s_freq_offset-28000000)*fft_len/DIRECT_BANDWIDTH;
            printf_warn("s_freq_offset=%llu,fft_len=%u,order_len=%u, offset=%u, bandwidth=%u\n",run_args->s_freq_offset,fft_len, order_len, offset, run_args->bandwidth);
        }else
#endif
         if(run_args->scan_bw > run_args->bandwidth){
             order_len = (order_len * run_args->bandwidth)/run_args->scan_bw;
             start_freq_hz = run_args->s_freq_offset - (run_args->m_freq_s - run_args->scan_bw/2);
             offset = (order_len * start_freq_hz)/run_args->scan_bw;
         }
     }
    *order_fft_len = order_len;
    printf_debug("order_len=%u, offset = %u, start_freq_hz=%llu, s_freq_offset=%llu, m_freq=%llu, scan_bw=%u\n", 
        order_len, offset, start_freq_hz, run_args->s_freq_offset, run_args->m_freq_s, run_args->scan_bw);
    return (fft_t *)run_args->fft_ptr + offset;
}

static int spm_send_fft_data(void *data, size_t fft_len, void *arg)
{
    #define HEAD_BUFFER_LEN  512 
    uint8_t *ptr_header = NULL;
    uint32_t header_len = 0;
    size_t data_byte_size = 0;
    uint8_t head_buf[HEAD_BUFFER_LEN];

    if(data == NULL || fft_len == 0 || arg == NULL)
        return -1;
    memset(head_buf, 0, HEAD_BUFFER_LEN);
    data_byte_size = fft_len * sizeof(fft_t);
#if (defined SUPPORT_DATA_PROTOCAL_AKT)
    struct spm_run_parm *hparam;
    hparam = (struct spm_run_parm *)arg;
    hparam->data_len = data_byte_size; 
    hparam->type = SPECTRUM_DATUM_FLOAT;
    hparam->ex_type = SPECTRUM_DATUM;
    if(akt_assamble_data_frame_header_data(head_buf, HEAD_BUFFER_LEN, &header_len, arg)!=0){
        return -1;
    }
    ptr_header = head_buf;
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
    #define HEAD_BUFFER_LEN  512 
    uint8_t *ptr = NULL, *ptr_header = NULL;
    uint32_t header_len = 0;
    struct _spm_stream *pstream = spm_stream;
    size_t _send_byte = (iq_send_unit_byte > 0 ? iq_send_unit_byte : DEFAULT_IQ_SEND_BYTE);
    uint8_t head_buf[HEAD_BUFFER_LEN];
    
    if(data == NULL || len == 0 || arg == NULL)
        return -1;

    if(len < _send_byte)
        return -1;
    memset(head_buf, 0, HEAD_BUFFER_LEN);
#ifdef SUPPORT_DATA_PROTOCAL_AKT
    struct spm_run_parm *hparam;
    hparam = (struct spm_run_parm *)arg;
    hparam->data_len = _send_byte;
    hparam->type = BASEBAND_DATUM_IQ;
    hparam->ex_type = DEMODULATE_DATUM;
    if(akt_assamble_data_frame_header_data(head_buf, HEAD_BUFFER_LEN, &header_len, arg)!=0){
        return -1;
    }
    ptr_header = head_buf;
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

/* 获取信号变化上限：
    @rf_mode: 射频模式
    @dst_val： 控制目标信号强度
    @max_val： 返回信号变化上限值
    
    return: 0 OK  -1: false
*/
static inline int _get_range_max(int ch, uint8_t rf_mode, int dst_val, int *max_val, struct spm_context *ctx)
{
    int mode = 0, i, mag_val = 0, dec_val = 0, found = 0, rang_max = 0;
    mode = rf_mode;
    /* 获取射频模式增益最大值 */
    for(i = 0; i< ARRAY_SIZE(ctx->pdata->cal_level.rf_mode.mag); i++){
        if(ctx->pdata->cal_level.rf_mode.mag[i].mode == mode){
            mag_val = ctx->pdata->cal_level.rf_mode.mag[i].magification;
            found = 1;
            break;
        }
    }
    if(found == 0){
        printf_err("Rf mode error:%d !!!\n", mode);
        return -1;
    }
    /* 获取信号增益最大值 */
    rang_max = mag_val + dst_val;

    /* 获取模式信号检测最大值 */
    found = 0;
    for(i = 0; i< ARRAY_SIZE(ctx->pdata->cal_level.rf_mode.detection); i++){
        if(ctx->pdata->cal_level.rf_mode.detection[i].mode == mode){
            dec_val = ctx->pdata->cal_level.rf_mode.detection[i].max;
            found = 1;
            break;
        }
    }
    if(found == 0){
        printf_err("NOT find Rf detection MAX value!!!\n");
        return -1;
    }
    /* 获取信号变化上限 */
    *max_val = (rang_max < dec_val ? rang_max : dec_val);
    printf_debug("max_val=%d, rang_max=%d, magification =%d, dst_val=%d,detection_max=%d\n", *max_val,
                rang_max, mag_val, dst_val, dec_val);
    
    return 0;
}

/* 获取信号变化下限 */
static inline int _get_range_min(int ch, uint8_t rf_mode, int dst_val, int *min_val, struct spm_context *ctx)
{
    int mode = 0, i, mag_val = 0, dec_val = 0, found = 0;
    int max_attenuation_value = 0, rang_min = 0;
    mode = rf_mode;
    /* 获取射频模式增益最大值 */
    for(i = 0; i< ARRAY_SIZE(ctx->pdata->cal_level.rf_mode.mag); i++){
        if(ctx->pdata->cal_level.rf_mode.mag[i].mode == mode){
            mag_val = ctx->pdata->cal_level.rf_mode.mag[i].magification;
            found = 1;
            break;
        }
    }
    if(found == 0){
        printf_err("Rf mode error:%d !!!\n", mode);
        return -1;
    }

    /* 获取最大衰减 */
    found = 0;
    for(i = 0; i< ARRAY_SIZE(ctx->pdata->cal_level.rf_mode.rf_distortion); i++){
        if(ctx->pdata->cal_level.rf_mode.rf_distortion[i].mode == mode){
            max_attenuation_value = ctx->pdata->cal_level.rf_mode.rf_distortion[i].end_range;
            found = 1;
            break;
        }
    }
    max_attenuation_value += ctx->pdata->cal_level.rf_mode.mgc_distortion.end_range;
    
    /* 获取衰减最小值后信号值 */
    rang_min = mag_val -max_attenuation_value + dst_val;
    
    printf_debug("Rang min:%d, max_attenuation_value=%d, magification=%d, dst_val=%d\n", 
                rang_min, max_attenuation_value, mag_val, dst_val);

    /* 获取检测范围最小值 */
    found = 0;
    for(i = 0; i< ARRAY_SIZE(ctx->pdata->cal_level.rf_mode.detection); i++){
        if(ctx->pdata->cal_level.rf_mode.detection[i].mode == mode){
            dec_val = ctx->pdata->cal_level.rf_mode.detection[i].min;
            found = 1;
            break;
        }
    }
    if(found == 0){
        printf_err("NOT find Rf detection MAX value!!!\n");
        return -1;
    }
    /* 获取信号变化下限 */
    *min_val = (rang_min > dec_val ? rang_min : dec_val);
    printf_debug("min_val=%d, rang_min=%d, detection min val=%d\n", *min_val, rang_min, dec_val);
    
    return 0;
}

static inline int _set_half_attenuation_value(int ch, struct spm_context *ctx)
{
    int8_t  rf_attenuation = 0, mgc_attenuation = 0;
    uint8_t rf_mode = ctx->pdata->rf_para[ch].rf_mode_code; /* 射频模式 */
    int i, half_attenuation_value = 0, found = 0;
    for(i = 0; i< ARRAY_SIZE(ctx->pdata->cal_level.rf_mode.rf_distortion); i++){
        if(ctx->pdata->cal_level.rf_mode.rf_distortion[i].mode == rf_mode){
            rf_attenuation = ctx->pdata->cal_level.rf_mode.rf_distortion[i].end_range;
            found = 1;
            break;
        }
    }
    mgc_attenuation= ctx->pdata->cal_level.rf_mode.mgc_distortion.end_range;
    half_attenuation_value = (rf_attenuation + mgc_attenuation)/2;
    if(half_attenuation_value > rf_attenuation){
        mgc_attenuation = half_attenuation_value - rf_attenuation; 
    }else{
        mgc_attenuation = 0;
    }
    executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, ch, &rf_attenuation);
    executor_set_command(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, ch, &mgc_attenuation);
    config_write_data(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, ch, &rf_attenuation);
    config_write_data(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, ch, &mgc_attenuation);
    
    printf_debug("half_attenuation_value=%d, rf_attenuation=%d, mgc_attenuation=%d\n",
                half_attenuation_value, rf_attenuation, mgc_attenuation);

    return half_attenuation_value;
}

static inline int _get_max_rf_attenuation_value(int ch, struct spm_context *ctx)
{
    uint8_t rf_mode_code = ctx->pdata->rf_para[ch].rf_mode_code;
    int max_attenuation_value = 0;
    int i, found = 0;
    for(i = 0; i< ARRAY_SIZE(ctx->pdata->cal_level.rf_mode.rf_distortion); i++){
        if(ctx->pdata->cal_level.rf_mode.rf_distortion[i].mode == rf_mode_code){
            max_attenuation_value = ctx->pdata->cal_level.rf_mode.rf_distortion[i].end_range;
            found = 1;
            break;
        }
    }
    if(found == 0)
        printf_err("not find max rf attenuation value\n");
    printf_debug("max_attenuation_value: %d\n", max_attenuation_value);
    return max_attenuation_value;
}

static inline int  _get_min_rf_attenuation_value(int ch, struct spm_context *ctx)
{
    uint8_t rf_mode_code = ctx->pdata->rf_para[ch].rf_mode_code;
    int min_attenuation_value = 0;
    int i, found = 0;
    for(i = 0; i< ARRAY_SIZE(ctx->pdata->cal_level.rf_mode.rf_distortion); i++){
        if(ctx->pdata->cal_level.rf_mode.rf_distortion[i].mode == rf_mode_code){
            min_attenuation_value = ctx->pdata->cal_level.rf_mode.rf_distortion[i].start_range;
            found = 1;
            break;
        }
    }
    if(found == 0)
        printf_err("not find min rf attenuation value\n");
    printf_debug("min_attenuation_value: %d\n", min_attenuation_value);
    return min_attenuation_value;

}

static inline int   _get_min_mgc_attenuation_value(int ch, struct spm_context *ctx)
{
    uint8_t rf_mode_code = ctx->pdata->rf_para[ch].rf_mode_code;
    uint8_t min_attenuation_value = 0;
    int i, found = 0;
    min_attenuation_value = ctx->pdata->cal_level.rf_mode.mgc_distortion.start_range;
    printf_debug("min_attenuation_value: %d\n", min_attenuation_value);
    return min_attenuation_value;

}


static inline int _get_max_mgc_attenuation_value(int ch, struct spm_context *ctx)
{
    uint8_t rf_mode_code = ctx->pdata->rf_para[ch].rf_mode_code;
    uint8_t max_attenuation_value = 0;
    int i, found = 0;
    max_attenuation_value = ctx->pdata->cal_level.rf_mode.mgc_distortion.end_range;
    printf_debug("max_attenuation_value: %d\n", max_attenuation_value);
    return max_attenuation_value;
}

static inline int _set_up_step_attenuation_value(int ch, struct spm_context *ctx, int8_t  *rf_attenuation ,int8_t  *mgc_attenuation)
{
    int8_t  _rf_attenuation = 0, _mgc_attenuation = 0;
    
    _rf_attenuation = *rf_attenuation;
    _mgc_attenuation = *mgc_attenuation;
    if(_rf_attenuation < _get_max_rf_attenuation_value(ch, ctx)){
        _rf_attenuation ++;
        executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, ch, &_rf_attenuation);
    }else if(_mgc_attenuation < _get_max_mgc_attenuation_value(ch, ctx)){
        _mgc_attenuation++;
        executor_set_command(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, ch, &_mgc_attenuation);
    }
    *rf_attenuation = _rf_attenuation;
    *mgc_attenuation = _mgc_attenuation;
    printf_info("UP: rf_attenuation=%d, mgc_attenuation=%d\n", _rf_attenuation, _mgc_attenuation);
    return 0;
}

static inline int _set_down_step_attenuation_value(int ch, struct spm_context *ctx, int8_t  *rf_attenuation ,int8_t  *mgc_attenuation)
{
    int8_t  _rf_attenuation = 0, _mgc_attenuation = 0;
    _rf_attenuation = *rf_attenuation;
    _mgc_attenuation = *mgc_attenuation;
    if(_rf_attenuation > _get_min_rf_attenuation_value(ch, ctx)){
        _rf_attenuation --;
        executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, ch, &_rf_attenuation);
    }else if(_mgc_attenuation > _get_min_mgc_attenuation_value(ch, ctx)){
        _mgc_attenuation--;
        executor_set_command(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, ch, &_mgc_attenuation);
    }
    *rf_attenuation = _rf_attenuation;
    *mgc_attenuation = _mgc_attenuation;
    printf_info("DOWN: rf_attenuation=%d, mgc_attenuation=%d\n", _rf_attenuation, _mgc_attenuation);
    return 0;
}


static inline _spm_read_signal_value(int ch)
{
    int8_t agc_dbm_val = 0;     /* 读取信号db值 */
    uint16_t agc_reg_val = 0;   /* 读取信号寄存器值,sigal power level read from fpga register */
    if((agc_reg_val = io_get_agc_thresh_val(ch)) < 0){
        return -1;
    }
    
    /* 信号强度转换为db值 */
    agc_dbm_val = (int32_t)(20 * log10((double)agc_reg_val / ((double)agc_ref_val_0dbm)));
    printf_debug("agc_reg_val=0x%x[%d], agc_dbm_val=%d\n", agc_reg_val, agc_reg_val, agc_dbm_val);
    return agc_dbm_val;
}

static int spm_agc_ctrl_v3(int ch, struct spm_context *ctx)
{
    #define AGC_CTRL_PRECISION      0       /* 控制精度+-2dbm */
    uint8_t gain_ctrl_method = ctx->pdata->rf_para[ch].gain_ctrl_method; /* 自动增益or手动增益 */
    uint16_t agc_ctrl_time= ctx->pdata->rf_para[ch].agc_ctrl_time;       /* 自动增益步进控制时间 */
    int8_t agc_ctrl_dbm = ctx->pdata->rf_para[ch].agc_mid_freq_out_level;/* 自动增益目标控制功率db值 */
    uint8_t rf_mode = ctx->pdata->rf_para[ch].rf_mode_code; /* 射频模式 */
    int8_t agc_dbm_val = 0;     /* 读取信号db值 */
    int ret = -1;
    static int8_t ctrl_method_dup[MAX_RADIO_CHANNEL_NUM]={-1};
    static int8_t rf_mode_dup[MAX_RADIO_CHANNEL_NUM]={-1};
    static int32_t dst_val[MAX_RADIO_CHANNEL_NUM] = {-1};
    static int32_t max_val[MAX_RADIO_CHANNEL_NUM] = {-1};
    static int32_t min_val[MAX_RADIO_CHANNEL_NUM] = {-1};
    static int32_t up_val[MAX_RADIO_CHANNEL_NUM] = {-1};
    static int32_t down_val[MAX_RADIO_CHANNEL_NUM] = {-1};
    static int8_t rf_attenuation[MAX_RADIO_CHANNEL_NUM] = {-1};
    static int8_t mgc_attenuation[MAX_RADIO_CHANNEL_NUM] = {-1};

    /* 当增益模式变化时 */
    if(ctrl_method_dup[ch] != gain_ctrl_method || rf_mode_dup[ch] != rf_mode){
        ctrl_method_dup[ch] = gain_ctrl_method;
        rf_mode_dup[ch] = rf_mode;
        printf_debug("ch:%d ctrl_method:%d rf_mode:%d\n",ch,ctrl_method_dup[ch],rf_mode);
        if(gain_ctrl_method == POAL_MGC_MODE){
            /* 手动模式直接退出 */
            return -1;
        }
        else if(gain_ctrl_method == POAL_AGC_MODE){
            /* 设置半增益 */
            _set_half_attenuation_value(ch, ctx);
            /* 读取衰减目标值 */
            usleep(100000);
            dst_val[ch] = agc_ctrl_dbm;//_spm_read_signal_value(ch);
            _get_range_max(ch, rf_mode, dst_val[ch], &max_val[ch], ctx);
            _get_range_min(ch, rf_mode, dst_val[ch], &min_val[ch], ctx);
            down_val[ch]= max_val[ch] - dst_val[ch];
            up_val[ch]   = dst_val[ch] - min_val[ch];
            if(up_val[ch] < 0)
                up_val[ch] = -up_val[ch];
            printf_info("up_val=%d, down_val=%d, dst_val=%d, max_val=%d, min_val=%d\n", 
                        up_val[ch], down_val[ch], dst_val[ch], max_val[ch], min_val[ch]);
            config_read_by_cmd(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, ch, &rf_attenuation[ch]);
            config_read_by_cmd(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, ch, &mgc_attenuation[ch]);
            printf_info("rf_attenuation[ch]=%d,mgc_attenuation[ch]=%d\n", rf_attenuation[ch],mgc_attenuation[ch]);
        }
    }

    /* 非自动模式退出控制 */
    if(gain_ctrl_method != POAL_AGC_MODE){
        return -1;
    }

    if(_get_run_timer(0, ch) < agc_ctrl_time){
        /* 控制步进时间未到 */
        return -1;
    }
    _reset_run_timer(0, ch);
    usleep(1000);
    /* 读取信号强度 */
    agc_dbm_val =  _spm_read_signal_value(ch);
    usleep(1000);

    printf_info("[%d]dst_val=%d, read signal value: %d, up_val=%d, down_val=%d, rf_attenuation=%d, mgc_attenuation=%d\n", agc_ctrl_dbm,dst_val[ch], agc_dbm_val, up_val[ch], down_val[ch], rf_attenuation[ch], mgc_attenuation[ch]);

    if(agc_dbm_val < (dst_val[ch] - AGC_CTRL_PRECISION)){
        if(up_val[ch] <= 0){
            printf_note(">>>Arrived TOP<<<\n");
            return -1;
        }
        up_val[ch]--;
        down_val[ch]++;
        _set_down_step_attenuation_value(ch, ctx, &rf_attenuation[ch], &mgc_attenuation[ch]);
    }else if(agc_dbm_val > (dst_val[ch] + AGC_CTRL_PRECISION)){
        if(down_val[ch] <= 0){
            printf_note(">>>Arrived DOWN<<<\n");
            return -1;
        }
        down_val[ch]--;
        up_val[ch]++;
        _set_up_step_attenuation_value(ch, ctx, &rf_attenuation[ch], &mgc_attenuation[ch]);
    }
    
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
    
    #define AGC_CTRL_PRECISION      0       /* 控制精度+-2dbm */
    //#define AGC_REF_VAL             0x5e8 /* 信号为0DBm时对应的FPGA幅度值 */
    //#define AGC_MODE                1       /* 自动增益模式 */
    //#define MGC_MODE                0       /* 手动增益模式 */
    #define RF_GAIN_THRE            30      /* 增益到达该阀值，开启射频增益/衰减 */
    #define MID_GAIN_THRE           60      /* 增益到达该阀值，开启中频增益 */

    uint8_t gain_ctrl_method = ctx->pdata->rf_para[ch].gain_ctrl_method;
    uint16_t agc_ctrl_time= ctx->pdata->rf_para[ch].agc_ctrl_time;
    int8_t agc_ctrl_dbm = ctx->pdata->rf_para[ch].agc_mid_freq_out_level;
    uint32_t fft_size_agc = ctx->pdata->multi_freq_point_param[ch].points[0].fft_size;
    uint16_t agc_val = 0;
    int8_t dbm_val = 0;
    int ret = -1,rf_mode;
    int8_t rf_gain = 0, mid_gain = 0;
    /* 各通道初始dbm为0 */
    //static uint8_t cur_dbm[MAX_RADIO_CHANNEL_NUM] = {0};
    static int8_t ctrl_method[MAX_RADIO_CHANNEL_NUM]={-1};
    static int  rf_mode_s = -1;
    uint8_t is_cur_dbm_change = false;
    
    printf_note("gain_ctrl_method:%d agc_ctrl_time:%d agc_ctrl_dbm:%d\n", gain_ctrl_method,agc_ctrl_time,agc_ctrl_dbm);
    //rf_mode = io_get_rf_mode();
    rf_mode = ctx->pdata->rf_para[ch].rf_mode_code;

    /* 当模式变化时， 需要通知内核更新模式信息 */
    if(ctrl_method[ch] != gain_ctrl_method){
        ctrl_method[ch] = gain_ctrl_method;
        printf_note("ch:%d ctrl_method:%d rf_mode:%d fft_size_agc:%d\n",ch,ctrl_method[ch],rf_mode,fft_size_agc);
        if(gain_ctrl_method == POAL_MGC_MODE){
            //executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_CALIBRATE, ch, &fft_size_agc,0, POAL_MGC_MODE);
            goto exit_mode;
        }
        else if(gain_ctrl_method == POAL_AGC_MODE){
            //executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_CALIBRATE, ch, &fft_size_agc,0,POAL_AGC_MODE);
        }
    }



    /* 非自动模式退出控制 */
    if(agc_ctrl_time == 0 ||gain_ctrl_method != POAL_AGC_MODE){
        return -1;
    }

    
    if(rf_mode_s != rf_mode){
        rf_mode_s = rf_mode;
        printf_note("ctrl_method:%d rf_mode:%d\n",rf_mode_s,rf_mode);
        if(gain_ctrl_method == POAL_AGC_MODE){
            //executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_CALIBRATE, ch, &fft_size_agc, 0,POAL_AGC_MODE);
        }
    }

    //executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_CALIBRATE, ch, 1024, 0);
    
    if(_get_run_timer(0, ch) < agc_ctrl_time){
        return -1;
    }
    _reset_run_timer(0, ch);
    if((agc_val = io_get_agc_thresh_val(ch)) < 0){
        return -1;
    }
    printf_note("read agc ch=%d value=%d agc_ref_val_0dbm=%d \n",ch, agc_val,agc_ref_val_0dbm);
    dbm_val = (int32_t)(20 * log10((double)agc_val / ((double)agc_ref_val_0dbm)));
    
    /* 判断读取的幅度值是否>设置的输出幅度+控制精度 */
    /*
    if(dbm_val > (agc_ctrl_dbm+AGC_CTRL_PRECISION)){
        is_cur_dbm_change = true;
        if(cur_dbm[ch] > 0){
            cur_dbm[ch] --;
            //is_cur_dbm_change = true;
        }
            
    // 判断读取的幅度值是否<设置的输出幅度-控制精度 
    }else if(dbm_val < (agc_ctrl_dbm-AGC_CTRL_PRECISION)){
        cur_dbm[ch] ++;
        is_cur_dbm_change = true;
    }*/


    /* 判断读取的幅度值是否>设置的输出幅度+控制精度 */
    if(dbm_val > (agc_ctrl_dbm+AGC_CTRL_PRECISION)){
        cur_dbm[ch] ++;
        is_cur_dbm_change = true;
            
    // 判断读取的幅度值是否<设置的输出幅度-控制精度 
    }else if(dbm_val < (agc_ctrl_dbm-AGC_CTRL_PRECISION)){
        is_cur_dbm_change = true;
        if(cur_dbm[ch] > 0){
            cur_dbm[ch] --;
            is_cur_dbm_change = true;
        }
            
    /* 判断读取的幅度值是否<设置的输出幅度-控制精度 */
    }

    
    /* 若当前幅度值需要修改；则按档次开始设置 ：
        
    */
    printf_note("==>dbm_val:%d  agc_ctrl_dbm:%d is_cur_dbm_change:%d\n",dbm_val,agc_ctrl_dbm,is_cur_dbm_change);
    if(is_cur_dbm_change == true){
        //int8_t rf_gain = 0, mid_gain = 0;
        if(cur_dbm[ch] <= RF_GAIN_THRE){
            rf_gain = cur_dbm[ch];
            mid_gain = 0;
        }else if((cur_dbm[ch] > RF_GAIN_THRE)&&(cur_dbm[ch] <= MID_GAIN_THRE)){
            rf_gain = RF_GAIN_THRE;
            mid_gain = cur_dbm[ch] - RF_GAIN_THRE;
        }else{  /* cur_dbm[ch] > MID_GAIN_THRE */
            cur_dbm[ch] = MID_GAIN_THRE;
            rf_gain = RF_GAIN_THRE;
            mid_gain = RF_GAIN_THRE;
        }
        usleep(500);
        ret = executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, ch, &rf_gain);
        usleep(500);
        ret = executor_set_command(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, ch, &mid_gain);
        usleep(500);
    }
    printf_note("rf_gain:%d  mid_gain:%d cur_dbm[ch]:%d\n", rf_gain,mid_gain,cur_dbm[ch]);
exit_mode:
    #if defined(SUPPORT_SPECTRUM_KERNEL) 
    ret = executor_set_command(EX_MID_FREQ_CMD, EX_FILL_RF_PARAM, ch, &cur_dbm[ch], gain_ctrl_method);
    #endif
    return 0;
}

static int spm_sample_ctrl(uint64_t freq_hz)
{
#if defined(SUPPORT_DIRECT_SAMPLE)
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
#endif
    return 0;
}

/*Signal Residency strategy*/
static long signal_residency_policy(int ch, int policy, bool is_signal)
{
    long residence_time = -1; /* 驻留时间:秒， -1为永久驻留 */
    #define POLICY1_SWITCH      0       /* 策略1：有信号驻留；无信号等3S无信号切换下一个点 */
    #define POLICY2_PENDING     -1      /* 策略2：有信号，永久驻留，无信号驻留1秒切换      */
    #define POLICY3_WAIT_TIME   1       /* 策略3： policy>0 有信号按驻留时间驻留切换，无信号驻留NO_SIGAL_WAIT_TIME毫秒立即切换 */

    #define NO_SIGAL_WAIT_TIME  1000 /* 无信号驻留时间 */
    
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
    //printf("is_signal:%d, policy:%d\n",is_signal,policy);
    residency_time = signal_residency_policy(ch, policy, is_signal);
    if(_get_run_timer(1, ch) < residency_time*1000){
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
static int32_t _get_signal_threshold_by_amp(uint8_t ch, uint32_t index, int32_t *sigal_thred)
{
    int ret = 0;
    uint8_t mute_sw = 0;
    int8_t mute_thre_val = 0, mute_thre_db = 0;

    ret = config_read_by_cmd(EX_MID_FREQ_CMD, EX_MUTE_SW, ch, &mute_sw, index);
    if(ret != 0){
        printf_err("Read Mute sw error\n");
        return -1;
    }
    ret = config_read_by_cmd(EX_MID_FREQ_CMD, EX_MUTE_THRE, ch, &mute_thre_db, index);
    if(ret != 0){
        printf_err("Read Mute thre error\n");
        return -1;
    }

    mute_thre_val = 20;//pow(10.0f,(double)(mute_thre_db)/20);
    printf_note("mute_thre_val=%d, mute_thre_db=%dDbm\n", mute_thre_val, mute_thre_db);
    *sigal_thred = mute_thre_val;
    
    return 0;
}

/* 判断对应通道是否有信号:      true: 有信号; false:无信号*/
static int32_t  spm_get_signal_strength(uint8_t ch,uint8_t subch, uint32_t index, bool *is_singal, uint16_t *strength)
{
    uint16_t sig_amp = 0, sig_max = 0;
    int32_t sigal_thred = 0;
    int32_t ret;
    uint8_t mute_sw = 0;
    int8_t mute_thre_db = 0;
    int32_t mute_thre_val = 0.0f;
    int i = 0;
    for(i = 0; i < 3; i++){
        sig_amp = io_get_signal_strength(subch);
        if(sig_max < sig_amp){
            sig_max = sig_amp;
        }
        usleep(20000);
        printf_debug("----------sig:%d\n", sig_amp);
    }
    if(strength != NULL)
        *strength = sig_max;

    sig_amp = sig_max;
    ret = config_read_by_cmd(EX_MID_FREQ_CMD, EX_MUTE_SW, ch, &mute_sw, index);
    if (ret != 0){
        printf_err("Read Mute sw error\n");
        *is_singal = true;
        return -1;
    }
    if (mute_sw < 1){
        *is_singal = true;
        return 0;
    }

    ret = config_read_by_cmd(EX_MID_FREQ_CMD, EX_MUTE_THRE, ch, &mute_thre_db, index);
    if(ret != 0){
        printf_err("Read Mute thre error\n");
        *is_singal = true;
        return -1;
    }
    
    mute_thre_val = subch_ref_val_0dbm * pow(10.0f, (double)mute_thre_db / 20);
    printf_note("sig_amp:%d, mute_thre_val:%d\n", sig_amp, mute_thre_val);
    if (sig_amp >= mute_thre_val){
        *is_singal = true;
    }else{
        *is_singal = false;
    }

    #if 0
    ret = _get_signal_threshold_by_amp(ch, index, &sigal_thred);
    if(ret == -1){
        *is_singal = true;
        return -1;
    }
    if(sig_amp > sigal_thred) {
        *is_singal = true;
    }else{
        *is_singal = false;
    }
    #endif
    
    return 0;
}

static int spm_stream_back_running_file(enum stream_type type, int fd)
{
    void *w_addr = NULL;
    int i, rc, ret = 0;
    ssize_t total_Byte = 0;
    unsigned int size;
    write_info info;
    int try_count = 0;
    int file_fd;

    struct _spm_stream *pstream = spm_stream;

    if(fd <= 0)
        return -1;
    file_fd = fd;
try_gain:
    ioctl(pstream[type].id, IOCTL_DMA_GET_ASYN_WRITE_INFO, &info);
    if(info.status != 0){   /* NOT OK */
        printf_debug("write status:%d, block_num:%d\n", info.status, info.block_num);
        if(info.status == WRITE_BUFFER_STATUS_FAIL){
            printf_warn("read error!\n");
            exit(1);
        }else if (info.status == WRITE_BUFFER_STATUS_EMPTY){
            printf_warn("write buffer empty\n");
        }else if (info.status == WRITE_BUFFER_STATUS_FULL){
            printf_warn("write buffer full\n");
            usleep(10);
        }else{
            printf_warn("unknown status[0x%x]\n", info.status);
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
            if(++try_count >= 2){ /* if the file is read over, need to check(2times) whether the DMA is write over! */
                printf_debug("read file over.\n");
                return 0;
            }else{
                usleep(10);
                printf_note("read over try again: %d\n", try_count);
                goto try_gain;
            }
            
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
    if(pstream[type].rd_wr == DMA_READ)
        ioctl(pstream[type].id, IOCTL_DMA_ASYN_READ_START, &para);
    else
        ioctl(pstream[type].id, IOCTL_DMA_ASYN_WRITE_START, &para);
    return 0;
}

static int spm_stream_stop(enum stream_type type)
{
    struct _spm_stream *pstream = spm_stream;
    if(pstream[type].rd_wr == DMA_READ)
        ioctl(pstream[type].id, IOCTL_DMA_ASYN_READ_STOP, NULL);
    else
        ioctl(pstream[type].id, IOCTL_DMA_ASYN_WRITE_STOP, NULL);
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
    .agc_ctrl = spm_agc_ctrl_v3,//spm_agc_ctrl,
    .residency_time_arrived = is_sigal_residency_time_arrived,
    .signal_strength = spm_get_signal_strength,
    .back_running_file = spm_stream_back_running_file,
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

    if( config_get_config()->oal_config.ctrl_para.subch_ref_val_0dbm != 0)
        subch_ref_val_0dbm = config_get_config()->oal_config.ctrl_para.subch_ref_val_0dbm;
        
    _init_run_timer(MAX_RADIO_CHANNEL_NUM);
err_set_errno:
    errno = -ret;
    return ctx;

}
