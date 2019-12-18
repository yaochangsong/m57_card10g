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

struct sem_st work_sem;


static void executor_send_config_data_to_clent(void *data)
{
    printf_note("send data to client\n");
    struct spectrum_header_param *send_param;
    uint8_t *send_data = NULL;
    uint32_t send_data_len = 0;
    send_param = (struct spectrum_header_param *)data;
    printf_note("ch=%d,bandwidth=%u,m_freq=%llu\n", send_param->ch, send_param->bandwidth, send_param->m_freq);
#ifdef SUPPORT_PROTOCAL_AKT
    DEVICE_SIGNAL_PARAM_ST notify_param;
    notify_param.bandwith = send_param->bandwidth;
    notify_param.mid_freq = send_param->m_freq;
    notify_param.cid = send_param->ch;
    notify_param.status = 0;
    send_data = (uint8_t *)&notify_param;
    send_data_len = sizeof(DEVICE_SIGNAL_PARAM_ST);
#endif
    poal_send_active_to_all_client(send_data, send_data_len);
}

static inline int8_t executor_wait_kernel_deal(void)
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

static  int8_t  executor_fragment_scan(uint32_t fregment_num,uint8_t ch, work_mode_type mode)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);

    uint64_t c_freq=0, s_freq=0, e_freq=0, m_freq=0;
    uint32_t scan_bw, left_band, fftsize;
    uint32_t scan_count, i;
    uint8_t is_remainder = 0;
    uint8_t *header_buf;
    uint32_t header_len = 0;

    /* we need assamble pakege for kernel */
    struct spectrum_header_param header_param;
    /* 
        Step 1: 扫描次数计算
        扫描次数 = (截止频率 - 开始频�?)/中频扫描带宽，这里中频扫描带宽认为和射频带宽一�?;
    */
    s_freq = poal_config->multi_freq_fregment_para[ch].fregment[fregment_num].start_freq;
    e_freq = poal_config->multi_freq_fregment_para[ch].fregment[fregment_num].end_freq;
#ifdef SUPPORT_PROJECT_SSA
    if((s_freq >= KU_FREQUENCY_START) && (s_freq <= KU_FREQUENCY_END)){
        s_freq -= KU_FREQUENCY_OFFSET;
    }
    if((e_freq >= KU_FREQUENCY_START) && (e_freq <= KU_FREQUENCY_END)){
        e_freq -= KU_FREQUENCY_OFFSET;
    }
#endif
    if(config_read_by_cmd(EX_RF_FREQ_CMD, EX_RF_MID_BW, ch, &scan_bw) == -1){
            printf_err("Error read scan bindwidth=%u\n", scan_bw);
            return -1;
    }

    fftsize= poal_config->multi_freq_fregment_para[ch].fregment[fregment_num].fft_size;
    executor_set_command(EX_MID_FREQ_CMD, EX_FFT_SIZE,  ch, &fftsize);
    
    if(e_freq < s_freq || scan_bw <= 0){
        printf_err("frequency error,c_freq=%llu, scan_bw=%u\n", c_freq, scan_bw);
        return -1;
    }
    /* 频谱分析带宽 */
    poal_config->ctrl_para.specturm_analysis_param.bandwidth_hz = e_freq - s_freq;
    
    c_freq = (e_freq - s_freq);
    scan_count = c_freq /scan_bw;

    /* 扫描次数不是整数�? 需要向上取�?*/
    if(c_freq % scan_bw){
        is_remainder = 1;
    }
    if((scan_count == 0) && (is_remainder == 0))
        return -1;
    printf_debug("e_freq=%llu, s_freq=%llu, fftsize=%u\n", e_freq, s_freq, fftsize);
    printf_note("###scan_bw =%u,scan_count=%d, is_remainder=%d\n", scan_bw, scan_count, is_remainder);
    //executor_set_command(EX_RF_FREQ_CMD,  EX_RF_MID_BW, ch, &scan_bw);
    /* 
           Step 2: 根据扫描带宽�?从开始频率到截止频率循环扫描
   */
    for(i = 0; i < scan_count + is_remainder; i++){
        printf_debug("Bandwidth Scan [%d][%u]......\n", i, scan_bw);
        if(i < scan_count){
            /* 计算扫描中心频率 */
            m_freq = s_freq + i * scan_bw + scan_bw/2;
            left_band = scan_bw;
        }
        else{
        #if defined (SUPPORT_SPECTRUM_USER)
            /*spectrum is more than one fragment */
            if(scan_count > 0){
                m_freq = s_freq + i * scan_bw + scan_bw/2;
                left_band = scan_bw;
            }else{
                left_band = e_freq - s_freq - i * scan_bw;
                m_freq = s_freq + i * scan_bw + left_band/2;
            }
        #else
            /* 若不是整数，需计算剩余带宽中心频率 */
            left_band = e_freq - s_freq - i * scan_bw;
            m_freq = s_freq + i * scan_bw + left_band/2;
        #endif
        }
        header_param.ch = ch;
        header_param.s_freq = s_freq;
        header_param.e_freq = e_freq;
        header_param.bandwidth = left_band;
        header_param.fft_sn = i;
        header_param.total_fft = scan_count + is_remainder;
        header_param.m_freq = m_freq;
        header_param.fft_size = fftsize;
        header_param.mode = mode;
        header_param.freq_resolution = poal_config->multi_freq_fregment_para[ch].fregment[fregment_num].freq_resolution;
        /* 为避免在一定带宽下，中心频率过小导致起始频率<0，设置前需要对中频做判断 */
        m_freq =  middle_freq_resetting(scan_bw, m_freq);
        executor_set_command(EX_RF_FREQ_CMD, EX_RF_MID_FREQ, ch, &m_freq);
        executor_set_command(EX_MID_FREQ_CMD, EX_BANDWITH, ch, &scan_bw);
#ifdef SUPPORT_PLATFORM_ARCH_ARM
    #if defined(SUPPORT_SPECTRUM_KERNEL)
        executor_set_command(EX_WORK_MODE_CMD, mode, ch, &header_param);
        io_set_enable_command(PSD_MODE_ENABLE, ch, header_param.fft_size);
        executor_wait_kernel_deal();
    #elif defined (SUPPORT_SPECTRUM_USER)
        if(is_spectrum_aditool_debug() == false){
                spectrum_psd_user_deal(&header_param);
        }else{
            usleep(500);
        }
    #else
        usleep(500);
    #endif
#else
    
#endif
        if(poal_config->enable.bit_reset == true){
            poal_config->enable.bit_reset = false;
            printf_note("receive reset task sigal......\n");
            return -1;
        }
    }
    printf_debug("Exit fregment scan function\n");
    return 0;
}

static inline int8_t  executor_points_scan(uint8_t ch, work_mode_type mode)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    uint32_t points_count, i;
    uint32_t scan_bw;
    uint64_t c_freq, s_freq, e_freq, m_freq;
    struct spectrum_header_param header_param;
    struct multi_freq_point_para_st *point;
    time_t s_time;
    struct io_decode_param_st decode_param;

    point = &poal_config->multi_freq_point_param[ch];
    points_count = point->freq_point_cnt;
    if(config_read_by_cmd(EX_RF_FREQ_CMD, EX_RF_MID_BW, ch, &scan_bw) == -1){
        printf_err("Error read scan bindwidth=%u\n", scan_bw);
        return -1;
    }
    printf_info("residence_time=%d, points_count=%d\n", point->residence_time, points_count);
    for(i = 0; i < points_count; i++){
        printf_info("Scan Point [%d]......\n", i);
        s_freq = point->points[i].center_freq - point->points[i].bandwidth/2;
        e_freq = point->points[i].center_freq + point->points[i].bandwidth/2;
        header_param.ch = ch;
        header_param.fft_sn = i;
        header_param.s_freq = s_freq;
        header_param.e_freq = e_freq;
        header_param.total_fft = points_count;
        header_param.fft_size = point->points[i].fft_size;
        header_param.bandwidth = point->points[i].bandwidth;
        header_param.m_freq = point->points[i].center_freq;
        header_param.mode = mode;
        header_param.d_method = point->points[i].d_method;
        header_param.scan_bw = scan_bw;

        header_param.freq_resolution = (float)point->points[i].bandwidth * BAND_FACTOR / (float)point->points[i].fft_size;
        printf_info("ch=%d, s_freq=%llu, e_freq=%llu, fft_size=%u, d_method=%d\n", ch, s_freq, e_freq, header_param.fft_size,header_param.d_method);
        printf_note("rf scan bandwidth=%u, middlebw=%u, m_freq=%llu, freq_resolution=%f\n",header_param.scan_bw,header_param.bandwidth , header_param.m_freq, header_param.freq_resolution);
        executor_set_command(EX_RF_FREQ_CMD,  EX_RF_MID_FREQ, ch, &point->points[i].center_freq);
        executor_set_command(EX_MID_FREQ_CMD, EX_BANDWITH, ch, &point->points[ch].bandwidth);
        //executor_set_command(EX_RF_FREQ_CMD,  EX_RF_MID_BW,   ch, &header_param.scan_bw);
#if defined(SUPPORT_SPECTRUM_KERNEL)
        executor_set_command(EX_MID_FREQ_CMD, EX_MID_FREQ,    ch, &point->points[i].center_freq);
        executor_set_command(EX_MID_FREQ_CMD, EX_FFT_SIZE, ch, &point->points[i].fft_size);
        executor_set_command(EX_WORK_MODE_CMD,mode, ch, &header_param);
        /* notify client that some paramter has changed */
       // poal_config->send_active((void *)&header_param);
        /* 解调参数: 音频 */
        printf_note("enable.audio_en=%d, residence_time=%u,points_count=%u\n",poal_config->enable.audio_en,point->residence_time, points_count);
        if(poal_config->enable.audio_en){
            printf_note("ch=%d, i=%d, center_freq=%llu, d_method=%d, d_bandwith=%u\n",ch, i,
                point->points[i].center_freq, point->points[i].d_method, point->points[i].d_bandwith);
            executor_set_command(EX_MID_FREQ_CMD, EX_DEC_MID_FREQ, ch,&point->points[i].center_freq,  point->points[i].center_freq);
            executor_set_command(EX_MID_FREQ_CMD, EX_DEC_METHOD, ch, &point->points[i].d_method);
            executor_set_command(EX_MID_FREQ_CMD, EX_DEC_BW, ch, &point->points[i].d_bandwith);
            executor_set_command(EX_MID_FREQ_CMD, EX_DEC_RAW_DATA, ch, &point->points[i].center_freq, 
                                point->points[i].d_bandwith, point->points[i].raw_d_method);
            io_set_enable_command(AUDIO_MODE_ENABLE, ch, 0);
        }else{
            io_set_enable_command(AUDIO_MODE_DISABLE, ch, 0);
        }
#endif
        s_time = time(NULL);
        do{
#if defined(SUPPORT_SPECTRUM_KERNEL)
            if(poal_config->enable.psd_en){
                io_set_enable_command(PSD_MODE_ENABLE, ch, point->points[i].fft_size);
            }
            executor_wait_kernel_deal();
#elif defined (SUPPORT_SPECTRUM_USER)
            if(is_spectrum_aditool_debug() == false){
                spectrum_psd_user_deal(&header_param);
            }else{
                usleep(500);
            }
#endif
            if((poal_config->enable.bit_en == 0 && poal_config->sub_ch_enable.bit_en == 0) || 
               poal_config->enable.bit_reset == true){
                    printf_warn("bit_reset...[%d, %d, %d]\n", poal_config->enable.bit_en, 
                        poal_config->sub_ch_enable.bit_en, poal_config->enable.bit_reset);
                    poal_config->enable.bit_reset = false;
                    return -1;
            }
        }while(time(NULL) < s_time + point->residence_time ||  /* multi-frequency switching */
               points_count == 1);                             /* single-frequency station */
    }
    printf_info("Exit points scan function\n");
    return 0;
}


void executor_work_mode_thread(void *arg)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    uint32_t fft_size;
    uint8_t ch = poal_config->enable.cid;
    uint8_t sub_ch = poal_config->enable.sub_id;
    uint32_t j;

    
    
    while(1)
    {
        
loop:   printf_note("######wait to deal work######\n");
        sem_wait(&work_sem.notify_deal);
        ch = poal_config->enable.cid;
        if(OAL_NULL_MODE == poal_config->work_mode){
            printf_warn("Work Mode not set\n");
            goto loop;
        }
        printf_note("receive notify, [Channel:%d]%s Work: [%s], [%s], [%s]\n", 
                     ch,
                     poal_config->enable.bit_en == 0 ? "Stop" : "Start",
                     poal_config->enable.psd_en == 0 ? "Psd Stop" : "Psd Start",
                     poal_config->enable.audio_en == 0 ? "Audio Stop" : "Audio Start",
                     poal_config->enable.iq_en == 0 ? "IQ Stop" : "IQ Start");
        if(poal_config->sub_ch_enable.bit_en){
            printf_note("SUB channel, [Channel:%d, sub:%d]%s Work: [%s], [%s], [%s]\n", 
                                 ch,poal_config->sub_ch_enable.sub_id,
                                 poal_config->sub_ch_enable.bit_en == 0 ? "SubStop" : "SubStart",
                                 poal_config->sub_ch_enable.psd_en == 0 ? "Sub Psd Stop" : "Sub Psd Start",
                                 poal_config->sub_ch_enable.audio_en == 0 ? "Sub Audio Stop" : "Sub Audio Start",
                                 poal_config->sub_ch_enable.iq_en == 0 ? "Sub IQ Stop" : "Sub IQ Start");

        }
        
        poal_config->enable.bit_reset = false;
        printf_note("-------------------------------------\n");
        poal_config->assamble_response_data = executor_assamble_header_response_data_cb;
        poal_config->send_active = executor_send_data_to_clent_cb;
        if(poal_config->work_mode == OAL_FAST_SCAN_MODE){
            printf_note("            FastScan             \n");
        }else if(poal_config->work_mode == OAL_MULTI_ZONE_SCAN_MODE){
            printf_note("             MultiZoneScan       \n");
        }else if(poal_config->work_mode == OAL_FIXED_FREQ_ANYS_MODE){
            printf_note("             Fixed Freq          \n");
        }else if(poal_config->work_mode == OAL_MULTI_POINT_SCAN_MODE){
            printf_note("             MultiPointScan       \n");
        }else{
            goto loop;
        }
        printf_note("-------------------------------------\n");
        for(;;){
            switch(poal_config->work_mode)
            {
                case OAL_FAST_SCAN_MODE:
                case OAL_MULTI_ZONE_SCAN_MODE:
                {   
                    printf_debug("scan segment count: %d\n", poal_config->multi_freq_fregment_para[ch].freq_segment_cnt);
                    if(poal_config->multi_freq_fregment_para[ch].freq_segment_cnt == 0){
                        sleep(1);
                        goto loop;
                    }
                    if(poal_config->enable.psd_en || poal_config->enable.spec_analy_en){
                        for(j = 0; j < poal_config->multi_freq_fregment_para[ch].freq_segment_cnt; j++){
                            printf_debug("Segment Scan [%d]\n", j);
                            if(executor_fragment_scan(j, ch, poal_config->work_mode) == -1){
                                usleep(1000);
                                goto loop;
                            }
                        }
                    }else{
                        io_set_enable_command(PSD_MODE_DISABLE, ch, 0);
                        sleep(1);
                        goto loop;
                    }
                }
                    break;
                case OAL_FIXED_FREQ_ANYS_MODE:
                case OAL_MULTI_POINT_SCAN_MODE:
                {
                    printf_info("start point scan: points_count=%d\n", poal_config->multi_freq_point_param[ch].freq_point_cnt);
                    if(poal_config->multi_freq_point_param[ch].freq_point_cnt == 0){
                        sleep(1);
                        goto loop;
                    }

                    if(poal_config->enable.bit_en || poal_config->sub_ch_enable.bit_en){
                        if(executor_points_scan(ch, poal_config->work_mode) == -1){
                            usleep(1000);
                            goto loop;
                        }
                    }else{
                        io_set_enable_command(PSD_MODE_DISABLE, ch, 0);
                        io_set_enable_command(AUDIO_MODE_DISABLE, ch, 0);
                        sleep(1);
                        goto loop;
                    }
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
}


static int8_t executor_set_kernel_command(uint8_t type, uint8_t ch, void *data, va_list ap)
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
            printf_warn("ch=%d, dec_middle=%llu, middle_freq=%llu\n", ch, *(uint64_t *)data, middle_freq);
            io_set_dec_middle_freq(ch, *(uint64_t *)data, middle_freq);
            break;
        }
        case EX_DEC_RAW_DATA:
        {
            uint64_t  middle_freq;
            uint32_t bindwidth;
            uint8_t  d_method;
            middle_freq = *(uint64_t *)data;
            bindwidth = va_arg(ap, uint32_t);
            d_method = (uint8_t)va_arg(ap, uint32_t);
            printf_warn("EX_DEC_RAW_DATA: ch=%d, middle_freq=%llu, bindwidth=%u, d_method=%d\n", ch, middle_freq, bindwidth, d_method);
            io_set_dec_parameter(ch, middle_freq, d_method, bindwidth);
            break;
        }
        case EX_SMOOTH_TIME:
        {
            io_set_smooth_time(*(uint16_t *)data);
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
            io_set_bandwidth(ch, *(uint32_t *)data);
            break;
        }
        case EX_FPGA_RESET:
        {
            io_reset_fpga_data_link();
            break;
        }
        case EX_FPGA_CALIBRATE:
        {
            uint32_t  value = 0;
            value = config_get_fft_calibration_value(config_get_fft_size(ch));
            printf_note("ch:%d, fft calibration value:%u\n", ch, value);
            io_set_calibrate_val(ch, value);
            break;
        }
        case EX_SUB_CH_ONOFF:
        {
            io_set_subch_onoff(ch, *(uint8_t *)data);
            break;
        }
        case EX_SUB_CH_DEC_BW:
        {
            io_set_subch_bandwidth(ch, *(uint32_t *)data);
            break;
        }
        case EX_SUB_CH_MID_FREQ:
        {
            uint64_t  middle_freq;
            middle_freq = va_arg(ap, uint64_t);
            io_set_subch_dec_middle_freq(ch, *(uint64_t *)data, middle_freq);
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
            executor_set_kernel_command(type, ch, data, argp);
            break;
        }
        case EX_RF_FREQ_CMD:
        {
            rf_set_interface(type, ch, data);
            //executor_set_rf_command(type,ch, data);
            break;
        }
        case EX_ENABLE_CMD:
        {
            /* notify thread to deal data */
            printf_note("notify thread to deal data\n");
            poal_config->enable.bit_reset = true; /* reset(stop) all working task */
            sem_post(&work_sem.notify_deal);
            break;
        }
        case EX_WORK_MODE_CMD:
        {
            char *pbuf= NULL;
            uint32_t len;
            printf_debug("set work mode[%d]\n", type);
            pbuf = poal_config->assamble_response_data(&len, data);
            io_set_work_mode_command((void *)pbuf);
            break;
        }
        case EX_NETWORK_CMD:
        {
            struct in_addr ipaddr, dst_ipaddr;
            struct network_st *network = &poal_config->network;
            char *ipstr=NULL;

            if(get_ipaddress(&ipaddr) == -1){
                break;
            }
            printf_warn("ipaddress[%s]\n", inet_ntoa(ipaddr));
            if(ipaddr.s_addr == network->ipaddress){
                printf_note("ipaddress[%s] is not change!\n", inet_ntoa(ipaddr));
                break;
            }
            dst_ipaddr.s_addr = network->ipaddress;
            printf_warn("ipaddress[%s] is changed to [%s]\n", inet_ntoa(ipaddr), inet_ntoa(dst_ipaddr));
            io_set_network_to_interfaces((void *)&poal_config->network);
            break;
        }
        default:
            printf_err("invalid set command[%d]\n", cmd);
     }
     va_end(argp);
     UNLOCK_SET_COMMAND();
     return 0;
}

int8_t executor_get_command(exec_cmd cmd, uint8_t type, uint8_t ch,  void *data)
{
    switch(cmd)
    {
        case EX_MID_FREQ_CMD:
        {
            executor_get_kernel_command(type, ch, data);
            break;
        }
        case EX_RF_FREQ_CMD:
        {
            rf_read_interface(type, ch, data);
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

int8_t executor_set_enable_command(uint8_t ch)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    int i;
    
    printf_debug("bit_en[%x]\n", poal_config->enable.bit_en);
    if(poal_config->enable.bit_en == 0){
        printf_info("all Work disabled, waite thread stop...\n");
    }else{
        printf_debug("akt_assamble work_mode[%d]\n", poal_config->work_mode);
        printf_debug("bit_en=%x,psd_en=%d, audio_en=%d,iq_en=%d\n", poal_config->enable.bit_en, 
            poal_config->enable.psd_en,poal_config->enable.audio_en,poal_config->enable.iq_en);
        switch (poal_config->work_mode)
        {
            case OAL_FIXED_FREQ_ANYS_MODE:
            {
                printf_debug("freq_point_cnt=%d\n", poal_config->multi_freq_point_param[ch].freq_point_cnt);
                executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, ch, &poal_config->rf_para[ch].attenuation);
                executor_set_command(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, ch, &poal_config->rf_para[ch].mgc_gain_value);
                printf_note("ch=%d,attenuation=%d, mgc_gain_value=%d\n",ch, poal_config->rf_para[ch].attenuation, 
                    poal_config->rf_para[ch].mgc_gain_value);
                //executor_set_command(EX_RF_FREQ_CMD, EX_RF_MID_FREQ, ch, &poal_config->rf_para[ch].mid_freq);
                //executor_set_command(EX_RF_FREQ_CMD, EX_RF_MID_BW, ch, &poal_config->rf_para[ch].mid_bw);
                //executor_set_command(EX_MID_FREQ_CMD, EX_BANDWITH, ch, &poal_config->rf_para[ch].mid_bw);
                executor_set_command(EX_MID_FREQ_CMD, EX_CHANNEL_SELECT, ch, &ch);
                executor_set_command(EX_MID_FREQ_CMD, EX_SMOOTH_TIME, ch, &poal_config->multi_freq_point_param[ch].smooth_time);
                executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_CALIBRATE, ch, NULL);
                break;
            }
            case OAL_FAST_SCAN_MODE:
            {
                uint32_t bw;
                struct multi_freq_fregment_para_st *frp = &poal_config->multi_freq_fregment_para[ch];
                struct rf_para_st *rf = &poal_config->rf_para[ch];
                
                executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, ch, &poal_config->rf_para[ch].attenuation);
                //executor_set_command(EX_RF_FREQ_CMD, EX_RF_MID_FREQ, ch, &poal_config->rf_para[ch].mid_freq);
                executor_set_command(EX_RF_FREQ_CMD,EX_RF_MID_FREQ,ch,&poal_config->rf_para[ch].mid_freq);
                executor_set_command(EX_RF_FREQ_CMD,EX_RF_MID_BW,ch,&poal_config->rf_para[ch].mid_bw);
                if(config_read_by_cmd(EX_RF_FREQ_CMD, EX_RF_MID_BW, 0, &bw) == -1){
                    printf_err("Error read scan bindwidth=%u\n", bw);
                    return -1;
                }
                executor_set_command(EX_RF_FREQ_CMD, EX_RF_MID_BW, ch, &bw);
                /* 中频带宽和射频带宽一�?*/
                //executor_set_command(EX_MID_FREQ_CMD, EX_CHANNEL_SELECT, ch, &ch);
                executor_set_command(EX_MID_FREQ_CMD, EX_SMOOTH_TIME, ch,&frp->smooth_time);
                executor_set_command(EX_MID_FREQ_CMD, EX_BANDWITH, ch,&bw);
                executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_CALIBRATE, ch, NULL);
                executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_RESET, ch, NULL);
                
                break;
            }
            case OAL_MULTI_ZONE_SCAN_MODE:
            {
                executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, ch, &poal_config->rf_para[ch].attenuation);
                executor_set_command(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, ch, &poal_config->rf_para[ch].mgc_gain_value);
                executor_set_command(EX_MID_FREQ_CMD, EX_SMOOTH_TIME, ch, &poal_config->multi_freq_fregment_para[ch].smooth_time);
                executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_CALIBRATE, ch, NULL);
                executor_set_command(EX_MID_FREQ_CMD, EX_CHANNEL_SELECT, ch, &ch);
                executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_RESET, ch, NULL);
                break;
            }
            case OAL_MULTI_POINT_SCAN_MODE:
                executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, ch, &poal_config->rf_para[ch].attenuation);
                executor_set_command(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, ch, &poal_config->rf_para[ch].mgc_gain_value);
                //executor_set_command(EX_MID_FREQ_CMD, EX_BANDWITH, ch, &poal_config->rf_para[ch].mid_bw);
                executor_set_command(EX_MID_FREQ_CMD, EX_SMOOTH_TIME, ch, &poal_config->multi_freq_fregment_para[ch].smooth_time);
                executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_CALIBRATE, ch, NULL);
                executor_set_command(EX_MID_FREQ_CMD, EX_CHANNEL_SELECT, ch, &ch);
                executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_RESET, ch, NULL);
                break;
            default:
                return -1;
        }
    }
    executor_set_command(EX_ENABLE_CMD, 0, ch, NULL);
    return 0;
}

void executor_timer_task1_cb(struct uloop_timeout *t)
{
#ifdef SUPPORT_SPECTRUM_USER
    spectrum_send_fft_data_interval();
#endif
    uloop_timeout_set(t, 200);
}
void executor_timer_task_init(void)
{
    static  struct uloop_timeout task1_timeout;
    printf_warn("executor_timer_task\n");
    if(!is_spectrum_continuous_mode()){
        printf_note("timer task: fft data send opened:%d\n", is_spectrum_continuous_mode());
        task1_timeout.cb = executor_timer_task1_cb;
        uloop_timeout_set(&task1_timeout, 2000); /* 5000 ms */
        printf_note("executor_timer_task ok\n");
    }else{
        printf_note("timer task: fft data send shutdown:%d\n", is_spectrum_continuous_mode());
    }
}

void executor_init(void)
{
    int ret, i;
    pthread_t work_id, work_id_iio;
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    io_init();

    executor_timer_task_init();
    /* set default network */
    executor_set_command(EX_NETWORK_CMD, 0, 0, NULL);
    /* shutdown all channel */
    for(i = 0; i<MAX_RADIO_CHANNEL_NUM ; i++){
        io_set_enable_command(PSD_MODE_DISABLE, i, 0);
        io_set_enable_command(AUDIO_MODE_DISABLE, i, 0);
        io_set_enable_command(FREQUENCY_BAND_ENABLE_DISABLE, i, 0);
        //executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, 0, &poal_config->rf_para[i].attenuation);
    }
    printf_note("clear all sub ch\n");
    uint8_t enable =0;
    for(i = 0; i< MAX_SIGNAL_CHANNEL_NUM; i++){
        executor_set_command(EX_MID_FREQ_CMD, EX_SUB_CH_ONOFF, i, &enable);
    }
    sem_init(&(work_sem.notify_deal), 0, 0);
    sem_init(&(work_sem.kernel_sysn), 0, 0);
    ret=pthread_create(&work_id,NULL,(void *)executor_work_mode_thread, NULL);
    if(ret!=0)
        perror("pthread cread work_id");
    pthread_detach(work_id);
}



