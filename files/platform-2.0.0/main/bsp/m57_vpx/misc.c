/******************************************************************************
*  Copyright 2021, Showay Technology Dev Co.,Ltd.
*  ---------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Showay Technology Dev Co.,Ltd. (C) 2019
******************************************************************************/
/*****************************************************************************     
*  Rev 1.0   30 July 2021   yaochangsong
*  Initial revision.
******************************************************************************/
#include "config.h"

static ssize_t data_uplink_handle(int fd, void *args)
{
     struct spm_context *pctx = get_spm_ctx();
    if(!pctx)
        return -1;

    ssize_t count = 0, r = 0;
    volatile uint8_t *ptr[2048] = {NULL};
    size_t len[2048] = {0};
    
    if(pctx->ops->read_raw_vec_data)
        count = pctx->ops->read_raw_vec_data(-1, (void **)ptr, len, NULL);
    for(int i = 0; i < count; i++){
        if(pctx->ops->send_data_by_fd)
            r += pctx->ops->send_data_by_fd(fd, ptr[i], len[i], NULL);
    }

    if(pctx->ops->read_raw_over_deal)
        pctx->ops->read_raw_over_deal(-1, NULL);
    return r;
}

static ssize_t data_downlink_handle(int fd, const void *data, size_t len)
{
    struct spm_context *pctx = get_spm_ctx();
    ssize_t w = 0;
    
    if(!pctx)
        return -1;

    if(pctx->ops->write_raw_data)
        w = pctx->ops->write_raw_data(-1, data, len, fd);

    return w;
}

static int data_pre_handle(int rw, void *args)
{
    struct spm_context *pctx = get_spm_ctx();
    if(!pctx)
        return -1;

    if(rw == MISC_WRITE){
        static int do_count = 0;
        io_set_enable_command(XDMA_MODE_ENABLE, 0, 0, 0);
    } else {
        io_set_enable_command(XDMA_MODE_ENABLE, 1, 0, 0);
    }
    return 0;
}

static int data_post_handle(int rw, void *args)
{
    if(rw == MISC_WRITE){
        io_set_enable_command(XDMA_MODE_DISABLE, 0, 0, 0);
    } else {
        io_set_enable_command(XDMA_MODE_DISABLE, 1, 0, 0);
    }
    return 0;
}


static const struct misc_ops misc_reg = {
    .post_handle = data_post_handle,
    .pre_handle = data_pre_handle,
    .write_handle = data_downlink_handle,
    .read_handle = data_uplink_handle,
};

const struct misc_ops * misc_create_ctx(void)
{
    const struct misc_ops *ctx = calloc(1, sizeof(*ctx));
     if (!ctx)
        return NULL;
     
    ctx = &misc_reg;
    return ctx;
}

