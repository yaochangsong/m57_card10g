#ifndef _RF_TEST_H_
#define _RF_TEST_H_

#include "../rf.h"

#define SPI_DEV_NAME "/dev/spidev2.0"

#ifndef MHZ
#define MHZ(x) ((long long)(x*1000000.0+0.5))
#endif
#ifndef GHZ
#define GHZ(x) ((long long)(x*1000000000.0+0.5))
#endif

extern struct rf_ctx * rf_create_context(void);
extern void set_rf_safe(int rfch, uint32_t *reg, uint32_t val);
extern uint32_t get_rf_safe(int rfch, uint32_t *reg);

#endif
