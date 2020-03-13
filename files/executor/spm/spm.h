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

typedef float fft_t;
typedef int16_t iq_t;

enum stream_type {
    STREAM_IQ = 0,
    STREAM_FFT,
    STREAM_ADC,
    STREAM_NUM,
};


struct spm_backend_ops {
    int (*create)(void);
    ssize_t (*read_iq_data)(void **);
    ssize_t (*read_fft_data)(void **);
    fft_t *(*data_order)(fft_t *, size_t,  size_t *, void *);
    int (*send_fft_data)(void *, size_t, void *);
    int (*send_iq_data)(void *, size_t, void *);
    int (*send_cmd)(void *, void *, size_t, void *);
    int (*agc_ctrl)(int, void *);
    int (*convet_iq_to_fft)(void *, void *, size_t);
    int (*set_psd_analysis_enable)(bool);
    int (*get_psd_analysis_result)(void *);
    int (*save_data)(void *, size_t);
    int (*backtrace_data)(void *, size_t);
    int (*stream_start)(uint32_t ,uint8_t , int);
    int (*stream_stop)(uint8_t);
    int (*close)(void);
    
};



struct spm_context {
    struct poal_config *pdata;
    const struct spm_backend_ops *ops;
    const void *run_args;
};

extern void *spm_init(void);
extern struct spm_context *get_spm_ctx(void);
extern void spm_deal(struct spm_context *ctx, void *args);
extern int spm_close(void);

#endif
