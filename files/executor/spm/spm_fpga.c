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

static int spm_stream_stop(enum stream_type type);


/* Allocate zeroed out memory */
static inline void *zalloc(size_t size)
{
	return calloc(1, size);
}


static int32_t  diff_time_us(void)
{
    static struct timeval oldTime = {0}; 
    struct timeval newTime; 
    int32_t _t_us, ntime_us; 

    if(oldTime.tv_sec == 0 && oldTime.tv_usec == 0){
        gettimeofday(&oldTime, NULL);
        return 0;
    }
    gettimeofday(&newTime, NULL);
    _t_us = (newTime.tv_sec - oldTime.tv_sec)*1000000 + (newTime.tv_usec - oldTime.tv_usec); 
    printf_debug("_t_ms=%d us!\n", _t_us);
    if(_t_us > 0)
        ntime_us = _t_us; 
    else 
        ntime_us = 0; 
    memcpy(&oldTime, &newTime, sizeof(struct timeval)); 
    printf_debug("Diff time = %u us!\n", ntime_us); 
    return ntime_us;
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


static struct _spm_stream spm_stream[STREAM_NUM] = {
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
    dma_status status;
    pstream = spm_stream;
    int i = 0;

    printf_note("SPM init.\n");
    
    /* create stream */
    for(i = 0; i< STREAM_NUM ; i++){
        pstream[i].id = open(pstream[i].devname, O_RDWR);
        if( pstream[i].id < 0){
            fprintf(stderr, "open:%s, %s\n", pstream[i].devname, strerror(errno));
            exit(-1);
        }
        /* first of all, stop stream */
        spm_stream_stop(i);
        
        ioctl(pstream[i].id, IOCTL_DMA_INIT_BUFFER, &pstream[i].len);
        pstream[i].ptr = mmap(NULL, DMA_BUFFER_SIZE, PROT_READ | PROT_WRITE,MAP_SHARED, pstream[i].id, 0);
        if (pstream[i].ptr == (void*) -1) {
            fprintf(stderr, "mmap: %s\n", strerror(errno));
            exit(-1);
        }
        printf_note("create stream[%s]: dev:%s, ptr=%p, len=%d\n", 
            pstream[i].name, pstream[i].devname, pstream[i].ptr, pstream[i].len);
        ioctl(pstream[i].id, IOCTL_DMA_GET_STATUS, &status);
        if(status != DMA_STATUS_IDLE){
            printf_err("DMA status error: %s[%d], exit!!\n", dma_str_status(status), status);
            exit(-1);
        }
        printf_note("Create [%s] OK, status:%s[%d]\n", pstream[i].name,dma_str_status(status), status);
    }
    
    return 0;
}

static ssize_t spm_stream_read(enum stream_type type, void **data)
{
    struct _spm_stream *pstream;
    dma_status status;
    pstream = spm_stream;
    read_info info;
    size_t readn = 0;

    memset(&info, 0, sizeof(read_info));
    ioctl(pstream[type].id, IOCTL_DMA_GET_STATUS, &status);
    printf_debug("DMA get [%s] status:%s[%d]\n", pstream[type].name, dma_str_status(status), status);
    if(status == DMA_STATUS_IDLE){
        return -1;
    }
    do{
        ioctl(pstream[type].id, IOCTL_DMA_GET_ASYN_READ_INFO, &info);
        printf_debug("read status:%d, block_num:%d\n", info.status, info.block_num);
        if(info.status == READ_BUFFER_STATUS_FAIL){
            printf_err("read iq data error\n");
            exit(-1);
        }else if(info.status == READ_BUFFER_STATUS_PENDING){
            usleep(10);
            printf_debug("no iq data, waitting\n");
        }else if(info.status == READ_BUFFER_STATUS_OVERRUN){
            printf_warn("iq data is overrun\n");
        }
    }while(info.status == READ_BUFFER_STATUS_PENDING);
        
    ioctl(pstream[type].id, IOCTL_DMA_GET_STATUS, &status);
    printf_debug("DMA get [%s] status:%s[%d]\n", pstream[type].name, dma_str_status(status), status);
    readn = info.blocks[0].length;
    *data = pstream[type].ptr + info.blocks[0].offset;
    printf_debug("%s, readn:%d\n", pstream[type].name, readn);

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

static  float get_side_band_rate(uint32_t bandwidth)
{
    #define DEFAULT_SIDE_BAND_RATE  (1.28)
    float side_rate = 0.0;
     /* 根据带宽获取边带率 */
    if(config_read_by_cmd(EX_CTRL_CMD, EX_CTRL_SIDEBAND,0, &side_rate, bandwidth) == -1){
        printf_note("!!!!!!!!!!!!!SideRate Is Not Set In Config File[bandwidth=%u]!!!!!!!!!!!!!\n", bandwidth);
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
    int i;
    size_t order_len = 0;

    if(fft_data == NULL || fft_len == 0){
        printf_note("null data\n");
        return NULL;
    }
        
    run_args = (struct spm_run_parm *)arg;
    
    /* 获取边带率 */
    side_rate  = get_side_band_rate(run_args->bandwidth);
    /* 去边带后FFT长度 */
    order_len = (size_t)((float)(fft_len) / side_rate);
    /* 信号倒谱 */
    /*
       原始信号（注意去除中间边带）：==>真实输出信号；
       __                     __                             ___
         \                   /              ==》             /   \
          \_______  ________/                     _________/     \_________
                  \/                                                      
                 |边带  |
    */
    memcpy((uint8_t *)run_args->fft_ptr,                (uint8_t *)(fft_data+fft_len -order_len/2) , order_len);
    memcpy((uint8_t *)(run_args->fft_ptr+order_len),    (uint8_t *)fft_data , order_len);
    *order_fft_len = order_len;
    
    return (fft_t *)run_args->fft_ptr;
}


static int spm_send_fft_data(void *data, size_t fft_len, void *arg)
{
    uint8_t *ptr = NULL, *ptr_header = NULL;
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
    ptr = (uint8_t *)safe_malloc(header_len+ data_byte_size+2);
    if (!ptr){
        printf_err("malloc failed\n");
        return -1;
    }
    memcpy(ptr, ptr_header, header_len);
    memcpy(ptr+header_len, data, data_byte_size);
    udp_send_data(ptr, header_len + data_byte_size);
#if (defined SUPPORT_DATA_PROTOCAL_XW)
    safe_free(ptr_header);
#endif
    safe_free(ptr);
    return (header_len + data_byte_size);
}

static int spm_send_iq_data(void *data, size_t len, void *arg)
{
    uint8_t *ptr = NULL, *ptr_header = NULL;
    uint32_t header_len = 0;
    struct _spm_stream *pstream = spm_stream;

    if(data == NULL || len == 0 || arg == NULL)
        return -1;
#ifdef SUPPORT_DATA_PROTOCAL_AKT
    struct spm_run_parm *hparam;
    hparam = (struct spm_run_parm *)arg;
    hparam->data_len = len; 
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

    #if 0
    if(ptr_header == NULL)
        return -1;
    udp_send_data(ptr_header, header_len);
    udp_send_data(data, len);
    #endif
    ptr = (uint8_t *)safe_malloc(header_len+ len);
    if (!ptr){
        printf_err("malloc failed\n");
        return -1;
    }
    memcpy(ptr, ptr_header, header_len);
    memcpy(ptr+header_len, data, len);
    udp_send_data(ptr, header_len + len);
    safe_free(ptr);
    /* 设置DMA已读数据块长度 */
    ioctl(pstream[STREAM_IQ].id, IOCTL_DMA_SET_ASYN_READ_INFO, &len);
    printf_note("send over %d\n", len);
    
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

/* 自动增益控制  : 该函数在频谱处理线程中，循环调用。通过不断控制，逐渐逼近设置幅度值 */
int spm_agc_ctrl(int ch, struct spm_context *ctx)
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
    #define AGC_REF_VAL             1000    /* 信号为0DBm时对应的FPGA幅度值 */
    #define AGC_MODE                1       /* 自动增益模式 */
    #define MGC_MODE                0       /* 手动增益模式 */
    #define RF_GAIN_THRE            30      /* 增益到达该阀值，开启射频增益/衰减 */
    #define MID_GAIN_THRE           60      /* 增益到达该阀值，开启中频增益 */
    
    int nread = 0, ret = -1;
    uint16_t agc_val = 0;
    uint32_t dbm_val = 0;
    /* 各通道初始dbm为0 */
    static uint8_t cur_dbm[MAX_RADIO_CHANNEL_NUM] = {0};
    static int8_t ctrl_method = -1;
    uint8_t is_cur_dbm_change = false;
    
    if(ctx == NULL)
        return -1;

    /* 当模式变化时， 需要通知内核更新模式信息 */
    if(ctrl_method != ctx->pdata->rf_para[ch].gain_ctrl_method){
        ctrl_method = ctx->pdata->rf_para[ch].gain_ctrl_method;
        if(ctx->pdata->rf_para[ch].gain_ctrl_method == MGC_MODE){
            goto exit_mode;
        }
    }
    /* 非自动模式退出控制 */
    if(ctx->pdata->rf_para[ch].agc_ctrl_time == 0 ||
        ctx->pdata->rf_para[ch].gain_ctrl_method != AGC_MODE){
            return -1;
    }
    if(diff_time_us() < ctx->pdata->rf_para[ch].agc_ctrl_time){
        return -1;
    }
    if((agc_val = io_get_agc_thresh_val(ch)) < 0){
        return -1;
    }
    printf_note("read agc value=%d\n", agc_val);
    dbm_val = (int32_t)(20 * log10((double)agc_val / ((double)AGC_REF_VAL)));
    /* 判断读取的幅度值是否>设置的输出幅度+控制精度 */
    if(dbm_val > (ctx->pdata->rf_para[ch].agc_mid_freq_out_level+AGC_CTRL_PRECISION)){
        if(cur_dbm[ch] > 0){
            cur_dbm[ch] --;
            is_cur_dbm_change = true;
        }
            
    /* 判断读取的幅度值是否<设置的输出幅度-控制精度 */
    }else if(dbm_val > (ctx->pdata->rf_para[ch].agc_mid_freq_out_level-AGC_CTRL_PRECISION)){
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
    ret = executor_set_command(EX_MID_FREQ_CMD, EX_FILL_RF_PARAM, ch, &cur_dbm[ch], ctx->pdata->rf_para[ch].gain_ctrl_method);
    return 0;
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

    for(i = 0; i< STREAM_NUM ; i++){
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
    .stream_start = spm_stream_start,
    .stream_stop = spm_stream_stop,
    .close = _spm_close,
};

struct spm_context * spm_create_fpga_context(void)
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
