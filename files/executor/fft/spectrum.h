#ifndef _SPRCTRUM_H_H
#define _SPRCTRUM_H_H
#include "config.h"

#ifdef SUPPORT_RF_ADRV9009
#define specturm_rx0_read_data adrv9009_iio_read_rx0_data
#define RF_BANDWIDTH  RF_ADRV9009_BANDWITH
#elif defined (SUPPORT_RF_ADRV9361)
#define specturm_rx0_read_data adrv9361_iio_read_rx0_data
#define RF_BANDWIDTH  RF_ADRV9361_BANDWITH
#else
#define specturm_rx0_read_data
#define RF_BANDWIDTH  MHZ(20)
#endif

#define fft_spectrum_iq_to_fft_handle fft_fftw_calculate_hann
#define fft_spectrum_fftdata_handle   fft_fftdata_handle
#define fft_spectrum_get_result       fft_get_result



#define SPECTRUM_START_FLAG 0x7E7E
#define SPECTRUM_DEFAULT_FFT_SIZE (512*1024)
#define SPECTRUM_MAX_SCAN_COUNT (50)

#define SIDE_BAND_RATE_1_28  (1.28)
#define SIDE_BAND_RATE_1_2228  (1.2288)
#define SINGLE_SIDE_BAND_POINT_RATE_1_28  (0.109375)                  /* (1-1/1.28)/2 */
#define SINGLE_SIDE_BAND_POINT_RATE_1_2228  (0.093098958333333333335) /* (1-1/1.2288)/2 */

#ifdef SUPPORT_RF_ADRV9009
#define DEFAULT_SIDE_BAND_RATE  SIDE_BAND_RATE_1_28
#define DEFAULT_SINGLE_SIDE_BAND_POINT_RATE  SINGLE_SIDE_BAND_POINT_RATE_1_28  
#define  SPECTRUM_IQ_SIZE   RF_ADRV9009_IQ_SIZE
#elif defined (SUPPORT_RF_ADRV9361)
#define DEFAULT_SIDE_BAND_RATE  SIDE_BAND_RATE_1_28
#define DEFAULT_SINGLE_SIDE_BAND_POINT_RATE  SINGLE_SIDE_BAND_POINT_RATE_1_28
#define  SPECTRUM_IQ_SIZE   RF_ADRV9361_IQ_SIZE
#else
#define DEFAULT_SIDE_BAND_RATE  SIDE_BAND_RATE_1_28
#define DEFAULT_SINGLE_SIDE_BAND_POINT_RATE  SINGLE_SIDE_BAND_POINT_RATE_1_28
#define  SPECTRUM_IQ_SIZE   (1024*1024)

#endif



//#define get_side_band_rate() (1.28)
//#define get_single_side_band_pointrate() (0.109375)


#define  SPECTRUM_SMALL_FFT_SIZE   (8192)
#define  SPECTRUM_BIG_FFT_SIZE  SPECTRUM_IQ_SIZE/2

//#define calc_resolution(bw_hz, fft_size)  (SIDE_BAND_RATE*bw_hz/fft_size)

#define KU_FREQUENCY_START   10700000000 //10.7G
#define KU_FREQUENCY_END     12750000000 //12.75G
#define KU_FREQUENCY_OFFSET  9750000000  //9.75G 


typedef int16_t fft_data_type;

struct sp_sem_nofity_st{
    sem_t   notify_iq_to_fft;
};
struct spectrum_st{
    long long freq_hz; 
    long long bw_hz;
    uint32_t level;
    int16_t *iq_payload;   /* IQ data */
    uint32_t iq_len;
    float *fft_float_payload;         /* FFT float data */
    fft_data_type fft_short_payload[10*1024];       /* FFT short data */
    uint32_t fft_len;
    struct spectrum_header_param param;
    volatile bool fft_data_ready;
    volatile bool iq_data_ready;
};

struct spectrum_fft_result_st{
    uint32_t result_num;
    long long peak_value;
    long long mid_freq_hz[SIGNALNUM]; 
    long long bw_hz[SIGNALNUM];
    float level[SIGNALNUM];
};

#define LOCK_SP_RESULT() do { \
    pthread_mutex_lock(&spectrum_result_mutex); \
} while (0)

#define UNLOCK_SP_RESULT() do { \
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

extern void spectrum_init(void);
extern void *spectrum_rw_fft_result(fft_result *result, uint64_t s_freq_hz, float freq_resolution, uint32_t fft_size);
extern int32_t spectrum_send_fft_data_interval(void);
extern void spectrum_psd_user_deal(struct spectrum_header_param *param);
#endif
