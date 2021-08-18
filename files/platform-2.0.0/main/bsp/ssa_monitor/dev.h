/******************************************************************************
*  Copyright 2021, Showay Technology Dev Co.,Ltd.
*  ---------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Showay Technology Dev Co.,Ltd. (C) 2019
******************************************************************************/
/*****************************************************************************     
*  Rev 1.0   2 June 2021   ycs
*  Initial revision.
******************************************************************************/

#ifndef _SPM_DEV_H
#define _SPM_DEV_H

#include "config.h"


enum gpio_func_code_ex{
    CONDI_GPIO_0     = 0x00,
    CONDI_GPIO_1     = 0x01,
    CONDI_GPIO_2     = 0x02,
    CONDI_GPIO_3     = 0x03,
    CONDI_GPIO_4     = 0x04,
    CONDI_GPIO_5     = 0x05,
    CONDI_GPIO_6     = 0x06,
    CONDI_GPIO_7     = 0x07,
    CONDI_GPIO_8     = 0x08,
    CONDI_GPIO_9     = 0x09,
    CONDI_GPIO_10     = 0x0a,
    CONDI_GPIO_11     = 0x0b,
    CONDI_GPIO_12     = 0x0c,
    CONDI_GPIO_13     = 0x0d,
    CONDI_GPIO_14     = 0x0e,
    CONDI_GPIO_15     = 0x0f,
    CONDI_GPIO_16     = 0x10,
    CONDI_GPIO_17     = 0x11,
    CONDI_GPIO_18     = 0x12,
    CONDI_GPIO_19     = 0x13,
    CONDI_GPIO_20     = 0x14,
    CONDI_GPIO_21     = 0x15,
    CONDI_GPIO_22     = 0x16,
    CONDI_GPIO_23     = 0x17,
};

#define GPIO_SET_FREQ_1(array) do{      \
        gpio_raw_write_value(CONDI_GPIO_20, array[0]);           \
        gpio_raw_write_value(CONDI_GPIO_19, array[1]);           \
        gpio_raw_write_value(CONDI_GPIO_12, array[2]);           \
        gpio_raw_write_value(CONDI_GPIO_13, array[3]);           \
        gpio_raw_write_value(CONDI_GPIO_16, array[4]);           \
        gpio_raw_write_value(CONDI_GPIO_17, array[5]);           \
        gpio_raw_write_value(CONDI_GPIO_23, array[6]);           \
        gpio_raw_write_value(CONDI_GPIO_18, array[7]);           \
    }while(0);

#define GPIO_SET_FREQ_2(array) do{      \
        gpio_raw_write_value(CONDI_GPIO_20, array[0]);           \
        gpio_raw_write_value(CONDI_GPIO_19, array[1]);           \
        gpio_raw_write_value(CONDI_GPIO_14, array[2]);           \
        gpio_raw_write_value(CONDI_GPIO_15, array[3]);           \
        gpio_raw_write_value(CONDI_GPIO_22, array[4]);           \
        gpio_raw_write_value(CONDI_GPIO_21, array[5]);           \
        gpio_raw_write_value(CONDI_GPIO_23, array[6]);           \
        gpio_raw_write_value(CONDI_GPIO_18, array[7]);           \
    }while(0);


#define GPIO_SET_ATTENUATION_1(array) do{      \
        gpio_raw_write_value(CONDI_GPIO_0, array[0]);           \
        gpio_raw_write_value(CONDI_GPIO_1, array[1]);           \
        gpio_raw_write_value(CONDI_GPIO_2, array[2]);           \
        gpio_raw_write_value(CONDI_GPIO_3, array[3]);           \
        gpio_raw_write_value(CONDI_GPIO_4, array[4]);           \
        gpio_raw_write_value(CONDI_GPIO_5, array[5]);           \
    }while(0);

#define GPIO_SET_ATTENUATION_2(array) do{      \
        gpio_raw_write_value(CONDI_GPIO_6, array[0]);           \
        gpio_raw_write_value(CONDI_GPIO_7, array[1]);           \
        gpio_raw_write_value(CONDI_GPIO_8, array[2]);           \
        gpio_raw_write_value(CONDI_GPIO_9, array[3]);           \
        gpio_raw_write_value(CONDI_GPIO_10, array[4]);          \
        gpio_raw_write_value(CONDI_GPIO_11, array[5]);          \
    }while(0);

    
#define SW_TO_2_4_G()    gpio_raw_write_value(CONDI_GPIO_16, 1)
#define SW_TO_7_75_G()   do{ \
            gpio_raw_write_value(CONDI_GPIO_16, 0); \
            gpio_raw_write_value(CONDI_GPIO_12, 1); \
        }while(0)
    
#define SW_TO_75_9_G()   do{ \
            gpio_raw_write_value(CONDI_GPIO_16, 0); \
            gpio_raw_write_value(CONDI_GPIO_12, 0); \
        }while(0)


extern struct _spm_stream* spm_dev_get_stream(int *count);
extern struct gpio_node_info* dev_get_gpio(int *count);

#endif
