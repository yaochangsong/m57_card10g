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

/*
    |-------------------|-------|--------------|
    304mhz             384mhz   412.5mhz       464mhz
*/
static fft_t *_spm_order_after(fft_t *ptr, size_t len, void *args, size_t *ordlen)
{
    #define _BINDWIDTH1 MHZ(160)
    #define _BINDWIDTH2 MHZ(20)
    #define _MID_FREQ1 MHZ(384)  // 160MHZ
    #define _START_FREQ MHZ(304) // 384-160/2
    #define _MID_FREQ2 MHZ(412.5)  // 20MHZ
    struct spm_run_parm *run_args = args;
    fft_t *tmp_ptr;
    size_t diff_points = 0, points_10m = 0;
   
    if(run_args->scan_bw != MHZ(20)){
        *ordlen = len;
        return ptr;
    } 
    diff_points = (_MID_FREQ2 - _START_FREQ)/run_args->freq_resolution;
    points_10m = MHZ(10) / run_args->freq_resolution;
    
#if defined(CONFIG_SPM_FFT_EXTRACT_POINT)
    diff_points = (diff_points - points_10m)/2;
    *ordlen = points_10m;  /* X2, 抽取点后，需要除2，x2,÷2抵消*/
#else
    diff_points = diff_points - points_10m;
    *ordlen = points_10m * 2;
#endif
    tmp_ptr = ptr + diff_points;

    return tmp_ptr;
}


static const struct misc_ops misc_reg = {
    .spm_order_after = _spm_order_after,
};


const struct misc_ops * misc_create_ctx(void)
{
    const struct misc_ops *ctx = calloc(1, sizeof(*ctx));
     if (!ctx)
        return NULL;
     
    ctx = &misc_reg;
    return ctx;
}

