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

/* Type of distribution */
enum spm_distributor_type {
    SPM_DIST_FFT = 0,
    SPM_DIST_IQ,
    SPM_DIST_MAX,
};

extern int spm_distributor_create(void);
extern int spm_distributor_fft_data_frame_producer(int ch, void **data, size_t len);

#endif
