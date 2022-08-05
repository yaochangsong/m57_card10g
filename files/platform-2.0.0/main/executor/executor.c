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
*  Rev 1.0   09 July 2019   yaochangsong
*  Initial revision.
******************************************************************************/
#include "config.h"

/**
 * Mutex for the set command, used by command setting related functions. 
 */
pthread_mutex_t set_cmd_mutex = PTHREAD_MUTEX_INITIALIZER;


int executor_net_disconnect_notify(struct sockaddr_in *addr)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    
#if (defined CONFIG_PROTOCOL_DATA_UDP)
    for(int type = 0; type < NET_DATA_TYPE_MAX ; type++){
        net_data_remove_client(addr, -1, type);
    }
#endif

    int clinets = get_cmd_server_clients();
    printf_note("cmd server:total client number: %d\n", clinets);
    
    if(clinets == 0){
        #if defined(CONFIG_FS_XW)
        xwfs_stop_backtrace(NULL);  /* 停止回溯 ，回到正常状 */
        #endif
        /* 关闭所有子通道 */
        uint8_t enable =0;
        uint8_t default_method = IO_DQ_MODE_IQ;
        int i = 0, ch;
        for(ch = 0; ch< MAX_RADIO_CHANNEL_NUM; ch++){
            for(i = 0; i< MAX_SIGNAL_CHANNEL_NUM; i++){
                executor_set_command(EX_MID_FREQ_CMD, EX_SUB_CH_ONOFF, ch, &enable, i);
                executor_set_command(EX_MID_FREQ_CMD, EX_SUB_CH_DEC_METHOD, i, &default_method);
                memset(&poal_config->channel[ch].sub_channel_para.sub_ch_enable[i], 0, sizeof(struct output_en_st));
                io_set_enable_command(IQ_MODE_DISABLE, ch,i, 0);
            }
            io_set_rf_calibration_source_enable(ch, 0);
            /* 所有客户端离线，关闭相关使能，线程复位到等待状�?*/
            memset(&poal_config->channel[ch].enable, 0, sizeof(poal_config->channel[ch].enable));
            poal_config->channel[ch].enable.bit_reset = true; /* reset(stop) all working task */
        }
    }
    return 0;
}

uint32_t executor_get_audio_point(uint8_t ch)
{
    struct spm_context *_ctx;
    _ctx = get_spm_ctx();
    if(_ctx == NULL)
        return 0;
    return _ctx->run_args[ch]->audio_points;
}

uint64_t executor_get_mid_freq(uint8_t ch)
{
    struct spm_context *_ctx;
    _ctx = get_spm_ctx();
    if(_ctx == NULL)
        return 0;
    return _ctx->run_args[ch]->m_freq;
}

uint32_t executor_get_bandwidth(uint8_t ch)
{
    struct spm_context *_ctx;
    if(ch >= MAX_RADIO_CHANNEL_NUM)
        return 0;
    _ctx = get_spm_ctx();
    if(_ctx == NULL)
        return 0;
    return _ctx->run_args[ch]->bandwidth;
}


static int8_t executor_get_kernel_command(uint8_t type, uint8_t ch, void *data)
{
    switch(type)
    {
        case EX_FPGA_TEMPERATURE:
            *(int16_t *)data = io_get_adc_temperature();
            break;
        default:
            printf_err("not support type[%d]\n", type);
    }
    
    return 0;
}


static int8_t executor_set_kernel_command(uint8_t type, uint8_t ch, void *data, va_list ap)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    switch(type)
     {
        case EX_CHANNEL_SELECT:
        {
            break;
        }
        case EX_MUTE_SW:
        {
            break;
        }
        case EX_MUTE_THRE:
        {
            uint32_t noise_en  = va_arg(ap, uint32_t);
            io_set_noise(ch,noise_en,*(int8_t *)data);
            break;
        }
        case EX_MID_FREQ:
        {
            uint64_t fm  = *(uint64_t *)data;
            uint64_t fp  = va_arg(ap, uint64_t);
            io_set_middle_freq(ch, fm, fp);
            break;
        }
        case EX_DEC_METHOD:
            io_set_dec_method(ch, *(uint8_t *)data);
            break;
        case EX_DEC_BW:
        {
            io_set_dec_bandwidth(ch, *(uint32_t *)data);
            break;
        }
        case EX_DEC_MID_FREQ:
        {
            uint64_t  middle_freq;
            int _ch;
            middle_freq = va_arg(ap, uint64_t);
            if(middle_freq == 0){
                middle_freq = *(uint64_t *)data;
            }
            io_set_dec_middle_freq(ch, *(uint64_t *)data, middle_freq);
            break;
        }
        case EX_WINDOW_TYPE:
        {
            io_set_window_type(ch, *(uint16_t *)data);
            break;
        } 
        case EX_SMOOTH_TIME:
        {
            io_set_smooth_time(ch, *(uint16_t *)data);
            break;
        }
        case EX_SMOOTH_TYPE:
            io_set_smooth_type(ch, *(uint16_t *)data);
            break;
        case EX_SMOOTH_THRE:
        {
            int32_t val = 0;
            val = config_get_fft_calibration_value(ch, 0, 0);
            val += *(uint16_t *)data * 2;
            //printf_debug("EX_SMOOTH_THRE val = %d, raw = %d\n", val, *(uint16_t *)data);
            io_set_smooth_threshold(ch, val);
            break;
        }
        case EX_RESIDENCE_TIME:
        {
            break;
        }
        case EX_RESIDENCE_POLICY:
        {
            break;
        }
        case EX_AUDIO_SAMPLE_RATE:
        {
            break;
        }
        case EX_FFT_SIZE:
        {
            io_set_fft_size(ch, *(uint32_t *)data);
            break;
        }
        case EX_FRAME_DROP_CNT:
        {
            break;
        }
        case EX_BANDWITH:
        {
            io_set_bandwidth(ch, *(uint32_t *)data);
            //io_set_side_rate(1.28);
            break;
        }
        case EX_FPGA_RESET:
        {
            io_reset_fpga_data_link();
            break;
        }
        case EX_FPGA_CALIBRATE:
        {
            int32_t  value = 0;
            uint32_t fftsize;
            uint64_t m_freq = 0;
            uint16_t smooth_type = 0;

            if(data !=NULL){
                fftsize = *(uint32_t *)data;
            }
            m_freq = (uint64_t)va_arg(ap, uint64_t);
            config_read_by_cmd(EX_MID_FREQ_CMD, EX_SMOOTH_TYPE, 0, &smooth_type);
            if(FFT_SMOOTH_TYPE_THRESHOLD == smooth_type){
                value = 0;
            }else {
                value = config_get_fft_calibration_value(ch, fftsize, m_freq);
            }
            //printf_debug("ch:%d, fft calibration value:%d m_freq :%d\n", ch, value,m_freq);
            io_set_calibrate_val(ch, value);
            #if defined(CONFIG_BSP_SSA) || defined(CONFIG_BSP_SSA_MONITOR)
                value = config_get_gain_calibration_value(ch, fftsize, m_freq);
                io_set_gain_calibrate_val(ch, value);
                value = config_get_dc_offset_nshift_calibration_value(ch, fftsize, m_freq);
                if(value > 0)
                    io_set_dc_offset_calibrate_val(ch, value);
            #endif
            break;
        }

        case EX_SUB_CH_ONOFF:
        {
            uint32_t subch;
            uint8_t onoff;
            subch = va_arg(ap, uint32_t);
            onoff = *(uint8_t *)data;
            io_set_subch_onoff(ch, subch, onoff);
            break;
        }
        case EX_SUB_CH_DEC_BW:
        {
            uint32_t  dec_method;
            dec_method = (uint32_t)va_arg(ap, uint32_t);
            io_set_subch_bandwidth(ch, *(uint32_t *)data, (uint8_t)dec_method);
            break;
        }
        case EX_SUB_CH_MID_FREQ:
        {
            uint64_t  middle_freq;
            int cid;
            middle_freq = va_arg(ap, uint64_t);
            if(middle_freq == 0){
                middle_freq = *(uint64_t *)data;
            }
            cid = va_arg(ap, int32_t);
            printf_debug("[ch:%d, sunch:%d]freq: %"PRIu64"Hz, data=%"PRIu64"Hz\n", cid, ch, middle_freq, *(uint64_t *)data);
            io_set_subch_dec_middle_freq(cid, ch, *(uint64_t *)data, middle_freq);
            break;
        }
        case EX_SUB_CH_DEC_METHOD:
        {
            io_set_subch_dec_method(ch, *(uint8_t *)data);
            break;
        }
        case EX_SUB_CH_MUTE_THRE:
        {
            break;
        }
        case EX_SUB_CH_MGC_GAIN:
        {
            uint32_t subch;
            int8_t val;
            subch = ch;
            val = *(int8_t *)data;
            io_set_subch_audio_gain(ch, subch, val);
            break;
        }
        case EX_SUB_CH_GAIN_MODE:
        {
            uint32_t subch;
            uint8_t val;
            subch = ch;
            val = *(uint8_t *)data;
            io_set_subch_audio_gain_mode(ch, subch, val);
            break;
        }
        case EX_SUB_CH_AUDIO_SAMPLE_RATE:
        {
            uint32_t subch;
            uint8_t val;
            subch = ch;
            val = *(uint8_t *)data;
            io_set_subch_audio_sample_rate(ch, subch, val);
            break;
        }
        case EX_SAMPLE_CTRL:
        {
            io_set_fpga_sample_ctrl(*(uint8_t *)data);
            break;
        }
        case EX_AUDIO_VOL_CTRL:
        {
            io_set_audio_volume(ch,*(uint8_t *)data);
            break;
        }
        default:
            printf_info("not support type[%d]\n", type);
     }
    return 0;
}

int8_t executor_set_command(exec_cmd cmd, uint8_t type, uint8_t ch,  void *data, ...)
{
     struct poal_config *poal_config = &(config_get_config()->oal_config);
     va_list argp;

     LOCK_SET_COMMAND();
     va_start (argp, data);
     switch(cmd)
     {
        case EX_MID_FREQ_CMD:
        {
#ifdef CONFIG_DEVICE_LCD
            lcd_printf(cmd, type,data, &ch);
#endif
            executor_set_kernel_command(type, ch, data, argp);
            break;
        }
        case EX_RF_FREQ_CMD:
        {
#ifdef CONFIG_DEVICE_LCD
            lcd_printf(cmd, type, data, &ch);
#endif
#ifdef CONFIG_DEVICE_RF
            rf_set_interface(type, ch, data);
#endif
            break;
        }
        case EX_FFT_ENABLE_CMD:
        {
            int32_t enable = 0;
            if(data != NULL)
                enable = *(int32_t *)data;
            executor_fft_work_nofity(NULL, ch, poal_config->channel[ch].work_mode, enable);
            break;
        }
        case EX_BIQ_ENABLE_CMD:
        {
            int32_t subch = -1;//va_arg(argp, int32_t);
            int32_t enable = *(int32_t *)data;
            printf_note("subch:%d, BIQ %s\n", subch, enable == 0 ? "disable" : "enable");
            if(enable){
                /* 1: stop all channnel */
                for(int i = 0; i < MAX_RADIO_CHANNEL_NUM; i++)
                    io_set_enable_command(BIQ_MODE_DISABLE, i, subch, 0);
                /* 2: start one channnel */
                io_set_enable_command(BIQ_MODE_ENABLE, ch, subch, 0);
                //spm_biq_deal_notify(&ch);
            }
            else{
                io_set_enable_command(BIQ_MODE_DISABLE, ch,subch, 0);
            }
            break;
        }
        case EX_NIQ_ENABLE_CMD:
        {
            int32_t subch = va_arg(argp, int32_t);
            int32_t enable = *(int32_t *)data;
            printf_note("ch=%d, subch:%d,enable=%d NIQ %s\n",ch, subch, enable, enable == 0 ? "disable" : "enable");
            #if 0
            if(enable){
                io_set_enable_command(IQ_MODE_ENABLE, ch, subch, 0);
                spm_niq_deal_notify(NULL);
            }
            else{
                io_set_enable_command(IQ_MODE_DISABLE, ch,subch, 0);
            }
            #else
            executor_niq_work_nofity(NULL, ch, subch, enable);
            #endif
            break;
        }
        default:
            printf_err("invalid set command[%d]\n", cmd);
     }
     va_end(argp);
     UNLOCK_SET_COMMAND();
     return 0;
}

int8_t executor_get_command(exec_cmd cmd, uint8_t type, uint8_t ch,  void *data, ...)
{    
    int ret = 0;
    va_list argp;
    va_start (argp, data);
    switch(cmd)
    {
        case EX_MID_FREQ_CMD:
        {
            executor_get_kernel_command(type, ch, data);
            break;
        }
        case EX_RF_FREQ_CMD:
        {
#ifdef CONFIG_DEVICE_RF
            ret = rf_read_interface(type, ch, data, argp);
#endif
            break;
        }
        default:
            printf_err("invalid get command[%d]\n", cmd);
    }
    va_end(argp);
    return ret;

}

void executor_timer_task1_cb(struct uloop_timeout *t)
{

}
void executor_timer_task_init(void)
{
    static  struct uloop_timeout task1_timeout;
    printf_warn("executor_timer_task\n");
    uloop_timeout_set(&task1_timeout, 5000); /* 5000 ms */
    task1_timeout.cb = executor_timer_task1_cb;
}

void executor_init(void)
{
    pthread_init();
    ch_bitmap_init();
    subch_bitmap_init();
#if defined(CONFIG_SPM_BOTTOM)
    bottom_init();
#endif
#if defined(CONFIG_SPM_STATISTICS)
    statistics_init();
#endif

    struct spm_context *spmctx = spm_init();
    if(spmctx == NULL){
        printf_err("spm create failed\n");
        exit(-1);
    }
    executor_thread_init();
#if defined(CONFIG_SPM_AGC)
    agc_init();
#endif
}

void executor_close(void)
{
    spm_close();
#if defined(CONFIG_FPGA_REGISTER)
    fpga_io_close();
#endif

#ifdef  CONFIG_DEVICE_ADC_CLOCK
    clock_adc_close();
#endif
}


