#ifndef _SPRCTRUM_H_H
#define _SPRCTRUM_H_H

#define specturm_rx_read_data iio_read_rx_data

#define SPECTRUM_START_FLAG 0x7E7E

struct spectrum_t{
    long long freq_hz; 
    long long bw_hz;
    uint32_t level;
    int16_t *iq_payload;   /* IQ data */
    uint32_t iq_len;
    float *fft_payload;   /* FFT data */
    uint32_t fft_len;
    volatile bool is_deal;
};

extern bool specturm_work_write_enable(bool enable);
extern void spectrum_init(void);
extern void spectrum_wait_user_deal( struct spectrum_header_param *param);
#endif