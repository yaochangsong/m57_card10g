#ifndef _ADRV_H_H
#define _ADRV_H_H

#include <stdint.h>
#include <sys/types.h>
#include "../rf.h"


extern int16_t *adrv_read_rx_data(ssize_t *rsize);
extern int8_t adrv_set_rx_dc_offset(uint8_t mshift);
extern size_t adrv_get_rx_samples_count(void);
extern struct rf_ctx * rf_create_context(void);


#endif
