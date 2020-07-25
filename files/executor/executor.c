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

/**
 * Mutex for the set command, used by command setting related functions. 
 */
pthread_mutex_t set_cmd_mutex = PTHREAD_MUTEX_INITIALIZER;

struct sem_st work_sem;

static void executor_send_config_data_to_clent(void *data)
{
    printf_note("send data to client\n");
    struct spm_run_parm *send_param;
    uint8_t *send_data = NULL;
    uint32_t send_data_len = 0;
    send_param = (struct spm_run_parm *)data;
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

int executor_tcp_disconnect_notify(void *cl)
{
    #define UDP_CLIENT_NUM 8
    struct net_udp_client *cl_list, *list_tmp;
    struct net_udp_server *srv = get_udp_server();
    struct net_tcp_client *tcp_cl = (struct net_tcp_client *)cl;
    int index = 0, find = 0;
    struct udp_client_info ucli[UDP_CLIENT_NUM];
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    memset(ucli, 0, sizeof(struct udp_client_info)*UDP_CLIENT_NUM);
    if(tcp_find_client(&tcp_cl->peer_addr)){
        find = 1;
        return 0;
    }
   /* release udp client */
    printf_info("disconnet tcp client%s:%d\n", inet_ntoa(tcp_cl->peer_addr.sin_addr), tcp_cl->peer_addr.sin_port);
    list_for_each_entry_safe(cl_list, list_tmp, &srv->clients, list){
        printf_info("udp client list %s:%d\n", inet_ntoa(cl_list->peer_addr.sin_addr), cl_list->peer_addr.sin_port);
        if(memcmp(&cl_list->peer_addr.sin_addr, &tcp_cl->peer_addr.sin_addr, sizeof(tcp_cl->peer_addr.sin_addr)) == 0){
           printf_info("del udp client %s:%d\n", inet_ntoa(cl_list->peer_addr.sin_addr), cl_list->peer_addr.sin_port);
           udp_free(cl_list);
        }
    }

    /* reload udp client */
    list_for_each_entry_safe(cl_list, list_tmp, &srv->clients, list){
        ucli[index].cid = cl_list->ch;
        /* UDP链表以大端模式存储客户端地址信息；内部以小端模式处理；注意转�?*/
        ucli[index].ipaddr = ntohl(cl_list->peer_addr.sin_addr.s_addr);
        ucli[index].port = ntohs(cl_list->peer_addr.sin_port);
        printf_note("reload client index=%d, cid=%d, [ip:%x][port:%d][10g_ipaddr=0x%x][10g_port=%d], online\n", 
                        index, ucli[index].cid, ucli[index].ipaddr, ucli[index].port);
        index ++;
        if(index >= UDP_CLIENT_NUM){
            break;
        }
    }
    printf_note("disconnect, free udp, remain udp client: %d\n", index);
    io_set_udp_client_info(ucli);
    /* client is 0 */
    if(index == 0){
        #if defined(SUPPORT_XWFS)
        xwfs_stop_backtrace(NULL);  /* 停止回溯 ，回到正常状�?/
        #endif
        #ifdef SUPPORT_NET_WZ
        io_set_10ge_net_onoff(0);   /* 客户端离线，关闭万兆传输 */
        #endif
        /* 关闭所有子通道 */
        uint8_t enable =0;
        uint8_t default_method = IO_DQ_MODE_IQ;
        int i = 0;
        for(i = 0; i< MAX_SIGNAL_CHANNEL_NUM; i++){
            executor_set_command(EX_MID_FREQ_CMD, EX_SUB_CH_ONOFF, i, &enable);
            executor_set_command(EX_MID_FREQ_CMD, EX_SUB_CH_DEC_METHOD, i, &default_method);
            memset(&poal_config->sub_ch_enable[i], 0, sizeof(struct output_en_st));
        }
        /* 所有客户端离线，关闭相关使能，线程复位到等待状�?*/
        memset(&poal_config->enable, 0, sizeof(poal_config->enable));
        poal_config->enable.bit_reset = true; /* reset(stop) all working task */
    }
    
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

static  int8_t  executor_fragment_scan(uint32_t fregment_num,uint8_t ch, work_mode_type mode, void *args)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);

    uint64_t c_freq=0, s_freq=0, e_freq=0, m_freq=0;
    uint32_t scan_bw, left_band, fftsize;
    uint32_t scan_count, i;
    uint8_t is_remainder = 0;

    struct spm_context *spmctx;
    struct spm_run_parm *r_args;
    spmctx = (struct spm_context *)args;
    r_args = spmctx->run_args;

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
            printf_err("Error read scan bandwidth=%u\n", scan_bw);
            return -1;
    }

    uint64_t _s_freq_hz_offset, _e_freq_hz,_m_freq_hz;
    uint32_t index = 0, _bw_hz;
    static uint32_t t_fft = 2;
    _s_freq_hz_offset = s_freq;
    _e_freq_hz = e_freq;
    fftsize= poal_config->multi_freq_fregment_para[ch].fregment[fregment_num].fft_size;
    executor_set_command(EX_MID_FREQ_CMD, EX_BANDWITH, ch, &scan_bw);
    executor_set_command(EX_MID_FREQ_CMD, EX_FFT_SIZE,  ch, &fftsize);
        r_args->ch = ch;
        r_args->fft_size = fftsize;
        r_args->mode = mode;
    do{
        r_args->s_freq_offset = _s_freq_hz_offset;
        r_args->s_freq = s_freq;
        spmctx->ops->spm_scan(&_s_freq_hz_offset, &_e_freq_hz, &scan_bw, &_bw_hz, &_m_freq_hz);
        printf_debug("[%d] s_freq_offset=%lluHz,scan_bw=%uHz _bw_hz=%u, _m_freq_hz=%lluHz\n", index, r_args->s_freq_offset, scan_bw, _bw_hz, _m_freq_hz);
        r_args->e_freq = e_freq;
        r_args->scan_bw = scan_bw;
        r_args->bandwidth = _bw_hz;
        r_args->m_freq = r_args->s_freq_offset + _bw_hz/2;
        r_args->m_freq_s = _m_freq_hz;
        r_args->fft_sn = index;
        r_args->total_fft = t_fft;
        r_args->freq_resolution = poal_config->multi_freq_fregment_para[ch].fregment[fregment_num].freq_resolution;
        printf_debug("[%d]s_freq=%llu, e_freq=%llu, scan_bw=%u, bandwidth=%u,m_freq=%llu, m_freq_s=%llu\n", 
            index, r_args->s_freq, r_args->e_freq, r_args->scan_bw, r_args->bandwidth, r_args->m_freq, r_args->m_freq_s);
        spmctx->ops->sample_ctrl(r_args->m_freq_s);
        executor_set_command(EX_RF_FREQ_CMD, EX_RF_MID_FREQ, ch, &_m_freq_hz);
        executor_set_command(EX_RF_FREQ_CMD, EX_RF_LOW_NOISE, ch, &_m_freq_hz);
        executor_set_command(EX_MID_FREQ_CMD, EX_MID_FREQ,    ch, &_m_freq_hz);
        executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_CALIBRATE, ch, &fftsize, _m_freq_hz,0);
        index ++;
        if(poal_config->enable.psd_en){
            io_set_enable_command(PSD_MODE_ENABLE, ch, -1, r_args->fft_size);
        }
        spm_deal(args, r_args);
        if(poal_config->enable.psd_en){
            io_set_enable_command(PSD_MODE_DISABLE, ch, -1, r_args->fft_size);
        }
        if(poal_config->enable.bit_reset == true){
            poal_config->enable.bit_reset = false;
            printf_note("receive reset task sigal......\n");
            return -1;
        }
    }while(_s_freq_hz_offset < _e_freq_hz);
    t_fft = index;

    printf_debug("Exit fregment scan function\n");
    return 0;
}

static int8_t  executor_points_scan(uint8_t ch, work_mode_type mode, void *args)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    uint32_t points_count, i;
    uint64_t c_freq, s_freq, e_freq, m_freq;
    struct spm_context *spmctx;
    struct spm_run_parm *r_args;
    struct multi_freq_point_para_st *point;
    time_t s_time;
    struct io_decode_param_st decode_param;
    int8_t subch = 0;
    bool residency_time_arrived = false;
    //int32_t policy = poal_config->ctrl_para.residency.policy[ch];
    int32_t policy = poal_config->multi_freq_point_param[ch].residence_time;
    spmctx = (struct spm_context *)args;
    r_args = spmctx->run_args;

    point = &poal_config->multi_freq_point_param[ch];
    points_count = point->freq_point_cnt;
    executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_CALIBRATE, ch, &point->points[0].fft_size, 0,poal_config->rf_para[ch].gain_ctrl_method);
    //executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_CALIBRATE, ch, &point->points[0].fft_size, 0);
    printf_note("residence_time=%d, points_count=%d\n", point->residence_time, points_count);
    for(i = 0; i < points_count; i++){
        printf_info("Scan Point [%d]......\n", i);
        s_freq = point->points[i].center_freq - point->points[i].bandwidth/2;
        e_freq = point->points[i].center_freq + point->points[i].bandwidth/2;
        r_args->ch = ch;
        r_args->fft_sn = i;
        r_args->s_freq = s_freq;
        r_args->e_freq = e_freq;
        r_args->total_fft = points_count;
        r_args->fft_size = point->points[i].fft_size;
        r_args->bandwidth = point->points[i].bandwidth;
        r_args->m_freq = point->points[i].center_freq;
        r_args->mode = mode;
        if(subch_bitmap_weight()!=0){
            if(i < MAX_SIGNAL_CHANNEL_NUM){
                subch = poal_config->sub_ch_enable[i].sub_id ;
                r_args->d_method = poal_config->sub_channel_para[ch].sub_ch[subch].raw_d_method;
            }
        }else{
            r_args->d_method = point->points[i].raw_d_method;
        }
        r_args->scan_bw = point->points[i].bandwidth;
        r_args->gain_mode = poal_config->rf_para[ch].gain_ctrl_method;
        r_args->gain_value = poal_config->rf_para[ch].mgc_gain_value;
        r_args->freq_resolution = (float)point->points[i].bandwidth * BAND_FACTOR / (float)point->points[i].fft_size;
        printf_note("ch=%d, s_freq=%llu, e_freq=%llu, fft_size=%u, d_method=%d\n", ch, s_freq, e_freq, r_args->fft_size,r_args->d_method);
        printf_note("rf scan bandwidth=%u, middlebw=%u, m_freq=%llu, freq_resolution=%f\n",r_args->scan_bw,r_args->bandwidth , r_args->m_freq, r_args->freq_resolution);
        spmctx->ops->sample_ctrl(r_args->m_freq);
        executor_set_command(EX_RF_FREQ_CMD,  EX_RF_MID_FREQ, ch, &point->points[i].center_freq);
        executor_set_command(EX_RF_FREQ_CMD,  EX_RF_LOW_NOISE, ch, &point->points[i].center_freq);
        executor_set_command(EX_MID_FREQ_CMD, EX_BANDWITH, ch, &point->points[i].bandwidth);
        executor_set_command(EX_MID_FREQ_CMD, EX_MID_FREQ,    ch, &point->points[i].center_freq);
        //executor_set_command(EX_RF_FREQ_CMD,  EX_RF_MID_BW,   ch, &r_args->.scan_bw);
#ifndef SUPPORT_SPECTRUM_FFT
        //executor_set_command(EX_MID_FREQ_CMD, EX_MID_FREQ,    ch, &point->points[i].center_freq);
        executor_set_command(EX_MID_FREQ_CMD, EX_FFT_SIZE, ch, &point->points[i].fft_size);
        /* 根据带宽设置边带�?*/
        executor_set_command(EX_CTRL_CMD, EX_CTRL_SIDEBAND, ch, &r_args->scan_bw);
        #if defined(SUPPORT_SPECTRUM_KERNEL)
        executor_set_command(EX_WORK_MODE_CMD,mode, ch, r_args);
        #endif
        /* notify client that some paramter has changed */
       // poal_config->send_active((void *)r_args);
        /* 解调参数: 音频 */
        printf_note("enable.audio_en=%d, residence_time=%u,points_count=%u\n",poal_config->enable.audio_en,point->residence_time, points_count);
        if(poal_config->enable.audio_en){
            printf_note("ch=%d, i=%d, center_freq=%llu, d_method=%d, d_bandwith=%u\n",ch, i,
                point->points[i].center_freq, point->points[i].d_method, point->points[i].d_bandwith);
            
            
            executor_set_command(EX_MID_FREQ_CMD, EX_DEC_RAW_DATA, ch, &point->points[i].center_freq, 
                                point->points[i].d_bandwith, point->points[i].raw_d_method);
            
            
        printf_note("d_center_freq=%llu, d_method=%d, d_bandwith=%u noise=%d noise_en=%d\n",poal_config->multi_freq_point_param[ch].points[i].center_freq,
                    poal_config->multi_freq_point_param[ch].points[i].d_method,poal_config->multi_freq_point_param[ch].points[i].d_bandwith,
                    poal_config->multi_freq_point_param[ch].points[i].noise_thrh,poal_config->multi_freq_point_param[ch].points[i].noise_en);

            executor_set_command(EX_MID_FREQ_CMD, EX_DEC_METHOD, CONFG_AUDIO_CHANNEL, &poal_config->multi_freq_point_param[ch].points[i].d_method);
            executor_set_command(EX_MID_FREQ_CMD, EX_DEC_BW, CONFG_AUDIO_CHANNEL, &poal_config->multi_freq_point_param[ch].points[i].d_bandwith);
            executor_set_command(EX_MID_FREQ_CMD, EX_DEC_MID_FREQ, CONFG_AUDIO_CHANNEL,&poal_config->multi_freq_point_param[ch].points[i].center_freq,poal_config->multi_freq_point_param[ch].points[i].center_freq);
            
            executor_set_command(EX_MID_FREQ_CMD, EX_MUTE_THRE, CONFG_AUDIO_CHANNEL,&poal_config->multi_freq_point_param[ch].points[i].noise_thrh,poal_config->multi_freq_point_param[ch].points[i].noise_en);

            io_set_enable_command(AUDIO_MODE_ENABLE, ch, CONFG_AUDIO_CHANNEL, 0);  //音频通道开�?
        }else{
            io_set_enable_command(AUDIO_MODE_DISABLE, ch, CONFG_AUDIO_CHANNEL, 0);  //音频通道开�?
        }
#endif
#if defined (SUPPORT_RESIDENCY_STRATEGY) 
        usleep(20000);
        uint16_t strength = 0;
        bool is_signal = false;
        int32_t ret = -1;
        ret = spmctx->ops->signal_strength(ch, &is_signal, &strength);
        //is_signal = true;
        if(ret == 0){
            printf_note("is sigal: %s, strength:%d\n", (is_signal == true ? "Yes":"No"), strength);
        }
#endif
        s_time = time(NULL);
        do{
            if(poal_config->enable.psd_en){
                io_set_enable_command(PSD_MODE_ENABLE, ch, -1, point->points[i].fft_size);
            }
#if defined(SUPPORT_SPECTRUM_KERNEL)
            executor_wait_kernel_deal();
#elif defined (SUPPORT_SPECTRUM_FFT)
            if(is_spectrum_aditool_debug() == false){
                spectrum_psd_user_deal(r_args);
            }else{
                usleep(500);
            }
#elif defined (SUPPORT_SPECTRUM_V2)
            spmctx->ops->agc_ctrl(ch, spmctx);
            if(args != NULL)
                spm_deal(args, r_args);
            if(poal_config->enable.psd_en){
                io_set_enable_command(PSD_MODE_DISABLE, ch, -1, point->points[i].fft_size);
            }
#endif
            if((poal_config->enable.bit_en == 0) || 
               poal_config->enable.bit_reset == true){
                    printf_warn("bit_reset...[%d, %d]\n", poal_config->enable.bit_en, poal_config->enable.bit_reset);
                    poal_config->enable.bit_reset = false;
                    return -1;
            }
#if defined (SUPPORT_RESIDENCY_STRATEGY) 
            /* 驻留时间是否到达判断;多频点模式下生效 */
            if(spmctx->ops->residency_time_arrived && (ret == 0) && (points_count > 1)){
                residency_time_arrived = spmctx->ops->residency_time_arrived(ch, policy, is_signal);
            }else
#endif
            {
                residency_time_arrived = (time(NULL) < s_time + point->residence_time) ? false : true;
            }
        }while((residency_time_arrived == false) ||  /* multi-frequency switching */
               points_count == 1);                             /* single-frequency station */
    }
    printf_info("Exit points scan function\n");
    return 0;
}


void executor_spm_thread(void *arg)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    uint32_t fft_size;
    uint8_t ch = poal_config->enable.cid;
    uint8_t sub_ch = poal_config->enable.sub_id;
    uint32_t j;

    //thread_bind_cpu(1);
    while(1)
    {
        
loop:   printf_note("######wait to deal work######\n");
        sem_wait(&work_sem.notify_deal);
        if(poal_config->enable.bit_en == 0)
            safe_system("/etc/led.sh transfer off &");
        else
            safe_system("/etc/led.sh transfer on &");
        ch = poal_config->enable.cid;
        if(OAL_NULL_MODE == poal_config->work_mode){
            printf_warn("Work Mode not set\n");
            sleep(1);
            goto loop;
        }
        printf_note("receive notify, [Channel:%d]%s Work: [%s], [%s], [%s]\n", 
                     ch,
                     poal_config->enable.bit_en == 0 ? "Stop" : "Start",
                     poal_config->enable.psd_en == 0 ? "Psd Stop" : "Psd Start",
                     poal_config->enable.audio_en == 0 ? "Audio Stop" : "Audio Start",
                     poal_config->enable.iq_en == 0 ? "IQ Stop" : "IQ Start");
        
        poal_config->enable.bit_reset = false;
        printf_note("-------------------------------------\n");
        #if (defined SUPPORT_PROTOCAL_AKT) || (defined SUPPORT_PROTOCAL_XNRP) 
        poal_config->assamble_response_data = executor_assamble_header_response_data_cb;
        poal_config->send_active = executor_send_data_to_clent_cb;
        #endif
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
                   // printf_debug("scan segment count: %d\n", poal_config->multi_freq_fregment_para[ch].freq_segment_cnt);
                    if(poal_config->multi_freq_fregment_para[ch].freq_segment_cnt == 0){
                        sleep(1);
                        goto loop;
                    }
                    if(poal_config->enable.psd_en || poal_config->enable.spec_analy_en){
                        for(j = 0; j < poal_config->multi_freq_fregment_para[ch].freq_segment_cnt; j++){
                            if(executor_fragment_scan(j, ch, poal_config->work_mode, arg) == -1){
                                io_set_enable_command(PSD_MODE_DISABLE, ch, -1, 0);
                                usleep(1000);
                                goto loop;
                            }
                        }
                    }else{
                        io_set_enable_command(PSD_MODE_DISABLE, ch, -1, 0);
                        sleep(1);
                        goto loop;
                    }
                }
                    break;
                case OAL_FIXED_FREQ_ANYS_MODE:
                case OAL_MULTI_POINT_SCAN_MODE:
                {
                    //printf_info("start point scan: points_count=%d\n", poal_config->multi_freq_point_param[ch].freq_point_cnt);
                    if(poal_config->multi_freq_point_param[ch].freq_point_cnt == 0){
                        sleep(1);
                        goto loop;
                    }

                    if(poal_config->enable.bit_en){
                        if(executor_points_scan(ch, poal_config->work_mode, arg) == -1){
                            io_set_enable_command(PSD_MODE_DISABLE, ch, -1,  0);
                            io_set_enable_command(AUDIO_MODE_DISABLE, ch, -1, 0);
                            usleep(1000);
                            goto loop;
                        }
                    }else{
                        io_set_enable_command(PSD_MODE_DISABLE, ch, -1, 0);
                        io_set_enable_command(AUDIO_MODE_DISABLE, ch, -1, 0);
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
    struct poal_config *poal_config = &(config_get_config()->oal_config);
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
            printf_note("ch=%d, dec_middle=%llu, middle_freq=%llu\n", ch, *(uint64_t *)data, middle_freq);
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
            //printf_warn("EX_DEC_RAW_DATA: ch=%d, middle_freq=%llu, bandwidth=%u, d_method=%d\n", ch, middle_freq, bandwidth, d_method);
            io_set_dec_parameter(ch, middle_freq, d_method, bandwidth);
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
            uint32_t contrl_type = 0;
            

            if(data !=NULL){
                fftsize = *(uint32_t *)data;
            }
            m_freq = (uint64_t)va_arg(ap, uint64_t);
            contrl_type = (uint32_t)va_arg(ap, uint32_t);
            value = config_get_fft_calibration_value(ch, fftsize, m_freq);
            printf_note("ch:%d, fft calibration value:%d m_freq :%d contrl_type:%d\n", ch, value,m_freq,contrl_type);
            io_set_calibrate_val(ch, (uint32_t)value);
            break;
        }

        case EX_SUB_CH_ONOFF:
        {
            io_set_subch_onoff(ch, *(uint8_t *)data);
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
        default:
            printf_err("not support type[%d]\n", type);
     }
    return 0;
}

static int8_t executor_set_ctrl_command(uint8_t type, uint8_t ch, void *data, va_list ap)
{
    switch(type){
        case EX_CTRL_SIDEBAND:
        {
            uint32_t bandwidth;
            float side_rate;
            bandwidth = *(uint32_t *)data;
            /* 根据带宽获取边带�?*/
            if(config_read_by_cmd(EX_CTRL_CMD, EX_CTRL_SIDEBAND,ch, &side_rate, bandwidth) == -1){
                printf_note("!!!SideRate Is Not Set In Config File[bandwidth=%u],user default siderate=%f!!!\n", bandwidth, side_rate);
            }
            /* 设置边带�?*/
            io_set_side_rate(ch, &side_rate);
            break;
        }
    }
    return 0;
}

#ifdef SUPPORT_NET_WZ
static int executor_set_10g_network(struct network_st *_network)
{
    safe_system("/etc/network.sh &");
    /* 设置默认板卡万兆ip和端�?*/
    //io_set_local_10g_net(ntohl(_network->ipaddress), ntohl(_network->netmask),ntohl(_network->gateway),_network->port);
    return 0;
}
#endif

static int executor_set_1g_network(struct network_st *_network)
{
    struct in_addr ipaddr, dst_addr, netmask, gateway;
    struct network_st *network = _network;
    uint32_t host_addr;
    char *ipstr=NULL;
    int need_set = 0;
    if(get_ipaddress(NETWORK_EHTHERNET_POINT, &ipaddr) != -1){
        if(ipaddr.s_addr == network->ipaddress){
            printf_note("ipaddress[%s] is not change!\n", inet_ntoa(ipaddr));
            goto set_netmask;
        }
        printf_note("ipaddress[%s] is changed to: ", inet_ntoa(ipaddr));
        dst_addr.s_addr = network->ipaddress;
        need_set ++;
        printfn("[%s]\n", inet_ntoa(dst_addr));
#ifdef SUPPORT_LCD
        host_addr = ntohl(network->ipaddress);
        lcd_printf(EX_NETWORK_CMD, EX_NETWORK_IP, &host_addr, NULL);
#endif
    }
    
set_netmask:
    if(get_netmask(NETWORK_EHTHERNET_POINT, &netmask) != -1){
         if(netmask.s_addr == network->netmask){
            printf_note("netmask[%s] is not change!\n", inet_ntoa(netmask));
            goto set_gateway;
        }
        printf_note("netmask[%s] is changed to: ", inet_ntoa(netmask));
        dst_addr.s_addr = network->netmask;
        need_set ++;
        printfn("[%s]\n", inet_ntoa(dst_addr));
#ifdef SUPPORT_LCD
        host_addr = ntohl(network->netmask);
        lcd_printf(EX_NETWORK_CMD, EX_NETWORK_MASK, &host_addr, NULL);
#endif
    }
    
set_gateway:
    if(get_gateway(NETWORK_EHTHERNET_POINT, &gateway) != -1){
         if(gateway.s_addr == network->gateway){
            printf_note("gateway[%s] is not change!\n", inet_ntoa(gateway));
            if(need_set != 0)
                goto set_network;
            else
                return -1;
        }
        printf_note("gateway[%s] is changed to: ", inet_ntoa(gateway));
        dst_addr.s_addr = network->gateway;
        printfn("[%s]\n", inet_ntoa(dst_addr));
#ifdef SUPPORT_LCD
        host_addr = ntohl(network->gateway);
        lcd_printf(EX_NETWORK_CMD, EX_NETWORK_GW, &host_addr, NULL);
#endif
    }
    
set_network:
    return 0;
    //return io_set_network_to_interfaces((void *)network);
}


int8_t executor_set_command(exec_cmd cmd, uint8_t type, uint8_t ch,  void *data, ...)
{
     struct poal_config *poal_config = &(config_get_config()->oal_config);
     va_list argp;
     uint8_t ch_dup = ch +1;
     LOCK_SET_COMMAND();
     va_start (argp, data);
     switch(cmd)
     {
        case EX_MID_FREQ_CMD:
        {
#ifdef SUPPORT_LCD
            lcd_printf(cmd, type,data, &ch_dup);
#endif
            executor_set_kernel_command(type, ch, data, argp);
            break;
        }
        case EX_RF_FREQ_CMD:
        {
#ifdef SUPPORT_LCD
            lcd_printf(cmd, type,data, &ch_dup);
#endif
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
            #if (defined SUPPORT_PROTOCAL_AKT) || (defined SUPPORT_PROTOCAL_XNRP) 
            pbuf = poal_config->assamble_response_data(&len, data);
            io_set_work_mode_command((void *)pbuf);
            #endif
            break;
        }
        case EX_NETWORK_CMD:
        {
            executor_set_1g_network(&poal_config->network);
            safe_system("/etc/network.sh &");
            break;
        }
        case EX_CTRL_CMD:
        {
            executor_set_ctrl_command(type, ch, data, argp);
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
                uint32_t bandwidth = 0;
                printf_debug("freq_point_cnt=%d\n", poal_config->multi_freq_point_param[ch].freq_point_cnt);
                executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, ch, &poal_config->rf_para[ch].attenuation);
                executor_set_command(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, ch, &poal_config->rf_para[ch].mgc_gain_value);
                printf_note("ch=%d,attenuation=%d, mgc_gain_value=%d\n",ch, poal_config->rf_para[ch].attenuation, 
                    poal_config->rf_para[ch].mgc_gain_value);
                //executor_set_command(EX_RF_FREQ_CMD, EX_RF_MID_FREQ, ch, &poal_config->rf_para[ch].mid_freq);
                //executor_set_command(EX_RF_FREQ_CMD, EX_RF_MID_BW, ch, &poal_config->rf_para[ch].mid_bw);
                //executor_set_command(EX_MID_FREQ_CMD, EX_BANDWITH, ch, &poal_config->rf_para[ch].mid_bw);
                //executor_set_command(EX_MID_FREQ_CMD, EX_CHANNEL_SELECT, ch, &ch);
                executor_set_command(EX_MID_FREQ_CMD, EX_SMOOTH_TIME, ch, &poal_config->multi_freq_point_param[ch].smooth_time);
                //executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_CALIBRATE, ch, NULL);
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
                executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, ch, &poal_config->rf_para[ch].attenuation);
                executor_set_command(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, ch, &poal_config->rf_para[ch].mgc_gain_value);
                executor_set_command(EX_MID_FREQ_CMD, EX_SMOOTH_TIME, ch, &poal_config->multi_freq_fregment_para[ch].smooth_time);
                //executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_CALIBRATE, ch, NULL);
                //executor_set_command(EX_MID_FREQ_CMD, EX_CHANNEL_SELECT, ch, &ch);
                executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_RESET, ch, NULL);
                break;
            }
            case OAL_MULTI_POINT_SCAN_MODE:
                executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, ch, &poal_config->rf_para[ch].attenuation);
                executor_set_command(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, ch, &poal_config->rf_para[ch].mgc_gain_value);
                //executor_set_command(EX_MID_FREQ_CMD, EX_BANDWITH, ch, &poal_config->rf_para[ch].mid_bw);
                executor_set_command(EX_MID_FREQ_CMD, EX_SMOOTH_TIME, ch, &poal_config->multi_freq_point_param[ch].smooth_time);
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
#ifdef SUPPORT_SPECTRUM_FFT
    spectrum_send_fft_data_interval();
#endif
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
    int ret, i;
    pthread_t work_id;
    void *spmctx;
    struct poal_config *poal_config = &(config_get_config()->oal_config);
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    io_init();
#elif defined(SUPPORT_SPECTRUM_V2) 
    //fpga_io_init();
    spmctx = spm_init();
    if(spmctx == NULL){
        printf_err("spm create failed\n");
        exit(-1);
    }
#endif
#endif /* SUPPORT_PLATFORM_ARCH_ARM */
   // executor_timer_task_init();
    /* set default network */
    executor_set_command(EX_NETWORK_CMD, EX_NETWORK, 0, NULL);
    /* shutdown all channel */
    for(i = 0; i<MAX_RADIO_CHANNEL_NUM ; i++){
        io_set_enable_command(PSD_MODE_DISABLE, i, -1, 0);
        io_set_enable_command(AUDIO_MODE_DISABLE, i, -1, 0);
        //io_set_enable_command(FREQUENCY_BAND_ENABLE_DISABLE, i, 0);
        //executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, 0, &poal_config->rf_para[i].attenuation);
    }
    subch_bitmap_init();
#ifdef SUPPORT_NET_WZ
    io_set_10ge_net_onoff(0); /* 关闭万兆传输 */
#endif

    printf_note("clear all sub ch\n");
    uint8_t enable =0;
    uint8_t default_method = IO_DQ_MODE_IQ;
    for(i = 0; i< MAX_SIGNAL_CHANNEL_NUM; i++){
        printf_debug("clear all sub ch: %d\n", i);
        executor_set_command(EX_MID_FREQ_CMD, EX_SUB_CH_ONOFF, i, &enable);
        executor_set_command(EX_MID_FREQ_CMD, EX_SUB_CH_DEC_METHOD, i, &default_method);
    }
    sem_init(&(work_sem.notify_deal), 0, 0);
    sem_init(&(work_sem.kernel_sysn), 0, 0);

    ret=pthread_create(&work_id,NULL,(void *)executor_spm_thread, spmctx);
    if(ret!=0)
        perror("pthread cread spm");
    pthread_detach(work_id);
}

void executor_close(void)
{
#ifdef SUPPORT_SPECTRUM_V2
    fpga_io_close();
    spm_close();
#endif
#ifdef  SUPPORT_CLOCK_ADC
    clock_adc_close();
#endif
}


