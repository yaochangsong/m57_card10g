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
*  Rev 1.0   30 June. 2022   yaochangsong
*  Initial revision.
******************************************************************************/
#ifndef _SPM_DISTRIBUTOR_H_
#define _SPM_DISTRIBUTOR_H_

#define MAX_FRAME_BUFFER_LEN (65536)
/* Type of distribution */
enum spm_distributor_type {
    SPM_DIST_FFT = 0,
    SPM_DIST_IQ,
    SPM_DIST_MAX,
};

struct spm_distributor_ops {
    int (*init)(void*);
    int (*reset)(int, int);
    int (*fft_prod)(void);
    int (*fft_consume)(int, void **, size_t);
    int (*close)(void *);
    
};

typedef struct spm_distributor_ctx {
    const struct spm_distributor_ops *ops;
}spm_distributor_ctx_t;

extern spm_distributor_ctx_t * spm_create_distributor_ctx(void);
extern int spm_distributor_fft_data_frame_producer(int ch, void **data, size_t len);

#endif
