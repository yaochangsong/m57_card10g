#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <poll.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/stat.h>
#include <termios.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include "reg_if.h"


static  bool _get_adc_status(void)
{
    FPGA_CONFIG_REG *reg = get_fpga_reg();
    uint32_t _reg = 0;
    #ifdef GET_ADC_STATUS
    _reg = GET_ADC_STATUS(reg);
    #endif
    if ((_reg & 0x03) == 0x03) {
        return true;
    } else {
        return false;
    }
}

static void _set_niq_channel(int ch, int subch, void *args, int enable)
{
    uint32_t _reg;

    FPGA_CONFIG_REG *reg = get_fpga_reg();
    _reg = enable &0x01;
    reg->narrow_band[subch]->enable = _reg;
}


static const struct if_reg_ops if_reg = {
    .get_adc_status = _get_adc_status,
    .set_niq_channel = _set_niq_channel,
};


struct if_reg_ops * if_create_reg_ctx(void)
{
    struct if_reg_ops *ctx = calloc(1, sizeof(*ctx));
     if (!ctx)
        return NULL;
     
    ctx = &if_reg;
    return ctx;
}
