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


//struct mq_ctx *mqctx;
//#define SPM_MQ_NAME "/spmmq"

static pthread_cond_t spm_iq_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t spm_iq_cond_mutex = PTHREAD_MUTEX_INITIALIZER;
/* send mutex */
pthread_mutex_t send_fft_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t send_fft2_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t send_iq_mutex = PTHREAD_MUTEX_INITIALIZER;


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



static inline void spm_iq_deal_notify(void *arg)
{
    if(arg == NULL)
        return;
    /* NONBLOCK send*/
    //mqctx->ops->send(mqctx->queue, arg, sizeof(struct spm_run_parm));
    /* 通知IQ处理线程开始处理IQ数据 */
    pthread_cond_signal(&spm_iq_cond);
}


/* 在DMA连续模式下；IQ读取线程 
   在该模式下，应以最快速度读取发送数据；
   使能后，不断读取、组包、发送；读取前无需停止DMA。
*/
   //static char notify;

void spm_iq_handle_thread(void *arg)
{
    void *ptr = NULL;
    ssize_t nbyte = 0;
    struct spm_run_parm *ptr_run = NULL, run;
    struct spm_context *ctx = NULL;
    iq_t *ptr_iq = NULL;
    ssize_t  len = 0, i;
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    int ch;
   // thread_bind_cpu(1);
    ctx = (struct spm_context *)arg;

loop:
    printf_warn("######Wait IQ enable######\n");
    //notify = 1;
    /* 通过条件变量阻塞方式等待数据使能 */
    pthread_mutex_lock(&spm_iq_cond_mutex);
    while(subch_bitmap_weight() == 0)
        pthread_cond_wait(&spm_iq_cond, &spm_iq_cond_mutex);
    pthread_mutex_unlock(&spm_iq_cond_mutex);

    ch = poal_config->cid;
    printf_note(">>>>>[ch=%d]IQ start\n", ch);
    memset(&run, 0, sizeof(run));
    memcpy(&run, ctx->run_args[ch], sizeof(run));

    do{
        len = ctx->ops->read_iq_data(&ptr_iq);
        
        if(len > 0){
            ctx->ops->send_iq_data(ptr_iq, len, &run);
        }
        if(subch_bitmap_weight() == 0){
            printf_note("iq disabled\n");
            sleep(1);
            goto loop;
        }
        //usleep(1);
    }while(1);
    
}


void spm_deal(struct spm_context *ctx, void *args)
{   
    struct spm_context *pctx = ctx;
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    int ch = poal_config->cid;
    if(pctx == NULL){
        printf_err("spm is not init!!\n");
        return;
    }
    if(subch_bitmap_weight() != 0){
        struct spm_run_parm *ptr_run;
        ptr_run = (struct spm_run_parm *)args;
        printf_debug("send:ch:%d, s_freq:%llu, e_freq:%llu, bandwidth=%u\n", 
                ptr_run->ch, ptr_run->s_freq, ptr_run->e_freq, ptr_run->bandwidth);
        spm_iq_deal_notify(&args);
        
    }
    if(pctx->pdata->channel[ch].enable.psd_en){
        volatile fft_t *ptr = NULL, *ord_ptr = NULL;
        ssize_t  byte_len = 0; /* fft byte size len */
        size_t fft_len = 0, fft_ord_len = 0;
        struct spm_run_parm *ptr_run;
        ptr_run = (struct spm_run_parm *)args;
        byte_len = pctx->ops->read_fft_data(&ptr, ptr_run);
        if(byte_len < 0){
            return;
        }
        if(byte_len > 0){
            fft_len = byte_len/sizeof(fft_t);
             printf_debug("size_len=%u, fft_len=%u, ptr=%p,%p, %p\n", byte_len, fft_len, ptr,ptr_run, ptr_run->fft_ptr);
            ord_ptr = pctx->ops->data_order(ptr, fft_len, &fft_ord_len, args);
            if(ord_ptr)
                pctx->ops->send_fft_data(ord_ptr, fft_ord_len, args);
        }
    }
}

static struct spm_context *spmctx = NULL;

struct spm_context *get_spm_ctx(void)
{
    if(spmctx == NULL){
        printf_err("Spectrum Creat Failed!!\n");
        exit(1);
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

void *spm_init(void)
{
    static pthread_t send_thread_id, recv_thread_id;
    int ret, ch;
    pthread_attr_t attr;
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_CHIP)
    spmctx = spm_create_chip_context();
#elif defined (SUPPORT_SPECTRUM_FPGA)
    spmctx = spm_create_fpga_context();
#endif

    if(spmctx != NULL){
        spmctx->ops->create();
    }
    if(spmctx == NULL){
        printf_warn("spm create failed\n");
        return NULL;
    }

    for(ch = 0; ch< MAX_RADIO_CHANNEL_NUM; ch++){
        spmctx->run_args[ch] = calloc(1, sizeof(struct spm_run_parm));
        spmctx->run_args[ch]->fft_ptr = malloc(MAX_FFT_SIZE*sizeof(fft_t));///calloc(1, MAX_FFT_SIZE*sizeof(fft_t));
        if(spmctx->run_args[ch]->fft_ptr == NULL){
        printf("malloc failed\n");
        exit(1);
        }
    }

    ret=pthread_create(&recv_thread_id,NULL,(void *)spm_iq_handle_thread, spmctx);
    if(ret!=0)
        perror("pthread cread spm");
    pthread_detach(recv_thread_id);
    
#endif /* SUPPORT_PLATFORM_ARCH_ARM */
    return (void *)spmctx;
}

int spm_close(void)
{
    if(spmctx){
        spmctx->ops->close(spmctx);
    }
}


