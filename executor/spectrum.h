#ifndef _SPRCTRUM_H_H
#define _SPRCTRUM_H_H
#include "fft.h"

#define specturm_rx0_read_data iio_read_rx0_data

#define SPECTRUM_START_FLAG 0x7E7E

#define calc_resolution(bw_hz, fft_size)  (bw_hz/fft_size)

struct spectrum_st{
    long long freq_hz; 
    long long bw_hz;
    uint32_t level;
    int16_t *iq_payload;   /* IQ data */
    uint32_t iq_len;
    float *fft_float_payload;         /* FFT float data */
    int16_t *fft_short_payload;       /* FFT short data */
    int16_t *fft_short_payload_back;  /* FFT short data back */
    uint32_t fft_len;
    uint32_t fft_len_back;
    volatile bool is_wait_deal;
};

struct spectrum_fft_result_st{
    uint32_t result_num;
    long long mid_freq_hz[SIGNALNUM]; 
    long long bw_hz[SIGNALNUM];
    uint32_t level[SIGNALNUM];
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

extern bool specturm_work_write_enable(bool enable);
extern void spectrum_init(void);
extern void spectrum_wait_user_deal( struct spectrum_header_param *param);
extern void *spectrum_rw_fft_result(fft_result *result, uint64_t s_freq_hz, float freq_resolution, uint32_t fft_size);
extern int16_t *spectrum_get_fft_data(uint32_t *len);
#endif