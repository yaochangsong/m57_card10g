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


typedef int16_t fft_data_type;

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


static int spm_create(void)
{
    //spm_assamble_fft_data
}

static ssize_t spm_read_iq_data(void **data)
{
    static char buffer[] = "IQ data........";
    *data = buffer;
    return sizeof(buffer);
}

static ssize_t spm_read_fft_data(void **data)
{
    static char buffer[] = "FFT data........";
    *data = buffer;
    return sizeof(buffer);
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
    struct spm_run_parm *hparam;
    float sigle_side_rate, side_rate;
    uint32_t single_sideband_size;

    if(fft_data == NULL || fft_len == 0)
        return NULL;
    hparam = (struct spm_run_parm *)arg;
    /* 1：去边带 */
    /* 获取边带率 */
    side_rate  = get_side_band_rate(hparam->bandwidth);
    sigle_side_rate = (1.0-1.0/side_rate)/2.0;
    printf_note("side_rate=%f, sigle_side_rate = %f\n", side_rate, sigle_side_rate);
    
    single_sideband_size = fft_len*sigle_side_rate;
    printf_note("single_sideband_size = %u\n", single_sideband_size);
    *order_fft_len = fft_len - 2 * single_sideband_size;
    return  fft_data + single_sideband_size;
}


static int spm_send_fft_data(void *data, size_t len, void *arg)
{
    uint8_t *ptr = NULL, *ptr_header = NULL;
    uint32_t header_len = 0;

    if(data == NULL || len == 0 || arg == NULL)
        return -1;
#ifdef SUPPORT_PROTOCAL_AKT
    struct spm_run_parm *hparam;
    hparam = (struct spm_run_parm *)arg;
    hparam->data_len = len; 
    hparam->type = SPECTRUM_DATUM_FLOAT;
    hparam->ex_type = SPECTRUM_DATUM;
    ptr_header = akt_assamble_data_frame_header_data(&header_len, arg);
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
}

static int spm_send_iq_data(void *data, size_t len, void *arg)
{
    uint8_t *ptr = NULL, *ptr_header = NULL;
    uint32_t header_len = 0;

    if(data == NULL || len == 0 || arg == NULL)
        return -1;
#ifdef SUPPORT_PROTOCAL_AKT
    struct spm_run_parm *hparam;
    hparam = (struct spm_run_parm *)arg;
    hparam->data_len = len; 
    hparam->type = BASEBAND_DATUM_IQ;
    hparam->ex_type = DEMODULATE_DATUM;
    ptr_header = akt_assamble_data_frame_header_data(&header_len, arg);
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
    nread = read(io_get_fd(), &agc_val, ch+1);
    if(nread <= 0){
        printf_err("read agc error[%d]\n", nread);
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


static int spm_close(void)
{
    return 0;
}


static const struct spm_backend_ops spm_ops = {
    .create = spm_create,
    .read_iq_data = spm_read_iq_data,
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
    .close = spm_close,
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
