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

struct spm_backend_ops {
    int (*create)(void);
    ssize_t (*read_iq_data)(void **);
    ssize_t (*read_fft_data)(void **);
    int (*data_order)(void *);
    fft_data_type *(*spm_data_order)(fft_data_type *fft_data, size_t fft_len, size_t *order_fft_len,void *arg)
    int (*send_fft_data)(void *data, size_t len, void *);
    int (*send_iq_data)(void *data, size_t len, void *);
    int (*send_cmd)(void *cmd, void *data, size_t len, void *);
    int (*agc_ctrl)(int ch, void *arg);
    int (*convet_iq_to_fft)(void *ptr_iq, void *ptr_fft, size_t fft_len);
    int (*set_psd_analysis_enable)(bool);
    int (*get_psd_analysis_result)(void *);
    int (*save_data)(*void);
    int (*backtrace_data)(void);
    int (*close)(void);
    
};

struct spm_context {
    struct poal_config *pdata;
    const struct spm_backend_ops *ops;
};



#endif
