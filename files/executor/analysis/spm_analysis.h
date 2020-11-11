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
*  Rev 1.0   19 Oct. 2020   yaochangsong
*  Initial revision.
******************************************************************************/
#ifndef _SPM_ANALYSIS_H
#define _SPM_ANALYSIS_H

struct spm_pool{
    memory_pool_t *iq;
    memory_pool_t *fft_raw_small;
    memory_pool_t *fft_raw_big;
    memory_pool_t *fft_s_deal;
    memory_pool_t *fft_b_deal;
     volatile bool iq_ready;
};

struct spectrum_analysis_result_vec{
    uint64_t mid_freq_hz; 
    uint32_t bw_hz;
    uint32_t peak_value;
    float level;
};

struct spm_analysis_info{
    uint32_t number;
    pthread_mutex_t mutex;
    struct spectrum_analysis_result_vec result[0];
    
};

#define SPECTRUM_MAX_SCAN_COUNT (50)

extern void spm_analysis_init(void);
extern void spm_analysis_start(int ch, void *data, size_t data_len, void *args);
extern ssize_t spm_analysis_get_info(struct spectrum_analysis_result_vec **real);
#endif