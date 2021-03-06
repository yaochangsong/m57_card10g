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
#include "spm/spm.h"
#include "../bsp/io.h"
#include "../utils/bitops.h"


/**
 * Mutex for the set command, used by command setting related functions. 
 */
pthread_mutex_t set_cmd_mutex = PTHREAD_MUTEX_INITIALIZER;


int executor_net_disconnect_notify(struct sockaddr_in *addr)
{
    struct net_udp_server *srv = get_udp_server();
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    
    udp_client_delete(addr);
    /* client is 0 */
    printf_info("udp client number: %d\n", srv->nclients);
    if(srv->nclients == 0){
        #if defined(SUPPORT_XWFS)
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
            }
            io_set_rf_calibration_source_enable(ch, 0);
            /* 所有客户端离线，关闭相关使能，线程复位到等待状�?*/
            memset(&poal_config->channel[ch].enable, 0, sizeof(poal_config->channel[ch].enable));
            poal_config->channel[ch].enable.bit_reset = true; /* reset(stop) all working task */
            #ifdef SUPPORT_FS
            struct fs_context *fs_ctx;
            fs_ctx = get_fs_ctx();
            if(fs_ctx != NULL){
                fs_ctx->ops->fs_stop_save_file(ch, NULL);
            }
            #endif
        }
    }
    return 0;
}

int8_t  executor_fragment_scan(uint32_t fregment_num,uint8_t ch, work_mode_type mode, void *args)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);

    uint64_t c_freq=0, s_freq=0, e_freq=0, m_freq=0;
    uint32_t scan_bw, left_band, fftsize;
    uint32_t scan_count, i;
    uint8_t is_remainder = 0;

    struct spm_context *spmctx;
    struct spm_run_parm *r_args;
    spmctx = (struct spm_context *)args;
    r_args = spmctx->run_args[ch];

    /* 
        Step 1: 扫描次数计算
        扫描次数 = (截止频率 - 开始频�?)/中频扫描带宽，这里中频扫描带宽认为和射频带宽一�?;
    */
    s_freq = poal_config->channel[ch].multi_freq_fregment_para.fregment[fregment_num].start_freq;
    e_freq = poal_config->channel[ch].multi_freq_fregment_para.fregment[fregment_num].end_freq;
#if defined(SUPPORT_PROJECT_WD_XCR_40G)
    uint64_t s_freq_origin, e_freq_origin;
    get_origin_start_end_freq(ch, s_freq, e_freq, &s_freq_origin, &e_freq_origin);
    printf_debug("s_freq:%"PRIu64", s_freq_origin:%llu\n", s_freq, s_freq_origin);
    printf_debug("e_freq:%"PRIu64", e_freq_origin:%llu\n", e_freq, e_freq_origin);
#endif
    if(config_read_by_cmd(EX_RF_FREQ_CMD, EX_RF_MID_BW, ch, &scan_bw) == -1){
            printf_err("Error read scan bandwidth=%u\n", scan_bw);
            return -1;
    }

    uint64_t _s_freq_hz_offset, _e_freq_hz,_m_freq_hz;
    uint32_t index = 0, _bw_hz;
    static uint32_t t_fft = 0;
    _s_freq_hz_offset = s_freq;
    _e_freq_hz = e_freq;
    fftsize= poal_config->channel[ch].multi_freq_fregment_para.fregment[fregment_num].fft_size;
    executor_set_command(EX_MID_FREQ_CMD, EX_BANDWITH, ch, &scan_bw);
    executor_set_command(EX_MID_FREQ_CMD, EX_FFT_SIZE,  ch, &fftsize);
        r_args->ch = ch;
        r_args->fft_size = fftsize;
        r_args->mode = mode;
        r_args->gain_mode = poal_config->channel[ch].rf_para.gain_ctrl_method;
        r_args->gain_value = poal_config->channel[ch].rf_para.mgc_gain_value;
        r_args->rf_mode = poal_config->channel[ch].rf_para.rf_mode_code;
    do{
        r_args->s_freq_offset = _s_freq_hz_offset;
        #if defined(SUPPORT_PROJECT_WD_XCR_40G)
        r_args->s_freq = s_freq_origin;
        r_args->e_freq = e_freq_origin;
        #else
        r_args->s_freq = s_freq;
        r_args->e_freq = e_freq;
        #endif
        if(spmctx->ops->spm_scan)
        spmctx->ops->spm_scan(&_s_freq_hz_offset, &_e_freq_hz, &scan_bw, &_bw_hz, &_m_freq_hz);
        printf_debug("[%d] s_freq_offset=%"PRIu64"Hz,scan_bw=%uHz _bw_hz=%u, _m_freq_hz=%"PRIu64"Hz\n", index, r_args->s_freq_offset, scan_bw, _bw_hz, _m_freq_hz);
        r_args->scan_bw = scan_bw;
        r_args->bandwidth = _bw_hz;
        r_args->m_freq = r_args->s_freq_offset + _bw_hz/2;
        r_args->m_freq_s = _m_freq_hz;
        r_args->fft_sn = index;
        r_args->total_fft = t_fft;
        r_args->fregment_num = fregment_num;
        r_args->freq_resolution = poal_config->channel[ch].multi_freq_fregment_para.fregment[fregment_num].freq_resolution;
        printf_debug("[%d]s_freq=%"PRIu64", e_freq=%"PRIu64", scan_bw=%u, bandwidth=%u,m_freq=%"PRIu64", m_freq_s=%"PRIu64"\n", 
            index, r_args->s_freq, r_args->e_freq, r_args->scan_bw, r_args->bandwidth, r_args->m_freq, r_args->m_freq_s);
        if(spmctx->ops->sample_ctrl)
            spmctx->ops->sample_ctrl(r_args);
        executor_set_command(EX_RF_FREQ_CMD, EX_RF_MID_FREQ, ch, &_m_freq_hz);
        executor_set_command(EX_RF_FREQ_CMD, EX_RF_MODE_CODE, ch, &r_args->rf_mode);
        //executor_set_command(EX_RF_FREQ_CMD, EX_RF_LOW_NOISE, ch, &_m_freq_hz);
        executor_set_command(EX_MID_FREQ_CMD, EX_MID_FREQ,    ch, &_m_freq_hz);
        executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_CALIBRATE, ch, &fftsize, r_args->m_freq,0);
        index ++;
        if(poal_config->channel[ch].enable.psd_en){
            io_set_enable_command(PSD_MODE_ENABLE, ch, -1, r_args->fft_size);
        }
        spm_deal(args, r_args, ch);
        if(poal_config->channel[ch].enable.psd_en){
            io_set_enable_command(PSD_MODE_DISABLE, ch, -1, r_args->fft_size);
        }
        if(poal_config->channel[ch].enable.bit_reset == true){
            poal_config->channel[ch].enable.bit_reset = false;
            printf_note("receive reset task sigal......\n");
            return -1;
        }
    }while(_s_freq_hz_offset < _e_freq_hz);
    t_fft = index;

    printf_debug("Exit fregment scan function\n");
    return 0;
}

static double difftime_us_val(const struct timeval *start, const struct timeval *end)
{
    double d;
    time_t s;
    suseconds_t u;
    s = end->tv_sec - start->tv_sec;
    u = end->tv_usec - start->tv_usec;
    d = s;
    d *= 1000000.0;//1 �� = 10^6 ΢��
    d += u;
    return d;
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

int8_t  executor_points_scan(uint8_t ch, work_mode_type mode, void *args)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    uint32_t points_count, i;
    uint64_t c_freq, s_freq, e_freq, m_freq;
    struct spm_context *spmctx;
    struct spm_run_parm *r_args;
    struct multi_freq_point_para_st *point;
    time_t s_time;
    int8_t subch = 0;
    struct timeval beginTime, endTime;
    double  diff_time_us = 0;
    bool residency_time_arrived = false;
    //int32_t policy = poal_config->ctrl_para.residency.policy[ch];
    int32_t policy = poal_config->channel[ch].multi_freq_point_param.residence_time;
    spmctx = (struct spm_context *)args;
    r_args = spmctx->run_args[ch];

    point = &poal_config->channel[ch].multi_freq_point_param;
    points_count = point->freq_point_cnt;
    printf_note("residence_time=%d, points_count=%d\n", point->residence_time, points_count);
    for(i = 0; i < points_count; i++){
        printf_info("Scan Point [%d]......\n", i);
        s_freq = point->points[i].center_freq - point->points[i].bandwidth/2;
        e_freq = point->points[i].center_freq + point->points[i].bandwidth/2;
        r_args->ch = ch;
        r_args->fft_sn = i;
        r_args->audio_points = i;
        r_args->s_freq = s_freq;
        r_args->e_freq = e_freq;
        r_args->total_fft = points_count;
        r_args->fft_size = point->points[i].fft_size;
        r_args->bandwidth = point->points[i].bandwidth;
        r_args->m_freq = point->points[i].center_freq;
        r_args->mode = mode;
        
        /* fixed bug for IQ &audio decode type error */
        r_args->d_method = 0;
        r_args->scan_bw = point->points[i].bandwidth;
        r_args->gain_mode = poal_config->channel[ch].rf_para.gain_ctrl_method;
        r_args->gain_value = poal_config->channel[ch].rf_para.mgc_gain_value;
        r_args->freq_resolution = (float)point->points[i].bandwidth * BAND_FACTOR / (float)point->points[i].fft_size;
        printf_info("ch=%d, s_freq=%"PRIu64", e_freq=%"PRIu64" fft_size=%u, d_method=%d\n", ch, s_freq, e_freq, r_args->fft_size,r_args->d_method);
        printf_info("rf scan bandwidth=%u, middlebw=%u, m_freq=%"PRIu64", freq_resolution=%f\n",r_args->scan_bw,r_args->bandwidth , r_args->m_freq, r_args->freq_resolution);
        if(spmctx->ops->sample_ctrl)
            spmctx->ops->sample_ctrl(r_args);
        executor_set_command(EX_RF_FREQ_CMD,  EX_RF_MID_FREQ, ch, &point->points[i].center_freq);
        executor_set_command(EX_MID_FREQ_CMD, EX_BANDWITH, ch, &point->points[i].bandwidth);
        executor_set_command(EX_MID_FREQ_CMD, EX_MID_FREQ,    ch, &point->points[i].center_freq);
        //executor_set_command(EX_RF_FREQ_CMD,  EX_RF_LOW_NOISE, ch, &point->points[i].center_freq);
        //executor_set_command(EX_RF_FREQ_CMD,  EX_RF_MID_BW,   ch, &r_args->.scan_bw);
        //executor_set_command(EX_MID_FREQ_CMD, EX_MID_FREQ,    ch, &point->points[i].center_freq);
        executor_set_command(EX_MID_FREQ_CMD, EX_FFT_SIZE, ch, &point->points[i].fft_size);
        /* 根据带宽设置边带�?*/
        //executor_set_command(EX_CTRL_CMD, EX_CTRL_SIDEBAND, ch, &r_args->scan_bw);
        printf_note("d_center_freq=%"PRIu64", d_method=%d, d_bandwith=%u noise=%d noise_en=%d volume=%d\n",poal_config->channel[ch].multi_freq_point_param.points[i].center_freq,
                    poal_config->channel[ch].multi_freq_point_param.points[i].d_method,poal_config->channel[ch].multi_freq_point_param.points[i].d_bandwith,
                    poal_config->channel[ch].multi_freq_point_param.points[i].noise_thrh,poal_config->channel[ch].multi_freq_point_param.points[i].noise_en,
                    poal_config->channel[ch].multi_freq_point_param.points[i].audio_volume);
        if(poal_config->channel[ch].enable.audio_en){
            subch = CONFIG_AUDIO_CHANNEL;
            executor_set_command(EX_MID_FREQ_CMD, EX_DEC_METHOD, subch, &poal_config->channel[ch].multi_freq_point_param.points[i].d_method);
            executor_set_command(EX_MID_FREQ_CMD, EX_DEC_BW, subch, &poal_config->channel[ch].multi_freq_point_param.points[i].d_bandwith);
            executor_set_command(EX_MID_FREQ_CMD, EX_DEC_MID_FREQ, subch,&poal_config->channel[ch].multi_freq_point_param.points[i].center_freq,poal_config->channel[ch].multi_freq_point_param.points[i].center_freq);
            executor_set_command(EX_MID_FREQ_CMD, EX_MUTE_THRE, subch,&poal_config->channel[ch].multi_freq_point_param.points[i].noise_thrh,poal_config->channel[ch].multi_freq_point_param.points[i].noise_en);
            
            executor_set_command(EX_MID_FREQ_CMD, EX_AUDIO_VOL_CTRL, subch,&point->points[i].audio_volume);
            io_set_enable_command(AUDIO_MODE_ENABLE, ch, -1, 0);  //音频通道开�?
            spm_niq_deal_notify(NULL);
        }else{
            io_set_enable_command(AUDIO_MODE_DISABLE, ch, -1, 0);  //音频通道开�?
        }
        
#if defined (SUPPORT_RESIDENCY_STRATEGY) 
        uint16_t strength = 0;
        bool is_signal = false;
        int32_t ret = -1;
        if(is_rf_calibration_source_enable() == false){
            usleep(20000);
            if(spmctx->ops->signal_strength){
            ret = spmctx->ops->signal_strength(ch, subch, i, &is_signal, &strength);
            if(ret == 0){
                printf_note("is sigal: %s, strength:%d\n", (is_signal == true ? "Yes":"No"), strength);
                }
            }
        }
#endif
        gettimeofday(&beginTime, NULL);
        do{
            executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_CALIBRATE, ch, &point->points[0].fft_size, r_args->m_freq);
            if(poal_config->channel[ch].enable.psd_en){
                io_set_enable_command(PSD_MODE_ENABLE, ch, -1, point->points[i].fft_size);
            }
            if(spmctx->ops->agc_ctrl)
                spmctx->ops->agc_ctrl(ch, spmctx);
            if(args != NULL)
                spm_deal(args, r_args, ch);
            if(poal_config->channel[ch].enable.psd_en){
                io_set_enable_command(PSD_MODE_DISABLE, ch, -1, point->points[i].fft_size);
            }
            if((poal_config->channel[ch].enable.bit_en == 0) || 
               poal_config->channel[ch].enable.bit_reset == true){
                    printf_warn("bit_reset...[%d, %d]\n", poal_config->channel[ch].enable.bit_en, poal_config->channel[ch].enable.bit_reset);
                    poal_config->channel[ch].enable.bit_reset = false;
                    return -1;
            }
#if defined (SUPPORT_RESIDENCY_STRATEGY) 
            /* 驻留时间是否到达判断;多频点模式下生效 */
            if(spmctx->ops->residency_time_arrived && (ret == 0) && (points_count > 1)){
                policy = poal_config->channel[ch].multi_freq_point_param.residence_time/1000;
                residency_time_arrived = spmctx->ops->residency_time_arrived(ch, policy, is_signal);
            }else
#endif
            {
                gettimeofday(&endTime, NULL);
                diff_time_us = difftime_us_val(&beginTime, &endTime);
                residency_time_arrived = (diff_time_us < point->residence_time*1000) ? false : true;
            }
        }while((residency_time_arrived == false) ||  /* multi-frequency switching */
               points_count == 1);                             /* single-frequency station */
    }
    printf_info("Exit points scan function\n");
    return 0;
}

int8_t  executor_serial_points_scan(uint8_t ch, work_mode_type mode, void *args)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    uint32_t points_count, i = 0;
    uint64_t c_freq, s_freq, e_freq, m_freq;
    struct spm_context *spmctx;
    struct spm_run_parm *r_args;
    struct multi_freq_point_para_st *point;
    time_t s_time;
    int8_t subch = 0;
    struct timeval beginTime, endTime;
    double  diff_time_us = 0;
    bool residency_time_arrived = false;
    //int32_t policy = poal_config->ctrl_para.residency.policy[ch];
    int32_t policy = poal_config->channel[ch].multi_freq_point_param.residence_time;
    spmctx = (struct spm_context *)args;
    r_args = spmctx->run_args[ch];

    point = &poal_config->channel[ch].multi_freq_point_param;
    
    i = 0;
    points_count = 1;
    s_freq = point->points[i].center_freq - point->points[i].bandwidth/2;
    e_freq = point->points[i].center_freq + point->points[i].bandwidth/2;
    r_args->ch = ch;
    r_args->fft_sn = i;
    r_args->audio_points = i;
    r_args->s_freq = s_freq;
    r_args->e_freq = e_freq;
    r_args->total_fft = points_count;
    r_args->fft_size = point->points[i].fft_size;
    r_args->bandwidth = point->points[i].bandwidth;
    r_args->m_freq = point->points[i].center_freq;
    r_args->mode = mode;
    
    /* fixed bug for IQ &audio decode type error */
    r_args->d_method = 0;
    r_args->scan_bw = point->points[i].bandwidth;
    r_args->gain_mode = poal_config->channel[ch].rf_para.gain_ctrl_method;
    r_args->gain_value = poal_config->channel[ch].rf_para.mgc_gain_value;
    r_args->freq_resolution = (float)point->points[i].bandwidth * BAND_FACTOR / (float)point->points[i].fft_size;
    printf_debug("ch=%d, s_freq=%"PRIu64", e_freq=%"PRIu64" fft_size=%u, d_method=%d\n", ch, s_freq, e_freq, r_args->fft_size,r_args->d_method);
    printf_debug("rf scan bandwidth=%u, middlebw=%u, m_freq=%"PRIu64", freq_resolution=%f\n",r_args->scan_bw,r_args->bandwidth , r_args->m_freq, r_args->freq_resolution);
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
    if(spmctx->ops->sample_ctrl)
        spmctx->ops->sample_ctrl(r_args);
    executor_set_command(EX_RF_FREQ_CMD,  EX_RF_MID_FREQ, 0, &point->points[i].center_freq);
    executor_set_command(EX_MID_FREQ_CMD, EX_BANDWITH, 0, &point->points[i].bandwidth);
    executor_set_command(EX_MID_FREQ_CMD, EX_MID_FREQ,    0, &point->points[i].center_freq);
    executor_set_command(EX_MID_FREQ_CMD, EX_FFT_SIZE, 0, &point->points[i].fft_size);
    executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_CALIBRATE, 0, &point->points[0].fft_size, r_args->m_freq);
    executor_set_command(EX_MID_FREQ_CMD, EX_SMOOTH_TIME, 0, &point->smooth_time);
    io_set_enable_command(PSD_MODE_ENABLE, -1, -1, point->points[i].fft_size);
    if(spmctx->ops->agc_ctrl)
        spmctx->ops->agc_ctrl(ch, spmctx);
    if(args != NULL)
        spm_deal(args, r_args, -1);
    io_set_enable_command(PSD_MODE_DISABLE, -1, -1, point->points[i].fft_size);
#endif
    return 0;
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
            io_set_middle_freq(ch, *(uint64_t *)data);
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
            middle_freq = va_arg(ap, uint64_t);
            if(middle_freq == 0){
                middle_freq = *(uint64_t *)data;
            }
            printf_note("ch=%d, dec_middle=%"PRIu64", middle_freq=%"PRIu64"\n", ch, *(uint64_t *)data, middle_freq);
            io_set_dec_middle_freq(ch, *(uint64_t *)data, middle_freq);
            break;
        }
        case EX_DEC_RAW_DATA:
        {
            uint64_t  middle_freq;
            uint32_t bandwidth;
            uint8_t  d_method;
            middle_freq = *(uint64_t *)data;
            bandwidth = va_arg(ap, uint32_t);
            d_method = (uint8_t)va_arg(ap, uint32_t);
            io_set_dec_parameter(ch, middle_freq, d_method, bandwidth);
            break;
        }
        case EX_SMOOTH_TIME:
        {
            io_set_smooth_time(ch, *(uint16_t *)data);
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
            

            if(data !=NULL){
                fftsize = *(uint32_t *)data;
            }
            m_freq = (uint64_t)va_arg(ap, uint64_t);
            value = config_get_fft_calibration_value(ch, fftsize, m_freq);
            //printf_debug("ch:%d, fft calibration value:%d m_freq :%d\n", ch, value,m_freq);
            io_set_calibrate_val(ch, value);
            #if defined(SUPPORT_PROJECT_SSA) || defined(SUPPORT_PROJECT_SSA_MONITOR)
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
            middle_freq = va_arg(ap, uint64_t);
            io_set_subch_dec_middle_freq(ch, *(uint64_t *)data, middle_freq);
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
            printf_err("not support type[%d]\n", type);
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
#ifdef SUPPORT_LCD
            lcd_printf(cmd, type,data, &ch);
#endif
            executor_set_kernel_command(type, ch, data, argp);
            break;
        }
        case EX_RF_FREQ_CMD:
        {
#ifdef SUPPORT_LCD
            lcd_printf(cmd, type, data, &ch);
#endif
#ifdef SUPPORT_RF
            rf_set_interface(type, ch, data);
#endif
            break;
        }
        case EX_FFT_ENABLE_CMD:
        {
#ifdef SUPPORT_SPECTRUM_SERIAL
            int32_t enable = *(int32_t *)data;
            if(enable)
                ch_bitmap_set(ch, CH_TYPE_FFT);
            else
                ch_bitmap_clear(ch, CH_TYPE_FFT);
#else
            if(type == PSD_MODE_DISABLE){
                poal_config->channel[ch].enable.psd_en = 0;
                INTERNEL_ENABLE_BIT_SET(poal_config->channel[ch].enable.bit_en,poal_config->channel[ch].enable);
            }
            if((get_spm_ctx()!= NULL) && get_spm_ctx()->ops->set_flush_trigger)
                get_spm_ctx()->ops->set_flush_trigger(true);
            /* notify thread to deal data */
            poal_config->channel[ch].enable.bit_reset = true; /* reset(stop) all working task */
#endif
            if(ch < MAX_RADIO_CHANNEL_NUM){
                printf_note("ch=%d, notify thread to deal fft data\n", ch);
                spm_fft_deal_notify(&ch);
            }
            break;
        }
        case EX_BIQ_ENABLE_CMD:
        {
            int32_t subch = -1;//va_arg(argp, int32_t);
            int32_t enable = *(int32_t *)data;
            printf_note("subch:%d, BIQ %s\n", subch, enable == 0 ? "disable" : "enable");
            if(enable){
                io_set_enable_command(BIQ_MODE_ENABLE, ch, subch, 0);
                spm_biq_deal_notify(&ch);
            }
            else
                io_set_enable_command(BIQ_MODE_DISABLE, ch,subch, 0);
            break;
        }
        case EX_NIQ_ENABLE_CMD:
        {
            int32_t subch = va_arg(argp, int32_t);
            int32_t enable = *(int32_t *)data;
            printf_note("ch=%d, subch:%d,enable=%d NIQ %s\n",ch, subch, enable, enable == 0 ? "disable" : "enable");
            if(enable){
                io_set_enable_command(IQ_MODE_ENABLE, ch, subch, 0);
                spm_niq_deal_notify(NULL);
            }
            else{
                io_set_enable_command(IQ_MODE_DISABLE, ch,subch, 0);
            }
            
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
#ifdef SUPPORT_RF
            rf_read_interface(type, ch, data, argp);
#endif
            break;
        }
        default:
            printf_err("invalid get command[%d]\n", cmd);
    }
    va_end(argp);
    return 0;

}

int8_t executor_set_enable_command(uint8_t ch)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    int i;
    
    printf_debug("bit_en[%x]\n", poal_config->channel[ch].enable.bit_en);
    if(poal_config->channel[ch].enable.bit_en == 0){
        printf_info("all Work disabled, waite thread stop...\n");
    }else{
        printf_debug("akt_assamble work_mode[%d]\n", poal_config->channel[ch].work_mode);
        printf_debug("bit_en=%x,psd_en=%d, audio_en=%d,iq_en=%d\n", poal_config->channel[ch].enable.bit_en, 
            poal_config->channel[ch].enable.psd_en,poal_config->channel[ch].enable.audio_en,poal_config->channel[ch].enable.iq_en);
        switch (poal_config->channel[ch].work_mode)
        {
            case OAL_FIXED_FREQ_ANYS_MODE:
            {
                uint32_t bandwidth = 0;
                printf_debug("freq_point_cnt=%d\n", poal_config->channel[ch].multi_freq_point_param.freq_point_cnt);
                executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, ch, &poal_config->channel[ch].rf_para.attenuation);
                executor_set_command(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, ch, &poal_config->channel[ch].rf_para.mgc_gain_value);
                printf_note("ch=%d,attenuation=%d, mgc_gain_value=%d\n",ch, poal_config->channel[ch].rf_para.attenuation, 
                    poal_config->channel[ch].rf_para.mgc_gain_value);
                //executor_set_command(EX_RF_FREQ_CMD, EX_RF_MID_FREQ, ch, &poal_config->channel[ch].rf_para.mid_freq);
                //executor_set_command(EX_RF_FREQ_CMD, EX_RF_MID_BW, ch, &poal_config->channel[ch].rf_para.mid_bw);
                //executor_set_command(EX_MID_FREQ_CMD, EX_BANDWITH, ch, &poal_config->channel[ch].rf_para.mid_bw);
                //executor_set_command(EX_MID_FREQ_CMD, EX_CHANNEL_SELECT, ch, &ch);
                executor_set_command(EX_MID_FREQ_CMD, EX_SMOOTH_TIME, ch, &poal_config->channel[ch].multi_freq_point_param.smooth_time);
                //executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_CALIBRATE, ch, NULL);
                break;
            }
            case OAL_FAST_SCAN_MODE:
            {
                uint32_t bw;
                struct multi_freq_fregment_para_st *frp = &poal_config->channel[ch].multi_freq_fregment_para;
                struct rf_para_st *rf = &poal_config->channel[ch].rf_para;
                
                executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, ch, &poal_config->channel[ch].rf_para.attenuation);
                executor_set_command(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, ch, &poal_config->channel[ch].rf_para.mgc_gain_value);
                //executor_set_command(EX_RF_FREQ_CMD, EX_RF_MID_FREQ, ch, &poal_config->channel[ch].rf_para.mid_freq);
                executor_set_command(EX_RF_FREQ_CMD,EX_RF_MID_FREQ,ch,&poal_config->channel[ch].rf_para.mid_freq);
                executor_set_command(EX_RF_FREQ_CMD,EX_RF_MID_BW,ch,NULL);
                /* 中频带宽和射频带宽一�?*/
                //executor_set_command(EX_MID_FREQ_CMD, EX_CHANNEL_SELECT, ch, &ch);
                executor_set_command(EX_MID_FREQ_CMD, EX_SMOOTH_TIME, ch,&frp->smooth_time);
                //executor_set_command(EX_MID_FREQ_CMD, EX_BANDWITH, ch,&bw);
               // executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_CALIBRATE, ch, NULL);
                executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_RESET, ch, NULL);
                break;
            }
            case OAL_MULTI_ZONE_SCAN_MODE:
            {
                uint32_t bandwidth = 0;
                executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, ch, &poal_config->channel[ch].rf_para.attenuation);
                executor_set_command(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, ch, &poal_config->channel[ch].rf_para.mgc_gain_value);
                executor_set_command(EX_MID_FREQ_CMD, EX_SMOOTH_TIME, ch, &poal_config->channel[ch].multi_freq_fregment_para.smooth_time);
                //executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_CALIBRATE, ch, NULL);
                //executor_set_command(EX_MID_FREQ_CMD, EX_CHANNEL_SELECT, ch, &ch);
                executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_RESET, ch, NULL);
                break;
            }
            case OAL_MULTI_POINT_SCAN_MODE:
                executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, ch, &poal_config->channel[ch].rf_para.attenuation);
                executor_set_command(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, ch, &poal_config->channel[ch].rf_para.mgc_gain_value);
                //executor_set_command(EX_MID_FREQ_CMD, EX_BANDWITH, ch, &poal_config->channel[ch].rf_para.mid_bw);
                executor_set_command(EX_MID_FREQ_CMD, EX_SMOOTH_TIME, ch, &poal_config->channel[ch].multi_freq_point_param.smooth_time);
                //executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_CALIBRATE, ch, NULL);
               // executor_set_command(EX_MID_FREQ_CMD, EX_CHANNEL_SELECT, ch, &ch);
                executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_RESET, ch, NULL);
                break;
            default:
                printf_warn("Work Mode Not Set!!!!!\n");
                return -1;
        }
    }
    executor_set_command(EX_ENABLE_CMD, 0, ch, NULL);
    return 0;
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
    #if 0
    if(!is_spectrum_continuous_mode()){ 
        printf_note("timer task: fft data send opened:%d\n", is_spectrum_continuous_mode());
        task1_timeout.cb = executor_timer_task1_cb;
        uloop_timeout_set(&task1_timeout, 5000); /* 5000 ms */
        printf_note("executor_timer_task ok\n");
    }else{
        printf_note("timer task: fft data send shutdown:%d\n", is_spectrum_continuous_mode());
    }
    #endif
}

void executor_init(void)
{
    int ret, i, j;
    static int ch[MAX_RADIO_CHANNEL_NUM];
    pthread_t work_id;
    void *spmctx;
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    ch_bitmap_init();
    subch_bitmap_init();
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2) 
    spmctx = spm_init();
    if(spmctx == NULL){
        printf_err("spm create failed\n");
        exit(-1);
    }
#endif
#endif /* SUPPORT_PLATFORM_ARCH_ARM */
   // executor_timer_task_init();
    /* set default network */
    /* shutdown all channel */
    for(i = 0; i<MAX_RADIO_CHANNEL_NUM ; i++){
        executor_set_command(EX_RF_FREQ_CMD, EX_RF_MID_BW, i, NULL);
        io_set_enable_command(PSD_MODE_DISABLE, i, -1, 0);
        io_set_enable_command(AUDIO_MODE_DISABLE, i, -1, 0);
        io_stop_backtrace_file(&i);
        //io_set_enable_command(FREQUENCY_BAND_ENABLE_DISABLE, i, 0);
        //executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, 0, &poal_config->rf_para[i].attenuation);
    }
    
    printf_note("clear all sub ch\n");
    uint8_t enable =0;
    uint8_t default_method = IO_DQ_MODE_IQ;

    for(i = 0; i <MAX_RADIO_CHANNEL_NUM; i++){
        for(j = 0; j< MAX_SIGNAL_CHANNEL_NUM; j++){
            printf_debug("clear all ch: %d, subch: %d\n", i, j);
            executor_set_command(EX_MID_FREQ_CMD, EX_SUB_CH_ONOFF, i, &enable, j);
            executor_set_command(EX_MID_FREQ_CMD, EX_SUB_CH_DEC_METHOD, j, &default_method);
        }
    }
}

void executor_close(void)
{
#ifdef SUPPORT_SPECTRUM_V2
    spm_close();
#if defined(SUPPORT_SPECTRUM_FPGA)
    fpga_io_close();
#endif

#endif
#ifdef  SUPPORT_CLOCK_ADC
    clock_adc_close();
#endif
}


