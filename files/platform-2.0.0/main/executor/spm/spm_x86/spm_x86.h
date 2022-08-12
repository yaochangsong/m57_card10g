#ifndef _SPM_X86_H_H
#define _SPM_X86_H_H

#ifdef CONFIG_SPM_FFT_IQ_CONVERT
#define fft_spectrum_iq_to_fft_handle fft_fftw_calculate_hann
#define fft_spectrum_iq_to_fft_handle_dup fft_fftw_calculate_hann_addsmooth_ex
#define fft_spectrum_fftdata_handle   fft_fftdata_handle
#define fft_spectrum_get_result       fft_get_result
#else
#define fft_spectrum_iq_to_fft_handle(i,j,k,l)
#define fft_spectrum_iq_to_fft_handle_dup
#define fft_spectrum_fftdata_handle    
#define fft_spectrum_get_result
#endif
extern struct spm_context * spm_create_context(void);
#endif