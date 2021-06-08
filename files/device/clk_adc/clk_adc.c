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
#include "clk_adc_spi.h"


static struct clock_adc_ctx *_ca_ctx = NULL;

extern struct clock_adc_ctx * clock_adc_fpga_cxt(void);
extern struct clock_adc_ctx * clock_adc_spi_cxt(void);

int clock_adc_init(void)
{
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_CLOCK_ADC_SPI)
    _ca_ctx = clock_adc_spi_cxt();
#elif defined (SUPPORT_CLOCK_ADC_FPGA)
    _ca_ctx = clock_adc_fpga_cxt();
#else
    #error "NOT define clock_adc function!!!!"
    return -1;
#endif
    bool adc_check = false;
    for(int c = 0; c < 3; c++){
        _ca_ctx->ops->init();
        sleep(1);
        if(io_get_adc_status()){
            adc_check = true;
            break;
        }
        sleep(1);
    }
    if(adc_check)
        printf_note("ADC init OK!\n");
    else
        printf_note("ADC init Faild!\n");
#endif
	return 0;
}

int clock_adc_close(void)
{
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
    if(_ca_ctx)
         _ca_ctx->ops->close();
#endif
	return 0;

}
