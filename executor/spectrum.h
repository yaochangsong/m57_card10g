#ifndef _SPRCTRUM_H_H
#define _SPRCTRUM_H_H

#define specturm_rx_read_data iio_read_rx_data

struct spectrum_t{
    long long freq_hz; 
    long long bw_hz;
    uint32_t level;
    int16_t *payload;   /* IQ data */
    volatile bool is_deal;
};

extern int8_t specturm_rx_work_deal(void);
extern bool specturm_work_write_enable(bool enable);
extern void spectrum_init(void);
#endif