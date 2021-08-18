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
#include "config.h"

struct register_ops reg;

static bool reg_is_init = false;
struct register_ops *reg_get(void)
{
    if(reg_is_init == false){
        reg_init();
        reg_is_init = true;
    }
    return &reg;
}

struct register_ops * reg_create(void)
{
    struct rf_reg_ops *_rf = NULL;
    struct if_reg_ops *_if = NULL;
    struct misc_reg_ops *_misc = NULL;
    
    struct register_ops *ctx = calloc(1, sizeof(*ctx));
    if (!ctx){
        printf_err("zalloc error!!\n");
        exit(1);
    }

#if defined(CONFIG_RF_FPGA_REGISTER)
    _rf = rf_create_reg_ctx();
#endif
#if defined(CONFIG_IF_FPGA_REGISTER)
    _if = if_create_reg_ctx();
    _misc = misc_create_reg_ctx();
#endif
    ctx->rf = _rf;
    ctx->iif = _if;
    ctx->misc = _misc;
    
    return ctx;
}


int reg_init(void)
{
    struct register_ops *_reg;
    int ret = 0;
    struct rf_reg_ops *_rf = NULL;
    struct if_reg_ops *_if = NULL;
    struct misc_reg_ops *_misc = NULL;

    _reg = &reg;
    
#if defined(CONFIG_RF_FPGA_REGISTER)
    _rf = rf_create_reg_ctx();
#endif
#if defined(CONFIG_IF_FPGA_REGISTER)
    _if = if_create_reg_ctx();
    _misc = misc_create_reg_ctx();
#endif
    _reg->rf = _rf;
    _reg->iif = _if;
    _reg->misc = _misc;
    reg_is_init = true;
    return ret;
}


