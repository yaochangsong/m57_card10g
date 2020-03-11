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

struct mq_ctx *mqctx;
#define SPM_MQ_NAME "/spmmq"

static pthread_cond_t spm_iq_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t spm_iq_cond_mutex = PTHREAD_MUTEX_INITIALIZER;

void spm_iq_deal_notify(void *arg)
{
    if(arg == NULL)
        return;
    /* NONBLOCK send*/
    mqctx->ops->send(mqctx->queue, arg, sizeof(struct spm_run_parm));
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
    struct spm_run_parm *ptr_run, run;
    struct spm_context *ctx = NULL;
    iq_t *ptr_iq = NULL;
    size_t  len = 0;

    //thread_bind_cpu(0);
    ctx = (struct spm_context *)arg;
loop:
    printf_warn("######Wait IQ enable######\n");
    /* 通过条件变量阻塞方式等待数据使能 */
    pthread_mutex_lock(&spm_iq_cond_mutex);
    pthread_cond_wait(&spm_iq_cond, &spm_iq_cond_mutex);
    pthread_mutex_unlock(&spm_iq_cond_mutex);
    if(ctx->pdata->enable.iq_en == 0){
        printf_warn("IQ is not enabled!![%d]\n", ctx->pdata->enable.iq_en);
        sleep(1);
        goto loop;
    }
    memset(&run, 0, sizeof(run));
    do{
        /* mquene NONBLOCK read,非阻塞方式从队列获取运行数据信息 */
        nbyte = mqctx->ops->recv(mqctx->queue, &ptr);
        if(nbyte >= 0 && ptr != NULL){
            ptr_run = (struct spm_run_parm *)ptr;
            printf_note("reve:[%d]%d, %d, %d\n", nbyte, ptr_run->data_len, ptr_run->type, ptr_run->ex_type);
            memcpy(&run, ptr_run, sizeof(run));
            free(ptr);
        }
        
        len = ctx->ops->read_iq_data(&ptr_iq);
        printf_note("reve handle: [%d]len=%d, ptr=%s\n",ptr_run->data_len, len, ptr_iq);
        ctx->ops->send_iq_data(ptr_iq, len, &run);
        if(ctx->pdata->enable.iq_en == 0){
            sleep(1);
            goto loop;
        }
        sleep(1);
    }while(1);
}




void spm_deal(struct spm_context *ctx, void *args)
{   
    struct spm_context *pctx = ctx;
    struct spm_backend_ops *ops = pctx->ops;
    
    
    if(pctx == NULL || ops == NULL){
        printf_err("spm is not init!!\n");
        return;
    }
    
    pctx->pdata->enable.iq_en = 1;
    pctx->run_args = args;
    if(pctx->pdata->enable.iq_en){
        spm_iq_deal_notify(&args);
    }else if(pctx->pdata->enable.psd_en){
        fft_t *ptr = NULL, *ord_ptr = NULL;
        size_t  len = 0, ord_len = 0;
        len = ops->read_fft_data(&ptr);
        printf_note("len=%d, ptr=%s", len, ptr);
        ord_ptr = ops->data_order(&ptr, len, &ord_len, args);
        ops->send_fft_data(ord_ptr, ord_len, args);
    }
}


void *spm_init(void)
{
    static pthread_t send_thread_id, recv_thread_id;
    int ret;
    struct spm_context *ctx = NULL;
    
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_CHIP)
    ctx = spm_create_chip_context();
#elif defined (SUPPORT_SPECTRUM_FPGA)
    ctx = spm_create_fpga_context();
#endif
    if(ctx != NULL)
        ctx->ops->create();
    if(ctx == NULL){
        printf_warn("spm create failed\n");
    }

    mqctx = mq_create_ctx(SPM_MQ_NAME, NULL, -1);

    ctx->run_args = NULL;
    ret=pthread_create(&send_thread_id,NULL,(void *)spm_send_thread, ctx);
    if(ret!=0)
        perror("pthread cread spm");
    pthread_detach(send_thread_id);

    ret=pthread_create(&recv_thread_id,NULL,(void *)spm_iq_handle_thread, ctx);
    if(ret!=0)
        perror("pthread cread spm");
    pthread_detach(recv_thread_id);
    
#endif /* SUPPORT_PLATFORM_ARCH_ARM */
    return (void *)ctx;
}


