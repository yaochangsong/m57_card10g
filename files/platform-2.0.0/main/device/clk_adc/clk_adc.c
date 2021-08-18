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
*  Rev 1.0   9 June. 2020   yaochangsong
*  Initial revision.
******************************************************************************/
#include "config.h"
#include "clk_adc.h"
#include "../../bsp/io.h"


static struct clock_adc_ctx *_ca_ctx = NULL;
extern struct clock_adc_ctx * clock_adc_fpga_cxt(void);

void *adclock_check_thread(void *s)
{
    bool adc_check = false;
    struct clock_adc_ctx *_ctx= (struct clock_adc_ctx *)s;
    
    if(_ctx == NULL){
        pthread_exit(0);
        return NULL;
    }
    
    while(1){
        if(adc_check == false)
            _ctx->ops->init();
        sleep(1);
        if(io_get_adc_status(NULL)){
            adc_check = true;
        }else
            adc_check = false;
        if(adc_check)
            printf_info("ADC init OK!\n");
        else
            printf_warn("ADC init Faild!\n");
        sleep(3);
    }
    
}


int adclock_loop(void)
{
    pthread_t tid;
    int err;
    err = pthread_create (&tid , NULL , adclock_check_thread , _ca_ctx);
    if (err != 0){
        printf("can't create thread: %s\n", strerror(err));
        return -1;
    }
    pthread_detach(tid);
    return 0;
}

int clock_adc_init(void)
{
#if defined(CONFIG_ARCH_ARM)
    _ca_ctx = clock_adc_fpga_cxt();
#else
    return -1;
#endif
    adclock_loop();
    return 0;
}

int clock_adc_close(void)
{
#if defined(CONFIG_ARCH_ARM)
    if(_ca_ctx)
         _ca_ctx->ops->close();
#endif
	return 0;

}
