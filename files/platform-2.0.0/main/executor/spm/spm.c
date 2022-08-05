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
#include "config.h"

static struct spm_context *spmctx = NULL;

/* send mutex */
pthread_mutex_t send_fft_mutex[MAX_RADIO_CHANNEL_NUM];
pthread_mutex_t send_iq_mutex = PTHREAD_MUTEX_INITIALIZER;

static void spm_cond_init(void)
{
     for(int i = 0; i < MAX_RADIO_CHANNEL_NUM; i++){
        pthread_mutex_init(& send_fft_mutex[i], NULL);
    }
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
        ord_ptr = ptr;
        fft_ord_len = fft_len;
        if(pctx->ops->data_order)
            ord_ptr = pctx->ops->data_order(ptr, fft_len, &fft_ord_len, args);
        if(ord_ptr)
            pctx->ops->send_fft_data(ord_ptr, fft_ord_len, args);
    }
}


struct spm_context *get_spm_ctx(void)
{
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
    int ret, ch, type;
    static int s_ch[MAX_RADIO_CHANNEL_NUM];
    pthread_attr_t attr;
    pthread_t work_id;
    spmctx = spm_create_context();
    if(spmctx != NULL && spmctx->ops->create){
        spmctx->ops->create();
    }
    if(spmctx == NULL){
        printf_warn("spm create failed\n");
        return NULL;
    }

    for(ch = 0; ch< MAX_RADIO_CHANNEL_NUM; ch++){
        spmctx->run_args[ch] = calloc(1, sizeof(struct spm_run_parm));
        spmctx->run_args[ch]->fft_ptr = calloc(1, MAX_FFT_SIZE*sizeof(fft_t));
        if(spmctx->run_args[ch]->fft_ptr == NULL){
            printf("malloc failed\n");
            exit(1);
        }
        spmctx->run_args[ch]->fft_ptr_swap = calloc(1, MAX_FFT_SIZE*sizeof(fft_t));
        if(spmctx->run_args[ch]->fft_ptr_swap == NULL){
            printf("malloc failed\n");
            exit(1);
        }
        for(type = 0; type < STREAM_NIQ_TYPE_MAX; type++){
            spmctx->run_args[ch]->dis_iq.ptr[type] = calloc(1, DMA_IQ_TYPE_BUFFER_SIZE);
            if(spmctx->run_args[ch]->dis_iq.ptr[type] == NULL){
                printf("malloc failed\n");
                exit(1);
            }
            printf_note("type=%d, ptr=%p\n", type, spmctx->run_args[ch]->dis_iq.ptr[type]);
            spmctx->run_args[ch]->dis_iq.len[type] = 0;
        }
    }
    spm_cond_init();
#ifdef CONFIG_SPM_DISTRIBUTOR
    spmctx->distributor = (void *)spm_create_distributor_ctx();
#endif

    return (void *)spmctx;
}

int spm_close(void)
{
    if(spmctx){
        spmctx->ops->close(spmctx);
    }
    return 0;
}


