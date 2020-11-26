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
#ifndef _SPM_CHIP_H


#ifdef SUPPORT_RF_ADRV
#define specturm_rx0_read_data adrv_read_rx_data
#else
#define specturm_rx0_read_data
#endif

#ifdef SUPPORT_SPECTRUM_FFT
#define fft_spectrum_iq_to_fft_handle fft_fftw_calculate_hann//fft_fftw_calculate_hann_addsmooth
#define fft_spectrum_iq_to_fft_handle_dup fft_fftw_calculate_hann_addsmooth_ex
#define fft_spectrum_fftdata_handle   fft_fftdata_handle
#define fft_spectrum_get_result       fft_get_result
#else
#define fft_spectrum_iq_to_fft_handle 
#define fft_spectrum_iq_to_fft_handle_dup 
#define fft_spectrum_fftdata_handle   
#define fft_spectrum_get_result       
#endif

#ifdef SUPPORT_SPECTRUM_SCAN_SEGMENT
#ifndef RF_ONE_CHANNEL_NUM
#define RF_ONE_CHANNEL_NUM 1            /* 一个通道射频数 */
#endif

#ifndef RF_DIVISION_FREQ_HZ
    #ifdef SUPPORT_PROJECT_SSA_MONITOR
    #define RF_DIVISION_FREQ_HZ   GHZ(7.5) 
    #endif  
#error "Not defined RF_DIVISION_FREQ_HZ!!"
#endif  /* RF_DIVISION_FREQ_HZ */
#endif  /*SUPPORT_SPECTRUM_SCAN_SEGMENT */

extern struct spm_context * spm_create_chip_context(void);

#endif
