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
#ifndef _SPM_H_
#define _SPM_H_

typedef int16_t fft_t;
typedef int16_t iq_t;

#define MAX_FFT_SIZE  (64768)

enum stream_type {
    STREAM_IQ = 0,
    STREAM_FFT,
    STREAM_ADC_WRITE,
    STREAM_ADC_READ,
};


struct spm_backend_ops {
    int (*create)(void);
    ssize_t (*read_iq_data)(void **);
    ssize_t (*read_fft_data)(void **);
    ssize_t (*read_adc_data)(void **);
    int (*read_adc_over_deal)(void *);
    int (*read_iq_over_deal)(void *);
    fft_t *(*data_order)(fft_t *, size_t,  size_t *, void *);
    int (*send_fft_data)(void *, size_t, void *);
    int (*send_iq_data)(void *, size_t, void *);
    int (*send_cmd)(void *, void *, size_t, void *);
    /* AGC自动增益控制*/
    int (*agc_ctrl)(int, void *);
    /* 获取驻留时间是否到达：根据通道驻留策略和是否有信号 */
    bool (*residency_time_arrived)(uint8_t, int, bool);
    /* 获取某通道是否有信号；并返回信号强度 */
    int32_t (*signal_strength)(uint8_t ch,uint8_t subch, uint32_t, bool *is_singal, uint16_t *strength);
    int (*convet_iq_to_fft)(void *, void *, size_t);
    int (*set_psd_analysis_enable)(bool);
    int (*get_psd_analysis_result)(void *);
    int (*save_data)(void *, size_t);
    int (*backtrace_data)(void *, size_t);
    int (*back_running_file)(uint8_t, char *);
    int (*stream_start)(uint32_t ,uint8_t , int);
    int (*stream_stop)(uint8_t);
    int (*sample_ctrl)(uint64_t);
    int (*spm_scan)(uint64_t *, uint64_t* , uint32_t* , uint32_t *, uint64_t *);
    int (*close)(void *);
    
};


#include "config.h"
struct spm_context {
    struct poal_config *pdata;
    const struct spm_backend_ops *ops;
    struct spm_run_parm *run_args[MAX_RADIO_CHANNEL_NUM];
};

extern void *spm_init(void);
extern struct spm_context *get_spm_ctx(void);
extern void spm_deal(struct spm_context *ctx, void *args);
extern int spm_close(void);

#endif
