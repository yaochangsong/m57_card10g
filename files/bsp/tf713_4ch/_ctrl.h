/******************************************************************************
*  Copyright 2020, Showay Technology Dev Co.,Ltd.
*  ---------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Showay Technology Dev Co.,Ltd. (C) 2019
******************************************************************************/
/*****************************************************************************     
*  Rev 1.0   8 Dec 2020    yaochangsong
*  Initial revision.
******************************************************************************/
#ifndef _CTRL_H
#define _CTRL_H

#include "platform.h"
static inline  void _ctrl_freq(void *args)
{
#if defined(SUPPORT_DIRECT_SAMPLE)
    struct spm_run_parm *r_args;
    r_args = (struct spm_run_parm *)args;
    uint64_t freq_hz = r_args->m_freq_s;
    uint8_t val = 0;
    static int8_t val_dup = -1;
    #define _200MHZ 200000000
    if(freq_hz < _200MHZ){
        val = 1;    /* 直采开启 */
    }else{
        val = 0;    /* 直采关闭 */
    }
    if(val_dup == val){
        return 0;
    }else{
        val_dup = val;
    }
    printf_note("samle ctrl:freq_hz =%lluHz, direct %d\n", freq_hz, val);
    executor_set_command(EX_MID_FREQ_CMD, EX_SAMPLE_CTRL,    0, &val);
    executor_set_command(EX_RF_FREQ_CMD,  EX_RF_SAMPLE_CTRL, 0, &val);
#endif
}

static inline int get_rf_status_code(bool is_ok)
{
    if(is_ok)
        return 5;
    else
        return 4;
}

static inline int get_adc_status_code(bool is_ok)
{
    if(is_ok)
        return 5;
    else
        return 4;
}

static inline int get_gps_status_code(bool is_ok)
{
    if(is_ok)
        return 7;
    else
        return 4;
}

static inline int get_gps_disk_code(bool is_ok,  void *args)
{
    args = args;
    if(is_ok)
        return 5;
    else
        return 4;
}

static inline int get_clock_frequency(void)
{
    /* 153.6Mhz */
    return 153600000;
}



#endif
