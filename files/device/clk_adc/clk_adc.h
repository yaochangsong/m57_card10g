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
*  Rev 1.0   09 June 2020   yaochangsong
*  Initial revision.
******************************************************************************/
#ifndef _CLOCK_ADC_H
#define _CLOCK_ADC_H

struct clock_adc_ops {
    int (*init)(void);
    int (*close)(void);
    
};

struct clock_adc_ctx {
    const struct clock_adc_ops *ops;
};

extern int clock_adc_init(void);
extern int clock_adc_close(void);

#endif

