#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <poll.h>
#include "reg_rf.h"


static const struct rf_reg_ops rf_reg = {

};


struct rf_reg_ops * rf_create_reg_ctx(void)
{
    struct rf_reg_ops *ctx = calloc(1, sizeof(*ctx));
     if (!ctx)
        return NULL;
     
    ctx = &rf_reg;
    return ctx;
}

