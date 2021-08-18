#ifndef CAD9361_H
#define CAD9361_H
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <iio.h>

/* helper macros */
#ifndef MHZ
#define MHZ(x) ((long long)(x*1000000.0+0.5))
#endif
#ifndef GHZ
#define GHZ(x) ((long long)(x*1000000000.0+0.5))
#endif


void iio_config_rx_freq(unsigned long long hz);
void iio_config_rx_bw(unsigned long long hz);

int ad9361_init(void);

#endif
