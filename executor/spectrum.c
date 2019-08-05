#include "config.h"
#include "fft.h"

struct spectrum_t _spectrum;

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
    int16_t *iqdata;
    int i;
    char strbuf[128];
    static int write_file_cnter = 0;
   
    
    iqdata = specturm_rx_read_data(&nbytes_rx);
    *iq_payload = iqdata;
    printf_info("iio read data len:[%d]\n", nbytes_rx);
    for(i = 0; i< 10; i++){
        printfi("%d ",*(int16_t*)(iqdata+i));
    }
    printfi("\n");

    if(write_file_cnter++ < 0){
        sprintf(strbuf, "/run/wav%d", write_file_cnter);
        printfi("write iq data to:[len=%d]%s\n", nbytes_rx/2, strbuf);
        write_file_in_int16((void*)(iqdata), nbytes_rx/2, strbuf);
    }
    //if(ps->is_deal == false){
    //    memcpy(ps->iq_payload, iqdata, nbytes_rx);
    //    pthread_cond_signal(&spectrum_cond);
   // }
    printf_info("end rx read:%d\n", nbytes_rx);
    return nbytes_rx;
}

void specturm_analysis_deal(void *arg)
{
    struct spectrum_t *ps;
    ps = &_spectrum;

    while(1){
        printf_info("Wait to analysis spcturm......\n");
        ps->is_deal = false;
        /* Mutex must be locked for pthread_cond_timedwait... */
        pthread_mutex_lock(&spectrum_cond_mutex);
        /* Thread safe "sleep" */
        pthread_cond_wait(&spectrum_cond, &spectrum_cond_mutex);
        /* No longer needs to be locked */
        pthread_mutex_unlock(&spectrum_cond_mutex);
        ps->is_deal = true;
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

    header_len = poal_config->assamble_response_data(header_buf, (void *)param);
    printf_debug("header_buf[0]=%x, header_len=%d\n", header_buf[0], header_len);

    fft_sendbuf = (uint8_t *)malloc(header_len+ fft_data_len);
    if (!fft_sendbuf){
        printf_err("calloc failed\n");
        return;
    }
    pbuf = fft_sendbuf;
    printf_info("malloc ok header_len:%d,fft_len=%d, len=%d\n",header_len,fft_data_len, header_len+ fft_data_len);
    memcpy(fft_sendbuf, header_buf, header_len);
    memcpy(fft_sendbuf + header_len, fft_data, fft_data_len);
    
    udp_send_data(fft_sendbuf, header_len + fft_data_len);
    printf_info("send over\n");
    safe_free(pbuf);
    printf_info("end send_fft_data\n");
}

void spectrum_wait_user_deal( struct spectrum_header_param *param)
{
    struct spectrum_t *ps;

    ps = &_spectrum;
    
    ps->iq_len = specturm_rx_iq_read(&ps->iq_payload);
    printf_debug("ps->iq_payload[0]=%d\n", ps->iq_payload[0]);
    TIME_ELAPSED(fft_iqdata_handle(6, ps->iq_payload, 8*1024, ps->iq_len/2));
    ps->fft_len = fft_get_data(&ps->fft_payload);
    printf_debug("ps->fft_len=%d, ps->fft_payload=%f\n", ps->fft_len, ps->fft_payload[0]);
    spectrum_send_fft_data(ps->fft_payload, ps->fft_len, param);
}

void spectrum_get_fft_result(uint8_t *header, uint32_t header_len)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    uint8_t *sendbuf= NULL;
    //fft_result *result;
    //result = fft_get_result();
}


void spectrum_init(void)
{
    struct spectrum_t *ps;
    ssize_t nbytes_rx = 0;

    fft_init();
    printf_info("fft_init\n");
    ps = &_spectrum;
    nbytes_rx = iio_get_rx_buf_size();
    printf_info("init rx read:%d\n", nbytes_rx);
    ps->iq_payload = (int16_t*)malloc(nbytes_rx);
    if (!ps->iq_payload){
        printf_err("calloc failed\n");
        return;
    }
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

