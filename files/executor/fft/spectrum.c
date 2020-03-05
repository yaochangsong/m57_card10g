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
*  Rev 1.0   28 July 2019   yaochangsong
*  Initial revision.
*  Rev 2.0   11 Oct. 2019   yaochangsong
*  add memory pool.
******************************************************************************/
#include "config.h"

struct spectrum_st _spectrum;

/*When the spectrum bandwidth is larger than the scan bandwidth, 
in order to calculate the FFT result of the entire spectrum, 
it is necessary to use the memory pool to store IQ and FFT intermediate data. 
*/
/* the memory pool to store IQ data */
memory_pool_t *mp_iq_buffer_head;
memory_pool_t *mp_iq_upload_buffer_head;

/* the memory pool to store FFT data: small & big */
memory_pool_t *mp_fft_small_buffer_head; /* for small FFT */
memory_pool_t *mp_fft_small_buffer_rsb_head; /* for small remove side band FFT */

memory_pool_t *mp_fft_big_buffer_head;   /* for bit FFT */
memory_pool_t *mp_fft_big_buffer_rsb_head;   /* for remove side band FFT */



pthread_mutex_t spectrum_result_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t spectrum_data_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t spectrum_iq_data_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Spectrum thread condition variable  */
pthread_cond_t spectrum_analysis_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t spectrum_analysis_cond_mutex = PTHREAD_MUTEX_INITIALIZER;
/* iq upload thread condition variable  */
pthread_cond_t spectrum_iq_upload_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t spectrum_iq_upload_cond_mutex = PTHREAD_MUTEX_INITIALIZER;


struct sp_sem_nofity_st sp_sem;

static inline float get_side_band_rate(void)
{
    float sidebind_rate;
    if(config_read_by_cmd(EX_CTRL_CMD, EX_CTRL_SIDEBAND, 0, &sidebind_rate) == -1){
        printf_note("Error read scan sidebind_rate, use default sidebind_rate[%f]\n", DEFAULT_SIDE_BAND_RATE);
        return DEFAULT_SIDE_BAND_RATE;
    }
    //printf_warn("sidebind_rate=%.6f, %d\n", sidebind_rate, f_sgn(sidebind_rate - SIDE_BAND_RATE_1_2228));
    if(f_sgn(sidebind_rate - SIDE_BAND_RATE_1_28) == 0){
        return SIDE_BAND_RATE_1_28;
    }
    else if(f_sgn(sidebind_rate - SIDE_BAND_RATE_1_2228) == 0){
        return SIDE_BAND_RATE_1_2228;
    }
    else{
        return SIDE_BAND_RATE_1_28;
    }
}

static inline double get_single_side_band_pointrate(void)
{
    float sidebind_rate;
    sidebind_rate = get_side_band_rate();
    if(f_sgn(sidebind_rate - SIDE_BAND_RATE_1_28) == 0)
        return SINGLE_SIDE_BAND_POINT_RATE_1_28;
    else if(f_sgn(sidebind_rate - SIDE_BAND_RATE_1_2228) == 0)
        return SINGLE_SIDE_BAND_POINT_RATE_1_2228;
    else
        return DEFAULT_SINGLE_SIDE_BAND_POINT_RATE;
}


static inline float calc_resolution(uint32_t bw_hz, uint32_t fft_size)  
{
        return (get_side_band_rate()*bw_hz/fft_size);
}

static inline  bool specturm_is_analysis_enable(void)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    if(poal_config->enable.spec_analy_en == 1)
        return true;
    else
        return false;
}

static inline  bool specturm_is_psd_enable(void)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    if(poal_config->enable.psd_en == 1)
        return true;
    else
        return false;
}

static inline  bool specturm_is_iq_enable(void)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    if(poal_config->enable.iq_en == 1 || poal_config->sub_ch_enable.iq_en == 1)
        return true;
    else
        return false;
}


/* Whether the signal analysis bandwidth exceeds the maximum operating bandwidth of the radio */
static inline bool spectrum_is_more_than_one_fragment(void)
{
    struct spectrum_st *ps;
    ps = &_spectrum;
    if(ps->param.total_fft > 1){
        return true;
    }
    return false;
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

/* get the whole rf work middle frequency, maybe not equal to signal middle frequency */
static inline uint64_t spectrum_get_work_middle_freq(struct spm_run_parm *param)
{
    uint64_t middle_freq = 0;
    /* If the signal analysis bandwidth exceeds the maximum operating bandwidth of the RF,
       Then our analysis bandwidth is an integer multiple of the RF working bandwidth, 
       otherwise the analysis bandwidth is based on the analysis bandwidth of the actual signal.
    */
    if(spectrum_is_more_than_one_fragment()){
        middle_freq = param->s_freq + alignment_up(spectrum_get_signal_bandwidth(param), param->scan_bw)/2;
    }else{
        middle_freq = spectrum_get_signal_middle_freq(param);
    }
    return  middle_freq;
}


/* Read Rx Raw IQ data*/
static int16_t *specturm_rx_iq_read(int32_t *nbytes)
{
    ssize_t nbytes_rx = 0;
    int16_t *iqdata, *p_iqdata_start;
    int i;
    char strbuf[128];
    static int write_file_cnter = 0;
    
    LOCK_IQ_DATA();
    iqdata = specturm_rx0_read_data(&nbytes_rx);
    p_iqdata_start = iqdata;
    if(p_iqdata_start == NULL){
        nbytes_rx = 0;
        goto exit;
    }
/*    
    if(write_file_cnter++ ==10) {
        sprintf(strbuf, "/run/wav_%d", write_file_cnter);
        printfi("write rx0 iq data to:[len=%d]%s\n", nbytes_rx/2, strbuf);
        write_file_in_int16((void*)(iqdata), nbytes_rx/2, strbuf);
    }
*/     
 //   printf_note("iio read data len:[%d]\n", nbytes_rx);
//    printf_note("rx0 iqdata:\n");
//    for(i = 0; i< 10; i++){
//        printfi("%d ",*(int16_t*)(iqdata+i));
 //   }
 //   printfi("\n");
   
exit:
    UNLOCK_IQ_DATA();
    *nbytes = nbytes_rx;
    return p_iqdata_start;
}



/* Send FFT spectrum Data to Client (UDP)*/
static  void spectrum_send_fft_data(void *fft_data, size_t fft_data_len, void *param)
{
    #if 1
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    uint8_t *fft_sendbuf, *pbuf;
    uint8_t *header_buf;
    uint32_t header_len = 0;

    /* fill header data len(byte) */
    struct spm_run_parm *hparam;
    hparam = (struct spm_run_parm *)param;
    hparam->data_len = fft_data_len; 
    hparam->type = SPECTRUM_DATUM_FLOAT;
    hparam->ex_type = SPECTRUM_DATUM;

    header_buf = poal_config->assamble_response_data(&header_len, (void *)param);
    if(header_buf == NULL){
        printf_err("send error!\n");
        return;
    }
    printf_debug("header_buf=%x %x, header_len=%d, malloc len=%u\n", header_buf[0],header_buf[1], header_len, header_len+ fft_data_len);

    fft_sendbuf = (uint8_t *)safe_malloc(header_len+ fft_data_len);
    if (!fft_sendbuf){
        printf_err("malloc failed\n");
        return;
    }
    pbuf = fft_sendbuf;
    printf_debug("malloc ok header_len:%d,fft_len=%d, len=%d\n",header_len,fft_data_len, header_len+ fft_data_len);
    memcpy(fft_sendbuf, header_buf, header_len);
    memcpy(fft_sendbuf + header_len, fft_data, fft_data_len);
    udp_send_data(fft_sendbuf, header_len + fft_data_len);
    safe_free(pbuf);
#endif
}

static inline void spectrum_send_iq_data(void *iq_data, size_t iq_data_len, void *param)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    uint8_t *iq_sendbuf, *pbuf;
    uint8_t *header_buf;
    uint32_t header_len = 0;

    /* fill header data len(byte) */
    struct spm_run_parm *hparam;
    hparam = (struct spm_run_parm *)param;
    hparam->data_len = iq_data_len; 
    hparam->type = BASEBAND_DATUM_IQ;
    hparam->ex_type = DEMODULATE_DATUM;

    header_buf = poal_config->assamble_response_data(&header_len, (void *)param);
    if(header_buf == NULL){
        printf_err("send error!\n");
        return;
    }
    printf_debug("header_buf=%x %x, header_len=%d, malloc len=%d\n", header_buf[0],header_buf[1], header_len, header_len+ iq_data_len);

    iq_sendbuf = (uint8_t *)safe_malloc(header_len+ iq_data_len);
    if (!iq_sendbuf){
        printf_err("malloc failed\n");
        return;
    }
    pbuf = iq_sendbuf;
    printf_debug("malloc ok header_len:%d,fft_len=%d, len=%d\n",header_len,iq_data_len, header_len+ iq_data_len);
    memcpy(iq_sendbuf, header_buf, header_len);
    memcpy(iq_sendbuf + header_len, iq_data, iq_data_len);
    udp_send_data(iq_sendbuf, header_len + iq_data_len);
    safe_free(pbuf);
}



/* Before send client, we need to remove sideband signals data  and  extra bandwidth data */
fft_data_type *spectrum_fft_data_order(struct spectrum_st *ps, uint32_t *result_fft_size)
{
   
    uint32_t extra_data_single_size = 0;
    /*-- 1 step:  
       remove sideband signals data
   */
    /* single sideband signal fft size */
    printf_debug("side_band_pointrate:%f\n", get_single_side_band_pointrate());
    uint32_t single_sideband_size = ps->fft_len*get_single_side_band_pointrate();
    
    /* odd number */
   // if(!EVEN(single_sideband_size)){
  //      single_sideband_size++;
  //  }

    *result_fft_size = ps->fft_len - 2 * single_sideband_size;
    printf_debug("ps->fft_len=%u, single_sideband_size=%u, result_fft_size=%u\n", ps->fft_len, single_sideband_size, *result_fft_size); 


    /*-- 2 step: when middle frequency is less than BW/2 [100MHz], we need to resetting middle frequency
               (see middle_freq_resetting function) ; when sending data, we need to restore the settings and remove 
               the redundant data.
                the left extra data:  (mf0 - bw0/2 - deltabw)* fftSize /BW; [mf0 - bw0/2 >= deltabw]
                the right extra data: (BW + deltabw - mf0 - bw0/2) *fftSize / BW; [BW + deltabw >= mf0 + bw0/2]
     */
    uint32_t left_extra_single_size = 0, right_extra_single_size = 0;
    if((ps->param.m_freq <= ps->param.scan_bw/2) && (ps->param.m_freq > 0)){
        if(ps->param.m_freq - ps->param.bandwidth/2 >= delta_bw){
            left_extra_single_size = (ps->param.m_freq - ps->param.bandwidth/2 - delta_bw) * (*result_fft_size) / ps->param.scan_bw;
        }else{
            left_extra_single_size = 0;
        }
        if(ps->param.scan_bw + delta_bw > ps->param.m_freq + ps->param.bandwidth/2){
            right_extra_single_size = (ps->param.scan_bw + delta_bw - ps->param.m_freq - ps->param.bandwidth/2) * (*result_fft_size) / ps->param.scan_bw;
        }else{
            right_extra_single_size = 0;
        }
        if(*result_fft_size > (right_extra_single_size + left_extra_single_size)){
            *result_fft_size = *result_fft_size - right_extra_single_size - left_extra_single_size;
        }
        printf_info("m_freq = %llu, bandwidth= %u, delta_bw=%llu, left=%u, right=%u,*result_fft_size=%u\n", ps->param.m_freq, ps->param.bandwidth, delta_bw, left_extra_single_size, right_extra_single_size,*result_fft_size);
    }else{
        /*-- 3 step:  
           If the remaining bandwidth is less than the actual working bandwidth,
           we need to remove the extra data. 
        */
        /* the left extra data bw range: (mFq-BW/2, mFq-ScanBW/2);   mFq is middle freqency;
           the rigth extra data bw range: (mFq+BW/2, mFq+ScanBW/2); 
              so, the single extra bw is: (BW-ScanBw)/2, It accounts for the working bandwidth ratio: 
              (BW-ScanBw)/2/BW = (1-ScanBw/Bw)/2
              There Bw is: RF_BANDWIDTH, ScanBw is ps->param.bandwidth,
               ScanBw must less than or equal to Bw
        */
        if((ps->param.scan_bw > ps->param.bandwidth) && (ps->param.bandwidth > 0)){
            extra_data_single_size = ((1-(float)ps->param.bandwidth/(float)ps->param.scan_bw)/2) * (*result_fft_size);
            printf_debug("bw:%u, m_freq:%llu,need to remove the single extra data: %f,%u,%u\n",ps->param.bandwidth, ps->param.m_freq, (1-(float)ps->param.bandwidth/(float)ps->param.scan_bw)/2, *result_fft_size, extra_data_single_size);
            if(ps->fft_len <  extra_data_single_size *2){
                *result_fft_size = 0;
            }else{
                *result_fft_size = *result_fft_size - extra_data_single_size *2;
            }
            printf_debug("the remaining fftdata is: %u\n", *result_fft_size);
        }
    }     

    /* Returning data requires removing sideband data and excess bandwidth data */
 exit:
    return (fft_data_type *)((fft_data_type *)ps->fft_short_payload + single_sideband_size + extra_data_single_size + left_extra_single_size);
}


static int specturm_write_oneframe_iq_data_to_buffer_for_analysis(struct spm_run_parm *param, void *data, size_t nbyte)
{
    static bool is_sysn_write= false;
    memory_pool_t *iq_mpool = mp_iq_buffer_head;
    struct spectrum_st *ps;
    ps = &_spectrum;
    static int old_fft_sn = -1;

    if(iq_mpool == NULL)
        return -1;
    if(param->total_fft > memory_pool_get_count(iq_mpool)){
        printf_err("One Frame FFT scan count is more than memory pool count!!!\n");
        return -1;
    }

    if((0 == param->fft_sn) && (ps->iq_data_ready == false) && (old_fft_sn != param->fft_sn)){
        is_sysn_write = true;
    }
    printf_debug("fft_sn=%u, total_fft=%u, %d, iq_data_ready=%d,is_sysn_write=%d\n", param->fft_sn, param->total_fft, memory_pool_get_count(iq_mpool), ps->iq_data_ready,is_sysn_write);
    if((is_sysn_write == true) && (old_fft_sn != param->fft_sn)){
        printf_debug("sn:%d, total_fft=%d\n", param->fft_sn, param->total_fft);
        memory_pool_write_value(memory_pool_alloc(iq_mpool), data, nbyte);
        printf_note("use_count=%d\n", memory_pool_get_use_count(iq_mpool));
        if(old_fft_sn != param->fft_sn){
            old_fft_sn = param->fft_sn;
        }
        if(param->fft_sn == param->total_fft-1){
            printf_info("write One frame IQ data ok\n");
            ps->iq_data_ready = true;
            is_sysn_write = false;
            /* save spectrum perameter */
            printf_debug("iq_mpool step:%d\n", memory_pool_step(iq_mpool));
            memory_pool_write_attr_value(iq_mpool, param, sizeof(struct spm_run_parm));
            old_fft_sn = -1;
            return 0;
        }
    }
    return -1;
}

static int specturm_write_oneframe_iq_data_to_buffer_for_upload(struct spm_run_parm *param, void *data, size_t nbyte)
{
    static bool is_sysn_write= false;
    memory_pool_t *iq_mpool = mp_iq_upload_buffer_head;
    struct spectrum_st *ps;
    ps = &_spectrum;
    static int old_fft_sn = -1;

    if(iq_mpool == NULL)
        return -1;
    if(param->total_fft > memory_pool_get_count(iq_mpool)){
        printf_err("One Frame FFT scan count is more than memory pool count!!!\n");
        return -1;
    }

    if((0 == param->fft_sn) && (ps->iq_data_upload_ready == false) && (old_fft_sn != param->fft_sn)){
        is_sysn_write = true;
    }
    printf_debug("fft_sn=%u, total_fft=%u, %d, iq_data_ready=%d,is_sysn_write=%d\n", param->fft_sn, param->total_fft, memory_pool_get_count(iq_mpool), ps->iq_data_ready,is_sysn_write);
    if((is_sysn_write == true) && (old_fft_sn != param->fft_sn)){
        printf_debug("sn:%d, total_fft=%d\n", param->fft_sn, param->total_fft);
        memory_pool_write_value(memory_pool_alloc(iq_mpool), data, nbyte);
        printf_note("use_count=%d\n", memory_pool_get_use_count(iq_mpool));
        if(old_fft_sn != param->fft_sn){
            old_fft_sn = param->fft_sn;
        }
        if(param->fft_sn == param->total_fft-1){
            printf_info("write One frame IQ data ok\n");
            ps->iq_data_upload_ready = true;
            is_sysn_write = false;
            /* save spectrum perameter */
            printf_debug("iq_mpool step:%d\n", memory_pool_step(iq_mpool));
            memory_pool_write_attr_value(iq_mpool, param, sizeof(struct spm_run_parm));
            old_fft_sn = -1;
            return 0;
        }
    }
    return -1;
}


/* We use this function to deal FFT spectrum wave in user space */
void spectrum_psd_user_deal(struct spm_run_parm *param)
{
    struct spectrum_st *ps;
    float resolution;
    uint32_t fft_size;
    float *fft_float_data;

    ps = &_spectrum;

    spectrum_mgc_calibration(param->ch, param->m_freq);
    /* Read Raw IQ data*/
    ps->iq_payload = specturm_rx_iq_read(&ps->iq_byte_len);
    if(ps->iq_byte_len == 0){
        printf_err("IQ data is NULL\n");
        return;
    }
    printf_debug("ps->iq_payload[0]=%d, %d,param->fft_size=%d, ps->iq_byte_len=%d\n", ps->iq_payload[0],ps->iq_payload[1], param->fft_size, ps->iq_byte_len);

#ifdef SUPPORT_SPECTRUM_FFT_ANALYSIS
    /* if spectrum analysis is enable, we start to save iq data(one frame) to memery pool; prepare to analysis spectrum */
    if(specturm_is_analysis_enable()){
        if(!specturm_write_oneframe_iq_data_to_buffer_for_analysis(param, ps->iq_payload, ps->iq_byte_len)){
            /* notify thread can start to conver iq to fft */
            printf_note("##New Frame IQ Data Created, Notify to Deal##\n");
            pthread_cond_signal(&spectrum_analysis_cond);
        }
    }
#endif

#ifdef SUPPORT_SPECTRUM_IQ_UPLOAD
    if(specturm_is_iq_enable()){
        if(!specturm_write_oneframe_iq_data_to_buffer_for_upload(param, ps->iq_payload, ps->iq_byte_len)){
            /* notify thread can start to upload iq data */
            printf_note("##New Frame IQ Data Created, Notify to uppoad IQ##\n");
            pthread_cond_signal(&spectrum_iq_upload_cond);
        }
    }
#endif
    if(specturm_is_psd_enable()){
        fft_size = param->fft_size;
        /* Start Convert IQ data to FFT */
        fft_float_data = (float *)safe_malloc(fft_size*4);
        ps->fft_float_payload = fft_float_data;
        fft_spectrum_iq_to_fft_handle(ps->iq_payload, fft_size, fft_size*2, ps->fft_float_payload);
        ps->fft_len = fft_size;
        if(ps->fft_len > sizeof(ps->fft_short_payload)/sizeof(fft_data_type)){
            printf_err("fft size[%u] is too big\n", ps->fft_len);
            return;
        }
        /* To the circumstances: the client's active acquisition of FFT data; Here we user backup buffer to store fft data; 
        when the scheduled task is processed[fft_data_ready=false],  we start filling in new fft data in back buffer [ps->fft_short_payload_back]*/
        LOCK_SP_DATA();
        /* Here we convert float to short integer processing */
        FLOAT_TO_SHORT(ps->fft_float_payload, ps->fft_short_payload, ps->fft_len);
        /*refill header parameter*/
        memcpy(&ps->param, param, sizeof(struct spm_run_parm));

        ps->fft_data_ready = true;

        //printf_warn("fft data and header is ready, notify to handle \n");
        UNLOCK_SP_DATA();
        if(is_spectrum_continuous_mode()){
            spectrum_send_fft_data_interval();
        }
        safe_free(fft_float_data);
    }
}

int32_t spectrum_analysis_level_calibration(uint64_t m_freq_hz)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    int32_t cal_value = 0, found = 0;
    int i;

    for(i = 0; i< sizeof(poal_config->cal_level.analysis.start_freq_khz)/sizeof(uint32_t); i++){
        if((m_freq_hz >= (uint64_t)poal_config->cal_level.analysis.start_freq_khz[i]*1000) && (m_freq_hz < (uint64_t)poal_config->cal_level.analysis.end_freq_khz[i]*1000)){
            cal_value = poal_config->cal_level.analysis.power_level[i];
            found = 1;
            break;
        }
    }
    if(found){
        printf_debug("find the calibration level: %llu, %d\n", m_freq_hz, cal_value);
    }else{
        printf_note("Not find the calibration level, use default value: %d\n", cal_value);
    }
    cal_value += poal_config->cal_level.analysis.global_roughly_power_lever;
    return cal_value;
}

static inline int32_t spectrum_low_noise_level_calibration(uint64_t m_freq_hz)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    int32_t cal_value = 0, found = 0;

    cal_value += poal_config->cal_level.low_noise.global_power_val;
    return cal_value;
}


void spectrum_level_calibration(fft_data_type *fftdata, uint32_t fft_valid_len, uint32_t fft_size, uint64_t m_freq_hz)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    int32_t cal_value = 0, found = 0;
    int i;
    
    for(i = 0; i< sizeof(poal_config->cal_level.analysis.start_freq_khz)/sizeof(uint32_t); i++){
        if((m_freq_hz >= (uint64_t)poal_config->cal_level.specturm.start_freq_khz[i]*1000) && (m_freq_hz < (uint64_t)poal_config->cal_level.specturm.end_freq_khz[i]*1000)){
            cal_value = poal_config->cal_level.specturm.power_level[i];
            found = 1;
            break;
        }
    }
    if(found){
        printf_debug("find the calibration level: %llu, %d\n", m_freq_hz, cal_value);
    }else{
        printf_debug("Not find the calibration level, use default value: %d\n", cal_value);
    }

    cal_value += poal_config->cal_level.specturm.global_roughly_power_lever;
    
    if(fft_size == 512){
        cal_value = cal_value - 20;
    }else if(fft_size == 1024){
        cal_value = cal_value - 10;
    }else if(fft_size == 2048){
        cal_value = cal_value;
    }else if(fft_size == 4096){
        cal_value = cal_value + 10;
    }else if(fft_size == 8192){
        cal_value = cal_value + 20;
    }else{
        printf_debug("fft size is not set, set default calibration level!!\n");
    }
    printf_debug("freq:[%llu], The final cal_value:%d, fft size:%u,%u\n", m_freq_hz, cal_value, fft_size, fft_valid_len);
    SSDATA_CUT_OFFSET(fftdata, cal_value, fft_valid_len);
    SSDATA_MUL_OFFSET(fftdata, 10, fft_valid_len);
}


/* mgc  calibration*/
void spectrum_mgc_calibration(uint8_t ch, uint64_t m_freq_hz)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    int32_t cal_value = 0, found = 0;
    int i;

    for(i = 0; i< sizeof(poal_config->cal_level.mgc.start_freq_khz)/sizeof(uint32_t); i++){
        if((m_freq_hz >= (uint64_t)poal_config->cal_level.mgc.start_freq_khz[i]*1000) && (m_freq_hz < (uint64_t)poal_config->cal_level.mgc.end_freq_khz[i]*1000)){
            cal_value = poal_config->cal_level.mgc.gain_val[i];
            found = 1;
            break;
        }
    }
    cal_value += poal_config->cal_level.mgc.global_gain_val;
    if(found){
        printf_debug("find the mgc calibration level: %llu, %d\n", m_freq_hz, cal_value);
    }else{
        
        printf_debug("Not find the  mgc calibration level, use default value: %d\n", 
                    poal_config->cal_level.mgc.global_gain_val);
    }
    executor_set_command(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, ch, &cal_value);
}


/* ÖÐÆµÐ¹Â¶´¦Àí */
void spectrum_middle_freq_leakage_calibration(char *data ,unsigned int len,unsigned int fft_size){
    short int renew_data_len,left_diff_one,left_diff_two,right_diff_one,right_diff_two,threshold;
    unsigned int mid_fft_point=0;
    #define DATA_TYPE_BYTE_LEN 2

    struct poal_config *poal_config = &(config_get_config()->oal_config);
    int32_t found = 0;
    int i;
    for(i = 0; i< sizeof(poal_config->cal_level.lo_leakage.fft_size)/sizeof(uint32_t); i++){
        if(fft_size == poal_config->cal_level.lo_leakage.fft_size[i]){
            threshold = poal_config->cal_level.lo_leakage.threshold[i];
            renew_data_len = poal_config->cal_level.lo_leakage.renew_data_len[i];
            found = 1;
            break;
        }
    }    

    threshold += poal_config->cal_level.lo_leakage.global_threshold;
    renew_data_len += poal_config->cal_level.lo_leakage.global_renew_data_len;
    
    if(found){
        printf_debug("fft_size=%d, find the lo_leakage threshold calibration level: %d, %d\n", fft_size, threshold, renew_data_len);
    }else{
        printf_debug("Not find the lo_leakage threshold calibration level, use default value: %d,%d\n", 
            poal_config->cal_level.lo_leakage.global_threshold, poal_config->cal_level.lo_leakage.global_renew_data_len);
    }

    mid_fft_point = len/2 ;// for half point number

    left_diff_one = (*(short *)(data+mid_fft_point)) - (*(short *)(data+mid_fft_point+DATA_TYPE_BYTE_LEN));
    left_diff_two = (*(short *)(data+mid_fft_point)) - (*(short *)(data+mid_fft_point+renew_data_len*DATA_TYPE_BYTE_LEN));

    right_diff_one =  (*(short *)(data+mid_fft_point-DATA_TYPE_BYTE_LEN)) - (*(short *)(data+mid_fft_point-2*DATA_TYPE_BYTE_LEN));
    right_diff_two =  (*(short *)(data+mid_fft_point-DATA_TYPE_BYTE_LEN)) - (*(short *)(data+mid_fft_point-DATA_TYPE_BYTE_LEN-renew_data_len*DATA_TYPE_BYTE_LEN));
    if(left_diff_one >0 && right_diff_one >0 && left_diff_two >0 && left_diff_two <=threshold && right_diff_two >0 && right_diff_two <=threshold){
        //for right part 
        memcpy(data+mid_fft_point,data+mid_fft_point+renew_data_len*DATA_TYPE_BYTE_LEN,renew_data_len*DATA_TYPE_BYTE_LEN);
        //for left part
        memcpy(data+mid_fft_point-renew_data_len*DATA_TYPE_BYTE_LEN,data+mid_fft_point-2*renew_data_len*DATA_TYPE_BYTE_LEN,renew_data_len*DATA_TYPE_BYTE_LEN);
    }
}


/* function: read or write fft result 
    @result =NULL : read
    @result != NUL: write
    @ return fft result
*/
void *spectrum_rw_fft_result(fft_result *result, uint64_t s_freq_hz, float freq_resolution, 
                                    uint32_t fft_size, struct spm_run_parm *param)
{
    int i;
    static struct spectrum_fft_result_st s_fft_result, *pfft = NULL;
    uint64_t analysis_middle_freq;
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    
    LOCK_SP_RESULT();
    if(result == NULL || param == NULL){
        printf_info("read result\n");
        goto exit;
    }
    
    pfft = &s_fft_result;
    pfft->result_num = result->signalsnumber;
    if(pfft->result_num > SIGNALNUM){
        printf_err("signals number error:%d\n", pfft->result_num);
        pfft = NULL;
        goto exit;
    }
    if(pfft->result_num == 0){
        pfft->mid_freq_hz[0] = result->centfeqpoint[0];
        pfft->bw_hz[0] = result->bandwidth[0];
        pfft->level[0] = result->arvcentfreq[0]+ spectrum_low_noise_level_calibration(analysis_middle_freq);
        goto exit;
    }
    uint64_t signal_start_freq = 0, signal_end_freq = 0;
    for(i = 0; i < pfft->result_num; i++){
        /* signal start frequency = signal middle frequency -  signal bandwidth/2 */
        signal_start_freq = s_freq_hz + result->centfeqpoint[i] *freq_resolution - result->bandwidth[i] * freq_resolution/2;
        signal_end_freq = s_freq_hz + result->centfeqpoint[i]*freq_resolution + result->bandwidth[i] * freq_resolution/2;
        printf_debug("signal_start_freq=%llu, signal_end_freq=%llu, frequency_hz=%llu\n",signal_start_freq, signal_end_freq, poal_config->ctrl_para.specturm_analysis_param.frequency_hz);
        /* Determine if the analysis point is within the signal range */
        if((poal_config->ctrl_para.specturm_analysis_param.frequency_hz < signal_start_freq) || 
           (poal_config->ctrl_para.specturm_analysis_param.frequency_hz > signal_end_freq)){
                printf_warn("The analysis point[%llu] is NOT within the signal range[%llu, %llu]\n", poal_config->ctrl_para.specturm_analysis_param.frequency_hz, signal_start_freq, signal_end_freq);
                pfft->result_num = 0;
                goto exit; 
           }
    }
    for(i = 0; i < pfft->result_num; i++){
        pfft->mid_freq_hz[i] = s_freq_hz + result->centfeqpoint[i]*freq_resolution;
        pfft->peak_value = s_freq_hz + result->maximum_x*freq_resolution;
        pfft->bw_hz[i] = result->bandwidth[i] * freq_resolution;
        analysis_middle_freq =  spectrum_get_signal_middle_freq(param);
        pfft->level[i] = result->arvcentfreq[i] + spectrum_analysis_level_calibration(analysis_middle_freq);//spectrum_analysis_level_calibration(pfft->mid_freq_hz[i]);
        printf_warn("m_freq=%llu,s_freq_hz=%llu, freq_resolution=%f, fft_size=%u\n", analysis_middle_freq, s_freq_hz, freq_resolution, fft_size);
        printf_warn("mid_freq_hz=%d, bw_hz=%d, level=%f\n", result->centfeqpoint[i], result->bandwidth[i], result->arvcentfreq[i]);
    }
exit:
    UNLOCK_SP_RESULT();
    return (void *)pfft;
}


/* timer callback function, send spectrum to client every interval */
int32_t spectrum_send_fft_data_interval(void)
{
    struct spectrum_st *ps;
    fft_data_type *fft_send_payload;
    uint32_t fft_order_len=0;
    ps = &_spectrum;
    
    LOCK_SP_DATA();
    if(ps->fft_data_ready == false){
        UNLOCK_SP_DATA();
        return -1;
    }
    fft_send_payload = spectrum_fft_data_order(ps, &fft_order_len);
    printf_debug("fft order len[%u], ps->fft_len=%u\n", fft_order_len, ps->fft_len);
    spectrum_middle_freq_leakage_calibration((char *)fft_send_payload, fft_order_len * 2, ps->fft_len);
    spectrum_level_calibration(fft_send_payload, fft_order_len, ps->param.fft_size, ps->param.m_freq);
    spectrum_send_fft_data((void *)fft_send_payload, fft_order_len*sizeof(fft_data_type), &ps->param);
    ps->fft_data_ready = false;
    UNLOCK_SP_DATA();
    return (ps->fft_len*sizeof(fft_data_type));
}


/* load specturm analysis paramter */
static int8_t inline spectrum_get_analysis_paramter(struct spm_run_parm *param, 
                                                              uint32_t *_analysis_bw, uint64_t *midd_freq_offset)
{
    uint32_t analysis_signal_bw;
    uint64_t analysis_signal_middle_freq;
    struct poal_config *poal_config = &(config_get_config()->oal_config);
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

    extra_data_single_size = fft_size * get_single_side_band_pointrate();//alignment_down(fft_size * SINGLE_SIDE_BAND_POINT_RATE, sizeof(float));
    printf_note("##extra_data_single_size=%u, fft_size=%u, pool_step=%u, use=%d\n", extra_data_single_size, fft_size, memory_pool_step(fftmpool), memory_pool_get_use_count(fftmpool));
    rsb_fft_size = fft_size/get_side_band_rate();  //fft_size-2 * extra_data_single_size;
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

static size_t spectrum_iq_convert_fft_store_mm_pool(memory_pool_t  *fftmpool, memory_pool_t  *iqmpool)
{
    void *p_dat, *p_end;
    size_t p_inc;
    size_t fft_size = memory_pool_step(fftmpool)/4;
    
    
    printf_note("GET FFT size: %d\n", fft_size);
    
    p_end = memory_pool_end(iqmpool);
    p_inc = memory_pool_step(iqmpool);
    
    printf_info("pool_first=%p,pool_end =%p, pool_step=%p\n", 
        memory_pool_first(iqmpool), memory_pool_end(iqmpool), memory_pool_step(iqmpool));
    for (p_dat = memory_pool_first(iqmpool); p_dat < p_end; p_dat += p_inc) {
        float *s_fft_mp = (float *)memory_pool_alloc(fftmpool);
        fft_spectrum_iq_to_fft_handle((int16_t *)p_dat, fft_size, fft_size*2, s_fft_mp);
        for(int i = 0; i<10; i++)
            printfd("%f ", *((float *)s_fft_mp+i));
        printfd("\n");
    }

    return (fft_size*memory_pool_get_use_count(fftmpool));
}

void specturm_analysis_deal_thread(void *arg)
{
    memory_pool_t *iq_mpool = mp_iq_buffer_head;
    memory_pool_t *fft_small_mpool = mp_fft_small_buffer_head;
    memory_pool_t *fft_big_mpool = mp_fft_big_buffer_head;
    memory_pool_t *fft_small_rsb_mpool = mp_fft_small_buffer_rsb_head;
    memory_pool_t *fft_big_rsb_mpool = mp_fft_big_buffer_rsb_head;
    int small_fft_size = 0, big_fft_size = 0;
    uint32_t small_fft_rsb_size = 0, big_fft_rsb_size = 0;
    struct spectrum_st *ps;
    struct spm_run_parm *param;
    float resolution;
    uint64_t analysis_midd_freq_offset, bw_fft_size, analysis_signal_start_freq;
    uint32_t analysis_bw; 
    uint32_t total_bw; 
    ps = &_spectrum;

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
        printf_note("bandwidth=%uHz, s_freq=%llu, e_freq=%llu\n", param->bandwidth, param->s_freq, param->e_freq);
        
        /* --1step IQ convert 4K(small) fft, and store fft data to small memory pool  */
        printf_note("###STEP2: IQ Convert to Small FFT###\n");
        small_fft_size = spectrum_iq_convert_fft_store_mm_pool(fft_small_mpool, iq_mpool);
        //printf_info("###Save FFT data###\n");
        /* remove side band */
        if(spectrum_sideband_deal_mm_pool(fft_small_mpool, fft_small_rsb_mpool, param->bandwidth) == -1){
            sleep(1);
            goto exit;
        }
        //spectrum_data_write_file(fft_small_rsb_mpool, sizeof(float), 1);
        
        /* --2 step: IQ convet 128K(big) fft, and store fft data to big memory pool */
        printf_note("###STEP4: IQ Convert to Big FFT###\n");
        big_fft_size = spectrum_iq_convert_fft_store_mm_pool(fft_big_mpool, iq_mpool);
        /* remove side band */
        if(spectrum_sideband_deal_mm_pool(fft_big_mpool, fft_big_rsb_mpool, param->bandwidth) == -1){
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
        spectrum_rw_fft_result(fft_spectrum_get_result(), analysis_signal_start_freq, resolution, bw_fft_size,param);
exit:
        memory_pool_free(fft_small_mpool);
        memory_pool_free(fft_big_mpool);
        memory_pool_free(fft_small_rsb_mpool);
        memory_pool_free(fft_big_rsb_mpool);
        memory_pool_free(iq_mpool);
        ps->iq_data_ready = false;
    }

}

#ifdef SUPPORT_SPECTRUM_IQ_UPLOAD
void specturm_iq_upload_thread(void *arg)
{
    #define IQ_UPLOAD_BUFFER_DATA_SIZE_LEN 512
    struct spectrum_st *ps;
    int i;
    int count = 0, remainder = 0;
    memory_pool_t *iq_mpool = mp_iq_upload_buffer_head;
    struct spm_run_parm *param;
    ps = &_spectrum;
    
    while(1){
        printf_note("###wait to send iq data###\n");
        pthread_mutex_lock(&spectrum_iq_upload_cond_mutex);
        pthread_cond_wait(&spectrum_iq_upload_cond, &spectrum_iq_upload_cond_mutex);
        pthread_mutex_unlock(&spectrum_iq_upload_cond_mutex);
        
        printf_note("###sending iq data###\n");
        param = (struct spm_run_parm *)memory_pool_get_attr(iq_mpool);
        printf_note("iq_byte_len=%d, bandwidth=%uHz, s_freq=%llu, e_freq=%llu\n", ps->iq_byte_len, param->bandwidth, param->s_freq, param->e_freq);
        count = ps->iq_byte_len/IQ_UPLOAD_BUFFER_DATA_SIZE_LEN;
        remainder = ps->iq_byte_len%IQ_UPLOAD_BUFFER_DATA_SIZE_LEN;
        printf_note("count=%d, remainder=%d\n", count, remainder);
        for(i = 0; i < count; i++){
            #if 0
            printfn("send i=%d, %p\n", i, (int8_t *)memory_pool_first(iq_mpool)+i*IQ_UPLOAD_BUFFER_DATA_SIZE_LEN);
            for(int j =0;j < IQ_UPLOAD_BUFFER_DATA_SIZE_LEN; j++){
                printfn("%02x ", *(uint8_t *)((int8_t *)memory_pool_first(iq_mpool)+i*IQ_UPLOAD_BUFFER_DATA_SIZE_LEN+j));
            }
            printfn("\n");
            #endif
            spectrum_send_iq_data((void *)((int8_t *)memory_pool_first(iq_mpool)+i*IQ_UPLOAD_BUFFER_DATA_SIZE_LEN), IQ_UPLOAD_BUFFER_DATA_SIZE_LEN, param);
        }
        if(remainder != 0){
            printf_warn("IQ data length is not %d byte aligned\n", IQ_UPLOAD_BUFFER_DATA_SIZE_LEN);
            spectrum_send_iq_data((void *)((int16_t *)memory_pool_first(iq_mpool)+i*IQ_UPLOAD_BUFFER_DATA_SIZE_LEN/2), remainder, param);
        }
        memory_pool_free(iq_mpool);
        ps->iq_data_upload_ready = false;
        printf_note("###send over iq data###\n");
    }
}
#endif

void spectrum_init(void)
{
    struct spectrum_st *ps;
    int ret, i;
    pthread_t work_id;
   
    fft_init();
    /* set default smooth count */
    fft_set_smoothcount(8);
    printf_warn("fft_init:%.10f, %.6f\n", get_single_side_band_pointrate(), get_side_band_rate());

    sem_init(&(sp_sem.notify_iq_to_fft), 0, 0);

    LOCK_SP_DATA();
    ps = &_spectrum;
    ps->fft_data_ready = false;/* fft data not vaild to deal */
    ps->iq_data_ready = false;
    ps->iq_data_upload_ready = false;
    UNLOCK_SP_DATA();
#ifdef SUPPORT_SPECTRUM_FFT_ANALYSIS
    /* create memory pool */
    /* 128K fft, 1fft=4byte */
    mp_iq_buffer_head = memory_pool_create(SPECTRUM_IQ_SIZE*4, SPECTRUM_MAX_SCAN_COUNT);
    mp_fft_small_buffer_head = memory_pool_create(SPECTRUM_SMALL_FFT_SIZE*4, SPECTRUM_MAX_SCAN_COUNT);
    mp_fft_big_buffer_head = memory_pool_create(SPECTRUM_BIG_FFT_SIZE*4, SPECTRUM_MAX_SCAN_COUNT);

    mp_fft_small_buffer_rsb_head = memory_pool_create(SPECTRUM_SMALL_FFT_SIZE*4, SPECTRUM_MAX_SCAN_COUNT);
    mp_fft_big_buffer_rsb_head = memory_pool_create(SPECTRUM_BIG_FFT_SIZE*4, SPECTRUM_MAX_SCAN_COUNT);

    ret=pthread_create(&work_id,NULL,(void *)specturm_analysis_deal_thread, NULL);
    if(ret!=0)
        perror("pthread cread work_id");
    pthread_detach(work_id);
#endif

#ifdef SUPPORT_SPECTRUM_IQ_UPLOAD
    mp_iq_upload_buffer_head = memory_pool_create(SPECTRUM_IQ_SIZE*4, SPECTRUM_MAX_SCAN_COUNT);
    ret=pthread_create(&work_id,NULL,(void *)specturm_iq_upload_thread, NULL);
    if(ret!=0)
        perror("pthread cread work_id");
    pthread_detach(work_id);

#endif

}

