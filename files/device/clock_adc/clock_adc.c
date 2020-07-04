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
#include "clock_adc.h"
#include "clock_adc_spi.h"
#include "clock_adc_fpga.h"


static struct clock_adc_ctx *_ca_ctx = NULL;


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
    _ca_ctx->ops->init();
#endif
}

int clock_adc_close(void)
{
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
    if(_ca_ctx)
         _ca_ctx->ops->close();
#endif
}
