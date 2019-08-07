#include "config.h"

struct spectrum_st _spectrum;

pthread_mutex_t spectrum_result_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t spectrum_data_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t spectrum_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t spectrum_cond_mutex = PTHREAD_MUTEX_INITIALIZER;


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


static int32_t specturm_rx_iq_read(int16_t **iq_payload)
{
    ssize_t nbytes_rx = 0;
    int16_t *iqdata, *iq1data;
    int i;
    char strbuf[128];
    static int write_file_cnter = 0;
   
    iqdata = iio_read_rx0_data(&nbytes_rx);
    *iq_payload = iqdata;

    printf_info("iio read data len:[%d]\n", nbytes_rx);
    printf_info("rx0 iqdata:\n");
    for(i = 0; i< 10; i++){
        printfi("%d ",*(int16_t*)(iqdata+i));
    }
    printfi("\n");

    if(write_file_cnter++ < 1){
        sprintf(strbuf, "/run/wav0_%d", write_file_cnter);
        printfi("write rx0 iq data to:[len=%d]%s\n", nbytes_rx/2, strbuf);
        write_file_in_int16((void*)(iqdata), nbytes_rx/2, strbuf);
    }

#if 0
    iq1data = specturm_rx1_read_data(&nbytes_rx);
    printf_info("rx1 iqdata:\n");
    for(i = 0; i< 10; i++){
        printfi("%d ",*(int16_t*)(iq1data+i));
    }
    printfi("\n");
    if(write_file_cnter++ < 2){
        sprintf(strbuf, "/run/wav1_%d", write_file_cnter);
        printfi("write rx1 iq data to:[len=%d]%s\n", nbytes_rx/2, strbuf);
        write_file_in_int16((void*)(iq1data), nbytes_rx/2, strbuf);
    }
#endif
    //if(ps->is_deal == false){
    //    memcpy(ps->iq_payload, iqdata, nbytes_rx);
    //    pthread_cond_signal(&spectrum_cond);
   // }
    return nbytes_rx;
}

void specturm_analysis_deal(void *arg)
{
    struct spectrum_st *ps;
    ps = &_spectrum;

    while(1){
        printf_info("Wait to analysis spcturm......\n");
        //ps->is_wait_deal = false;
        /* Mutex must be locked for pthread_cond_timedwait... */
        pthread_mutex_lock(&spectrum_cond_mutex);
        /* Thread safe "sleep" */
        pthread_cond_wait(&spectrum_cond, &spectrum_cond_mutex);
        /* No longer needs to be locked */
        pthread_mutex_unlock(&spectrum_cond_mutex);
       // ps->is_wait_deal = true;
        //fft_deal()
        printf_info("start to deal iq data###################\n");
        sleep(1);
        printf_info("end  deal iq data###################\n");
    }
}

static void spectrum_send_fft_data(float *fft_data, uint32_t fft_data_len, void *param)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    uint8_t *fft_sendbuf, *pbuf;
    uint8_t *header_buf;
    uint32_t header_len = 0;

    header_buf = poal_config->assamble_response_data(&header_len, (void *)param);
    printf_debug("header_buf=%x %x, header_len=%d\n", header_buf[0],header_buf[1], header_len);

    fft_sendbuf = (uint8_t *)malloc(header_len+ fft_data_len);
    if (!fft_sendbuf){
        printf_err("calloc failed\n");
        return;
    }
    pbuf = fft_sendbuf;
    printf_debug("malloc ok header_len:%d,fft_len=%d, len=%d\n",header_len,fft_data_len, header_len+ fft_data_len);
    memcpy(fft_sendbuf, header_buf, header_len);
    memcpy(fft_sendbuf + header_len, fft_data, fft_data_len);
    
    udp_send_data(fft_sendbuf, header_len + fft_data_len);
    safe_free(pbuf);
}

void spectrum_wait_user_deal( struct spectrum_header_param *param)
{
    struct spectrum_st *ps;

    ps = &_spectrum;
    
    ps->iq_len = specturm_rx_iq_read(&ps->iq_payload);
    printf_debug("ps->iq_payload[0]=%d\n", ps->iq_payload[0]);
    TIME_ELAPSED(fft_iqdata_handle(6, ps->iq_payload, 8*1024, ps->iq_len/2));
    ps->fft_payload = fft_get_data(&ps->fft_len);
    spectrum_rw_fft_result(fft_get_result());
    printf_debug("ps->fft_len=%d, ps->fft_payload=%f,%f\n", ps->fft_len, ps->fft_payload[0],ps->fft_payload[1]);
    param->data_len = ps->fft_len*sizeof(float); /* fill  header data len */
    LOCK_SP_DATA();
    if(ps->is_wait_deal == false){
        printf_debug("safe_malloc[%d]\n", ps->fft_len*sizeof(float));
        ps->fft_payload_back = safe_malloc(ps->fft_len*sizeof(float));
        memcpy(ps->fft_payload_back, ps->fft_payload, ps->fft_len*sizeof(float));
        ps->fft_len_back = ps->fft_len;
        ps->is_wait_deal = true;
    }
    UNLOCK_SP_DATA();
    spectrum_send_fft_data(ps->fft_payload, ps->fft_len*sizeof(float), param);
}

/* function: read or write fft result 
    @result =NULL : read
    @result != NUL: write
    @ return fft result
*/
void *spectrum_rw_fft_result(fft_result *result)
{
    int i;
    static fft_result s_fft_result, *pfft;
    pfft = &s_fft_result;
    LOCK_SP_RESULT();
    if(result == NULL){
        pfft = NULL;
        goto exit;
    }
    pfft->signalsnumber = result->signalsnumber;
    if(pfft->signalsnumber > SIGNALNUM){
        printf_err("signals number error:%d\n", pfft->signalsnumber);
        pfft = NULL;
        goto exit;
    }
    for(i = 0; i < pfft->signalsnumber; i++){
        pfft->centfeqpoint[i] = result->centfeqpoint[i];
        pfft->bandwidth[i] = result->bandwidth[i];
        pfft->arvcentfreq[i] = result->arvcentfreq[i];
    }
exit:
    UNLOCK_SP_RESULT();
    return (void *)pfft;
}


int16_t *spectrum_get_fft_data(uint32_t *len)
{
    struct spectrum_st *ps;
    int16_t *data;
    ps = &_spectrum;
    
    LOCK_SP_DATA();
    if(ps->is_wait_deal == false){
        *len = 0;
        UNLOCK_SP_DATA();
        return NULL;
    }
    data = ps->fft_payload_back;
    *len = ps->fft_len_back;
    ps->is_wait_deal = false;
    safe_free(ps->fft_payload_back);
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
#if 0
    int ret, i;
    pthread_t work_id;
    struct spectrum_t *ps;

    pthread_cond_init(&spectrum_cond,NULL); 
    pthread_mutex_init(&spectrum_cond_mutex,NULL);  

    ps = &_spectrum;
    ps->iq_payload = calloc(1, iio_get_rx_buf_size());
    if (!ps->iq_payload){
        printf_err("calloc failed\n");
        return;
    }

    ret=pthread_create(&work_id,NULL,(void *)specturm_analysis_deal, NULL);
    if(ret!=0)
        perror("pthread cread work_id");
    pthread_detach(work_id);

#endif
}

