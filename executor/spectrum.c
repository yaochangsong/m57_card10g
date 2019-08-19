#include "config.h"

struct spectrum_st _spectrum;

pthread_mutex_t spectrum_result_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t spectrum_data_mutex = PTHREAD_MUTEX_INITIALIZER;

static  bool specturm_work_read_enable(void)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    if((poal_config->enable.psd_en == 1) || (poal_config->enable.spec_analy_en == 1))
        return true;
    else
        return false;
}

bool specturm_work_write_enable(bool enable)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    poal_config->enable.spec_analy_en = (uint8_t)enable;
    return enable;
}

/* Read Rx Raw IQ data*/
static int32_t specturm_rx_iq_read(int16_t **iq_payload)
{
    ssize_t nbytes_rx = 0;
    int16_t *iqdata, *iq1data;
    int i;
    char strbuf[128];
    static int write_file_cnter = 0;
   #if (RF_ADRV9009_IIO == 1)
    iqdata = iio_read_rx0_data(&nbytes_rx);
   #else    
    nbytes_rx = 0;
   #endif
    *iq_payload = iqdata;

    printf_info("iio read data len:[%d]\n", nbytes_rx);
    printf_info("rx0 iqdata:\n");
    for(i = 0; i< 10; i++){
        printfi("%d ",*(int16_t*)(iqdata+i));
    }
    printfi("\n");

    if(write_file_cnter++ == 10) {
        sprintf(strbuf, "/run/wav0_%d", write_file_cnter);
        printfi("write rx0 iq data to:[len=%d]%s\n", nbytes_rx/2, strbuf);
        write_file_in_int16((void*)(iqdata), nbytes_rx/2, strbuf);
    }
    return nbytes_rx;
}

/* Send FFT spectrum Data to Client (UDP)*/
static void spectrum_send_fft_data(void *fft_data, size_t fft_data_len, void *param)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    uint8_t *fft_sendbuf, *pbuf;
    uint8_t *header_buf;
    uint32_t header_len = 0;

    header_buf = poal_config->assamble_response_data(&header_len, (void *)param);
    printf_debug("header_buf=%x %x, header_len=%d\n", header_buf[0],header_buf[1], header_len);

    fft_sendbuf = (uint8_t *)malloc(header_len+ fft_data_len);
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

/* Before send client, we need to remove sideband signals data */
fft_data_type *spectrum_fft_data_order(void *fft_data, uint32_t fft_data_len, uint32_t *result_fft_size)
{
    /* single sideband signal fft size */
    uint32_t single_sideband_size = fft_data_len*SINGLE_SIDE_BAND_POINT_RATE;

    /* odd number */
    if(!EVEN(single_sideband_size)){
        single_sideband_size++;
    }

    
    *result_fft_size = fft_data_len - 2 * single_sideband_size;

    return (fft_data_type *)((fft_data_type *)fft_data+single_sideband_size);
}


/* We use this function to deal FFT spectrum in user space */
void spectrum_wait_user_deal( struct spectrum_header_param *param)
{
    struct spectrum_st *ps;
    float resolution;
    uint32_t fft_size;

    ps = &_spectrum;

    /* Read Raw IQ data*/
    ps->iq_len = specturm_rx_iq_read(&ps->iq_payload);
    printf_debug("ps->iq_payload[0]=%d, %d,param->fft_size=%d, ps->iq_len=%d\n", ps->iq_payload[0],ps->iq_payload[1], param->fft_size, ps->iq_len);

    fft_size =param->fft_size;
    /* Start Convert IQ data to FFT, Noise threshold:8 */
    TIME_ELAPSED(fft_iqdata_handle(8, ps->iq_payload, fft_size, fft_size*2));

    
    /* get FFT float data*/
    ps->fft_float_payload = fft_get_data(&ps->fft_len);
    ps->fft_len = fft_size;
    
    /* Here we convert float to short integer processing */
    ps->fft_short_payload = (fft_data_type *)safe_malloc(ps->fft_len*sizeof(fft_data_type));
    FLOAT_TO_SHORT(ps->fft_float_payload, ps->fft_short_payload, ps->fft_len);

    /* Calculation resolution(Point bandwidth) by Total bandWidth& fft size */
    resolution = calc_resolution(param->bandwidth, fft_size);
    
    /*  write fft result to buffer: middle point, bandwidth point, power level
        fft_get_result(): GET FFT result
    */
    spectrum_rw_fft_result(fft_get_result(), param->s_freq, resolution, fft_size);
    printf_debug("resolution=%f, param->s_freq=%llu\n", resolution, param->s_freq);
    printf_debug("ps->fft_len=%d,fft_size=%d, ps->fft_payload=%d,%d, %f, %f\n", ps->fft_len, param->fft_size, 
        ps->fft_short_payload[0],ps->fft_short_payload[1], ps->fft_float_payload[0], ps->fft_float_payload[1]);

    LOCK_SP_DATA();
    /* To the circumstances: the client's active acquisition of FFT data; Here we user backup buffer to store fft data; 
    when the scheduled task is processed[is_wait_deal=false],  we start filling in new fft data in back buffer [ps->fft_short_payload_back]*/
    if(ps->is_wait_deal == false){
        printf_debug("safe_malloc[%d]\n", ps->fft_len*sizeof(float));
        ps->fft_short_payload_back = safe_malloc(ps->fft_len*sizeof(fft_data_type));
        FLOAT_TO_SHORT(ps->fft_float_payload, ps->fft_short_payload_back, ps->fft_len);
        ps->fft_len_back = ps->fft_len;
        ps->is_wait_deal = true;
    }
    UNLOCK_SP_DATA();
    
    uint32_t fft_order_len=0;
    fft_data_type *fft_send_payload;
        
    fft_send_payload = spectrum_fft_data_order((void *)ps->fft_short_payload, ps->fft_len, &fft_order_len);
    printf_debug("fft order len[%u], ps->fft_len=%u\n", fft_order_len, ps->fft_len);
    
    /* fill header data len(byte) */
    param->data_len = fft_order_len*sizeof(fft_data_type); 
    
    /* Send FFT data to Client */
    spectrum_send_fft_data((fft_data_type *)fft_send_payload, fft_order_len*sizeof(fft_data_type), param);
    safe_free(ps->fft_short_payload);
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
    #define SIGNAL_ADD_FIXED_VALUE 144
    
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
        pfft->bw_hz[i] = result->bandwidth[i] * freq_resolution;
        pfft->level[i] = result->arvcentfreq[i] - SIGNAL_ADD_FIXED_VALUE;
        printf_debug("s_freq_hz=%llu, freq_resolution=%f, fft_size=%u, %f\n", s_freq_hz, freq_resolution, fft_size, (SINGLE_SIDE_BAND_POINT_RATE*fft_size));
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
    if(ps->is_wait_deal == false){
        *len = 0;
        UNLOCK_SP_DATA();
        return NULL;
    }
    data = ps->fft_short_payload_back;
    *len = ps->fft_len_back;
    ps->is_wait_deal = false;
    safe_free(ps->fft_short_payload_back);
    UNLOCK_SP_DATA();
    return data;
}


void spectrum_init(void)
{
    struct spectrum_st *ps;
    ssize_t nbytes_rx = 0;

    fft_init();
    printf_info("fft_init\n");
    LOCK_SP_DATA();
    ps = &_spectrum;
    ps->is_wait_deal = false;/* fft data not vaild to deal */
    UNLOCK_SP_DATA();
}

