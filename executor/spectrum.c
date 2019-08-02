#include "config.h"

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


int8_t specturm_rx_work_deal(void)
{
    ssize_t nbytes_rx = 0;
    int16_t *iqdata;
    struct spectrum_t *ps;
    int i;
    char strbuf[128];
    static int write_file_cnter = 0;
    
    ps = &_spectrum;
    
    iqdata = specturm_rx_read_data(&nbytes_rx);
    
    printf_info("iio read data len:[%d]\n", nbytes_rx);
    for(i = 0; i< 10; i++){
        printfi("%d ",*(int16_t*)(iqdata+i));
    }
    printfi("\n");

    if(write_file_cnter++ < 3){
        sprintf(strbuf, "/run/wav%d", write_file_cnter);
        printfi("write iq data to:[len=%d]%s\n", nbytes_rx/2, strbuf);
        write_file_in_int16((void*)(iqdata), nbytes_rx/2, strbuf);
    }
    if(ps->is_deal == false){
        memcpy(ps->payload, iqdata, nbytes_rx);
        pthread_cond_signal(&spectrum_cond);
    }
    printf_info("[is_deal=%d]assamble data..\n", ps->is_deal);
    printf_info("send data..\n");
    //assamble_data
    //send_data

    return 0;
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

void spectrum_init(void)
{
    int ret, i;
    pthread_t work_id;
    struct spectrum_t *ps;

    pthread_cond_init(&spectrum_cond,NULL); 
    pthread_mutex_init(&spectrum_cond_mutex,NULL);  

    ps = &_spectrum;
    ps->payload = calloc(1, iio_get_rx_buf_size());
    if (!ps->payload){
        printf_err("calloc failed\n");
        return;
    }

    ret=pthread_create(&work_id,NULL,(void *)specturm_analysis_deal, NULL);
    if(ret!=0)
        perror("pthread cread work_id");
    pthread_detach(work_id);


}

