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

//struct mq_ctx *mqctx;
//#define SPM_MQ_NAME "/spmmq"

static pthread_cond_t spm_iq_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t spm_iq_cond_mutex = PTHREAD_MUTEX_INITIALIZER;

static inline void spm_iq_deal_notify(void *arg)
{
    if(arg == NULL)
        return;
    /* NONBLOCK send*/
    //mqctx->ops->send(mqctx->queue, arg, sizeof(struct spm_run_parm));
    /* 通知IQ处理线程开始处理IQ数据 */
    pthread_cond_signal(&spm_iq_cond);
}

void spm_send_thread(void *arg)
{
    //char send_buf[46];
    int i = 0, j =0;
    struct spm_context *ctx = NULL;
    
   // thread_bind_cpu(2);
    ctx = (struct spm_context *)arg;
    struct spm_run_parm hparam;
    memset(&hparam, 0, sizeof(struct spm_run_parm));
    hparam.data_len = 1; 
    hparam.type = 2;
    hparam.ex_type = 3;
    sleep(1);
    for(;;){
        ctx->pdata->enable.iq_en = 1;
        j = 10;
        do{
            /* NONBLOCK send*/
            hparam.data_len ++;
            if(ctx->pdata->enable.iq_en){
                spm_iq_deal_notify(&hparam);
            }
            sleep(1);
            //ctx->pdata->enable.iq_en = !ctx->pdata->enable.iq_en;
            i ++;
        }while(j--);
        ctx->pdata->enable.iq_en  = 0;
        sleep(10);
    }
}



/* 在DMA连续模式下；IQ读取线程 
   在该模式下，应以最快速度读取发送数据；
   使能后，不断读取、组包、发送；读取前无需停止DMA。
*/

void spm_iq_handle_thread(void *arg)
{
    void *ptr = NULL;
    ssize_t nbyte = 0;
    struct spm_run_parm *ptr_run = NULL, run;
    struct spm_context *ctx = NULL;
    iq_t *ptr_iq = NULL;
    ssize_t  len = 0, i;

    //thread_bind_cpu(0);
    ctx = (struct spm_context *)arg;
loop:
    printf_warn("######Wait IQ enable######\n");
    /* 通过条件变量阻塞方式等待数据使能 */
    pthread_mutex_lock(&spm_iq_cond_mutex);
    pthread_cond_wait(&spm_iq_cond, &spm_iq_cond_mutex);
    pthread_mutex_unlock(&spm_iq_cond_mutex);
    if(ctx->pdata->sub_ch_enable.iq_en == 0){
        printf_warn("IQ is not enabled!![%d]\n", ctx->pdata->sub_ch_enable.iq_en);
        sleep(1);
        goto loop;
    }
    memset(&run, 0, sizeof(run));
    memcpy(&run, ctx->run_args, sizeof(run));
    do{
        #if 0
        /* mquene NONBLOCK read,非阻塞方式从队列获取运行数据信息 */
        nbyte = mqctx->ops->recv(mqctx->queue, &ptr);
        if(nbyte >= 0 && ptr != NULL){
            ptr_run = (struct spm_run_parm *)ptr;
            ptr_run = ctx->run_args;
            printf_note("reve:[%d]ch:%d, s_freq:%llu, e_freq:%llu, bandwidth=%u\n", nbyte, 
                ptr_run->ch, ptr_run->s_freq, ptr_run->e_freq, ptr_run->bandwidth);
            memcpy(&run, ptr_run, sizeof(run));
            free(ptr);
        }
        #endif
        
        len = ctx->ops->read_iq_data(&ptr_iq);
        
        printf_note("reve handle: len=%d, ptr=%p\n", len, ptr_iq);
        if(len > 0){
            for(i = 0; i < 16; i++){
               printfd("%x ", ptr_iq[i]);
            }
            printfd("\n----------[%d]---------\n", len);
            ctx->ops->send_iq_data(ptr_iq, len, &run);
        }
        if(ctx->pdata->sub_ch_enable.iq_en == 0){
            sleep(1);
            goto loop;
        }
    }while(1);
    
}


void spm_deal(struct spm_context *ctx, void *args)
{   
    struct spm_context *pctx = ctx;

    if(pctx == NULL){
        printf_err("spm is not init!!\n");
        return;
    }
    if(pctx->pdata->sub_ch_enable.iq_en){
        struct spm_run_parm *ptr_run;
        ptr_run = (struct spm_run_parm *)args;
        printf_info("send:ch:%d, s_freq:%llu, e_freq:%llu, bandwidth=%u\n", 
                ptr_run->ch, ptr_run->s_freq, ptr_run->e_freq, ptr_run->bandwidth);
        spm_iq_deal_notify(&args);
    }
    if(pctx->pdata->enable.psd_en){
        fft_t *ptr = NULL, *ord_ptr = NULL;
        ssize_t  byte_len = 0; /* fft byte size len */
        size_t fft_len = 0, fft_ord_len = 0;

        byte_len = pctx->ops->read_fft_data(&ptr);
        if(byte_len < 0){
            return;
        }
        if(byte_len > 0){
            fft_len = byte_len/sizeof(fft_t);
            printf_debug("size_len=%u, fft_len=%u\n", byte_len, fft_len);
            ord_ptr = pctx->ops->data_order(ptr, fft_len, &fft_ord_len, args);
            if(ord_ptr)
                pctx->ops->send_fft_data(ord_ptr, fft_ord_len, args);
        }
    }
}

static struct spm_context *spmctx = NULL;

struct spm_context *get_spm_ctx(void)
{
    return spmctx;
}

void *spm_init(void)
{
    static pthread_t send_thread_id, recv_thread_id;
    int ret;

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

    //mqctx = mq_create_ctx(SPM_MQ_NAME, NULL, -1);
    //mqctx->ops->getattr(SPM_MQ_NAME);

    spmctx->run_args = calloc(1, sizeof(struct spm_run_parm));
    spmctx->run_args->fft_ptr = calloc(1, MAX_FFT_SIZE*sizeof(fft_t));

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


