#ifndef _ADRV_H_H
#define _ADRV_H_H

#include <stdint.h>
#include <sys/types.h>


extern void adrv_init(void);
extern int16_t *adrv_read_rx_data(ssize_t *rsize);
extern int8_t adrv_set_rx_dc_offset(uint8_t mshift);
extern int8_t adrv_set_rx_bw(uint32_t bw_hz);
extern int16_t adrv_set_rx_freq(uint64_t freq_hz);
extern int8_t adrv_set_rx_gain(uint8_t gain);
extern size_t adrv_get_rx_samples_count(void);

#endif
