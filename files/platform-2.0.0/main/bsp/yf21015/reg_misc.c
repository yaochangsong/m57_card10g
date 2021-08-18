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
#include "reg_misc.h"


/*
    @back:1 playback  0: normal
*/
static  void _set_ssd_mode(int ch,int back)
{
    FPGA_CONFIG_REG *reg = get_fpga_reg();
    uint32_t _reg = 0;
#if 0
    if(ch >= 0)
        _reg = ((back &0x01) << ch);
    else
        _reg = back &0x01;
#else
    if(back)
        _reg = 0x1;
    else
        _reg = 0x0;
#endif
    reg->system->ssd_mode = _reg;
}

static void _set_iq_time_mark(int ch, bool is_mark)
{
    FPGA_CONFIG_REG *reg = get_fpga_reg();
    /* bit16为时标使能位 */
    ch = ch;
    if(is_mark)
        reg->signal->trig = 0x10000;
    else
        reg->signal->trig = 0x00000;
}


static const struct misc_reg_ops misc_reg = {
    .set_ssd_mode = _set_ssd_mode,
    .set_iq_time_mark = _set_iq_time_mark,
};


struct misc_reg_ops * misc_create_reg_ctx(void)
{
    struct misc_reg_ops *ctx = calloc(1, sizeof(*ctx));
     if (!ctx)
        return NULL;
     
    ctx = &misc_reg;
    return ctx;
}
