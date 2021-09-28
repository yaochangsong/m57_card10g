#include "config.h"
#include "executor_thread.h"
#include "spm/spm.h"
#include "../bsp/io.h"
#include "../utils/bitops.h"
#include "../conf/conf.h"


struct executor_context *exec_ctx = NULL;
static inline bool _is_executor_thread_reset(struct executor_thread_m *thread);


static const struct executor_thread_ops exec_ops = {
 //   .close = _net_thread_close,
};


static double _difftime_us_val(const struct timeval *start, const struct timeval *end)
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


static inline void _printf_nchar(char c, int n)
{
    for(int i = 0; i < n; i++){
        printf("%c", c);
    }
    printf("\n");
}


static bool  _executor_fragment_scan(uint32_t fregment_num,uint8_t ch, work_mode_type mode, struct executor_thread_m *args)
{
    uint64_t c_freq=0, s_freq=0, e_freq=0, m_freq=0;
    uint32_t scan_bw, left_band;
    uint32_t scan_count, i;
    uint8_t is_remainder = 0;

    struct spm_context *spmctx;
    struct spm_run_parm *r_args;
    
    spmctx = (struct spm_context *)args->args;
    r_args = spmctx->run_args[ch];
    struct poal_config *poal_config = spmctx->pdata;
    
    /* 
        Step 1: 扫描次数计算
        扫描次数 = (截止频率 - 开始频�?)/中频扫描带宽，这里中频扫描带宽认为和射频带宽一�?;
    */
    s_freq = poal_config->channel[ch].multi_freq_fregment_para.fregment[fregment_num].start_freq;
    e_freq = poal_config->channel[ch].multi_freq_fregment_para.fregment[fregment_num].end_freq;
#if defined(CONFIG_BSP_40G)
    uint64_t s_freq_origin, e_freq_origin;
    get_origin_start_end_freq(ch, s_freq, e_freq, &s_freq_origin, &e_freq_origin);
    printf_debug("s_freq:%"PRIu64", s_freq_origin:%"PRIu64"\n", s_freq, s_freq_origin);
    printf_debug("e_freq:%"PRIu64", e_freq_origin:%"PRIu64"\n", e_freq, e_freq_origin);
#endif
    if(config_read_by_cmd(EX_RF_FREQ_CMD, EX_RF_MID_BW, ch, &scan_bw) == -1){
            printf_err("Error read scan bandwidth=%u\n", scan_bw);
            return -1;
    }

    uint64_t _s_freq_hz_offset, _e_freq_hz,_m_freq_hz;
    uint32_t index = 0, _bw_hz, dma_fft_size;
    static uint32_t t_fft = 0;
    _s_freq_hz_offset = s_freq;
    _e_freq_hz = e_freq;
    config_get_fft_resolution(mode, ch, fregment_num, scan_bw,  &r_args->fft_size, &r_args->freq_resolution);
    executor_set_command(EX_MID_FREQ_CMD, EX_BANDWITH, ch, &scan_bw);
    executor_set_command(EX_MID_FREQ_CMD, EX_FFT_SIZE,  ch, &r_args->fft_size);
    r_args->ch = ch;
    //r_args->fft_size = fftsize;
    dma_fft_size = r_args->fft_size * SAMPLING_FFT_TIMES;
    r_args->mode = mode;
    r_args->gain_mode = poal_config->channel[ch].rf_para.gain_ctrl_method;
    r_args->gain_value = poal_config->channel[ch].rf_para.mgc_gain_value;
    r_args->rf_mode = poal_config->channel[ch].rf_para.rf_mode_code;
    do{
        r_args->s_freq_offset = _s_freq_hz_offset;
        #if defined(CONFIG_BSP_40G)
        r_args->s_freq = s_freq_origin;
        r_args->e_freq = e_freq_origin;
        #else
        r_args->s_freq = s_freq;
        r_args->e_freq = e_freq;
        #endif
        if(spmctx->ops->spm_scan)
        spmctx->ops->spm_scan(&_s_freq_hz_offset, &_e_freq_hz, &scan_bw, &_bw_hz, &_m_freq_hz);
        printf_info("[%d] s_freq_offset=%"PRIu64"Hz,scan_bw=%uHz _bw_hz=%u, _m_freq_hz=%"PRIu64"Hz\n", index, r_args->s_freq_offset, scan_bw, _bw_hz, _m_freq_hz);
        r_args->scan_bw = scan_bw;
        r_args->bandwidth = _bw_hz;
        r_args->m_freq = r_args->s_freq_offset + _bw_hz/2;
        r_args->m_freq_s = _m_freq_hz;
        r_args->fft_sn = index;
        r_args->total_fft = t_fft;
        r_args->fregment_num = fregment_num;
        printf_info("[ch:%d, %d]s_freq=%"PRIu64", e_freq=%"PRIu64", scan_bw=%u, bandwidth=%u,m_freq=%"PRIu64", m_freq_s=%"PRIu64", freq_resolution = %u, fftsize=%u\n", 
            ch, index, r_args->s_freq, r_args->e_freq, r_args->scan_bw, r_args->bandwidth, r_args->m_freq, r_args->m_freq_s, r_args->freq_resolution, r_args->fft_size);
        if(spmctx->ops->sample_ctrl)
            spmctx->ops->sample_ctrl(r_args);
        executor_set_command(EX_RF_FREQ_CMD, EX_RF_MID_FREQ, ch, &_m_freq_hz);
        executor_set_command(EX_RF_FREQ_CMD, EX_RF_MODE_CODE, ch, &r_args->rf_mode);
        //executor_set_command(EX_RF_FREQ_CMD, EX_RF_LOW_NOISE, ch, &_m_freq_hz);
        executor_set_command(EX_MID_FREQ_CMD, EX_MID_FREQ,    ch, &_m_freq_hz, _m_freq_hz);
        executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_CALIBRATE, ch, &r_args->fft_size, r_args->m_freq,0);
        index ++;
        if(poal_config->channel[ch].enable.map.bit.fft){
            io_set_enable_command(PSD_MODE_ENABLE, ch, -1, dma_fft_size);
        }
        spm_deal(spmctx, r_args, ch);
        if(poal_config->channel[ch].enable.map.bit.fft){
            io_set_enable_command(PSD_MODE_DISABLE, ch, -1, dma_fft_size);
        }
        if(_is_executor_thread_reset(args)){
            printf_info("Recv reset\n");
            return false;
        }
    }while(_s_freq_hz_offset < _e_freq_hz);
    t_fft = index;

    printf_info("Exit fregment scan function\n");
    return true;
}

static bool  _executor_points_scan_mode(uint8_t ch, int mode, void *args)
{
    uint32_t points_count, i, scan_bw = MHZ(40);;
    uint32_t dma_fft_size = 0;
    uint64_t c_freq, s_freq, e_freq, m_freq;
    struct spm_context *spmctx;
    struct spm_run_parm *r_args;
    struct multi_freq_point_para_st *point;
    time_t s_time;
    int8_t subch = 0;
    struct timeval beginTime, endTime;
    double  diff_time_us = 0;
    bool residency_time_arrived = false;
    struct executor_thread_m *arg = args;
    spmctx = (struct spm_context *)arg->args;
    r_args = spmctx->run_args[ch];
    struct poal_config *poal_config = spmctx->pdata;
    
    if(config_read_by_cmd(EX_RF_FREQ_CMD, EX_RF_MID_BW, ch, &scan_bw) == -1){
        printf_info("Error read scan bandwidth=%u\n", scan_bw);
    }
    int32_t policy = poal_config->channel[ch].multi_freq_point_param.residence_time;
    point = &poal_config->channel[ch].multi_freq_point_param;
    points_count = point->freq_point_cnt;
    printf_info("residence_time=%d, points_count=%d, scan_bw=%u\n", point->residence_time, points_count, scan_bw);
    for(i = 0; i < points_count; i++){
        printf_info("Scan Point [%d]......\n", i);
        r_args->ch = ch;
        r_args->fft_sn = i;
        r_args->audio_points = i;
        r_args->total_fft = points_count;
        r_args->bandwidth = point->points[i].bandwidth;
        s_freq = point->points[i].center_freq - point->points[i].bandwidth/2; //MHZ(304);
        e_freq = point->points[i].center_freq + point->points[i].bandwidth/2; //MHZ(464);
        r_args->s_freq = s_freq;
        r_args->e_freq = e_freq;
        r_args->m_freq = point->points[i].center_freq;
        r_args->mode = mode;

        /* fixed bug for IQ &audio decode type error */
        r_args->d_method = 0;
        #if defined(CONFIG_BSP_SSA) || defined(CONFIG_BSP_SSA_MONITOR)
        r_args->scan_bw =  scan_bw;
        #else
        r_args->scan_bw =  point->points[i].bandwidth;
        #endif
        r_args->s_freq_offset = s_freq;
        r_args->m_freq_s = r_args->m_freq;
        r_args->gain_mode = poal_config->channel[ch].rf_para.gain_ctrl_method;
        r_args->gain_value = poal_config->channel[ch].rf_para.mgc_gain_value;
        config_get_fft_resolution(mode, ch, i, r_args->scan_bw,  &r_args->fft_size, &r_args->freq_resolution);
        printf_note("ch=%d, s_freq=%"PRIu64", e_freq=%"PRIu64" fft_size=%u, d_method=%d\n", ch, s_freq, e_freq, r_args->fft_size,r_args->d_method);
        printf_note("rf scan bandwidth=%u, middlebw=%u, m_freq=%"PRIu64", freq_resolution=%u\n",r_args->scan_bw,r_args->bandwidth , r_args->m_freq, r_args->freq_resolution);
        if(spmctx->ops->sample_ctrl)
            spmctx->ops->sample_ctrl(r_args);
        executor_set_command(EX_RF_FREQ_CMD,  EX_RF_MID_FREQ, ch, &point->points[i].center_freq);
        executor_set_command(EX_RF_FREQ_CMD,  EX_RF_MID_BW, ch, &point->points[i].bandwidth);
        executor_set_command(EX_MID_FREQ_CMD, EX_BANDWITH, ch, &point->points[i].bandwidth);
        executor_set_command(EX_MID_FREQ_CMD, EX_MID_FREQ,    ch, &point->points[i].center_freq, point->points[i].center_freq);
        //executor_set_command(EX_RF_FREQ_CMD,  EX_RF_LOW_NOISE, ch, &point->points[i].center_freq);
        //executor_set_command(EX_MID_FREQ_CMD, EX_FFT_SIZE, ch, &point->points[i].fft_size);
        /* 根据带宽设置边带�?*/
        //executor_set_command(EX_CTRL_CMD, EX_CTRL_SIDEBAND, ch, &r_args->scan_bw);
        printf_note("d_center_freq=%"PRIu64", d_method=%d, d_bandwith=%u noise=%d noise_en=%d volume=%d\n",poal_config->channel[ch].multi_freq_point_param.points[i].center_freq,
                    poal_config->channel[ch].multi_freq_point_param.points[i].d_method,poal_config->channel[ch].multi_freq_point_param.points[i].d_bandwith,
                    poal_config->channel[ch].multi_freq_point_param.points[i].noise_thrh,poal_config->channel[ch].multi_freq_point_param.points[i].noise_en,
                    poal_config->channel[ch].multi_freq_point_param.points[i].audio_volume);
        if(poal_config->channel[ch].enable.map.bit.audio){
            subch = CONFIG_AUDIO_CHANNEL;
            executor_set_command(EX_MID_FREQ_CMD, EX_DEC_METHOD, subch, &poal_config->channel[ch].multi_freq_point_param.points[i].d_method);
            executor_set_command(EX_MID_FREQ_CMD, EX_SUB_CH_DEC_BW, subch, &poal_config->channel[ch].multi_freq_point_param.points[i].d_bandwith);
            executor_set_command(EX_MID_FREQ_CMD, EX_SUB_CH_MID_FREQ, subch,&poal_config->channel[ch].multi_freq_point_param.points[i].center_freq,poal_config->channel[ch].multi_freq_point_param.points[i].center_freq, ch);
            executor_set_command(EX_MID_FREQ_CMD, EX_MUTE_THRE, subch,&poal_config->channel[ch].multi_freq_point_param.points[i].noise_thrh,poal_config->channel[ch].multi_freq_point_param.points[i].noise_en);
            
            executor_set_command(EX_MID_FREQ_CMD, EX_AUDIO_VOL_CTRL, subch,&point->points[i].audio_volume);
            io_set_enable_command(AUDIO_MODE_ENABLE, ch, -1, 0);  //音频通道开�?
            spm_niq_deal_notify(NULL);
        }else{
            io_set_enable_command(AUDIO_MODE_DISABLE, ch, -1, 0);  //音频通道开�?
        }
        
#if defined (CONFIG_RESIDENCY_STRATEGY) 
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
            config_get_fft_resolution(mode, ch, i, r_args->scan_bw,  &r_args->fft_size, &r_args->freq_resolution);
            dma_fft_size = r_args->fft_size * SAMPLING_FFT_TIMES;
            executor_set_command(EX_MID_FREQ_CMD, EX_FFT_SIZE, ch, &point->points[i].fft_size);
            executor_set_command(EX_MID_FREQ_CMD, EX_SMOOTH_TIME, ch, &poal_config->channel[ch].multi_freq_point_param.smooth_time);
            executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_CALIBRATE, ch, &point->points[i].fft_size, r_args->m_freq);
            if(poal_config->channel[ch].enable.map.bit.fft){
                io_set_enable_command(PSD_MODE_ENABLE, ch, -1, dma_fft_size);
            }
            if(spmctx->ops->agc_ctrl)
                spmctx->ops->agc_ctrl(ch, spmctx);
            if(spmctx != NULL)
                spm_deal(spmctx, r_args, ch);
            if(poal_config->channel[ch].enable.map.bit.fft){
                io_set_enable_command(PSD_MODE_DISABLE, ch, -1, dma_fft_size);
            }
            
            if(_is_executor_thread_reset(arg))
                return false;
#if defined (CONFIG_RESIDENCY_STRATEGY) 
            /* 驻留时间是否到达判断;多频点模式下生效 */
            if(spmctx->ops->residency_time_arrived && (ret == 0) && (points_count > 1)){
                policy = poal_config->channel[ch].multi_freq_point_param.residence_time/1000;
                residency_time_arrived = spmctx->ops->residency_time_arrived(ch, policy, is_signal);
            }else
#endif
            {
                gettimeofday(&endTime, NULL);
                diff_time_us = _difftime_us_val(&beginTime, &endTime);
                residency_time_arrived = (diff_time_us < point->residence_time*1000) ? false : true;
            }
        }while((residency_time_arrived == false) ||  /* multi-frequency switching */
               points_count == 1);                             /* single-frequency station */
    }
    printf_info("Exit points scan function\n");
    return true;
}

static uint64_t _get_sp_middle_freq(int points, uint64_t mfreq, uint32_t bw)
{

    uint64_t _m_freq;
    uint32_t _bw = bw;
    
    if(points == 0){
        _m_freq = mfreq - _bw;
    } else if(points == 1){
        _m_freq = mfreq;
    } else if(points == 2){
        _m_freq = mfreq + _bw;
    } else{
        printf_err("err points: %d\n", points);
    }
    return _m_freq;
    return 0;
}


static int8_t  _executor_serial_points_scan(uint8_t ch, work_mode_type mode, int 
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    uint32_t points_count, i = 0;
    uint32_t fft_size;
    uint64_t c_freq, s_freq, e_freq, m_freq,relative_mfreq;
    struct spm_run_parm *r_args;
    struct multi_freq_point_para_st *point;
    time_t s_time;
    int8_t subch = 0;
    struct timeval beginTime, endTime;
    double  diff_time_us = 0;
    bool residency_time_arrived = false;
    uint8_t _ch;
    uint32_t _bandwidth = 0;
    struct executor_thread_m *arg = args;
    struct spm_context *spmctx = (struct spm_context *)arg->args;
    
#if defined(CONFIG_SPM_FFT_PACKAGING_PROCESS)
    _ch = 0;
#else
    _ch = ch;
#endif
    //int32_t policy = poal_config->ctrl_para.residency.policy[ch];
    int32_t policy = poal_config->channel[_ch].multi_freq_point_param.residence_time;
    r_args = spmctx->run_args[ch];
    point = &poal_config->channel[_ch].multi_freq_point_param;
    i = 0;
    points_count = 1;
    r_args->bandwidth = point->points[i].bandwidth;
    _bandwidth = point->points[i].bandwidth/SEGMENT_FREQ_NUM;
#if defined(CONFIG_SPM_FFT_PACKAGING_PROCESS)
    s_freq = MHZ(107.5);//point->start_freq; 
    e_freq = MHZ(570);//point->end_freq; 
    //r_args->bandwidth = e_freq - s_freq;
    if(reg_get()->misc && reg_get()->misc->get_rel_middle_freq_by_channel)
        poal_config->channel[ch].multi_freq_point_param.points[i].center_freq = reg_get()->misc->get_rel_middle_freq_by_channel(ch, NULL);
    //poal_config->channel[ch].multi_freq_point_param.points[i].center_freq = _get_rel_middle_freq_by_channel(ch, NULL);
    r_args->m_freq = s_freq + (e_freq - s_freq) / 2;
    r_args->m_freq_s = _get_sp_middle_freq(_points, poal_config->channel[ch].multi_freq_point_param.points[i].center_freq, _bandwidth);
#else
    s_freq = point->start_freq; 
    e_freq = point->end_freq; 
    r_args->m_freq = _get_sp_middle_freq(_points, point->points[i].center_freq, _bandwidth);
#endif
    r_args->ch = ch; 
    r_args->point = _points;
    r_args->fft_sn = i;
    r_args->audio_points = i;
    r_args->s_freq = s_freq;
    r_args->e_freq = e_freq;
    r_args->total_fft = points_count;
    r_args->mode = mode;
    r_args->d_method = 0;
    r_args->scan_bw = point->points[i].bandwidth;
    r_args->gain_mode = poal_config->channel[_ch].rf_para.gain_ctrl_method;
    r_args->gain_value = poal_config->channel[_ch].rf_para.mgc_gain_value;
    config_get_fft_resolution(mode, ch, i, r_args->scan_bw,  &r_args->fft_size, &r_args->freq_resolution);
    //r_args->freq_resolution =point->points[i].freq_resolution;
    //r_args->fft_size = point->points[i].fft_size;
    fft_size =  r_args->fft_size;
    printf_debug("ch:%d, fft_size=%u, m_freq=%"PRIu64", freq=%u\n", ch, fft_size, r_args->m_freq, r_args->freq_resolution);
#if defined(CONFIG_ARCH_ARM)
    if(spmctx->ops->sample_ctrl)
        spmctx->ops->sample_ctrl(r_args);
    if(reg_get()->misc && reg_get()->misc->get_middle_freq_by_channel)
        reg_get()->misc->get_middle_freq_by_channel(ch, NULL);
    //m_freq = _get_middle_freq_by_channel(ch, NULL);
    executor_set_command(EX_MID_FREQ_CMD, EX_MID_FREQ,ch, &m_freq, _get_sp_middle_freq(_points, poal_config->channel[ch].multi_freq_point_param.points[i].center_freq, _bandwidth));
    executor_set_command(EX_MID_FREQ_CMD, EX_FFT_SIZE, ch, &fft_size);
    executor_set_command(EX_MID_FREQ_CMD, EX_FPGA_CALIBRATE, ch, &fft_size, m_freq);
    executor_set_command(EX_MID_FREQ_CMD, EX_SMOOTH_TIME, ch, &point->smooth_time);
    executor_set_command(EX_MID_FREQ_CMD, EX_SMOOTH_TYPE, ch, &point->smooth_mode);
    executor_set_command(EX_MID_FREQ_CMD, EX_SMOOTH_THRE, ch, &point->smooth_threshold);
    io_set_enable_command(PSD_MODE_ENABLE, ch, -1, fft_size);
    if(spmctx->ops->agc_ctrl)
        spmctx->ops->agc_ctrl(ch, spmctx);
    if(spmctx != NULL)
        spm_deal(spmctx, r_args, -1);
    io_set_enable_command(PSD_MODE_DISABLE, ch, -1, fft_size);
    
    if(_is_executor_thread_reset(arg))
        return false;
#endif
    return true;
}

static bool  _executor_points_scan_mode2(uint8_t ch, int mode, void *args)
{
    for(int i = 0; i < MAX_RADIO_CHANNEL_NUM; i ++){
        for(int j = 0; j < SEGMENT_FREQ_NUM; j++){
            if(_executor_serial_points_scan(i, mode, j, args) == false){
                return false;
            }
        }
    }
    return true;
}


static bool  _executor_fragment_scan_mode(uint8_t ch, int mode, void *args)
{
    struct executor_thread_m *arg = args;
    struct spm_context *spmctx = (struct spm_context *)arg->args;
    struct poal_config *conf = spmctx->pdata;
    
    int cnt = conf->channel[ch].multi_freq_fregment_para.freq_segment_cnt;
    for(int i = 0; i < cnt; i ++){
        if(_executor_fragment_scan(i, ch, mode, arg) == false){
            return false;
        }
    }
    return true;
}


static int _executor_thread_stop(struct executor_thread_m *thread)
{
    pthread_mutex_lock(&thread->pwait.t_mutex);
    thread->pwait.is_wait = false;
    //printf_note("[%s]notify thread %s\n", ptd->name, ptd->pwait.is_wait == true ? "PAUSE" : "RUN");
    pthread_cond_signal(&thread->pwait.t_cond);
    pthread_mutex_unlock(&thread->pwait.t_mutex);
    printf_info("[%s]notify thread %s\n", thread->name,  "RUN");
    return 0;
}

static inline bool _is_executor_thread_reset(struct executor_thread_m *thread)
{
    bool reset = false;
    pthread_mutex_lock(&thread->pwait.t_mutex);
    reset = thread->reset;
    pthread_mutex_unlock(&thread->pwait.t_mutex);
    return reset;
}


static int _executor_thread_wait(struct executor_thread_m *thread, int *mode)
{
    struct executor_thread_wait *wait = &thread->pwait;
    
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE , NULL);
    pthread_mutex_lock(&wait->t_mutex);
    while(wait->is_wait == true){
        pthread_cond_wait(&wait->t_cond, &wait->t_mutex);
    }
   // wait->is_wait = true;
    *mode = thread->mode->mode;
    thread->reset = false;
    pthread_mutex_unlock(&wait->t_mutex);
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE , NULL);
    return 0;
}

struct executor_mode_table mode_table[] = {
    [OAL_FAST_SCAN_MODE] = {
        .mode = OAL_FAST_SCAN_MODE,
        .name = "Fast Scan Mode",
        .exec = _executor_fragment_scan_mode,
    },
    [OAL_FIXED_FREQ_ANYS_MODE] = {
        .mode = OAL_FIXED_FREQ_ANYS_MODE,
        .name = "Fixed Mode",
        .exec = _executor_points_scan_mode,
    },
    [OAL_MULTI_ZONE_SCAN_MODE] = {
        .mode = OAL_MULTI_ZONE_SCAN_MODE,
        .name = "Multi Zone Mode",
        .exec = _executor_fragment_scan_mode,
    },
    [OAL_MULTI_POINT_SCAN_MODE] = {
        .mode = OAL_MULTI_POINT_SCAN_MODE,
        .name = "Multi Points Mode",
        .exec = _executor_points_scan_mode,
    },
    [OAL_FIXED_FREQ_ANYS_MODE2] = {
        .mode = OAL_FIXED_FREQ_ANYS_MODE2,
        .name = "Serial Scan Fixed Points Mode",
        .exec = _executor_points_scan_mode2,
    }
    
};

static int _executor_thread_main_loop(void *args)
{
    int ch = 0, mode = 0;
    struct executor_thread_m *thread = args;
    static bool p_note[_EXEC_THREAD_NUM] = {[0 ... _EXEC_THREAD_NUM-1] = true};

    ch = thread->ch;
    if(ch >= _EXEC_THREAD_NUM){
        printf_note("ch: %d is bigger than thread num:%d\n", ch, _EXEC_THREAD_NUM);
        return -1;
    }
    printf_info("-------------------------------------\n");
    printf_info("Thread Wait [name:%s, ch:%d] \n", thread->name, ch);
    printf_info("-------------------------------------\n");
    _executor_thread_wait(thread, &mode);
    
    if (mode >= ARRAY_SIZE(mode_table) || !mode_table[mode].exec){
        _executor_thread_stop(thread);
        return -1;
    }
    if(p_note[ch]){
        _printf_nchar('-', 60);
        printf("Thread \e[32mRUN\e[0m ==> ch:%2d, name:%-14s, mode: %-32s\n", ch, thread->name, mode_table[mode].name);
        _printf_nchar('-', 60);
        printf("\n");
        p_note[ch] = false;
    }
    
    if (!mode_table[mode].exec(ch, mode, args)) {
        io_set_enable_command(PSD_MODE_DISABLE, ch, -1, 0);
        io_set_enable_command(AUDIO_MODE_DISABLE, ch, -1, 0);
         _printf_nchar('-', 60);
         printf("Thread \e[33mSTOP\e[0m <== ch:%2d, name:%-14s, mode: %-32s\n", ch, thread->name, mode_table[mode].name);
         _printf_nchar('-', 60);
         printf("\n");
         p_note[ch] = true;
    }
    return 0;
}

static void *_get_client_thread(void *cl)
{
#if (defined CONFIG_PROTOCOL_ACT)
    struct net_tcp_client *client = cl;
    return client->section.thread;
#elif defined(CONFIG_PROTOCOL_XW)
    struct uh_client *client = cl;
    return client->section.thread;
#endif
    return NULL;
}

int executor_fft_work_nofity(void *cl, int ch, int mode, bool enable)
{
    struct executor_context  *pctx;
    if(cl){
        pctx = _get_client_thread(cl);
    } else{
        pctx = exec_ctx;
    }
    printf_note("ch=%d, mode=%d, enable=%d\n", ch, mode, enable);
    
    if(enable)
        ch_bitmap_set(ch, CH_TYPE_FFT);
    else{
        ch_bitmap_clear(ch, CH_TYPE_FFT);
    }
    struct executor_thread_m *thread = &pctx->thread[ch];
    
    if(thread == NULL)
        return -1;

    pthread_mutex_lock(&thread->pwait.t_mutex);
    thread->pwait.is_wait = !enable;
    thread->mode->mode = mode;
    thread->reset = true;
    printf_note("[%s]notify thread %s\n", thread->name, thread->pwait.is_wait == true ? "PAUSE" : "RUN");
    pthread_cond_signal(&thread->pwait.t_cond);
    pthread_mutex_unlock(&thread->pwait.t_mutex);
    //("[%s]notify thread %s\n", ptd->name,  enable == true ? "RUN" : "PAUSE");
    return 0;
}


static void _executor_thread_exit(void *arg)
{
    struct executor_thread_m *thread = arg;
    int ch = thread->ch;
    io_set_enable_command(PSD_MODE_DISABLE, ch, -1, 0);
    io_set_enable_command(AUDIO_MODE_DISABLE, ch, -1, 0);
    safe_free(thread->name);
}

static void _wait_init(struct executor_thread_wait *wait)
{
    if(wait){
        pthread_cond_init(&wait->t_cond, NULL);
        pthread_mutex_init(&wait->t_mutex, NULL);
        wait->is_wait = true;
    }
}

struct executor_context * _executor_thread_create_context(void)
{
    struct executor_context *ctx = calloc(1, sizeof(*ctx));
    if (!ctx){
        printf_err("zalloc error!!\n");
        exit(1);
    }
    
    int ret = -1;
    pthread_t tid = 0;
    char thread_name[64];
    ctx->args = get_spm_ctx();
    ctx->ops = &exec_ops;
    
    for(int ch = 0; ch < _EXEC_THREAD_NUM; ch++){
        snprintf(thread_name, sizeof(thread_name) -1, "FFT_thread_%d", ch);
        ctx->thread[ch].name = strdup(thread_name);
        ctx->thread[ch].ch = ch;
        ctx->thread[ch].mode = mode_table;
        ctx->thread[ch].mode->mode = OAL_NULL_MODE;
        ctx->thread[ch].reset = false;
        ctx->thread[ch].args = (void *)get_spm_ctx();
        _wait_init(&ctx->thread[ch].pwait);
        ret =  pthread_create_detach (NULL,NULL, _executor_thread_main_loop, _executor_thread_exit,  ctx->thread[ch].name ,&ctx->thread[ch], &ctx->thread[ch], &tid);
        if(ret != 0){
            perror("pthread err");
            exit(1);
        }
        ctx->thread[ch].tid = tid;
    }
    
    return ctx;
}


struct executor_context *get_executor_ctx(void)
{
    if(exec_ctx == NULL){
        exec_ctx = _executor_thread_create_context();
    }
    return exec_ctx;
}

void executor_thread_init(void)
{
    printf_note("Executor thread Init.\n");
    exec_ctx = _executor_thread_create_context();
  
    for(int i = 0; i<MAX_RADIO_CHANNEL_NUM ; i++){
        executor_set_command(EX_RF_FREQ_CMD, EX_RF_MID_BW, i, NULL);
        io_set_enable_command(PSD_MODE_DISABLE, i, -1, 0);
        io_set_enable_command(AUDIO_MODE_DISABLE, i, -1, 0);
        io_stop_backtrace_file(&i);
    }
    
    uint8_t enable =0;
    uint8_t default_method = IO_DQ_MODE_IQ;

    for(int i = 0; i <MAX_RADIO_CHANNEL_NUM; i++){
        for(int j = 0; j< MAX_SIGNAL_CHANNEL_NUM; j++){
            printf_debug("clear all ch: %d, subch: %d\n", i, j);
            executor_set_command(EX_MID_FREQ_CMD, EX_SUB_CH_ONOFF, i, &enable, j);
            executor_set_command(EX_MID_FREQ_CMD, EX_SUB_CH_DEC_METHOD, j, &default_method);
        }
    }
}
