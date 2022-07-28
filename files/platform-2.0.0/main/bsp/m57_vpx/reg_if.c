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

static void set_xdma_channel(int ch,  int enable)
{
    FPGA_CONFIG_REG *reg = get_fpga_reg();
    if(!reg)
        return;
    
    uint32_t ch_map = GET_CHANNEL_SEL(reg);
    if(enable){
        ch_map |= (1 << ch);
    } else {
        ch_map &= ~(1 << ch);
    }
   // printf_note("ch:%d, ch_map:0x%x\n", ch, ch_map);
    SET_CHANNEL_SEL(reg, ch_map);
}


static const struct if_reg_ops if_reg = {
    .set_channel = set_xdma_channel,
};


struct if_reg_ops * if_create_reg_ctx(void)
{
    struct if_reg_ops *ctx = calloc(1, sizeof(*ctx));
     if (!ctx)
        return NULL;
     
    ctx = &if_reg;
    return ctx;
}
