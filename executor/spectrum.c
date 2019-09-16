#include "config.h"

struct spectrum_st _spectrum;

pthread_mutex_t spectrum_result_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t spectrum_data_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t spectrum_iq_data_mutex = PTHREAD_MUTEX_INITIALIZER;


pthread_cond_t spectrum_analysis_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t spectrum_analysis_cond_mutex = PTHREAD_MUTEX_INITIALIZER;


static inline  bool specturm_is_analysis_enable(void)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    if(poal_config->enable.spec_analy_en == 1)
        return true;
    else
        return false;
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
   #if (RF_ADRV9009_IIO == 1)
    iqdata = iio_read_rx0_data(&nbytes_rx);
    p_iqdata_start = iqdata;
    if(p_iqdata_start == NULL){
        nbytes_rx = 0;
        goto exit;
    }
   #else    
    nbytes_rx = 0;
    goto exit;
   #endif
    
    if(write_file_cnter++ ==10) {
        sprintf(strbuf, "/run/wav_%d", write_file_cnter);
        printfi("write rx0 iq data to:[len=%d]%s\n", nbytes_rx/2, strbuf);
        write_file_in_int16((void*)(iqdata), nbytes_rx/2, strbuf);
    }
    printf_info("iio read data len:[%d]\n", nbytes_rx);
    printf_info("rx0 iqdata:\n");
    for(i = 0; i< 10; i++){
        printfi("%d ",*(int16_t*)(iqdata+i));
    }
    printfi("\n");
exit:
    UNLOCK_IQ_DATA();
    *nbytes = nbytes_rx;
    return p_iqdata_start;
}



/* Send FFT spectrum Data to Client (UDP)*/
static void spectrum_send_fft_data(void *fft_data, size_t fft_data_len, void *param)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    uint8_t *fft_sendbuf, *pbuf;
    uint8_t *header_buf;
    uint32_t header_len = 0;

    /* fill header data len(byte) */
    struct spectrum_header_param *hparam;
    hparam = (struct spectrum_header_param *)param;
    hparam->data_len = fft_data_len; 

    header_buf = poal_config->assamble_response_data(&header_len, (void *)param);
    printf_debug("header_buf=%x %x, header_len=%d\n", header_buf[0],header_buf[1], header_len);

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
}


/* Before send client, we need to remove sideband signals data  and  extra bandwidth data */
fft_data_type *spectrum_fft_data_order(struct spectrum_st *ps, uint32_t *result_fft_size)
{
    /*-- 1 step: when scan count >2 , need to exchange sn order */
    static uint64_t mfreq[32] = {0}; 
    static uint32_t mbw[32] = {0};

    
    if(ps->param.total_fft > sizeof(mfreq)/sizeof(uint64_t)){
        printf_err("Requency range is too long:%d", ps->param.total_fft);
        goto exit;
    }
    if(ps->param.total_fft >= 2){
        mfreq[ps->param.fft_sn] = ps->param.m_freq;
        mbw[ps->param.fft_sn] = ps->param.bandwidth;
        if(ps->param.fft_sn >= 2){
           ps->param.fft_sn -= 2;
        }else{
           ps->param.fft_sn += ps->param.total_fft - 2;
        }
        ps->param.m_freq = mfreq[ps->param.fft_sn]; 
        ps->param.bandwidth = mbw[ps->param.fft_sn]; 

        printf_debug("m_freq=%llu,param->total_fft=%u, sn=%u, bandwidth=%u\n", ps->param.m_freq, ps->param.total_fft, ps->param.fft_sn, ps->param.bandwidth);
    }


   
    uint32_t extra_data_single_size = 0;
    /*-- 2 step:  
       remove sideband signals data
   */
    /* single sideband signal fft size */
    uint32_t single_sideband_size = ps->fft_len*SINGLE_SIDE_BAND_POINT_RATE;
    
    /* odd number */
    if(!EVEN(single_sideband_size)){
        single_sideband_size++;
    }

    *result_fft_size = ps->fft_len - 2 * single_sideband_size;


    /*-- 3 step: when middle frequency is less than BW/2 [100MHz], we need to resetting middle frequency
               (see middle_freq_resetting function) ; when sending data, we need to restore the settings and remove 
               the redundant data.
                the left extra data:  (mf0 - bw0/2 - deltabw)* fftSize /BW; [mf0 - bw0/2 >= deltabw]
                the right extra data: (BW + deltabw - mf0 - bw0/2) *fftSize / BW; [BW + deltabw >= mf0 + bw0/2]
     */
    uint32_t left_extra_single_size = 0, right_extra_single_size = 0;
    if(ps->param.m_freq <= RF_ADRV9009_BANDWITH/2){
        if(ps->param.m_freq - ps->param.bandwidth/2 >= delta_bw){
            left_extra_single_size = (ps->param.m_freq - ps->param.bandwidth/2 - delta_bw) * (*result_fft_size) / RF_ADRV9009_BANDWITH;
        }else{
            left_extra_single_size = 0;
        }
        if(RF_ADRV9009_BANDWITH + delta_bw > ps->param.m_freq + ps->param.bandwidth/2){
            right_extra_single_size = (RF_ADRV9009_BANDWITH + delta_bw - ps->param.m_freq - ps->param.bandwidth/2) * (*result_fft_size) / RF_ADRV9009_BANDWITH;
        }else{
            right_extra_single_size = 0;
        }
        *result_fft_size = *result_fft_size - right_extra_single_size - left_extra_single_size;
        printf_info("m_freq = %llu, bandwidth= %u, delta_bw=%llu, left=%u, right=%u,*result_fft_size=%u\n", ps->param.m_freq, ps->param.bandwidth, delta_bw, left_extra_single_size, right_extra_single_size,*result_fft_size);
    }else{
        /*-- 4 step:  
           If the remaining bandwidth is less than the actual working bandwidth,
           we need to remove the extra data. 
        */
        /* the left extra data bw range: (mFq-BW/2, mFq-ScanBW/2);   mFq is middle freqency;
           the rigth extra data bw range: (mFq+BW/2, mFq+ScanBW/2); 
              so, the single extra bw is: (BW-ScanBw)/2, It accounts for the working bandwidth ratio: 
              (BW-ScanBw)/2/BW = (1-ScanBw/Bw)/2
              There Bw is: RF_ADRV9009_BANDWITH, ScanBw is ps->param.bandwidth,
               ScanBw must less than or equal to Bw
        */
        if((RF_ADRV9009_BANDWITH > ps->param.bandwidth) && (ps->param.bandwidth > 0)){
            extra_data_single_size = ((1-(float)ps->param.bandwidth/(float)RF_ADRV9009_BANDWITH)/2) * (*result_fft_size);
            printf_info("bw:%u, m_freq:%llu,need to remove the single extra data: %f,%u,%u\n",ps->param.bandwidth, ps->param.m_freq, (1-(float)ps->param.bandwidth/(float)RF_ADRV9009_BANDWITH)/2, *result_fft_size, extra_data_single_size);
            if(ps->fft_len <  extra_data_single_size *2){
                *result_fft_size = 0;
            }else{
                *result_fft_size = *result_fft_size - extra_data_single_size *2;
            }
            printf_info("the remaining fftdata is: %u\n", *result_fft_size);
        }
    }     

    /* Returning data requires removing sideband data and excess bandwidth data */
 exit:
    return (fft_data_type *)((fft_data_type *)ps->fft_short_payload + single_sideband_size + extra_data_single_size + left_extra_single_size);
}


/* We use this function to deal FFT spectrum wave in user space */
void spectrum_psd_user_deal(struct spectrum_header_param *param)
{
    struct spectrum_st *ps;
    float resolution;
    uint32_t fft_size;
    float *fft_float_data;

    ps = &_spectrum;

    /* Read Raw IQ data*/
    ps->iq_payload = specturm_rx_iq_read(&ps->iq_len);
    if(ps->iq_len == 0){
        printf_err("IQ data is NULL\n");
        return;
    }
    printf_debug("ps->iq_payload[0]=%d, %d,param->fft_size=%d, ps->iq_len=%d\n", ps->iq_payload[0],ps->iq_payload[1], param->fft_size, ps->iq_len);
    
    fft_size = param->fft_size;
    /* Start Convert IQ data to FFT, Noise threshold:8 */
    fft_float_data = (float *)safe_malloc(fft_size*4);
    ps->fft_float_payload = fft_float_data;
    TIME_ELAPSED(fft_spectrum_iq_to_fft_handle(ps->iq_payload, fft_size, fft_size*2, ps->fft_float_payload));

    ps->fft_len = fft_size;
    if(ps->fft_len > sizeof(ps->fft_short_payload)){
        printf_err("fft size[%u] is too big\n", ps->fft_len);
        return;
    }
    /* To the circumstances: the client's active acquisition of FFT data; Here we user backup buffer to store fft data; 
    when the scheduled task is processed[fft_data_ready=false],  we start filling in new fft data in back buffer [ps->fft_short_payload_back]*/
    LOCK_SP_DATA();
    /* Here we convert float to short integer processing */
    FLOAT_TO_SHORT(ps->fft_float_payload, ps->fft_short_payload, ps->fft_len);
    /*refill header parameter*/
    memcpy(&ps->param, param, sizeof(struct spectrum_header_param));

    ps->fft_data_ready = true;

    //printf_warn("fft data and header is ready, notify to handle \n");
    UNLOCK_SP_DATA();
    if(get_spectrum_demo()){
        spectrum_send_fft_data_interval();
    }
    safe_free(fft_float_data);

}

/* after scan, notify thread start to analysis fft data */
void spectrum_analysis_user_deal(struct spectrum_header_param *param)
{
    struct spectrum_st *ps;
    ps = &_spectrum;
     /*refill header parameter*/
    LOCK_SP_DATA();
    memcpy(&ps->param, param, sizeof(struct spectrum_header_param));
    UNLOCK_SP_DATA();
    /* notify thread can start to deal fft result */
    pthread_cond_signal(&spectrum_analysis_cond);
}

int32_t spectrum_analysis_level_calibration(uint64_t m_freq)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    int32_t cal_value = -196, found = 0;
    int i;
    
    for(i = 0; i< sizeof(poal_config->cal_level.analysis.start_freq)/sizeof(uint64_t); i++){
        if((m_freq >= poal_config->cal_level.analysis.start_freq[i]*1000) && (m_freq < poal_config->cal_level.analysis.end_freq[i]*1000)){
            cal_value = poal_config->cal_level.analysis.power_level[i];
            found = 1;
            break;
        }
    }
    if(found){
        printf_debug("find the calibration level: %llu, %d\n", m_freq, cal_value);
    }else{
        printf_note("Not find the calibration level, use default value: %d\n", cal_value);
    }
    return cal_value;
}

/* function: read or write fft result 
    @result =NULL : read
    @result != NUL: write
    @ return fft result
*/
void *spectrum_rw_fft_result(fft_result *result, uint64_t s_freq_hz, float freq_resolution, uint32_t fft_size)
{
    int i;
    static struct spectrum_fft_result_st s_fft_result, *pfft = NULL;
    struct spectrum_st *ps;
    ps = &_spectrum;
    #define SIGNAL_ADD_FIXED_VALUE 196//217
    
    LOCK_SP_RESULT();
    if(result == NULL){
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
    for(i = 0; i < pfft->result_num; i++){
        pfft->mid_freq_hz[i] = s_freq_hz + (result->centfeqpoint[i] - (SINGLE_SIDE_BAND_POINT_RATE*fft_size))*freq_resolution;
        pfft->peak_value = s_freq_hz + (result->maximum_x - (SINGLE_SIDE_BAND_POINT_RATE*fft_size))*freq_resolution;
        pfft->bw_hz[i] = result->bandwidth[i] * freq_resolution;
        pfft->level[i] = result->arvcentfreq[i] + spectrum_analysis_level_calibration(ps->param.m_freq);//spectrum_analysis_level_calibration(pfft->mid_freq_hz[i]);
        printf_debug("m_freq=%llu,s_freq_hz=%llu, freq_resolution=%f, fft_size=%u, %f\n", ps->param.m_freq, s_freq_hz, freq_resolution, fft_size, (SINGLE_SIDE_BAND_POINT_RATE*fft_size));
        printf_debug("mid_freq_hz=%d, bw_hz=%d, level=%f\n", result->centfeqpoint[i], result->bandwidth[i], result->arvcentfreq[i]);
    }
exit:
    UNLOCK_SP_RESULT();
    return (void *)pfft;
}


fft_data_type *spectrum_get_fft_data(uint32_t *len)
{
    struct spectrum_st *ps;
    fft_data_type *data;
    ps = &_spectrum;
    
    LOCK_SP_DATA();
    if(ps->fft_data_ready == false){
        *len = 0;
        UNLOCK_SP_DATA();
        return NULL;
    }
    data = ps->fft_short_payload;
    *len = ps->fft_len;
    ps->fft_data_ready = false;
    UNLOCK_SP_DATA();
    return data;
}

void spectrum_level_calibration(fft_data_type *fftdata, uint32_t fft_valid_len, uint32_t fft_size, uint64_t m_freq)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    int32_t cal_value = -4660, found = 0;
    int i;
    
    for(i = 0; i< sizeof(poal_config->cal_level.analysis.start_freq)/sizeof(uint64_t); i++){
        if((m_freq >= poal_config->cal_level.specturm.start_freq[i]*1000) && (m_freq < poal_config->cal_level.specturm.end_freq[i]*1000)){
            cal_value = poal_config->cal_level.specturm.power_level[i];
            found = 1;
            break;
        }
    }
    if(found){
        printf_debug("find the calibration level: %llu, %d\n", m_freq, cal_value);
    }else{
        printf_note("Not find the calibration level, use default value: %d\n", cal_value);
    }
        
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
        printf_warn("fft size is not set, set default calibration level!!\n");
    }
    printf_debug("freq:[%llu], The final cal_value:%d, fft size:%u,%u\n", m_freq, cal_value, fft_size, fft_valid_len);
    SSDATA_MUL_OFFSET(fftdata, 10, fft_valid_len);
    SSDATA_CUT_OFFSET(fftdata, cal_value, fft_valid_len);

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
    spectrum_level_calibration(fft_send_payload, fft_order_len, ps->param.fft_size, ps->param.m_freq);
    spectrum_send_fft_data((void *)fft_send_payload, fft_order_len*sizeof(fft_data_type), &ps->param);
    ps->fft_data_ready = false;
    UNLOCK_SP_DATA();
    return (ps->fft_len*sizeof(fft_data_type));
}

void specturm_analysis_deal_thread(void *arg)
{
    struct spectrum_st *ps;
    float resolution;
    uint32_t fft_size;
    uint32_t iq_len;
    int16_t *iq_payload; 
loop:
    while(1){
        printf_debug("prepare to read iq data and analysis specturm......\n");

        /* Before start specturm analysis, we need to wait the center frequency to be set */
        /*and also some other parameter[bandwidth,s_freq] to be set  */
        
        /* Mutex must be locked for pthread_cond_timedwait... */
        pthread_mutex_lock(&spectrum_analysis_cond_mutex);
        /* Thread safe "sleep" */
        pthread_cond_wait(&spectrum_analysis_cond, &spectrum_analysis_cond_mutex);
        /* No longer needs to be locked */
        pthread_mutex_unlock(&spectrum_analysis_cond_mutex);
        printf_debug("start to read iq data and analysis specturm .\n");
        ps = &_spectrum;
        /* Read Raw IQ data*/
        iq_payload = specturm_rx_iq_read(&iq_len);
        if((iq_len == 0) || (iq_len < SPECTRUM_DEFAULT_FFT_SIZE*2)){
            printf_err("error iq len[%u]\n", iq_len);
            goto loop;
        }

        fft_size = SPECTRUM_DEFAULT_FFT_SIZE;
        printf_note("ps->iq_payload[0]=%d, %d,ps->iq_len=%u\n", iq_payload[0],iq_payload[1], iq_len);
        /* Start Convert IQ data to FFT */
        TIME_ELAPSED(fft_iqdata_handle(get_power_level_threshold(), iq_payload, fft_size, fft_size*2));

        LOCK_SP_DATA();
        /* Calculation resolution(Point bandwidth) by Total bandWidth& fft size */
        resolution = calc_resolution(RF_ADRV9009_BANDWITH, fft_size);
        
        /*  write fft result to buffer: middle point, bandwidth point, power level
            fft_get_result(): GET FFT result
        */
        uint64_t s_freq;
       
        if(ps->param.m_freq <= RF_ADRV9009_BANDWITH/2){
            s_freq = delta_bw;
        }else{
            s_freq = ps->param.m_freq - RF_ADRV9009_BANDWITH/2;
        }
        printf_note("refill fft result[s_freq:%llu,resolution:%f, fft_size:%u]\n",s_freq, resolution, fft_size);
        spectrum_rw_fft_result(fft_get_result(), s_freq, resolution, fft_size);
        printf_note("bandwidth=%u, m_freq=%llu, resolution=%f, param->s_freq=%llu\n", ps->param.bandwidth, ps->param.m_freq, resolution, s_freq);
        UNLOCK_SP_DATA();
        printf_note("specturm_analysis_deal over\n");
    }
}


void spectrum_init(void)
{
    struct spectrum_st *ps;
    int ret, i;
    pthread_t work_id;

    fft_init();
    printf_info("fft_init\n");
    LOCK_SP_DATA();
    ps = &_spectrum;
    ps->fft_data_ready = false;/* fft data not vaild to deal */
    UNLOCK_SP_DATA();

    ret=pthread_create(&work_id,NULL,(void *)specturm_analysis_deal_thread, NULL);
    if(ret!=0)
        perror("pthread cread work_id");
    pthread_detach(work_id);
}

