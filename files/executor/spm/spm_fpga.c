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
#include "../../bsp/io.h"
#include "agc.h"


extern void *akt_assamble_data_frame_header_data(uint32_t *len, void *config);
static int spm_stream_stop(int ch, int subch, enum stream_type type);
static int spm_stream_back_stop(enum stream_type type);


#define DIRECT_FREQ_THR (200000000) /* 直采截止频率 */
#define DIRECT_BANDWIDTH (256000000)
#define DEFAULT_IQ_SEND_BYTE 512
#define DEFAULT_SUBCH_REF_VAL 0x3000

uint8_t cur_dbm[MAX_RADIO_CHANNEL_NUM] = {0};

size_t pagesize = 0;
size_t iq_send_unit_byte = DEFAULT_IQ_SEND_BYTE;    /* IQ发送长度 */
int32_t agc_ref_val_0dbm[MAX_RF_NUM];     /* 信号为0DBm时对应的FPGA幅度值 */
int32_t subch_ref_val_0dbm[MAX_RF_NUM];



FILE *_file_fd;
static  void init_write_file(char *filename)
{
    _file_fd = fopen(filename, "w+b");
    if(!_file_fd){
        printf_err("Open file error!\n");
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

#define RUN_MAX_TIMER 1
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

    return "null";
}


static struct _spm_stream spm_stream[] = {
//        {DMA_BIQ0_DEV,     -1, 0, NULL, DMA_IQ_BUFFER_SIZE,  "BIQ0 Stream",      DMA_READ, STREAM_IQ},
        {DMA_FFT_DEV,      -1, 0, NULL, DMA_BUFFER_16M_SIZE, "FFT0 Stream",    DMA_READ, STREAM_FFT},
        {DMA_NIQ_DEV,      -1,-1, NULL, DMA_IQ_BUFFER_SIZE,  "NIQ Stream",     DMA_READ, STREAM_IQ},
#if 0
        {DMA_BIQ1_DEV,        -1, 1, NULL, DMA_IQ_BUFFER_SIZE,  "BIQ1 Stream",      DMA_READ, STREAM_IQ},
        {DMA_BIQ2_DEV,        -1, 2, NULL, DMA_IQ_BUFFER_SIZE,  "BIQ2 Stream",      DMA_READ, STREAM_IQ},
        {DMA_BIQ3_DEV,        -1, 3, NULL, DMA_IQ_BUFFER_SIZE,  "BIQ3 Stream",      DMA_READ, STREAM_IQ},
        
        {DMA_FFT_DEV,         -1, 0, NULL, DMA_BUFFER_16M_SIZE, "FFT Stream",       DMA_READ, STREAM_FFT},
#endif
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

    /* create stream */
    for(i = 0; i< ARRAY_SIZE(spm_stream) ; i++){
        pstream[i].id = open(pstream[i].devname, O_RDWR);
        if( pstream[i].id < 0){
            fprintf(stderr, "[%d]open:%s, %s\n", i, pstream[i].devname, strerror(errno));
            continue;
        }
        /* first of all, stop stream */
        spm_stream_stop(-1, -1, i);
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

static ssize_t spm_stream_read(int type, void **data)
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
           // usleep(5);
            if(pstream[type].type == STREAM_FFT){
                usleep(5);
                printf_debug("[%s]no data, waitting\n", pstream[type].name);
            }
        }else if(info.status == READ_BUFFER_STATUS_OVERRUN){
            printf_warn("[%s]data is overrun\n", pstream[type].name);
        }
    }while(info.status == READ_BUFFER_STATUS_PENDING);
    readn = info.blocks[0].length;
    *data = pstream[type].ptr + info.blocks[0].offset;
    printf_debug("[%p, %p, offset=0x%x]%s, readn:%lu\n", *data, pstream[type].ptr, info.blocks[0].offset,  pstream[type].name, readn);

    return readn;
}

static inline int spm_find_index_by_type(int ch, int subch, enum stream_type type)
{
    struct _spm_stream *pstream = spm_stream;
    int i, find = 0, index = -1;

    subch = subch;
    for(i = 0; i < ARRAY_SIZE(spm_stream); i++){
        if((type == pstream[i].type) && (ch == pstream[i].ch)){
                index = i;
                find = 1;
                break;
        }
    }
    
    if(find == 0)
        return -1;
    
    return index;
}

static ssize_t spm_read_biq_data(int ch, void **data)
{
    int index;
    index = spm_find_index_by_type(ch, -1, STREAM_IQ);
    if(index < 0)
        return -1;
    
    return spm_stream_read(index, data);
}


static ssize_t spm_read_niq_data(void **data)
{
    int index;
    index = spm_find_index_by_type(-1, -1, STREAM_IQ);
    if(index < 0)
        return -1;
    
    return spm_stream_read(index, data);
}

static ssize_t spm_read_fft_data(void **data, void *args)
{
    struct spm_run_parm *run_args = args;
    
    int index, ch;
    ch = run_args->ch;
    index = spm_find_index_by_type(ch, -1, STREAM_FFT);
    if(index < 0)
        return -1;

    return spm_stream_read(index, data);
}

static ssize_t spm_read_adc_data(int ch, void **data)
{
    int index;
    index = spm_find_index_by_type(ch, -1, STREAM_ADC_READ);
    if(index < 0)
        return -1;
    
    return spm_stream_read(index, data);
}


static int spm_read_adc_over_deal(int ch, void *arg)
{
    unsigned int nwrite_byte;
    nwrite_byte = *(unsigned int *)arg;
    struct _spm_stream *pstream = spm_stream;
    int index;
    
    index = spm_find_index_by_type(ch, -1, STREAM_ADC_READ);
    if(index < 0)
        return -1;
    
    if(pstream){
        ioctl(pstream[index].id, IOCTL_DMA_SET_ASYN_READ_INFO, &nwrite_byte);
    }
    return 0;
}
        
static int spm_read_biq_over_deal(int ch, void *arg)
{
    unsigned int nwrite_byte;
    nwrite_byte = *(unsigned int *)arg;
    struct _spm_stream *pstream = spm_stream;
    int index;
    
    index = spm_find_index_by_type(ch, -1, STREAM_IQ);
    if(index < 0)
        return -1;

    if(pstream){
        ioctl(pstream[index].id, IOCTL_DMA_SET_ASYN_READ_INFO, &nwrite_byte);
    }
    return 0;
}

static int spm_read_niq_over_deal(void *arg)
{
    unsigned int nwrite_byte;
    nwrite_byte = *(unsigned int *)arg;
    struct _spm_stream *pstream = spm_stream;
    int index;
    
    index = spm_find_index_by_type(-1, -1, STREAM_IQ);
    if(index < 0)
        return -1;

    if(pstream){
        ioctl(pstream[index].id, IOCTL_DMA_SET_ASYN_READ_INFO, &nwrite_byte);
    }
    return 0;
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
static fft_t *spm_data_order(fft_t *fft_data, 
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
    printf_debug("side_rate = %f[fft_len:%lu, order_len=%lu], scan_bw=%u\n", side_rate, fft_len, order_len, run_args->scan_bw);
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
    #if 1
    memcpy((uint8_t *)run_args->fft_ptr_swap,         (uint8_t *)(fft_data) , fft_len*2);
    memcpy((uint8_t *)run_args->fft_ptr,              (uint8_t *)(run_args->fft_ptr_swap + fft_len*2 -order_len), order_len);
    memcpy((uint8_t *)(run_args->fft_ptr+order_len),  (uint8_t *)run_args->fft_ptr_swap , order_len);
    #else
    memcpy((uint8_t *)run_args->fft_ptr,                (uint8_t *)(fft_data+fft_len -order_len/2) , order_len);
    memcpy((uint8_t *)(run_args->fft_ptr+order_len),    (uint8_t *)fft_data , order_len);
    #endif
     if(run_args->mode == OAL_FAST_SCAN_MODE || run_args->mode ==OAL_MULTI_ZONE_SCAN_MODE){
#if defined(SUPPORT_DIRECT_SAMPLE)
        if((run_args->m_freq_s < DIRECT_FREQ_THR) && (run_args->m_freq_s > 0)){
            order_len = (fft_len * run_args->bandwidth)/DIRECT_BANDWIDTH;//run_args->scan_bw;
            offset = (run_args->s_freq_offset-28000000)*fft_len/DIRECT_BANDWIDTH;
            printf_warn("s_freq_offset=%"PRIu64",fft_len=%u,order_len=%u, offset=%u, bandwidth=%u\n",run_args->s_freq_offset,fft_len, order_len, offset, run_args->bandwidth);
        }else
#endif
         if(run_args->scan_bw > run_args->bandwidth){
             start_freq_hz = run_args->s_freq_offset - (run_args->m_freq_s - run_args->scan_bw/2);
             offset = (order_len * start_freq_hz)/run_args->scan_bw;
             order_len = (order_len * run_args->bandwidth)/run_args->scan_bw;
         }
     }
    *order_fft_len = order_len;
    printf_debug("order_len=%lu, offset = %u, start_freq_hz=%"PRIu64", s_freq_offset=%"PRIu64", m_freq=%"PRIu64", scan_bw=%u\n", 
        order_len, offset, start_freq_hz, run_args->s_freq_offset, run_args->m_freq_s, run_args->scan_bw);
    return (fft_t *)run_args->fft_ptr + offset;
}

/*********************************************************
    功能:FFT频谱数据发送
    输入参数：
        @data: FFT数据指针
        @fft_len： FFT数据长度（双字节）
        @arg: 属性指针
    输出参数:
        -1: 失败
        >0: 发送字节总长度
*********************************************************/
static int spm_send_fft_data(void *data, size_t fft_len, void *arg)
{
    #define HEAD_BUFFER_LEN  512 
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
    hparam->sample_rate = get_side_band_rate(hparam->scan_bw)*hparam->scan_bw;
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
    
    if(hparam->ch == 0)
        __lock_fft_send__();
    else
        __lock_fft2_send__();
    udp_send_vec_data(iov, 2, TAG_FFT);
    if(hparam->ch == 0)
        __unlock_fft_send__();
    else
        __unlock_fft2_send__();
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
    uint32_t header_len = 0, ch;

    if(data_len == 0 || arg == NULL)
        return NULL;

#ifdef SUPPORT_DATA_PROTOCAL_AKT
    struct spm_run_parm *hparam;
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    struct sub_channel_freq_para_st *sub_channel_array;

    hparam = (struct spm_run_parm *)arg;
    hparam->data_len = data_len;
    hparam->type = BASEBAND_DATUM_IQ;
    hparam->ex_type = DEMODULATE_DATUM;
    ch = hparam->ch;
    sub_channel_array = &poal_config->channel[ch].sub_channel_para;
    hparam->sub_ch_para.bandwidth_hz = sub_channel_array->sub_ch[subch].d_bandwith;
    hparam->sub_ch_para.m_freq_hz = sub_channel_array->sub_ch[subch].center_freq;
    hparam->sub_ch_para.d_method = sub_channel_array->sub_ch[subch].raw_d_method;
    hparam->sub_ch_para.sample_rate = io_get_narrowband_iq_factor(hparam->bandwidth) * hparam->sub_ch_para.bandwidth_hz; 
    
    printf_debug("ch=%d, subch = %ld, factor:%f,m_freq_hz=%"PRIu64", bandwidth:%uhz, sample_rate=%u, d_method=%d\n", 
        ch,
        subch,
        io_get_narrowband_iq_factor(hparam->bandwidth), 
        hparam->sub_ch_para.m_freq_hz,
        hparam->sub_ch_para.bandwidth_hz, 
        hparam->sub_ch_para.sample_rate,  
        hparam->sub_ch_para.d_method );
    
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

static void *_assamble_audio_header(int subch, size_t *hlen, size_t data_len, void *arg)
{
    uint8_t *ptr = NULL, *ptr_header = NULL;
    uint32_t header_len = 0;
    struct spm_run_parm *hparam;
    uint32_t i, ch;
    uint64_t d_m_freq_hz;
    uint32_t d_method;
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    hparam = (struct spm_run_parm *)arg;
    struct multi_freq_point_para_st  *points = &poal_config->channel[hparam->ch].multi_freq_point_param;
    
    if(data_len == 0 || arg == NULL)
        return NULL;

    ch = hparam->ch;
    i = executor_get_audio_point(ch);
    hparam->sub_ch_para.bandwidth_hz = points->points[i].d_bandwith;
    hparam->sub_ch_para.m_freq_hz =  points->points[i].center_freq;
    hparam->sub_ch_para.d_method = points->points[i].raw_d_method;
    hparam->sub_ch_para.sample_rate = 32000;    /* audio 32Khz */

    printf_debug("ch=%d, subch = %d, i=%d,m_freq_hz=%"PRIu64", bandwidth:%uhz, sample_rate=%u, d_method=%d\n", 
        ch,
        subch, i,
        hparam->sub_ch_para.m_freq_hz,
        hparam->sub_ch_para.bandwidth_hz, 
        hparam->sub_ch_para.sample_rate,  
        hparam->sub_ch_para.d_method );

#ifdef SUPPORT_DATA_PROTOCAL_AKT
    hparam->data_len = data_len;
    hparam->type = DIGITAL_AUTDIO;
    hparam->ex_type = DEMODULATE_DATUM;
    if((ptr_header = akt_assamble_data_frame_header_data(&header_len, arg))== NULL){
        return NULL;
    }
    
#elif defined(SUPPORT_DATA_PROTOCAL_XW)
    hparam->data_len = data_len; 
    hparam->type = DEFH_DTYPE_AUDIO;
    hparam->ex_type = DFH_EX_TYPE_DEMO;
   // hparam->sub_ch = subch;
    ptr_header = xw_assamble_frame_data(&header_len, arg);
    if(ptr_header == NULL)
        return NULL;
#endif
    *hlen = header_len;

    return ptr_header;

}

static int spm_send_dispatcher_niq(enum stream_niq_type type, iq_t *data, size_t len, void *arg)
{
    size_t header_len = 0, subch = 0;
    struct _spm_stream *pstream = spm_stream;
    size_t _send_byte = (iq_send_unit_byte > 0 ? iq_send_unit_byte : DEFAULT_IQ_SEND_BYTE);
    void *hptr = NULL;
    struct spm_run_parm *hparam;
    hparam = (struct spm_run_parm *)arg;

    if(data == NULL || len == 0 || arg == NULL)
        return -1;

    if(len < _send_byte)
        return -1;

    if(type == STREAM_NIQ_TYPE_AUDIO){
        if((hptr = _assamble_audio_header(CONFIG_AUDIO_CHANNEL, &header_len, _send_byte, arg)) == NULL){
            printf_err("assamble head error\n");
            return -1;
        }
    } else if(type == STREAM_NIQ_TYPE_RAW){
        if((hptr = _assamble_iq_header(hparam->sub_ch_index, &header_len, _send_byte, arg)) == NULL){
            printf_err("assamble head error\n");
            return -1;
        }

    } else{
        printf_err("unknow type: %d\n", type);
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
    __lock_iq_send__();
	if(type == STREAM_NIQ_TYPE_AUDIO){
		for(i = 0; i<index; i++){
	        iov[1].iov_base = pdata;
	        iov[1].iov_len = _send_byte;
	        udp_send_vec_data(iov, 2, TAG_AUDIO);
	        pdata += _send_byte;
	    }
	} else if(type == STREAM_NIQ_TYPE_RAW){
		for(i = 0; i<index; i++){
	        iov[1].iov_base = pdata;
	        iov[1].iov_len = _send_byte;
	        udp_send_vec_data(iov, 2, TAG_NIQ);
	        pdata += _send_byte;
	    }
	}
    __unlock_iq_send__();

    safe_free(hptr);
    if(hparam->dis_iq.offset[type] >= (sbyte/sizeof(iq_t))){
        hparam->dis_iq.offset[type] = hparam->dis_iq.offset[type] - (sbyte/sizeof(iq_t));
    }
    else{
        printf_err("iq offset err %u < %ld\n", hparam->dis_iq.offset[type], sbyte/sizeof(iq_t));
        hparam->dis_iq.offset[type] = 0;
    }
    
    printf_debug("%s, offset = %u, send=%u, len=%lu\n", 
            type == STREAM_NIQ_TYPE_AUDIO ? "audio" : "iq" , hparam->dis_iq.offset[type], sbyte, len);

    return 0;
}



/* 将窄带iq不同类型进行分发 */
static int spm_niq_dispatcher(iq_t *ptr_iq, size_t len, void *arg)
{
#define IQ_DATA_PACKAGE_BYTE 512
#define IQ_DATA_TYPE_PACKAGE_NUM 262144
#define AUDIO_CHANNEL CONFIG_AUDIO_CHANNEL

    #define IQ_HEADER_1 0x55aa
    #define IQ_HEADER_2 0xe700
    
    size_t offset = 0, i, index = 0;
    int subch, type;
    iq_t *ptr_offset = ptr_iq, *type_offset[STREAM_NIQ_TYPE_MAX], *iq_raw_offset;
    struct spm_run_parm *hparam;
    hparam = (struct spm_run_parm *)arg;

    offset = IQ_DATA_PACKAGE_BYTE/sizeof(iq_t);
    
    for(i = 0; i< STREAM_NIQ_TYPE_MAX; i++){
        type_offset[i] = hparam->dis_iq.ptr[i] + hparam->dis_iq.offset[i];
        hparam->dis_iq.len[i] = 0;
    }
    
    for(i = 0, index = 0; i< len; i += IQ_DATA_PACKAGE_BYTE, index++){
        if(((*ptr_offset & 0x0ff00) == IQ_HEADER_2) && (*(ptr_offset+1) == IQ_HEADER_1)){
            subch = *ptr_offset & 0x00ff;
            if(AUDIO_CHANNEL == subch){
                type = STREAM_NIQ_TYPE_AUDIO;
            }else {
                type = STREAM_NIQ_TYPE_RAW;
                hparam->sub_ch_index = subch;
            }
            memcpy(type_offset[type], ptr_offset, IQ_DATA_PACKAGE_BYTE);
            type_offset[type] += offset;
            hparam->dis_iq.len[type] += IQ_DATA_PACKAGE_BYTE;
            if(hparam->dis_iq.len[type] >= DMA_IQ_TYPE_BUFFER_SIZE) {
                printf_warn("type: %s, is overrun len[%u], %u\n",
                    type == STREAM_NIQ_TYPE_AUDIO ? "audio" : "iq" , hparam->dis_iq.len[type], DMA_IQ_TYPE_BUFFER_SIZE);
                hparam->dis_iq.len[type] = DMA_IQ_TYPE_BUFFER_SIZE;
                break;
            }
        }
        ptr_offset += offset;
    }
   
    for(i = 0; i< STREAM_NIQ_TYPE_MAX; i++){
        if(hparam->dis_iq.len[i] > 0){
            hparam->dis_iq.offset[i] +=  hparam->dis_iq.len[i]/sizeof(iq_t);
            if(hparam->dis_iq.offset[i] > DMA_IQ_TYPE_BUFFER_SIZE/sizeof(iq_t)){
                hparam->dis_iq.offset[i] = DMA_IQ_TYPE_BUFFER_SIZE/sizeof(iq_t);
                printf_warn("offset type: %s, is overrun[%u]\n",
                    i == STREAM_NIQ_TYPE_AUDIO ? "audio" : "iq" , hparam->dis_iq.offset[i]);
            }
        }
    }
    return 0;
}


int spm_send_niq_data(void *data, size_t len, void *arg)
{
    size_t header_len = 0;
    struct _spm_stream *pstream = spm_stream;
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
    __lock_iq_send__();
    for(i = 0; i<index; i++){
        iov[1].iov_base = pdata;
        iov[1].iov_len = _send_byte;
        udp_send_vec_data(iov, 2, TAG_NIQ);
        pdata += _send_byte;
    }
    __unlock_iq_send__();
    
    spm_read_niq_over_deal(&sbyte);
    safe_free(hptr);
    
    return (int)(header_len + len);
}

int spm_send_biq_data(int ch, void *data, size_t len, void *arg)
{
    size_t header_len = 0;
    struct _spm_stream *pstream = spm_stream;
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
    __lock_iq_send__();
    for(i = 0; i<index; i++){
        iov[1].iov_base = pdata;
        iov[1].iov_len = _send_byte;
        udp_send_vec_data(iov, 2, TAG_BIQ);
        pdata += _send_byte;
    }
    __unlock_iq_send__();
    
    spm_read_biq_over_deal(ch, &sbyte);
    safe_free(hptr);
    
    return (int)(header_len + len);
}


static int spm_scan(uint64_t *s_freq_offset, uint64_t *e_freq, uint32_t *scan_bw, uint32_t *bw, uint64_t *m_freq)
{
   // #define MAX_SCAN_FREQ_HZ (6000000000)
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
        _m_freq = _s_freq + _bw/2;
        //fix bug:中频超6G无信号 wzq
        if (_m_freq > MAX_SCAN_FREQ_HZ){
            _m_freq = MAX_SCAN_FREQ_HZ;
        }
        *bw = _bw;
    }
    *m_freq = _m_freq;

    return 0;
}


static int spm_sample_ctrl(void *args)
{
    _ctrl_freq(args);
    return 0;
}

static int spm_agc_ctrl(int ch, void *args)
{
    agc_ctrl(ch, args);
    return 0;
}

/*Signal Residency strategy*/
static long signal_residency_policy(int ch, int policy, bool is_signal)
{
    long residence_time = -1; /* 驻留时间:秒， -1为永久驻留 */
    #define POLICY1_SWITCH      0       /* 策略1：有信号驻留；无信号等3S无信号切换下一个点 */
    #define POLICY2_PENDING     -1      /* 策略2：有信号，永久驻留，无信号驻留NO_SIGAL_WAIT_TIME毫秒切换         */
    #define POLICY3_WAIT_TIME   1       /* 策略3： policy>0 有信号按驻留时间驻留切换，无信号驻留NO_SIGAL_WAIT_TIME毫秒立即切换 */
    #define POLICY4_NEXT        -2      /* 策略：处理一帧频谱后，马上切换到下一个频点 */
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
    #define POLICY4_NEXT        -2      /* 策略：处理一帧频谱后，马上切换到下一个频点 */
    long residency_time = 0;
    //printf("is_signal:%d, policy:%d\n",is_signal,policy);
    if(policy == POLICY4_NEXT)
        return true;
    
    residency_time = signal_residency_policy(ch, policy, is_signal);
    if(_get_run_timer(0, ch) < residency_time*1000){
        return false;
    }
    _reset_run_timer(0, ch);
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
    
    mute_thre_val = subch_ref_val_0dbm[ch] * pow(10.0f, (double)mute_thre_db / 20);
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

static int spm_stream_back_running_file(int ch, enum stream_type type, int fd)
{
    void *w_addr = NULL;
    int i, rc, ret = 0;
    ssize_t total_Byte = 0;
    unsigned int size;
    write_info info;
    int try_count = 0;
    int file_fd;
    int index;

    struct _spm_stream *pstream = spm_stream;

    if(fd <= 0)
        return -1;
    file_fd = fd;

    index = spm_find_index_by_type(ch, -1, type);
    if(index < 0)
        return -1;
try_gain:
    ioctl(pstream[index].id, IOCTL_DMA_GET_ASYN_WRITE_INFO, &info);
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
        w_addr = pstream[index].ptr + info.blocks[i].offset;
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
        ioctl(pstream[index].id, IOCTL_DMA_SET_ASYN_WRITE_INFO, &rc);
        total_Byte += rc;
        ret = total_Byte;
    }
    return ret;
}

static int spm_stream_start(int ch, int subch, uint32_t len,uint8_t continuous, enum stream_type type)
{
    struct _spm_stream *pstream = spm_stream;
    IOCTL_DMA_START_PARA para;
    int index;

    if(continuous)
        para.mode = DMA_MODE_CONTINUOUS;
    else{
        para.mode = DMA_MODE_ONCE;
        para.trans_len = len;
    }
        
    index = spm_find_index_by_type(ch, subch, type);
    if(index < 0)
        return -1;

    printf_debug("%d stream start, pstream[%d].name = %s, continuous[%d], len=%u\n",
                type,  index, pstream[index].name, continuous, len);
    
    if(pstream[index].rd_wr == DMA_READ)
        ioctl(pstream[index].id, IOCTL_DMA_ASYN_READ_START, &para);
    else
        ioctl(pstream[index].id, IOCTL_DMA_ASYN_WRITE_START, &para);

    return 0;
}

static int spm_stream_stop(int ch, int subch, enum stream_type type)
{
    struct _spm_stream *pstream = spm_stream;
    int index;
    
    index = spm_find_index_by_type(ch, subch, type);
    if(index < 0)
        return -1;

    if(pstream[index].rd_wr == DMA_READ)
        ioctl(pstream[index].id, IOCTL_DMA_ASYN_READ_STOP, NULL);
    else
        ioctl(pstream[index].id, IOCTL_DMA_ASYN_WRITE_STOP, NULL);
    printf_debug("stream_stop: %d, %s\n", index, pstream[index].name);
    return 0;
}

static int _spm_close(void *_ctx)
{
    struct spm_context *ctx = _ctx;
    struct _spm_stream *pstream = spm_stream;
    int i, ch;

    for(i = 0; i< ARRAY_SIZE(spm_stream) ; i++){
        spm_stream_stop(-1, -1, i);
        close(pstream[i].id);
    }
    for(ch = 0; ch< MAX_RADIO_CHANNEL_NUM; ch++){
        safe_free(ctx->run_args[ch]->fft_ptr);
        safe_free(ctx->run_args[ch]);
    }
    printf_note("close stream\n");
    return 0;
}


static const struct spm_backend_ops spm_ops = {
    .create = spm_create,
    .read_niq_data = spm_read_niq_data,
    .read_biq_data = spm_read_biq_data,
    .read_fft_data = spm_read_fft_data,
    .read_adc_data = spm_read_adc_data,
    .read_adc_over_deal = spm_read_adc_over_deal,
    .read_niq_over_deal = spm_read_niq_over_deal,
    .data_order = spm_data_order,
    .send_fft_data = spm_send_fft_data,
    .send_niq_data = spm_send_niq_data,
    .send_biq_data = spm_send_biq_data,
    .niq_dispatcher = spm_niq_dispatcher,
    .send_niq_type = spm_send_dispatcher_niq,
    .agc_ctrl = spm_agc_ctrl,
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
    int ret = -ENOMEM, ch;
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
    printf_note("iq send unit byte:%lu\n", iq_send_unit_byte);
    ctx->ops = &spm_ops;
    ctx->pdata = &config_get_config()->oal_config;

    for(ch = 0; ch<MAX_RF_NUM; ch++){
        if( config_get_config()->oal_config.channel[ch].rf_para.subch_ref_val_0dbm != 0)
            subch_ref_val_0dbm[ch] = config_get_config()->oal_config.channel[ch].rf_para.subch_ref_val_0dbm;
        else
            subch_ref_val_0dbm[ch] = DEFAULT_SUBCH_REF_VAL;
        
    }
    _init_run_timer(MAX_RADIO_CHANNEL_NUM);
err_set_errno:
    errno = -ret;
    return ctx;

}
