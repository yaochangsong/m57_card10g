#ifndef _SPRCTRUM_H_H
#define _SPRCTRUM_H_H
#include "config.h"

#define MHZ(x) ((long long)(x*1000000.0 + .5))

#if (RF_ADRV9009_IIO == 1)
#define specturm_rx0_read_data iio_read_rx0_data
#else
#define specturm_rx0_read_data
#endif

#define fft_spectrum_iq_to_fft_handle fft_fftw_calculate_hann

#if (RF_ADRV9009_IIO == 1)
#define RF_BANDWIDTH  RF_ADRV9009_BANDWITH
#else
#define RF_BANDWIDTH  MHZ(20)
#endif

#define SPECTRUM_START_FLAG 0x7E7E
#define SPECTRUM_DEFAULT_FFT_SIZE (512*1024)
#define SIDE_BAND_RATE  (1.2288) 
#define SINGLE_SIDE_BAND_POINT_RATE  (0.093098958333333)  /* (1-1/1.2288)/2 */


#define calc_resolution(bw_hz, fft_size)  (SIDE_BAND_RATE*bw_hz/fft_size)

#define delta_bw  MHZ(30)
#define middle_freq_resetting(bw, mfreq)    ((bw/2) >= (mfreq) ? (bw/2+delta_bw) : (mfreq))

#define KU_FREQUENCY_START   10700000000 //10.7G
#define KU_FREQUENCY_END     12750000000 //12.75G
#define KU_FREQUENCY_OFFSET  9750000000  //9.75G 


typedef int16_t fft_data_type;

struct spectrum_st{
    long long freq_hz; 
    long long bw_hz;
    uint32_t level;
    int16_t *iq_payload;   /* IQ data */
    uint32_t iq_len;
    float *fft_float_payload;         /* FFT float data */
    fft_data_type fft_short_payload[10*1024];       /* FFT short data */
    //fft_data_type *fft_short_payload_back;  /* FFT short data back */
    uint32_t fft_len;
    //uint32_t fft_len_back;
    struct spectrum_header_param param;
    volatile bool fft_data_ready;
};

struct spectrum_fft_result_st{
    uint32_t result_num;
    long long peak_value;
    long long mid_freq_hz[SIGNALNUM]; 
    long long bw_hz[SIGNALNUM];
    float level[SIGNALNUM];
};

#define LOCK_SP_RESULT() do { \
    printf_debug("lock sp result\n");\
    pthread_mutex_lock(&spectrum_result_mutex); \
} while (0)

#define UNLOCK_SP_RESULT() do { \
    printf_debug("unlock sp result\n");\
    pthread_mutex_unlock(&spectrum_result_mutex); \
} while (0)

#define LOCK_SP_DATA() do { \
    pthread_mutex_lock(&spectrum_data_mutex); \
} while (0)

#define UNLOCK_SP_DATA() do { \
    pthread_mutex_unlock(&spectrum_data_mutex); \
} while (0)

#define LOCK_IQ_DATA() do { \
    pthread_mutex_lock(&spectrum_iq_data_mutex); \
} while (0)

#define UNLOCK_IQ_DATA() do { \
    pthread_mutex_unlock(&spectrum_iq_data_mutex); \
} while (0)

extern bool specturm_work_write_enable(bool enable);
extern void spectrum_init(void);
extern void *spectrum_rw_fft_result(fft_result *result, uint64_t s_freq_hz, float freq_resolution, uint32_t fft_size);
extern int16_t *spectrum_get_fft_data(uint32_t *len);
extern int32_t spectrum_send_fft_data_interval(void);
extern void spectrum_psd_user_deal(struct spectrum_header_param *param);
extern void spectrum_analysis_user_deal(struct spectrum_header_param *param);
#endif
