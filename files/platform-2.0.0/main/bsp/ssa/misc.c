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
*  Rev 1.0   30 July 2021   yaochangsong
*  Initial revision.
******************************************************************************/
#include "config.h"

static uint64_t freq_convert(uint64_t m_freq)
{
    uint64_t tmp_freq;
    if (m_freq >= LVTTL_FREQ_START2 && m_freq < LVTTL_FREQ_end2) {
        tmp_freq = LVTTL_FREQ_BZ2 - m_freq;
    } else if (m_freq >= LVTTL_FREQ_START3 && m_freq <= LVTTL_FREQ_end3) {
        tmp_freq = LVTTL_FREQ_BZ3 - m_freq;
    } else {
        tmp_freq = m_freq;
    }

    return tmp_freq;
}


static  void _ctrl_freq(void *args)
{
    static int old_switch = -1;
    int tmp_switch = 0;
    struct spm_run_parm *r_args;
    r_args = (struct spm_run_parm *)args;
    uint64_t freq_hz = r_args->m_freq_s;
    if (freq_hz >= LVTTL_FREQ_START1 && freq_hz <= LVTTL_FREQ_end1) {
        tmp_switch = 1;
        if (old_switch != tmp_switch) {
            old_switch = tmp_switch;
            printf_info("switch to 2-4G\n");
            SW_TO_2_4_G();
        }
    } else if (freq_hz >= LVTTL_FREQ_START2 && freq_hz < LVTTL_FREQ_end2) {
        tmp_switch = 2;
        if (old_switch != tmp_switch) {
            old_switch = tmp_switch;
            printf_info("switch to 7-7.5G\n");
            SW_TO_7_75_G();
        }
    } else if (freq_hz >= LVTTL_FREQ_START3 && freq_hz <= LVTTL_FREQ_end3) {
        tmp_switch = 3;
        if (old_switch != tmp_switch) {
            old_switch = tmp_switch;
            printf_info("switch to 7.5-9G\n");
            SW_TO_75_9_G();
        }
    } else {
        printf_info("rf unsupported freq:%"PRIu64" hz\n", freq_hz);
    }

}


static const struct misc_ops misc_reg = {
    .freq_ctrl = _ctrl_freq,
};


const struct misc_ops * misc_create_ctx(void)
{
    const struct misc_ops *ctx = calloc(1, sizeof(*ctx));
     if (!ctx)
        return NULL;
     
    ctx = &misc_reg;
    return ctx;
}

