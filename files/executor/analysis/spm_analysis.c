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
*  Rev 1.0   19 Oct. 2020   yaochangsong
*  Initial revision.
******************************************************************************/
#include <math.h>
#include "config.h"
#include "spm_analysis.h"
#include "../spm/spm_chip.h"


struct spm_pool pool;
struct spm_analysis_info *sa_info = NULL;
/* Spectrum thread condition variable  */
pthread_cond_t spectrum_analysis_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t spectrum_analysis_cond_mutex = PTHREAD_MUTEX_INITIALIZER;

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


/* get the signal bandwidth */
static inline uint64_t spectrum_get_signal_bandwidth(struct spm_run_parm *param)
{
    return (param->e_freq - param->s_freq > 0? (param->e_freq - param->s_freq) : 0);
}

/* get the signal middle frequency */
static inline uint64_t spectrum_get_signal_middle_freq(struct spm_run_parm *param)
{
    return (param->s_freq + spectrum_get_signal_bandwidth(param)/2);
}

/* Whether the signal analysis bandwidth exceeds the maximum operating bandwidth of the radio */
static inline bool spectrum_is_more_than_one_fragment(struct spm_run_parm *param)
{
    if(param->total_fft > 1){
        return true;
    }
    return false;
}

static inline int32_t spectrum_low_noise_level_calibration(uint64_t m_freq_hz)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    int32_t cal_value = 0, found = 0;

    cal_value += poal_config->cal_level.low_noise.global_power_val;
    return cal_value;
}

static inline  bool specturm_is_analysis_enable(int ch)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    if(poal_config->channel[ch].enable.spec_analy_en)
        return true;
    else
        return false;
}

/* get the whole rf work middle frequency, maybe not equal to signal middle frequency */
static inline uint64_t spectrum_get_work_middle_freq(struct spm_run_parm *param)
{
    uint64_t middle_freq = 0;
    /* If the signal analysis bandwidth exceeds the maximum operating bandwidth of the RF,
       Then our analysis bandwidth is an integer multiple of the RF working bandwidth, 
       otherwise the analysis bandwidth is based on the analysis bandwidth of the actual signal.
    */
    if(spectrum_is_more_than_one_fragment(param)){
        middle_freq = param->s_freq + alignment_up(spectrum_get_signal_bandwidth(param), param->scan_bw)/2;
    }else{
        middle_freq = spectrum_get_signal_middle_freq(param);
    }
    return  middle_freq;
}


static size_t spectrum_iq_convert_fft_store_mm_pool(memory_pool_t  *fftmpool, memory_pool_t  *iqmpool)
{
    void *p_dat, *p_end;
    size_t p_inc;
    size_t fft_size = memory_pool_step(fftmpool)/4;
    
    
    printf_debug("GET FFT size: %d\n", fft_size);
    
    p_end = memory_pool_end(iqmpool);
    p_inc = memory_pool_step(iqmpool);
    
    printf_debug("pool_first=%p,pool_end =%p, pool_step=%p\n", 
        memory_pool_first(iqmpool), memory_pool_end(iqmpool), memory_pool_step(iqmpool));
    for (p_dat = memory_pool_first(iqmpool); p_dat < p_end; p_dat += p_inc) {
        float *s_fft_mp = (float *)memory_pool_alloc(fftmpool);
        fft_spectrum_iq_to_fft_handle_dup((int16_t *)p_dat, fft_size, fft_size*2, s_fft_mp);
        for(int i = 0; i<10; i++)
            printfd("%f ", *((float *)s_fft_mp+i));
        printfd("\n");
    }
    return (fft_size*memory_pool_get_use_count(fftmpool));
}

static inline float calc_resolution(uint32_t bw_hz, uint32_t fft_size)  
{
        return (get_side_band_rate(bw_hz)*bw_hz/fft_size);
}

#define SIDE_BAND_RATE_1_28  (1.28)
#define SIDE_BAND_RATE_1_2228  (1.2288)
#define SINGLE_SIDE_BAND_POINT_RATE_1_28  (0.109375)                  /* (1-1/1.28)/2 */
#define SINGLE_SIDE_BAND_POINT_RATE_1_2228  (0.093098958333333333335) /* (1-1/1.2288)/2 */
#define DEFAULT_SINGLE_SIDE_BAND_POINT_RATE  SINGLE_SIDE_BAND_POINT_RATE_1_28


static inline double get_single_side_band_pointrate(uint32_t bw)
{
    float sidebind_rate;
    sidebind_rate = get_side_band_rate(bw);
    if(f_sgn(sidebind_rate - SIDE_BAND_RATE_1_28) == 0)
        return SINGLE_SIDE_BAND_POINT_RATE_1_28;
    else if(f_sgn(sidebind_rate - SIDE_BAND_RATE_1_2228) == 0)
        return SINGLE_SIDE_BAND_POINT_RATE_1_2228;
    else
        return DEFAULT_SINGLE_SIDE_BAND_POINT_RATE;
}

/* load specturm analysis paramter */
static int8_t inline spectrum_get_analysis_paramter(struct spm_run_parm *param, 
                                                              uint32_t *_analysis_bw, uint64_t *midd_freq_offset)
{
    uint32_t analysis_signal_bw;
    uint64_t analysis_signal_middle_freq;
    uint64_t s_freq = param->s_freq, e_freq = param->e_freq;
    uint32_t wk_bandwidth=param->scan_bw;

    analysis_signal_bw = spectrum_get_signal_bandwidth(param);//poal_config->ctrl_para.specturm_analysis_param.bandwidth_hz;
    analysis_signal_middle_freq =  spectrum_get_signal_middle_freq(param);
    
    if(analysis_signal_middle_freq < s_freq || analysis_signal_middle_freq > e_freq){
        printf_err("analysis frequency[%llu] is not  NOT within the bandwidth range[%llu, %llu]\n", analysis_signal_middle_freq, s_freq, e_freq);
        return -1;
    }
    if(analysis_signal_bw > wk_bandwidth*SPECTRUM_MAX_SCAN_COUNT || analysis_signal_bw == 0){
        printf_err("analysis bandwidth [%u] not surpport!!\n", analysis_signal_bw);
        return -1;
    }
    
    *_analysis_bw = analysis_signal_bw;

    /* Calculate the center frequency offset value (relative to the rf operating bandwidth) for spectrum analysis */
    if(analysis_signal_middle_freq <= wk_bandwidth/2){
        if(s_freq >= delta_bw){
            *midd_freq_offset = wk_bandwidth/2+delta_bw - analysis_signal_middle_freq;
        }else{
            printf_err("start frequency[%llu] is too small\n", s_freq);
            return -1;
        }
        
    }else{
        //*midd_freq_offset = 0;
        /*real rf work middle frequency - signal middle frequency */
        *midd_freq_offset = spectrum_get_work_middle_freq(param) - analysis_signal_middle_freq;
    }
    
    return 0;
}

static inline int8_t spectrum_sideband_deal_mm_pool(memory_pool_t  *fftmpool, memory_pool_t  *rsb_fftmpool, uint32_t bandwidth_hz)
{   
    uint32_t extra_data_single_size = 0;
    void *p_dat, *p_end;
    size_t p_inc, rsb_fft_size;
    size_t fft_size = memory_pool_step(fftmpool)/4;

    p_end = memory_pool_end(fftmpool);
    p_inc = memory_pool_step(fftmpool);

    extra_data_single_size = fft_size * get_single_side_band_pointrate(bandwidth_hz);//alignment_down(fft_size * SINGLE_SIDE_BAND_POINT_RATE, sizeof(float));
    printf_note("##extra_data_single_size=%u, fft_size=%u, pool_step=%u, use=%d\n", extra_data_single_size, fft_size, memory_pool_step(fftmpool), memory_pool_get_use_count(fftmpool));
    rsb_fft_size = fft_size/get_side_band_rate(bandwidth_hz);  //fft_size-2 * extra_data_single_size;
    if(rsb_fft_size > memory_pool_step(rsb_fftmpool)/4){
        printf_err("remove side band size[%u] is bigger than raw fft size[%u]\n", rsb_fft_size, memory_pool_step(rsb_fftmpool));
        return -1;
    }
    for (p_dat = memory_pool_first(fftmpool); p_dat < p_end; p_dat += p_inc) {
        memory_pool_write_value(memory_pool_alloc(rsb_fftmpool), (uint8_t *)p_dat+extra_data_single_size*4, rsb_fft_size*4);
    }
    memory_pool_set_pool_step(rsb_fftmpool, rsb_fft_size*4);
    return 0;
}

static int spectrum_data_write_file(memory_pool_t  *mpool, uint8_t data_type_len, int index)
{
    char strbuf[128];
    void *p_dat, *p_end;
    size_t p_inc;

    p_end = memory_pool_end(mpool);
    p_inc = memory_pool_step(mpool);

    sprintf(strbuf, "/run/%s%d", data_type_len == sizeof(int16_t) ? "IQ":"FFT",  index);
    printf_info("##start save iq:%s, %d, %d\n", strbuf, p_inc, memory_pool_get_use_count(mpool));
    if(data_type_len == sizeof(int16_t)){  //int16
        write_file_in_int16((void*)(memory_pool_first(mpool)), p_inc * memory_pool_get_use_count(mpool)/data_type_len, strbuf);
    }
    else if(data_type_len == sizeof(float)){ //float
        write_file_in_float((void*)(memory_pool_first(mpool)), p_inc * memory_pool_get_use_count(mpool)/data_type_len, strbuf);
    }
    return 0;
}

/*  GET analysis result info; 
    NOTE: need to free @real 
 */
ssize_t spm_analysis_get_info(struct spectrum_analysis_result_vec **real)
{
    uint32_t number = 0;
    struct spectrum_analysis_result_vec **presult;
    int i;
    struct spm_analysis_info *ptr = sa_info;

    if(ptr == NULL)
        return -1;
    
    pthread_mutex_lock(&ptr->mutex);
    
    number = ptr->number;
    if(number == 0){
        *real = NULL;
        goto exit;
    }
        
    presult = safe_malloc(number*sizeof(intptr_t));
    for(i = 0; i < number; i++){
        presult[i] = safe_malloc(sizeof(struct spectrum_analysis_result_vec));
    }

    for(i = 0; i< number; i++){
        memcpy(presult[i], &ptr->result[i], sizeof(struct spectrum_analysis_result_vec));
    }
    
    *real = presult;
exit:
    pthread_mutex_unlock(&ptr->mutex);
    return number;
}

static int spm_analysis_deal(uint64_t s_freq_hz, float freq_resolution, uint32_t fft_size, struct spm_run_parm *param)
{
    int i, _len;
    uint64_t _signal_start_freq = 0, _signal_end_freq = 0;
    fft_result *r= fft_spectrum_get_result();
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    static uint32_t _sig_number_dup = 0;
    uint64_t analysis_middle_freq;
    struct spm_analysis_info *ptr = sa_info;
    if(ptr == NULL)
        return -1;
    pthread_mutex_lock(&ptr->mutex);
    if(r->signalsnumber == 0){
        goto exit;
    }
    if(_sig_number_dup < r->signalsnumber){
        _sig_number_dup = r->signalsnumber;
        _len = sizeof(struct spm_analysis_info) + sizeof(struct spectrum_analysis_result_vec) * r->signalsnumber;
        ptr = realloc(ptr, _len);
        sa_info = ptr;
    }
    if(ptr == NULL){
        return -1;
    }
    
    for(i = 0; i < r->signalsnumber; i++){
        _signal_start_freq = s_freq_hz + r->centfeqpoint[i] *freq_resolution - r->bandwidth[i] * freq_resolution/2;
        _signal_end_freq   = s_freq_hz + r->centfeqpoint[i] *freq_resolution + r->bandwidth[i] * freq_resolution/2;
    }
    /* Determine if the analysis point is within the signal range */
    if((poal_config->ctrl_para.specturm_analysis_param.frequency_hz < _signal_start_freq) || 
    (poal_config->ctrl_para.specturm_analysis_param.frequency_hz > _signal_end_freq)){
        printf_warn("The analysis point[%llu] is NOT within the signal range[%llu, %llu]\n", 
            poal_config->ctrl_para.specturm_analysis_param.frequency_hz, _signal_start_freq, _signal_end_freq);
    }

    for(i = 0; i < r->signalsnumber; i++){
        ptr->result[i].mid_freq_hz = s_freq_hz + r->centfeqpoint[i]*freq_resolution;
        ptr->result[i].peak_value = s_freq_hz + r->maximum_x*freq_resolution;
        ptr->result[i].bw_hz = r->bandwidth[i] * freq_resolution;
        analysis_middle_freq =  spectrum_get_signal_middle_freq(param);
        ptr->result[i].level = r->arvcentfreq[i]+config_get_analysis_calibration_value(analysis_middle_freq);
        printf_warn("m_freq=%llu,s_freq_hz=%llu, freq_resolution=%f, fft_size=%u\n", analysis_middle_freq, s_freq_hz, freq_resolution, fft_size);
        printf_warn("mid_freq_hz=%d, bw_hz=%d, level=%f\n", r->centfeqpoint[i], r->bandwidth[i], r->arvcentfreq[i]);
        printf_warn("mid_freq_hz=%llu, peak_value=%u, bw_hz=%u,level=%f\n", 
            ptr->result[i].mid_freq_hz, ptr->result[i].peak_value, ptr->result[i].bw_hz,ptr->result[i].level);
    }
    
exit:
    ptr->number = r->signalsnumber;
    pthread_mutex_unlock(&ptr->mutex);
    return 0;
}


void specturm_analysis_deal_thread(void *arg)
{
    memory_pool_t *iq_mpool = pool.iq;
    memory_pool_t *fft_small_mpool = pool.fft_raw_small;
    memory_pool_t *fft_big_mpool = pool.fft_raw_big;
    memory_pool_t *fft_small_rsb_mpool = pool.fft_s_deal;
    memory_pool_t *fft_big_rsb_mpool = pool.fft_b_deal;
    int small_fft_size = 0, big_fft_size = 0;
    uint32_t small_fft_rsb_size = 0, big_fft_rsb_size = 0;
    struct spm_run_parm *param;
    float resolution;
    uint64_t analysis_midd_freq_offset, bw_fft_size, analysis_signal_start_freq;
    uint32_t analysis_bw; 
    uint32_t total_bw; 

loop:
    while(1){
         /* wait to product iq data */
        printf_note("###wait to analysis specturm###\n");
        //sem_wait(&sp_sem.notify_iq_to_fft);
        
        /* Mutex must be locked for pthread_cond_timedwait... */
        pthread_mutex_lock(&spectrum_analysis_cond_mutex);
        /* Thread safe "sleep" */
        pthread_cond_wait(&spectrum_analysis_cond, &spectrum_analysis_cond_mutex);
        /* No longer needs to be locked */
        pthread_mutex_unlock(&spectrum_analysis_cond_mutex);
           
        /* Here we have get IQ data in memory pool (name: iq_mpool)*/
        //printf_info("###Save IQ data###\n");
        //spectrum_data_write_file(iq_mpool, sizeof(int16_t), 0);

        printf_note("###STEP1: Write Attr to  Small Pool###\n");
        memory_pool_write_attr_value(fft_small_mpool, memory_pool_get_attr(iq_mpool), memory_pool_get_attr_len(iq_mpool));
        
        param = (struct spm_run_parm *)memory_pool_get_attr(fft_small_mpool);
        printf_note("scan bandwidth=%uHz, s_freq=%llu, e_freq=%llu\n", param->scan_bw, param->s_freq, param->e_freq);
        
        /* --1step IQ convert 4K(small) fft, and store fft data to small memory pool  */
        printf_note("###STEP2: IQ Convert to Small FFT###\n");
        small_fft_size = spectrum_iq_convert_fft_store_mm_pool(fft_small_mpool, iq_mpool);
        //printf_info("###Save FFT data###\n");
        /* remove side band */
        if(spectrum_sideband_deal_mm_pool(fft_small_mpool, fft_small_rsb_mpool, param->scan_bw) == -1){
            sleep(1);
            goto exit;
        }
        //spectrum_data_write_file(fft_small_rsb_mpool, sizeof(float), 1);
        
        /* --2 step: IQ convet 128K(big) fft, and store fft data to big memory pool */
        printf_note("###STEP4: IQ Convert to Big FFT###\n");
        big_fft_size = spectrum_iq_convert_fft_store_mm_pool(fft_big_mpool, iq_mpool);
        /* remove side band */
        if(spectrum_sideband_deal_mm_pool(fft_big_mpool, fft_big_rsb_mpool, param->scan_bw) == -1){
            sleep(1);
            goto exit;
        }
        //printf_info("###Save Big FFT data###\n");
        //spectrum_data_write_file(fft_big_rsb_mpool, sizeof(float), 2);

        /* now, we can refill IQ data to memory pool again */
        
        printf_note("###IQ Deal Over,Free, Wait New IQ data###\n");

        /* load specturm analysis paramter: analysis_bw, analysis_midd_freq_offset*/
        if(spectrum_get_analysis_paramter(param, &analysis_bw,   &analysis_midd_freq_offset) == -1){
            sleep(1);
            goto exit;
       }
        
        total_bw = param->scan_bw*memory_pool_get_use_count(fft_big_mpool);
        small_fft_rsb_size = memory_pool_step(fft_small_rsb_mpool)*memory_pool_get_use_count(fft_small_rsb_mpool)/4;
        big_fft_rsb_size = memory_pool_step(fft_big_rsb_mpool)*memory_pool_get_use_count(fft_big_rsb_mpool)/4;
        
        printf_note("analysis_bw=%u, analysis_midd_freq_offset=%llu\n", analysis_bw, analysis_midd_freq_offset);
        printf_note("small fft data len=%d, use_count=%d\n", small_fft_size, memory_pool_get_use_count(fft_small_mpool));
        printf_note("big fft data len=%d, use_count=%d\n", big_fft_size, memory_pool_get_use_count(fft_big_mpool));
        printf_note("fft small mpool step =%u, big fft size step=%u\n", memory_pool_step(fft_small_mpool), memory_pool_step(fft_big_mpool));
        
        printf_note("rsb small fft data len=%d, use_count=%d\n", small_fft_rsb_size, memory_pool_get_use_count(fft_small_rsb_mpool));
        printf_note("rsb big fft data len=%d, use_count=%d\n", big_fft_rsb_size, memory_pool_get_use_count(fft_big_rsb_mpool));
        printf_note("rsb fft small mpool step=%u, rsb big fft mpool step=%u\n", memory_pool_step(fft_small_rsb_mpool), memory_pool_step(fft_big_rsb_mpool));
        
        printf_note("work total bw=%u, big_fft_size/small_fft_size=%d\n", total_bw, big_fft_size/small_fft_size);
        printf_note("###STEP4: v2. Start to analysis FFT data###\n");

        TIME_ELAPSED(fft_spectrum_fftdata_handle(get_power_level_threshold(),       /* signal threshold */
                    (float *)memory_pool_first(fft_small_rsb_mpool),                /* remove side band small fft data */
                    small_fft_rsb_size,                                             /* small fft data len */
                    (float *)memory_pool_first(fft_big_rsb_mpool),                  /* remove side band big fft data */
                    big_fft_rsb_size,                                               /* big fft data len */
                    analysis_midd_freq_offset,                                      /* analysis signal middle frequency */
                    analysis_bw,                                                    /* analysis signal bandwidth */
                    total_bw,                                                       /* total bw */
                    big_fft_size/small_fft_size
                    ));                                                 

        resolution = calc_resolution(total_bw, big_fft_size);
        bw_fft_size = ((float)analysis_bw/(float)total_bw) *big_fft_size ;
        analysis_signal_start_freq =  spectrum_get_signal_middle_freq(param) - analysis_bw/2;
        printf_note("analysis_signal_start_freq = %llu,s_freq=%llu, resolution=%f, bw_fft_size=%llu\n", analysis_signal_start_freq, param->s_freq, resolution, bw_fft_size);
        //spectrum_rw_fft_result(fft_spectrum_get_result(), analysis_signal_start_freq, resolution, bw_fft_size,param);
        spm_analysis_deal(analysis_signal_start_freq, resolution, bw_fft_size,param);
exit:
        memory_pool_free(fft_small_mpool);
        memory_pool_free(fft_big_mpool);
        memory_pool_free(fft_small_rsb_mpool);
        memory_pool_free(fft_big_rsb_mpool);
        memory_pool_free(iq_mpool);
        pool.iq_ready = false;
    }
}

static int specturm_write_oneframe_iq_data_to_buffer(struct spm_run_parm *param, void *data, size_t nbyte)
{
    static bool is_sysn_write= false;
    memory_pool_t *iq_mpool = pool.iq;
    static int old_fft_sn = -1;

    if(param->total_fft > memory_pool_get_count(iq_mpool)){
        printf_err("One Frame FFT scan count is more than memory pool count!!!\n");
        return -1;
    }

    if((0 == param->fft_sn) && (pool.iq_ready == false) && (old_fft_sn != param->fft_sn)){
        is_sysn_write = true;
    }
    printf_debug("fft_sn=%u, total_fft=%u, %d, iq_data_ready=%d,is_sysn_write=%d\n", param->fft_sn, param->total_fft, memory_pool_get_count(iq_mpool), pool.iq_ready,is_sysn_write);
    if((is_sysn_write == true) && (old_fft_sn != param->fft_sn)){
        printf_debug("sn:%d, total_fft=%d\n", param->fft_sn, param->total_fft);
        memory_pool_write_value(memory_pool_alloc(iq_mpool), data, nbyte);
        //printf_note("use_count=%d\n", memory_pool_get_use_count(iq_mpool));
        if(old_fft_sn != param->fft_sn){
            old_fft_sn = param->fft_sn;
        }
        if(param->fft_sn == param->total_fft-1){
            printf_note("write One frame IQ data ok\n");
            pool.iq_ready = true;
            is_sysn_write = false;
            /* save spectrum perameter */
            printf_note("iq_mpool step:%d\n", memory_pool_step(iq_mpool));
            memory_pool_write_attr_value(iq_mpool, param, sizeof(struct spm_run_parm));
            old_fft_sn = -1;
            return 0;
        }
    }
    return -1;
}

void spm_analysis_start(int ch, void *data, size_t data_len, void *args)
{
    struct spm_run_parm *param;
    
    if(specturm_is_analysis_enable(ch)){
        param = (struct spm_run_parm *)args;
        if(!specturm_write_oneframe_iq_data_to_buffer(param, data, data_len)){
            /* notify thread can start to conver iq to fft */
            printf_note("##New Frame IQ Data Created, Notify to Deal##\n");
            pthread_cond_signal(&spectrum_analysis_cond);
        }
    }
}

void spm_analysis_init(void)
{
    #define  SPECTRUM_SMALL_FFT_SIZE   (2048)
    #define  SPECTRUM_IQ_SIZE   RF_ADRV9009_IQ_SIZE
    #define  SPECTRUM_BIG_FFT_SIZE  (SPECTRUM_IQ_SIZE/2)
    int ret, i;
    pthread_t work_id;

    memset(&pool, 0, sizeof(pool));
    /* create memory pool */
    /* 128K fft, 1fft=4byte */
    pool.iq = memory_pool_create(SPECTRUM_IQ_SIZE*4, SPECTRUM_MAX_SCAN_COUNT);
    pool.fft_raw_small = memory_pool_create(SPECTRUM_SMALL_FFT_SIZE*4, SPECTRUM_MAX_SCAN_COUNT);
    pool.fft_raw_big = memory_pool_create(SPECTRUM_BIG_FFT_SIZE*4, SPECTRUM_MAX_SCAN_COUNT);

    pool.fft_s_deal = memory_pool_create(SPECTRUM_SMALL_FFT_SIZE*4, SPECTRUM_MAX_SCAN_COUNT);
    pool.fft_b_deal = memory_pool_create(SPECTRUM_BIG_FFT_SIZE*4, SPECTRUM_MAX_SCAN_COUNT);
    
    struct spm_analysis_info *ptr;
    ptr = safe_malloc(sizeof(struct spm_analysis_info));
    sa_info = ptr;

    ret=pthread_create(&work_id,NULL,(void *)specturm_analysis_deal_thread, NULL);
    if(ret!=0)
        perror("pthread cread work_id");
    pthread_detach(work_id);
}
