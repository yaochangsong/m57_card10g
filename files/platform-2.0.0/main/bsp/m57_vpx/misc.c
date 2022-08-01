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

    if(count <= 0){
        r = 1;/* loop read */
    }
    for(int i = 0; i < count; i++){
        if(pctx->ops->send_data_by_fd)
            r += pctx->ops->send_data_by_fd(fd, ptr[i], len[i], NULL);
    }
    if(count > 0){
        if(pctx->ops->read_raw_over_deal)
                pctx->ops->read_raw_over_deal(-1, NULL);
    }
    
    return r;
}

static inline int _align_4byte(int *rc)
{
    int _rc = *rc;
    int reminder = _rc % 4;
    if(reminder != 0){
        _rc += (4 - reminder);
        *rc = _rc;
        return (4 - reminder);
    }
    return 0;
}

static int _assamble_srio_data(uint8_t *buffer,  size_t buffer_len, 
                                      void *data, size_t data_len, uint32_t addr)
{
    static uint32_t tid = 0;
    uint64_t *ll_header;
    struct srio_header{
        uint8_t src_tid;
        uint8_t ttype  : 4;
        uint8_t ftype5: 4;
        uint8_t size_h;
        uint8_t size_l;
        uint32_t addr_h;
    };
    
    uint8_t *ptr = buffer;
    size_t header_len = sizeof(struct srio_header);

    if(ptr == NULL)
        return -1;

    if(header_len + data_len > buffer_len){
        printf_warn("length error: header len[%lu], data len[%lu]\n", header_len, data_len);
        return -1;
    }
    memset(ptr, 0, buffer_len);

    struct srio_header *header;
    uint8_t *pdata;
    header = (struct srio_header *)ptr;
    int _size = data_len -1;

    header->src_tid = tid++;
    header->ftype5 = 5;
    header->ttype= 4;
    header->size_l = (_size & 0xf) << 0x04;
    header->size_h = (_size>>4) & 0xf;
    header->addr_h = addr << 16;
    pdata = ptr + sizeof(*header);
    printfd("_size=%d, size_l=%02x, size_h=%02x\n", _size, header->size_l, header->size_h);
    ll_header = (uint64_t *)header;

    printfd("Befault htonl:\n");

    for(int i = 0; i < header_len; i++){
        printfd("%02x ", *ptr++);
    }
    printfd("\n");
    uint32_t header_h, header_l;
    header_l = *ll_header & 0xffffffff;
    header_h = (*ll_header >> 32) & 0xffffffff;
    *ll_header = htonl(header_l);
    *ll_header = (*ll_header <<32) | htonl(header_h);
    printfd("After htonl:\n");

    ptr = buffer;
    for(int i = 0; i < header_len; i++){
        printfd("%02x ", *ptr++);
    }
    printfd("\n");

    memcpy(pdata, data, data_len);

    return (header_len + data_len);
}

static ssize_t data_downlink_handle(int fd, const void *data, size_t len)
{
    struct spm_context *pctx = get_spm_ctx();
    ssize_t consume_len = 0, remain_len = len;
    uint8_t *buffer = NULL, *ptr = NULL;
    int max_data_len = 248, r = 0, ret = 0;
    int max_pkt_len = 256;
    
    if(!pctx)
        return -1;

    buffer = malloc_align(max_pkt_len);
    ptr = data;
    do{
        consume_len = min(max_data_len, remain_len);
        r = _assamble_srio_data(buffer, max_pkt_len,  ptr, consume_len, 0x06);
        if(r < 0){
            ret = -1;
            break;
        }
        if(r < max_pkt_len){
            _align_4byte(&r);
        }
        if(r > max_pkt_len)
            r = max_pkt_len;

        if(pctx->ops->write_data){
            if(pctx->ops->write_data(-1, buffer, r) < 0){
                ret = -1;
                break;
            }
        }
        
        if(remain_len >= consume_len){
            remain_len -= consume_len;
            ptr += consume_len;
        }
        //printf_note("remain_len:%ld, consume_len:%ld\n", remain_len, consume_len);
    }while(remain_len > 0);
    free(buffer);

    return ret;
}


static int data_pre_handle(int rw, void *args)
{
    struct spm_context *pctx = get_spm_ctx();
    if(!pctx)
        return -1;
    if(rw == MISC_WRITE){
#ifdef SET_SRIO_SRC_DST_ID1
        SET_SRIO_SRC_DST_ID1(get_fpga_reg(), 0x00060007); //SRIO1_ID
#endif
    }
    if(rw == MISC_READ){
        io_set_enable_command(XDMA_MODE_ENABLE, -1, 0, 0);
    }
    return 0;
}

static int data_post_handle(int rw, void *args)
{
    if(rw == MISC_READ){
        io_set_enable_command(XDMA_MODE_DISABLE, -1, 0, 0);
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

