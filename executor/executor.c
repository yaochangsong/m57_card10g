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


struct sem_st work_sem;

static int8_t executor_wait_kernel_deal(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        printf_note("clock_gettime failed\n");
        sem_wait(&work_sem.kernel_sysn);
    }else{
        ts.tv_sec += DEFAULT_FFT_TIMEOUT;
        if(sem_timedwait(&work_sem.kernel_sysn, &ts) == -1){
            printf_note("sem waited timeout\n");
        }
    }
    return 0;
}

static inline void  executor_fregment_scan(uint8_t ch)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);

    uint64_t c_freq, s_freq, e_freq, m_freq;
    uint32_t scan_bw, left_band;
    uint32_t scan_count, i;
    uint8_t is_remainder = 0;

    /* we need assamble pakege for kernel */
    struct kernel_header_param fast_param;

    /* 
        Step 1: 扫描次数计算
        扫描次数 = (截止频率 - 开始频率 )/中频扫描带宽，这里中频扫描带宽认为和射频带宽一样 ;
    */
    s_freq = poal_config->multi_freq_fregment_para[ch].fregment[0].start_freq;
    e_freq = poal_config->multi_freq_fregment_para[ch].fregment[0].end_freq;
    scan_bw = poal_config->rf_para[ch].mid_bw;
    
    if(e_freq < s_freq || scan_bw <= 0){
        printf_err("frequency error,c_freq=%llu, scan_bw=%u\n", c_freq, scan_bw);
        return;
    }
    
    c_freq = (e_freq - s_freq);
    scan_count = c_freq /scan_bw;

    /* 扫描次数不是整数倍, 需要向上取整 */
    if(c_freq % scan_bw){
        is_remainder = 1;
    }
    printf_info("e_freq=%llu, s_freq=%llu\n", e_freq, s_freq);
    printf_info("scan_bw =%u,scan_count=%d, is_remainder=%d\n", scan_bw, scan_count, is_remainder);
    /* 
           Step 2: 根据扫描带宽， 从开始频率到截止频率循环扫描
   */
    for(i = 0; i < scan_count + is_remainder; i++){
        if(i < scan_count){
            /* 计算扫描中心频率 */
            m_freq = s_freq + i * scan_bw + scan_bw/2;
        }
        else{
            /* 若不是整数，需计算剩余带宽中心频率 */ */
            left_band = e_freq - s_freq + i * scan_bw;
            m_freq = s_freq + i * scan_bw + left_band/2;
        }
        fast_param.ch = ch;
        fast_param.fft_sn = i;
        fast_param.total_fft = scan_count + is_remainder;
        fast_param.m_freq = m_freq;
        executor_set_command(EX_RF_FREQ_CMD, EX_RF_MID_FREQ, ch, &m_freq);
        executor_set_command(EX_WORK_MODE_CMD, EX_FAST_SCAN_MODE, ch, &fast_param);
    }
}

static inline void  executor_points_scan(uint8_t ch)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    uint32_t points_count, i;
    uint64_t c_freq, s_freq, e_freq, m_freq;
    struct kernel_header_param header_param;
    struct multi_freq_point_para_st *point;
    time_t s_time;

    point = &poal_config->multi_freq_point_param[ch];
    points_count = point->freq_point_cnt;
    
    printf_info("residence_time=%d\n", point->residence_time);
    for(i = 0; i < points_count; i++){
        s_freq = point->points[i].center_freq - point->points[i].bandwidth/2;
        e_freq = point->points[i].center_freq + point->points[i].bandwidth/2;
        header_param.ch = ch;
        header_param.fft_sn = i;
        header_param.s_freq = s_freq;
        header_param.e_freq = e_freq;
        header_param.total_fft = points_count;
        header_param.freq_resolution = (float)point->points[i].bandwidth * BAND_FACTOR / (float)point->points[i].fft_size;
        executor_set_command(EX_RF_FREQ_CMD,  EX_RF_MID_BW,   ch, &point->points[i].bandwidth);
        executor_set_command(EX_RF_FREQ_CMD,  EX_RF_MID_FREQ, ch, &point->points[i].center_freq);
        executor_set_command(EX_MID_FREQ_CMD, EX_MID_FREQ,    ch, &point->points[i].center_freq);
        executor_set_command(EX_WORK_MODE_CMD,EX_FAST_SCAN_MODE, ch, &header_param);
        if(poal_config->enable.audio_en || poal_config->enable.iq_en){
            io_set_enable_command(AUDIO_MODE_ENABLE, ch, point->points[i].fft_size);
        }else{
            io_set_enable_command(AUDIO_MODE_DISABLE, ch, point->points[i].fft_size);
        }
        if(poal_config->enable.psd_en){
            executor_set_command(EX_MID_FREQ_CMD, EX_FFT_SIZE, ch, &point->points[i].fft_size);
            executor_set_command(EX_MID_FREQ_CMD, EX_DEC_BW, ch, &point->points[i].fft_size); 
        }else{
            io_set_enable_command(PSD_MODE_DISABLE, ch, point->points[i].fft_size);
        }
        s_time = time(NULL);
        do{
            if(poal_config->enable.psd_en){
                io_set_enable_command(PSD_MODE_ENABLE, ch, point->points[i].fft_size);
                executor_wait_kernel_deal();
            }
            if(poal_config->enable.bit_en == 0){
                break;
            }
        }while(time(NULL) < s_time + point->residence_time);
    }
}


void executor_work_mode_thread(void *arg)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    uint32_t fft_size;
    uint8_t ch = poal_config->enable.cid;
    uint8_t sub_ch = poal_config->enable.sub_id;
    uint32_t i;
    
    while(1)
    {
loop:   printf_info("######wait to deal work######\n");
        sem_wait(&work_sem.notify_deal);
        ch = poal_config->enable.cid;
        printf_info("receive notify, [Channel:%d]%s Work: [%s], [%s], [%s]\n", 
                     ch,
                     poal_config->enable.bit_en == 0 ? "Stop" : "Start",
                     poal_config->enable.psd_en == 0 ? "Psd Stop" : "Psd Start",
                     poal_config->enable.audio_en == 0 ? "Audio Stop" : "Audio Start",
                     poal_config->enable.iq_en == 0 ? "IQ Stop" : "IQ Start");
        
        io_set_enable_command(AUDIO_MODE_DISABLE, ch, 0);
        if(poal_config->enable.audio_en || poal_config->enable.iq_en){
            io_set_enable_command(AUDIO_MODE_ENABLE, ch, 0);
        }else{
            io_set_enable_command(AUDIO_MODE_DISABLE, ch, 0);
        }
        for(;;){
            switch(poal_config->work_mode)
            {
                case OAL_FIXED_FREQ_ANYS_MODE:
                {
                    fft_size = poal_config->multi_freq_point_param[ch].points[sub_ch].fft_size;
                    if(poal_config->enable.psd_en){
                        printf_info("start fixed freq thread, psd_en:%d \n", poal_config->enable.psd_en);
                        io_set_enable_command(PSD_MODE_ENABLE, ch, fft_size);
                        executor_wait_kernel_deal();
                    }else{
                        printf_info("stop fixed freq thread\n");
                        goto loop;
                    }
                }
                    break;
                case OAL_FAST_SCAN_MODE:
                {
                    fft_size = poal_config->multi_freq_fregment_para[ch].fregment[0].fft_size;
                    if(poal_config->enable.psd_en){
                        printf_info("start fast scan thread\n");
                        executor_fregment_scan(ch);
                        io_set_enable_command(PSD_MODE_ENABLE, ch, fft_size);
                        executor_wait_kernel_deal();
                    }else{
                        printf_info("stop fast scan thread\n");
                        io_set_enable_command(PSD_MODE_DISABLE, ch, fft_size);
                        goto loop;
                    }
                    break;
                }
                case OAL_MULTI_ZONE_SCAN_MODE:
                {   
                    printf_info("start multi zone thread cnt: %d\n", poal_config->multi_freq_fregment_para[ch].freq_segment_cnt);
                    if(poal_config->enable.psd_en){
                        for(i = 0; i < poal_config->multi_freq_fregment_para[ch].freq_segment_cnt; i++){
                            fft_size = poal_config->multi_freq_fregment_para[ch].fregment[i].fft_size;
                            executor_fregment_scan(ch);
                            io_set_enable_command(PSD_MODE_ENABLE, ch, fft_size);
                            executor_wait_kernel_deal();
                        }
                    }else{
                        io_set_enable_command(PSD_MODE_DISABLE, ch, fft_size);
                        goto loop;
                    }
                }
                    break;
                case OAL_MULTI_POINT_SCAN_MODE:
                    printf_info("start multi point thread\n");
                    if(poal_config->enable.bit_en){
                        executor_points_scan(ch);
                        executor_wait_kernel_deal();
                    }else{
                        io_set_enable_command(PSD_MODE_DISABLE, ch, 0);
                        io_set_enable_command(AUDIO_MODE_DISABLE, ch, 0);
                        goto loop;
                    }
                    break;
                default:
                    printf_err("not support work thread\n");
                    goto loop;
                    break;
            }
        }
    }
}

static int8_t executor_get_kernel_command(uint8_t type, void *data)
{

    
}


static int8_t executor_set_kernel_command(uint8_t type, uint8_t ch, void *data)
{
    switch(type)
     {
        case EX_CHANNEL_SELECT:
        {
            printf_debug("channel select: %d\n", *(uint8_t *)data);
            io_set_para_command(type, ch, data);
            break;
        }
        case EX_MUTE_SW:
        {
            break;
        }
        case EX_MUTE_THRE:
        {
            break;
        }
        case EX_MID_FREQ:
        {
            break;
        }
        case EX_DEC_METHOD:
        case EX_DEC_BW:
        {
            io_set_dq_param();
            break;
        }
        case EX_SMOOTH_TIME:
        {
            io_set_smooth_factor(*(uint32_t *)data);
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
            io_set_para_command(type, ch, data);
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
            printf_debug("ch:%d, bw:%u\n", ch, *(uint32_t *)data);
            break;
        }
        default:
            printf_err("not support type[%d]\n", type);
     }
    return 0;
}

static int8_t executor_get_rf_command(uint8_t type, uint8_t ch, void *data)
{
    switch(type)
    {
        case EX_RF_STATUS_TEMPERAT:
            rf_read_interface(EX_RF_STATUS_TEMPERAT, ch, data);
            break;
        default:
            printf_err("not support get type[%d]\n", type);
    }
    return 0;
}

static int8_t executor_set_rf_command(uint8_t type, uint8_t ch, void *data)
{
    switch(type)
     {
        case EX_RF_MODE_CODE:
        {
            printf_info("set rf bw: ch: %d, rf_mode_code:%d\n", ch, *(uint8_t *)data);
            rf_set_interface(type, ch, data);
            break;
        }
        case EX_RF_GAIN_MODE:
        {
            printf_info("set rf bw: ch: %d, gain_ctrl_method:%d\n", ch, *(uint8_t *)data);
            rf_set_interface(type, ch, data);
            break;
        }
        case EX_RF_MGC_GAIN:
        {
            rf_set_interface(type, ch, data);
            printf_info("set mgc gain: ch: %d, mgc_gain_value:%d\n", ch, *(int8_t *)data);
            
            break;
        }
        case EX_RF_AGC_CTRL_TIME:
        {
            break;
        }
        case EX_RF_AGC_OUTPUT_AMP:
        {
            break;
        }
        case EX_RF_MID_FREQ:
        {
            break;
        }
        case EX_RF_MID_BW:
        {
            printf_info("set rf bw: ch: %d, bw:%u\n", ch, *(uint32_t *)data);
            break;
        }
        case EX_RF_ANTENNA_SELECT:
        {
            rf_set_interface(type, ch, data);
            break;
        }
        case EX_RF_ATTENUATION:
        {
            rf_set_interface(type, ch, data);
            printf_info("set rf bw: ch: %d, attenuation:%d\n", ch, *(uint8_t *)data);
            break;
        }
        default:
            printf_err("not support type[%d]\n", type);
     }
    return 0;
}


int8_t executor_set_command(exec_cmd cmd, uint8_t type, uint8_t ch,  void *data)
{
     struct poal_config *poal_config = &(config_get_config()->oal_config);

     switch(cmd)
     {
        case EX_MID_FREQ_CMD:
        {
            executor_set_kernel_command(type, ch, data);
            break;
        }
        case EX_RF_FREQ_CMD:
        {
            executor_set_rf_command(type,ch, data);
            break;
        }
        case EX_ENABLE_CMD:
        {
            /* notify thread to deal data */
            printf_debug("notify thread to deal data\n");
            sem_post(&work_sem.notify_deal);
            break;
        }
        case EX_WORK_MODE_CMD:
        {
            char *pbuf;
            printf_info("set work mode[%d]\n", type);
            poal_config->assamble_kernel_response_data(pbuf, type, data);
            io_set_work_mode_command((void *)pbuf);
            break;
        }
        case EX_NETWORK_CMD:
        {
            io_set_network_to_interfaces((void *)&poal_config->network);
            break;
        }
        default:
            printf_err("invalid set command[%d]\n", cmd);
     }
     return 0;
}

int8_t executor_get_command(exec_cmd cmd, uint8_t type, uint8_t ch,  void *data)
{
    switch(cmd)
    {
        case EX_MID_FREQ_CMD:
        {
            break;
        }
        case EX_RF_FREQ_CMD:
        {
            executor_get_rf_command(type, ch, data);
            break;
        }
        case EX_NETWORK_CMD:
        {
            break;
        }
        default:
            printf_err("invalid get command[%d]\n", cmd);
    }
    return 0;

}


void executor_init(void)
{
    int ret;
    pthread_t work_id;
    io_init();
    /* set default network */
    executor_set_command(EX_NETWORK_CMD, 0, 0, NULL);
    sem_init(&(work_sem.notify_deal), 0, 0);
    sem_init(&(work_sem.kernel_sysn), 0, 0);
    ret=pthread_create(&work_id,NULL,(void *)executor_work_mode_thread, NULL);
    if(ret!=0)
        perror("pthread cread work_id");
    pthread_detach(work_id);
}



