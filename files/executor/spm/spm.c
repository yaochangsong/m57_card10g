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
*  Rev 1.0   21 Feb. 2020   yaochangsong
*  Initial revision.
******************************************************************************/
#include <mqueue.h>
#include <signal.h>
#include <sys/wait.h>
#include "config.h"
#include "spm.h"
#include "utils/mq.h"
#include <assert.h>
#include "spm_fpga.h"
#include "spm_chip.h"
#include "spm_xdma.h"
#include "../../bsp/io.h"
#include "../../utils/bitops.h"
#include "../../net/net_sub.h"
#include "../../net/net_thread.h"



//struct mq_ctx *mqctx;
//#define SPM_MQ_NAME "/spmmq"

/* send mutex */
pthread_mutex_t send_fft_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t send_fft2_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t send_iq_mutex = PTHREAD_MUTEX_INITIALIZER;

/* biq */
static pthread_cond_t spm_biq_cond[MAX_RADIO_CHANNEL_NUM];
static pthread_mutex_t spm_biq_cond_mutex[MAX_RADIO_CHANNEL_NUM];

/* niq */
static pthread_cond_t spm_niq_cond;
static pthread_mutex_t spm_niq_cond_mutex;

/* fft */
static pthread_cond_t spm_fft_cond;
static pthread_mutex_t spm_fft_cond_mutex;

/* xdma */
static pthread_cond_t spm_xdma_cond[MAX_XDMA_NUM];
static pthread_mutex_t spm_xdma_cond_mutex[MAX_XDMA_NUM];

struct sem_st work_sem;

void  *spm_xdma_param_init(int ch);
static struct spm_context *spmctx = NULL;

static void spm_cond_init(void)
{
    for(int i = 0; i < MAX_XDMA_NUM; i++){
        pthread_cond_init(&spm_xdma_cond[i], NULL);
        pthread_mutex_init(&spm_xdma_cond_mutex[i], NULL);
    }
#if 0
    for(int i = 0; i < MAX_RADIO_CHANNEL_NUM; i++){
        pthread_cond_init(&spm_biq_cond[i], NULL);
        pthread_mutex_init(&spm_biq_cond_mutex[i], NULL);
    }
    pthread_cond_init(&spm_niq_cond, NULL);
    pthread_mutex_init(&spm_niq_cond_mutex, NULL);
    
    pthread_cond_init(&spm_fft_cond, NULL);
    pthread_mutex_init(&spm_fft_cond_mutex, NULL);
#endif
}


static void show_thread_priority(pthread_attr_t *attr,int policy)
{
    int priority = sched_get_priority_max(policy);
    assert(priority!=-1);
    printf("max_priority=%d\n",priority);
    priority= sched_get_priority_min(policy);
    assert(priority!=-1);
    printf("min_priority=%d\n",priority);
}

static int get_thread_priority(pthread_attr_t *attr)
{
  struct sched_param param;
  int rs = pthread_attr_getschedparam(attr,&param);
  assert(rs==0);
  printf("priority=%d",param.__sched_priority);
  return param.__sched_priority;
}


void spm_biq_deal_notify(void *arg)
{
    int ch;
    
    if(arg == NULL)
        return;
    
    ch = *(uint8_t *)arg;
    
    if(ch >= MAX_RADIO_CHANNEL_NUM)
        return;
    /* ??????IQ????????????????????????IQ?????? */
    pthread_cond_signal(&spm_biq_cond[ch]);
}

void spm_xdma_deal_notify(void *arg)
{
    int ch;
    
    if(arg == NULL)
        return;
    
    ch = *(uint8_t *)arg;
    if(ch >= MAX_XDMA_NUM)
        return;
    pthread_cond_signal(&spm_xdma_cond[ch]);
}

void spm_niq_deal_notify(void *arg)
{
    pthread_cond_signal(&spm_niq_cond);
}

void spm_fft_deal_notify(void *arg)
{
#ifdef SUPPORT_SPECTRUM_SERIAL
    pthread_cond_signal(&spm_fft_cond);
#else
    int ch;
    if(arg == NULL)
        return;
    
    ch = *(uint8_t *)arg;
    sem_post(&work_sem.notify_deal[ch]);
#endif
}


void spm_fft_deal_before(int ch)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    executor_set_command(EX_MID_FREQ_CMD, EX_SMOOTH_TIME, ch, &poal_config->channel[ch].multi_freq_point_param.smooth_time);
    executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, ch, &poal_config->channel[ch].rf_para.attenuation);
    executor_set_command(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, ch, &poal_config->channel[ch].rf_para.mgc_gain_value);
}

void spm_fft_serial_thread(void *arg)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    volatile unsigned long ch = 0;
    void *spm_arg = (void *)get_spm_ctx();
    int find_work = 0;
    pthread_detach(pthread_self());
    
wait:
    printf_note("######Wait FFT enable######\n");
   // sleep(1);
    pthread_mutex_lock(&spm_fft_cond_mutex);
    pthread_cond_wait(&spm_fft_cond, &spm_fft_cond_mutex);
    pthread_mutex_unlock(&spm_fft_cond_mutex);
    
    #if 0
    for_each_set_bit(ch, get_ch_bitmap(CH_TYPE_FFT), MAX_RADIO_CHANNEL_NUM){
        if(ch >= MAX_RADIO_CHANNEL_NUM){
            printf_err("channel[%ld] is too big\n", ch);
        }
        printf_note("ch: %ld,Fixed mode FFT start\n", ch);
        poal_config->channel[ch].enable.bit_reset = false;
    }
    #endif
    poal_config->channel[0].enable.bit_reset = false;
    while(1){
        find_work = 0;
        if(poal_config->channel[0].enable.bit_reset == true){
            poal_config->channel[0].enable.bit_reset = false;
            printf_note("[ch:%lu]receive reset task sigal......\n", ch);
            goto wait;
        }
#if defined(SUPPORT_SPM_SEND_BUFFER)
         for(ch = 0; ch < MAX_RADIO_CHANNEL_NUM; ch ++)
#else
         for_each_set_bit(ch, get_ch_bitmap(CH_TYPE_FFT), MAX_RADIO_CHANNEL_NUM)
#endif
        {
            if(ch >= MAX_RADIO_CHANNEL_NUM){
                printf_err("channel[%ld] is too big\n", ch);
                continue;
            }
            find_work ++;
            for(int j = 0; j < SEGMENT_FREQ_NUM; j++){
                executor_serial_points_scan(ch, poal_config->channel[0].work_mode, j, spm_arg);
            }
        }
        if(find_work == 0)
            goto wait;
    }
}


void spm_fft_parallel_thread(void *arg)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    uint32_t fft_size;
    uint32_t j, i;
    int ch = *(int *)arg;
    void *spm_arg = (void *)get_spm_ctx();
    pthread_detach(pthread_self());
    
    if(ch >= MAX_RADIO_CHANNEL_NUM){
        printf_err("channel[%d, %d] is too big\n", ch, *(int *)arg);
        pthread_exit(0);
    }
    //thread_bind_cpu(1);
    while(1)
    {
        
loop:   printf_note("######channel[%d] wait to deal work######\n", ch);
        sem_wait(&work_sem.notify_deal[ch]);
        
#if defined(SUPPORT_PROJECT_WD_XCR) || defined(SUPPORT_PROJECT_WD_XCR_40G)
        if(poal_config->channel[ch].enable.bit_en == 0)
            safe_system("/etc/led.sh transfer off &");
        else
            safe_system("/etc/led.sh transfer on &");
#endif
        if(OAL_NULL_MODE == poal_config->channel[ch].work_mode){
            printf_warn("Work Mode not set\n");
            sleep(1);
            goto loop;
        }
        printf_note("receive notify, [Channel:%d]%s Work: [%s], [%s], [%s]\n", 
                     ch,
                     poal_config->channel[ch].enable.bit_en == 0 ? "Stop" : "Start",
                     poal_config->channel[ch].enable.psd_en == 0 ? "Psd Stop" : "Psd Start",
                     poal_config->channel[ch].enable.audio_en == 0 ? "Audio Stop" : "Audio Start",
                     poal_config->channel[ch].enable.iq_en == 0 ? "IQ Stop" : "IQ Start");
        
        poal_config->channel[ch].enable.bit_reset = false;
        printf_note("-------------------------------------\n");
        if(poal_config->channel[ch].work_mode == OAL_FAST_SCAN_MODE){
            printf_note("            FastScan             \n");
        }else if(poal_config->channel[ch].work_mode == OAL_MULTI_ZONE_SCAN_MODE){
            printf_note("             MultiZoneScan       \n");
        }else if(poal_config->channel[ch].work_mode == OAL_FIXED_FREQ_ANYS_MODE){
            printf_note("             Fixed Freq          \n");
        }else if(poal_config->channel[ch].work_mode == OAL_MULTI_POINT_SCAN_MODE){
            printf_note("             MultiPointScan       \n");
        }else{
            goto loop;
        }
        printf_note("-------------------------------------\n");
        //spm_fft_deal_before(ch);
        for(;;){
            switch(poal_config->channel[ch].work_mode)
            {
                case OAL_FAST_SCAN_MODE:
                case OAL_MULTI_ZONE_SCAN_MODE:
                {   
                    if(poal_config->channel[ch].multi_freq_fregment_para.freq_segment_cnt == 0){
                        printf_warn("freqency segment count is 0\n");
                        sleep(1);
                        goto loop;
                    }
                    if(poal_config->channel[ch].enable.psd_en || poal_config->channel[ch].enable.spec_analy_en){
                        for(j = 0; j < poal_config->channel[ch].multi_freq_fregment_para.freq_segment_cnt; j++){
                            if(executor_fragment_scan(j, ch, poal_config->channel[ch].work_mode, spm_arg) == -1){
                                io_set_enable_command(PSD_MODE_DISABLE, ch, -1, 0);
                                usleep(1000);
                                goto loop;
                            }
                        }
                    }else{
                        io_set_enable_command(PSD_MODE_DISABLE, ch, -1, 0);
                        usleep(1000);
                        goto loop;
                    }
                }
                    break;
                case OAL_FIXED_FREQ_ANYS_MODE:
                case OAL_MULTI_POINT_SCAN_MODE:
                {
                    if(poal_config->channel[ch].multi_freq_point_param.freq_point_cnt == 0){
                        printf_warn("freqency segment count is 0\n");
                        sleep(1);
                        goto loop;
                    }

                    if(poal_config->channel[ch].enable.bit_en){
                        if(executor_points_scan(ch, poal_config->channel[ch].work_mode, spm_arg) == -1){
                            io_set_enable_command(PSD_MODE_DISABLE, ch, -1,  0);
                            #if 0
                            io_set_enable_command(AUDIO_MODE_DISABLE, ch, -1, 0);
                            for(i = 0; i< MAX_SIGNAL_CHANNEL_NUM; i++){
                                io_set_enable_command(IQ_MODE_DISABLE, ch, i, 0);
                            }
                            #endif
                            usleep(1000);
                            goto loop;
                        }
                    }else{
                        io_set_enable_command(PSD_MODE_DISABLE, ch, -1, 0);
                        io_set_enable_command(AUDIO_MODE_DISABLE, ch, -1, 0);
                        for(i = 0; i< MAX_SIGNAL_CHANNEL_NUM; i++){
                            io_set_enable_command(IQ_MODE_DISABLE, ch, i, 0);
                        }
                        usleep(1000);
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

static void spm_niq_dispatcher_buffer_clear(void)
{
    int type, ch;
    struct spm_context *ctx = NULL;
    
    ctx = get_spm_ctx();
    for(ch = 0; ch< MAX_RADIO_CHANNEL_NUM; ch++){
        for(type = 0; type < STREAM_NIQ_TYPE_MAX; type++){
            memset(ctx->run_args[ch]->dis_iq.ptr[type], 0, DMA_IQ_TYPE_BUFFER_SIZE);
            ctx->run_args[ch]->dis_iq.len[type] = 0;
            ctx->run_args[ch]->dis_iq.offset[type] = 0;
        }
    }
}

/* ???DMA??????????????????IQ???????????? 
   ?????????????????????????????????????????????????????????
   ??????????????????????????????????????????????????????????????????DMA???
*/
void spm_niq_handle_thread(void *arg)
{
    void *ptr = NULL;
    ssize_t nbyte = 0;
    struct spm_run_parm *ptr_run = NULL, run;
    struct spm_context *ctx = NULL;
    iq_t *ptr_iq = NULL;
    ssize_t  len = 0, i;
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    int ch, type;
    //thread_bind_cpu(1);
    ctx = (struct spm_context *)arg;
    pthread_detach(pthread_self());
    
loop:
    printf_note("######Wait NIQ or Radio enable######\n");
    /* ???????????????????????????????????????????????? */
    pthread_mutex_lock(&spm_niq_cond_mutex);
    while(subch_bitmap_weight(CH_TYPE_IQ) == 0)
        pthread_cond_wait(&spm_niq_cond, &spm_niq_cond_mutex);
    pthread_mutex_unlock(&spm_niq_cond_mutex);

    ch = poal_config->cid;
    printf_note(">>>>>[ch=%d]NIQ or Radio start\n", ch);
    memset(&run, 0, sizeof(run));
    memcpy(&run, ctx->run_args[ch], sizeof(run));
    spm_niq_dispatcher_buffer_clear();
    do{
        len = ctx->ops->read_niq_data((void **)&ptr_iq);
        if(len > 0){
            #if 0
            if(ctx->ops->niq_dispatcher && test_audio_on()){
                ctx->ops->niq_dispatcher(ptr_iq, len, &run);
                for_each_niq_type(type, run){
                    if(ctx->ops->send_niq_type){
                        ctx->ops->send_niq_type(type, run.dis_iq.send_ptr, run.dis_iq.send_len, &run);
                    }
                }
                ctx->ops->read_niq_over_deal(&len);
            }else
            #endif
            {
                ctx->ops->send_niq_data(ptr_iq, len, &run);
            }
        }
        if(subch_bitmap_weight(CH_TYPE_IQ) == 0){
            printf_debug("iq disabled\n");
            usleep(1000);
            goto loop;
        }
    }while(1);
    
}

void spm_biq_handle_thread(void *arg)
{
    void *ptr = NULL;
    ssize_t nbyte = 0;
    struct spm_run_parm *ptr_run = NULL, run;
    struct spm_context *ctx = NULL;
    iq_t *ptr_iq = NULL;
    ssize_t  len = 0, i;
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    int ch, type;
   // thread_bind_cpu(1);
    ctx = spmctx;
    ch = *(int *)arg;
    if(ch >= MAX_RADIO_CHANNEL_NUM)
        pthread_exit(0);
    
    pthread_detach(pthread_self());
loop:
    printf_note("######Wait BIQ[ch:%d] enable######\n", ch);
    /* ???????????????????????????????????????????????? */
    pthread_mutex_lock(&spm_biq_cond_mutex[ch]);
    while(test_ch_iq_on(ch) == false)
        pthread_cond_wait(&spm_biq_cond[ch], &spm_biq_cond_mutex[ch]);
    pthread_mutex_unlock(&spm_biq_cond_mutex[ch]);

    
    printf_note(">>>>>[ch=%d]BIQ start\n", ch);
    memset(&run, 0, sizeof(run));
    memcpy(&run, ctx->run_args[ch], sizeof(run));
    do{
        if(ctx->ops->read_biq_data)
            len = ctx->ops->read_biq_data(ch, (void **)&ptr_iq);
        if(len > 0){
                if(ctx->ops->send_biq_data)
                    ctx->ops->send_biq_data(ch, ptr_iq, len, &run);
        }
        if(test_ch_iq_on(ch) == false){
            printf_note("iq disabled\n");
            usleep(1000);
            goto loop;
        }
    }while(1);
    
}


void spm_deal(struct spm_context *ctx, void *args, int ch)
{   
    struct spm_context *pctx = ctx;
    struct poal_config *poal_config = &(config_get_config()->oal_config);

    if(pctx == NULL){
        printf_err("spm is not init!!\n");
        return;
    }
    volatile fft_t *ptr = NULL, *ord_ptr = NULL;
    ssize_t  byte_len = 0; /* fft byte size len */
    size_t fft_len = 0, fft_ord_len = 0;
    struct spm_run_parm *ptr_run;
    ptr_run = (struct spm_run_parm *)args;
    byte_len = pctx->ops->read_fft_data(ch, (void **)&ptr, ptr_run);
    if(byte_len < 0){
        return;
    }
    if(byte_len > 0){
        fft_len = byte_len/sizeof(fft_t);
         printf_debug("size_len=%lu, fft_len=%lu, ptr=%p,%p, %p\n", byte_len, fft_len, ptr,ptr_run, ptr_run->fft_ptr);
        ord_ptr = pctx->ops->data_order(ptr, fft_len, &fft_ord_len, args);
        if(ord_ptr)
            pctx->ops->send_fft_data(ord_ptr, fft_ord_len, args);
    }
}

void spm_xdma_read_data_handle_thread(void *arg)
{
    struct spm_context *ctx = NULL;
    char *ptr_data[XDMA_TRANSFER_MAX_DESC] = {NULL};
    uint32_t  len[XDMA_TRANSFER_MAX_DESC] = {0};
    int count = 0;
    int ch, section_id = 0;

    ctx = spmctx;
    ch = *(int *)arg;
    if(ch >= MAX_XDMA_NUM)
        pthread_exit(0);
    
    pthread_detach(pthread_self());
loop:
    printf_note(">>>>>XDMA read Wait!\n");
    pthread_mutex_lock(&spm_xdma_cond_mutex[ch]);
    while(socket_bitmap_weight() == 0)
        pthread_cond_wait(&spm_xdma_cond[ch], &spm_xdma_cond_mutex[ch]);
    pthread_mutex_unlock(&spm_xdma_cond_mutex[ch]);
    printf_note(">>>>>XDMA read start\n");
    do{
        for_each_set_bit(section_id, socket_get_bitmap(), MAX_CLINET_SOCKET_NUM){
            io_socket_set_id(section_id);
            do{
                if(ctx->ops->read_xdma_data)
                    count = ctx->ops->read_xdma_data(ch, (void **)ptr_data, len, NULL);
                if(count > 0){
                    if(ctx->ops->send_xdma_data)
                        ctx->ops->send_xdma_data(ch, ptr_data, len, count, &section_id);
                }
                if(socket_bitmap_weight() == 0){
                    printf_note("all client offline\n");
                    usleep(1000);
                    goto loop;
                }
            }while(count > 0);
        }
    }while(1);
    
}

static inline void _net_thread_dispatcher_refresh(void *args, int ch)
{
    struct spm_run_parm *arg = args;
    spm_clear_all_hash(arg->hash);
}


void spm_xdma_data_handle_thread_dispatcher(void *arg)
{
    void *ptr = NULL;
    struct spm_context *ctx = NULL;
    char *ptr_data[XDMA_TRANSFER_MAX_DESC] = {NULL};
    uint32_t  len[XDMA_TRANSFER_MAX_DESC] = {0};
    int ch, section_id = 0;
    int count = 0;
    struct spm_run_parm *run = NULL;
    
    ctx = spmctx;
    ch = *(int *)arg;
    if(ch >= MAX_XDMA_NUM)
        pthread_exit(0);
    
    pthread_detach(pthread_self());
   // thread_bind_cpu(1);
    run = spm_xdma_param_init(ch);
    io_set_enable_command(XDMA_MODE_DISABLE, ch, 0, 0);
loop:
    printf_note(">>>>>Read XDMA%d Wait!\n", ch);
    pthread_mutex_lock(&spm_xdma_cond_mutex[ch]);
    while(socket_bitmap_weight() == 0)
        pthread_cond_wait(&spm_xdma_cond[ch], &spm_xdma_cond_mutex[ch]);
    pthread_mutex_unlock(&spm_xdma_cond_mutex[ch]);
    io_set_enable_command(XDMA_MODE_ENABLE, ch, 0, 0);
    //spm_hash_delete(run->hash);/* clear hash table */
    printf_note(">>>>>XDMA%d read start\n", ch);
    do{
        if(ctx->ops->read_xdma_data)
            count = ctx->ops->read_xdma_data(ch, (void **)ptr_data, len, run);
        if(count > 0 && (count < sizeof(len))){
#if (TCP_DATA_SEND_BY_THREADS == 1)
            net_thread_con_broadcast(ch, run);
            _net_thread_dispatcher_refresh(run, ch);
#else
            if(ctx->ops->send_xdma_data)
                ctx->ops->send_xdma_data(ch, ptr_data, len, count, run);
#endif
        }
        ctx->ops->read_xdma_over_deal(ch, NULL);
        if(socket_bitmap_weight() == 0){
            io_set_enable_command(XDMA_MODE_DISABLE, ch, 0, 0);
            m57_unload_all_bitfile_from_fpga();
            printf_note("all client offline\n");
            usleep(1000);
            goto loop;
        }
    }while(1);
    
}


struct spm_context *get_spm_ctx(void)
{
    if(spmctx == NULL){
#ifdef DEBUG_TEST
        spmctx = spm_create_xdma_context();
#else
        printf_err("Spectrum Creat Failed!!\n");
        exit(1);
#endif
    }
    return spmctx;
}

void thread_attr_set(pthread_attr_t *attr, int policy, int prio)
{
    
    struct sched_param param;
    pthread_attr_init(attr);

    param.sched_priority = prio;
    pthread_attr_setschedpolicy(attr,policy);  /* SCHED_FIFO SCHED_RR SCHED_OTHER */
    pthread_attr_setschedparam(attr,&param);
    pthread_attr_setinheritsched(attr,PTHREAD_EXPLICIT_SCHED);
}

void  *spm_xdma_param_init(int ch)
{
    struct spm_run_parm *param;

    param = calloc(1, sizeof(*param));
    if (!param)
        return NULL;

    printf_note("ch:%d SPM Hash create!\n", ch);
    spm_hash_create(&(param->hash));
    if(spmctx)
        spmctx->run_args[ch] = param;
    return param;
}
void *spm_init(void)
{
    static pthread_t send_thread_id, recv_thread_id;
    int ret, ch, type;
    //static int s_ch[MAX_RADIO_CHANNEL_NUM];
    static int xdma_ch[MAX_XDMA_NUM];
    pthread_attr_t attr;
    pthread_t work_id;

//#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_CHIP)
    spmctx = spm_create_chip_context();
#elif defined (SUPPORT_SPECTRUM_FPGA)
    spmctx = spm_create_xdma_context();
   // spmctx = spm_create_fpga_context();
#endif

    if(spmctx != NULL){
        spmctx->ops->create();
    }
    if(spmctx == NULL){
        printf_warn("spm create failed\n");
        return NULL;
    }
    spm_cond_init();

    for(int i = 0; i <MAX_XDMA_NUM; i++){
        xdma_ch[i] = i;
        ret=pthread_create(&recv_thread_id,NULL,(void *)spm_xdma_data_handle_thread_dispatcher, &xdma_ch[i]);
        if(ret!=0)
            perror("pthread cread xdma");
    }
    


//#endif /* SUPPORT_PLATFORM_ARCH_ARM */
    return (void *)spmctx;
}

int spm_close(void)
{
    if(spmctx){
        spmctx->ops->close(spmctx);
    }
    return 0;
}


