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
/* the memory pool to store FFT data: small & big */
memory_pool_t *mp_fft_small_buffer_head; /* for small FFT */
memory_pool_t *mp_fft_small_buffer_rsb_head; /* for small remove side band FFT */

memory_pool_t *mp_fft_big_buffer_head;   /* for bit FFT */
memory_pool_t *mp_fft_big_buffer_rsb_head;   /* for remove side band FFT */



pthread_mutex_t spectrum_result_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t spectrum_data_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t spectrum_iq_data_mutex = PTHREAD_MUTEX_INITIALIZER;

struct sp_sem_nofity_st sp_sem;

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

static inline uint64_t spectrum_get_analysis_middle_freq(uint64_t sig_sfreq)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    if(poal_config->ctrl_para.specturm_analysis_param.frequency_hz == 0){
        poal_config->ctrl_para.specturm_analysis_param.frequency_hz = sig_sfreq + poal_config->ctrl_para.specturm_analysis_param.bandwidth_hz/2;
    }
    return poal_config->ctrl_para.specturm_analysis_param.frequency_hz;
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
static inline void spectrum_send_fft_data(void *fft_data, size_t fft_data_len, void *param)
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
    printf_debug("header_buf=%x %x, header_len=%d, malloc len=%d\n", header_buf[0],header_buf[1], header_len, header_len+ fft_data_len);

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
   
    uint32_t extra_data_single_size = 0;
    /*-- 1 step:  
       remove sideband signals data
   */
    /* single sideband signal fft size */
    uint32_t single_sideband_size = ps->fft_len*SINGLE_SIDE_BAND_POINT_RATE;
    
    /* odd number */
    if(!EVEN(single_sideband_size)){
        single_sideband_size++;
    }

    *result_fft_size = ps->fft_len - 2 * single_sideband_size;
    printf_debug("result_fft_size=%u\n", *result_fft_size); 


    /*-- 2 step: when middle frequency is less than BW/2 [100MHz], we need to resetting middle frequency
               (see middle_freq_resetting function) ; when sending data, we need to restore the settings and remove 
               the redundant data.
                the left extra data:  (mf0 - bw0/2 - deltabw)* fftSize /BW; [mf0 - bw0/2 >= deltabw]
                the right extra data: (BW + deltabw - mf0 - bw0/2) *fftSize / BW; [BW + deltabw >= mf0 + bw0/2]
     */
    uint32_t left_extra_single_size = 0, right_extra_single_size = 0;
    if((ps->param.m_freq <= RF_BANDWIDTH/2) && (ps->param.m_freq > 0)){
        if(ps->param.m_freq - ps->param.bandwidth/2 >= delta_bw){
            left_extra_single_size = (ps->param.m_freq - ps->param.bandwidth/2 - delta_bw) * (*result_fft_size) / RF_BANDWIDTH;
        }else{
            left_extra_single_size = 0;
        }
        if(RF_BANDWIDTH + delta_bw > ps->param.m_freq + ps->param.bandwidth/2){
            right_extra_single_size = (RF_BANDWIDTH + delta_bw - ps->param.m_freq - ps->param.bandwidth/2) * (*result_fft_size) / RF_BANDWIDTH;
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
        if((RF_BANDWIDTH > ps->param.bandwidth) && (ps->param.bandwidth > 0)){
            extra_data_single_size = ((1-(float)ps->param.bandwidth/(float)RF_BANDWIDTH)/2) * (*result_fft_size);
            printf_info("bw:%u, m_freq:%llu,need to remove the single extra data: %f,%u,%u\n",ps->param.bandwidth, ps->param.m_freq, (1-(float)ps->param.bandwidth/(float)RF_BANDWIDTH)/2, *result_fft_size, extra_data_single_size);
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

static int specturm_write_oneframe_iq_data_to_buffer(struct spectrum_header_param *param, void *data, size_t nbyte)
{
    static bool is_sysn_write= false;
    memory_pool_t *iq_mpool = mp_iq_buffer_head;
    struct spectrum_st *ps;
    ps = &_spectrum;
    static int old_fft_sn = -1;
    
    if(param->total_fft > memory_pool_get_count(iq_mpool)){
        printf_err("One Frame FFT scan count is more than memory pool count!!!\n");
        return -1;
    }

    if((0 == param->fft_sn) && (ps->iq_data_ready == false) && (old_fft_sn != param->fft_sn)){
        is_sysn_write = true;
    }
    printf_debug("fft_sn=%u, total_fft=%u, %d, iq_data_ready=%d,is_sysn_write=%d\n", param->fft_sn, param->total_fft, memory_pool_get_count(iq_mpool), ps->iq_data_ready,is_sysn_write);
    if((is_sysn_write == true) && (old_fft_sn != param->fft_sn)){
        printf_note("sn:%d, total_fft=%d\n", param->fft_sn, param->total_fft);
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
            printf_note("iq_mpool step:%d\n", memory_pool_step(iq_mpool));
            memory_pool_write_attr_value(iq_mpool, param, sizeof(struct spectrum_header_param));
            old_fft_sn = -1;
            return 0;
        }
    }
    return -1;
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
    
    /* if spectrum analysis is enable, we start to save iq data(one frame) to memery pool; prepare to analysis spectrum */
    if(specturm_is_analysis_enable()){
        if(!specturm_write_oneframe_iq_data_to_buffer(param, ps->iq_payload, ps->iq_len)){
            /* notify thread can start to conver iq to fft */
            printf_note("##New Frame IQ Data Created, Notify to Deal##\n");
            sem_post(&sp_sem.notify_iq_to_fft);
        }
    }

    if(specturm_is_psd_enable()){
        fft_size = param->fft_size;
        /* Start Convert IQ data to FFT, Noise threshold:8 */
        fft_float_data = (float *)safe_malloc(fft_size*4);
        ps->fft_float_payload = fft_float_data;
        fft_spectrum_iq_to_fft_handle(ps->iq_payload, fft_size, fft_size*2, ps->fft_float_payload);
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
        if(is_spectrum_continuous_mode()){
            spectrum_send_fft_data_interval();
        }
        safe_free(fft_float_data);
    }
}

int32_t spectrum_analysis_level_calibration(uint64_t m_freq)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    int32_t cal_value = 0, found = 0;
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
    cal_value += poal_config->cal_level.analysis.global_roughly_power_lever;
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
    uint64_t analysis_middle_freq;
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    
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
    if(pfft->result_num == 0){
        pfft->mid_freq_hz[0] = result->centfeqpoint[0];
        pfft->bw_hz[0] = result->bandwidth[0];
        pfft->level[0] = result->arvcentfreq[0]; //+ spectrum_analysis_level_calibration(ps->param.m_freq);
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
        analysis_middle_freq =  spectrum_get_analysis_middle_freq(s_freq_hz);
        pfft->level[i] = result->arvcentfreq[i] + spectrum_analysis_level_calibration(analysis_middle_freq);//spectrum_analysis_level_calibration(pfft->mid_freq_hz[i]);
        printf_warn("m_freq=%llu,s_freq_hz=%llu, freq_resolution=%f, fft_size=%u\n", analysis_middle_freq, s_freq_hz, freq_resolution, fft_size);
        printf_warn("mid_freq_hz=%d, bw_hz=%d, level=%f\n", result->centfeqpoint[i], result->bandwidth[i], result->arvcentfreq[i]);
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


/* load specturm analysis paramter */
static int8_t inline spectrum_get_analysis_paramter(uint64_t s_freq, uint64_t e_freq,uint32_t *_analysis_bw, uint64_t *midd_freq_offset)
{
    uint32_t analysis_bw;
    uint64_t analysis_middle_freq;
    struct poal_config *poal_config = &(config_get_config()->oal_config);

    /* When calculating the FFT, the calculation analysis bandwidth is set to an integral multiple of the rf  bandwidth. */
    analysis_bw = alignment_up(poal_config->ctrl_para.specturm_analysis_param.bandwidth_hz, RF_BANDWIDTH); 
    analysis_middle_freq =  spectrum_get_analysis_middle_freq(s_freq);
    
    if(analysis_middle_freq < s_freq || analysis_middle_freq > e_freq){
        printf_err("analysis frequency[%llu] is not  NOT within the bandwidth range[%llu, %llu]\n", analysis_middle_freq, s_freq, e_freq);
        return -1;
    }
    if(analysis_bw > RF_BANDWIDTH*SPECTRUM_MAX_SCAN_COUNT || analysis_bw == 0){
        printf_err("analysis bandwidth [%u] not surpport!!\n", analysis_bw);
        return -1;
    }
    
    *_analysis_bw = analysis_bw;

    /* Calculate the center frequency offset value (relative to the rf operating bandwidth) for spectrum analysis */
    if(analysis_middle_freq <= RF_BANDWIDTH/2){
        if(s_freq >= delta_bw){
            *midd_freq_offset = RF_BANDWIDTH/2+delta_bw - analysis_middle_freq;
        }else{
            printf_err("start frequency[%llu] is too small\n", s_freq);
            return -1;
        }
        
    }else{
        *midd_freq_offset = 0;
    }
    
    return 0;
}

static inline int8_t spectrum_sideband_deal_mm_pool(memory_pool_t  *fftmpool, memory_pool_t  *rsb_fftmpool)
{   
    uint32_t extra_data_single_size = 0;
    void *p_dat, *p_end;
    size_t p_inc, rsb_fft_size;
    size_t fft_size = memory_pool_step(fftmpool)/4;

    p_end = memory_pool_end(fftmpool);
    p_inc = memory_pool_step(fftmpool);

    extra_data_single_size = fft_size * SINGLE_SIDE_BAND_POINT_RATE;//alignment_down(fft_size * SINGLE_SIDE_BAND_POINT_RATE, sizeof(float));
    printf_note("##extra_data_single_size=%u, fft_size=%u, pool_step=%u, use=%d\n", extra_data_single_size, fft_size, memory_pool_step(fftmpool), memory_pool_get_use_count(fftmpool));
    rsb_fft_size = fft_size/SIDE_BAND_RATE;  //fft_size-2 * extra_data_single_size;
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
    struct spectrum_header_param *param;
    float resolution;
    uint64_t analysis_midd_freq_offset, bw_fft_size;
    uint32_t analysis_bw; 
    uint32_t total_bw; 
    ps = &_spectrum;

loop:
    while(1){
         /* wait to product iq data */
        printf_note("###wait to analysis specturm###\n");
        sem_wait(&sp_sem.notify_iq_to_fft);
        /* Here we have get IQ data in memory pool (name: iq_mpool)*/
        printf_info("###Save IQ data###\n");
        spectrum_data_write_file(iq_mpool, sizeof(int16_t), 0);
        
        /* --1step IQ convert 4K(small) fft, and store fft data to small memory pool  */
        printf_note("###STEP1: IQ Convert to Small FFT###\n");
        small_fft_size = spectrum_iq_convert_fft_store_mm_pool(fft_small_mpool, iq_mpool);
        printf_info("###Save FFT data###\n");
        /* remove side band */
        if(spectrum_sideband_deal_mm_pool(fft_small_mpool, fft_small_rsb_mpool) == -1){
            sleep(1);
            goto loop;
        }
        spectrum_data_write_file(fft_small_rsb_mpool, sizeof(float), 1);
        
        /* --2 step: IQ convet 128K(big) fft, and store fft data to big memory pool */
        printf_note("###STEP2: IQ Convert to Big FFT###\n");
        big_fft_size = spectrum_iq_convert_fft_store_mm_pool(fft_big_mpool, iq_mpool);
        /* remove side band */
        if(spectrum_sideband_deal_mm_pool(fft_big_mpool, fft_big_rsb_mpool) == -1){
            sleep(1);
            goto loop;
        }
        printf_info("###Save Big FFT data###\n");
        spectrum_data_write_file(fft_big_rsb_mpool, sizeof(float), 2);
        
        /* save specturm perameter to big fft memory pool */
        printf_note("###STEP3: Write FFT Attr to Pool###\n");
        memory_pool_write_attr_value(fft_big_mpool, memory_pool_get_attr(iq_mpool), memory_pool_get_attr_len(iq_mpool));
        /* now, we can refill IQ data to memory pool again */
        
        printf_note("###IQ Deal Over,Free, Wait New IQ data###\n");

        param = (struct spectrum_header_param *)memory_pool_get_attr(fft_big_mpool);
        printf_info("s_freq=%llu, e_freq=%llu\n", param->s_freq, param->e_freq);

        /* load specturm analysis paramter: analysis_bw, analysis_midd_freq_offset*/
        if(spectrum_get_analysis_paramter(param->s_freq, param->e_freq, &analysis_bw, &analysis_midd_freq_offset) == -1){
            sleep(1);
            goto loop;
        }
        
        total_bw = RF_BANDWIDTH*memory_pool_get_use_count(fft_big_mpool);
        small_fft_rsb_size = memory_pool_step(fft_small_rsb_mpool)*memory_pool_get_use_count(fft_small_rsb_mpool)/4;
        big_fft_rsb_size = memory_pool_step(fft_big_rsb_mpool)*memory_pool_get_use_count(fft_big_rsb_mpool)/4;
        
        printf_info("analysis_bw=%u, analysis_midd_freq_offset=%llu\n", analysis_bw, analysis_midd_freq_offset);
        printf_note("small fft data len=%d, use_count=%d\n", small_fft_size, memory_pool_get_use_count(fft_small_mpool));
        printf_note("big fft data len=%d, use_count=%d\n", big_fft_size, memory_pool_get_use_count(fft_big_mpool));
        printf_note("fft small mpool step =%u, big fft size step=%u\n", memory_pool_step(fft_small_mpool), memory_pool_step(fft_big_mpool));
        
        printf_note("rsb small fft data len=%d, use_count=%d\n", small_fft_rsb_size, memory_pool_get_use_count(fft_small_rsb_mpool));
        printf_note("rsb big fft data len=%d, use_count=%d\n", big_fft_rsb_size, memory_pool_get_use_count(fft_big_rsb_mpool));
        printf_note("rsb fft small mpool step=%u, rsb big fft mpool step=%u\n", memory_pool_step(fft_small_rsb_mpool), memory_pool_step(fft_big_rsb_mpool));
        
        printf_info("work total bw=%u\n", total_bw);
        printf_note("###STEP4: Start to analysis FFT data###\n");

        TIME_ELAPSED(fft_fftdata_handle(get_power_level_threshold(),                /* signal threshold */
                    (float *)memory_pool_first(fft_small_rsb_mpool),                /* remove side band small fft data */
                    small_fft_rsb_size,                                             /* small fft data len */
                    (float *)memory_pool_first(fft_big_rsb_mpool),                  /* remove side band big fft data */
                    big_fft_rsb_size,                                               /* big fft data len */
                    analysis_midd_freq_offset,                                      /* analysis signal middle frequency */
                    analysis_bw,                                                    /* analysis signal bandwidth */
                    total_bw                                                        /* total bw */
                    ));                                                 

        resolution = calc_resolution(total_bw, big_fft_size);
        bw_fft_size = ((float)analysis_bw/(float)total_bw) *big_fft_size ;
        printf_note("s_freq=%llu, resolution=%f, bw_fft_size=%llu\n", param->s_freq, resolution, bw_fft_size);
        spectrum_rw_fft_result(fft_get_result(), param->s_freq, resolution, bw_fft_size);

        memory_pool_free(fft_small_mpool);
        memory_pool_free(fft_big_mpool);
        memory_pool_free(fft_small_rsb_mpool);
        memory_pool_free(fft_big_rsb_mpool);
        memory_pool_free(iq_mpool);
        ps->iq_data_ready = false;
    }

}



void spectrum_init(void)
{
    struct spectrum_st *ps;
    int ret, i;
    pthread_t work_id;

    fft_init();
    printf_info("fft_init\n");

    sem_init(&(sp_sem.notify_iq_to_fft), 0, 0);

    LOCK_SP_DATA();
    ps = &_spectrum;
    ps->fft_data_ready = false;/* fft data not vaild to deal */
    ps->iq_data_ready = false;
    UNLOCK_SP_DATA();

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
}

