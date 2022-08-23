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
#include "bsp.h"
#include "misc.h"
#include "../main/utils/queue.h"
#include "../main/utils/lqueue.h"
#include "../main/utils/thread.h"


#define DIST_RAW_XDMA 0 

typedef struct _distributor_buffer_s{
    int len;
    void *ptr;
    void *tail;
}_distributor_buffer_t;
struct _distributor_type_attr_s{
    char *name;
    /* The max number of distributor consume thread */
    int idx_num;
    int type;
    int pkt_len;
    int pkt_header_len;
    uint16_t pkt_header_flags;
    int (*distributor_loop)(void *);
    void *pkt_queue[8];
    spm_dist_statistics_t stat;
    _distributor_buffer_t buffer;
    int dst_board;
    pthread_t tid;
} xdma_dist_type[] = {
    {"xdma", 4, DIST_RAW_XDMA, 264, 8, 0x5157, NULL},
};

struct _distributor_pkt_data_s{
    int idx;
    int type;
    void *pkt_buffer_ptr;
    size_t pkt_len;
};

struct _data_frame_pkt_s_{
    uint16_t header_flags;
    uint8_t sid;
    uint8_t  len;
    uint16_t rev;
    uint8_t type;
};

struct _data_frame_pkt_s{
    uint8_t rev2;
    uint8_t type;
    uint16_t rev;
    uint8_t  len;
    uint8_t sid;
    uint16_t header_flags;
};


static void _gettime(struct timeval *tv)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    tv->tv_sec = ts.tv_sec;
    tv->tv_usec = ts.tv_nsec / 1000;
}

static int _tv_diff(struct timeval *t1, struct timeval *t2)
{
    return
        (t1->tv_sec - t2->tv_sec) * 1000 +
        (t1->tv_usec - t2->tv_usec) / 1000;
}


static ssize_t _find_header(uint8_t *ptr, uint16_t header, size_t len)
{
    size_t offset = 0;
    do{
        if(ptr != NULL && *(uint16_t *)ptr != header){
            ptr += 1;
            offset += 1;
        }else{
            break;
        }
    }while(offset < len);

    if(offset >= len || ptr == NULL)
        return -1;

    if(offset >= sizeof(struct _data_frame_pkt_s)-2){
        return (offset - (sizeof(struct _data_frame_pkt_s) -2));
    } else{
        printf_note("offset too short\n");
    }

    return offset;
}

static int _distributor_reset(int ch)
{
    if(ch >= xdma_dist_type[0].idx_num)
        return -1;

     queue_ctx_t *qctx;
    if(ch < 0){
        for(int c = 0; c < xdma_dist_type[0].idx_num; c++){
            qctx =  xdma_dist_type[0].pkt_queue[c];
            if(qctx && qctx->ops->clear)
                qctx->ops->clear(qctx);
        }
    } else{
            qctx =  xdma_dist_type[0].pkt_queue[ch];
            if(qctx && qctx->ops->clear)
                qctx->ops->clear(qctx);
    }

    return 0;
}

static void print_array(uint8_t *ptr, ssize_t len)
{
    if(ptr == NULL || len <= 0)
        return;
    
    for(int i = 0; i< len; i++){
        if(i % 16 == 0 && i != 0)
            printf("\n");
        printf("%02x ", *ptr++);
    }
    printf("\n----------------------\n");
}


static int _get_idx_by_type(int type)
{
    if(type == 0x00)
        return 0;
    if(type == 0x01)
        return 1;
    if(type == 0x07)
        return 2;
    if(type == 0x06)
        return 0;
    return -1;
}

static int _get_type_by_idx(int idx)
{
    if(idx == 0)
        return 0;
    if(idx == 1)
        return 1;
    if(idx == 2)
        return 7;
    if(idx == 3)
        return 6;
    return -1;
}


ssize_t _distributor_analysis(int type, uint8_t *mbufs, size_t len, struct _distributor_type_attr_s *dtype)
{
    uint8_t *ptr = mbufs;
    ssize_t data_len = len;
    ssize_t dirty_offset = 0, consume_len = 0;
    uint64_t fidx = 0;
    struct _data_frame_pkt_s *header = NULL;
    int idx = 0;

    do{
        header= (struct _data_frame_pkt_s *)ptr;
        if(!header)
            break;

        if(data_len < dtype->pkt_len){
            if(data_len == 0){
                break;
            }
            //printf_note("data len too short: %ld\n", data_len);
            dirty_offset = _find_header(ptr, dtype->pkt_header_flags, data_len);
            if(dirty_offset < 0){
                ptr += data_len;
                consume_len += data_len;
                data_len = 0;
                printf_debug("not find header in remain data len:%ld\n", data_len);
                printf_note("Maybe read over, read len:%ld\n", dtype->stat.read_bytes + consume_len);
            } else {
                printf_debug("find header,offset:%ld,consume_len:0x%lx\n", dirty_offset, consume_len);
                //print_array(ptr, 16);
                ptr += dirty_offset;
                data_len -= dirty_offset;
                consume_len += dirty_offset;
            }
            break;
        }

        if(header->header_flags != dtype->pkt_header_flags){
            printf_info("find err header,  offset: 0x%lx\n", consume_len+dtype->stat.read_bytes);
            dirty_offset = _find_header(ptr, dtype->pkt_header_flags, dtype->pkt_len);
            //printf_note("fft header[0x%x] error, offset len:%ld, consume_len:0x%lx\n", header->header_flags, dirty_offset,consume_len);
            if(dirty_offset >= 0){
               // print_array(ptr, dirty_offset);
                ptr += dirty_offset;
                data_len -= dirty_offset;
                consume_len += dirty_offset;
                printf_debug("find header, offset:%ld, consume_len:%ld[0x%lx], 0x%lx\n", dirty_offset, consume_len, consume_len, dtype->stat.read_bytes);
                continue;
            }else{
                /* find faild */
                printf_warn("fft header faild\n");
                ptr += dtype->pkt_len;
                data_len -= dtype->pkt_len;
                consume_len += dtype->pkt_len;
                continue;
            }
        }
        
        idx = _get_idx_by_type(header->type);
        //printf_note("idx:%d, header type: 0x%x\n", idx, header->type);
        if(idx == -1 || idx >= dtype->idx_num){
            printf_warn("Unkown type: %x\n", header->type);
            ptr += dtype->pkt_len;
            data_len -= dtype->pkt_len;
            consume_len += dtype->pkt_len;
            continue;
        }
        
        #if 0
        if(header->len != dtype->pkt_len){
            printf_warn("pkt_len[%d] error[%d]\n", header->len, dtype->pkt_len);
            ptr += dtype->pkt_len;
            data_len -= dtype->pkt_len;
            consume_len += dtype->pkt_len;
            continue;
        }
        #endif

        struct _distributor_pkt_data_s *pkt;
        pkt = calloc(1, sizeof(*pkt));
        if(!pkt){
            break;
        }
        pkt->idx = idx;
        pkt->type = header->type;
        if(dtype->pkt_len > dtype->pkt_header_len){
            pkt->pkt_len = dtype->pkt_len - dtype->pkt_header_len;
            pkt->pkt_buffer_ptr = ptr + dtype->pkt_header_len;
        }
        else{
            printf_warn("header len[%d] is bigger than pkt len[%d]\n", dtype->pkt_header_len, dtype->pkt_len);
            ptr += dtype->pkt_len;
            data_len -= dtype->pkt_len;
            consume_len += dtype->pkt_len;
            free(pkt);
            continue;
        }

        queue_ctx_t *qctx = dtype->pkt_queue[idx];
        printf_debug("[%p]PUSH pkt:  idx:%d, type: %d, pkt_len:%lu, buffer:%p, consume_len=%ld,qctx:%p\n", 
            ptr, pkt->idx, pkt->type, pkt->pkt_len, pkt->pkt_buffer_ptr, consume_len, qctx);
        if(qctx->ops->push(qctx, pkt) == -1){
            consume_len = len;
            free(pkt);
            break;
        }

        ptr += dtype->pkt_len;
        data_len -= dtype->pkt_len;
        consume_len += dtype->pkt_len;
        dtype->stat.read_ok_pkts ++;
    }while(1);
    dtype->stat.read_bytes += consume_len;
    return consume_len;
}

static ssize_t _distributor_process_v2(int type, size_t count, uint8_t **mbufs, size_t len[])
{
    struct _distributor_type_attr_s *dtype = &xdma_dist_type[0];
    void *ptr = NULL;
    size_t prepare_consume_len = 0,consume_len = 0, total_consume_len = 0;
    ssize_t remain_len = 0, buffer_copy_len = 0;

    for(int i = 0; i < count; i++){
        if(len[i] + dtype->buffer.len < dtype->pkt_len){
            /* 新数据包长度过短 */
            continue;
        }
        if(dtype->buffer.len > 0){  /* buffer不为空，说明上一数据未消费完 */
            buffer_copy_len = dtype->pkt_len - dtype->buffer.len;
            if(buffer_copy_len > 0){    /* 新数据包填充部分头数据到buffer中，保证buffer有一包完成的数据帧 */
                memcpy(dtype->buffer.tail, mbufs[i], buffer_copy_len);
                printf_debug("copy PKT data len[0x%lx] to buffer\n", buffer_copy_len);
            }
            else{
                printf_warn("buffer len too long:%d\n", dtype->buffer.len);
                continue;
            }
            /* 消费buffer全部数据 */
            _distributor_analysis(type, dtype->buffer.ptr,dtype->pkt_len, dtype);
            /*  清空buffer数据长度 */
            dtype->buffer.len = 0;
            dtype->buffer.tail = dtype->buffer.ptr;
            /* 新数据包数据偏移，准备消费剩余数据 */
            ptr = (uint8_t *)mbufs[i] + buffer_copy_len;
            prepare_consume_len = len[i] - buffer_copy_len;
        } else {
            ptr = mbufs[i];
            prepare_consume_len = len[i];
        }
        /* 消费数据包数据 */
        consume_len = _distributor_analysis(type, ptr,prepare_consume_len, dtype);
        printf_debug("PKT prepare_consume_len:%lu, consume_len:%lu\n", prepare_consume_len, consume_len);
        remain_len = prepare_consume_len - consume_len;
        /* 未消费完毕，说明有剩余数据（保证长度<pkt_len）待消费，且包含数据头 */
        if(remain_len > 0 && remain_len < dtype->pkt_len){
            /* 未消费数据拷贝到buffer中,待下一包数据填充buffer为完整一包（pkt_len） */
            ptr = ptr + (prepare_consume_len - remain_len);
            printf_debug("COPY remain data[len: %ld] to buffer\n", remain_len);
            memcpy(dtype->buffer.ptr, ptr, remain_len);
            dtype->buffer.tail = dtype->buffer.ptr + remain_len;
            dtype->buffer.len = remain_len;
        } else if(remain_len > dtype->pkt_len){
            printf_warn("remain len too long:%ld\n", remain_len);
        } else if((remain_len < 0)) {
            printf_note("remain len err,prepare_consume_len:%lu, remain_len:%ld, consume_len:%lu, buffer_copy_len:%ld\n",
                    prepare_consume_len, remain_len, consume_len, buffer_copy_len);
        }
        /* 拷贝到buffer中的数据，可认为是已被消费数据 */
        total_consume_len += len[i];
    }
    return total_consume_len;
}

static ssize_t _distributor_process(int type, size_t count, uint8_t **mbufs, size_t len[])
{
    ssize_t consume_size = 0;
    struct _distributor_type_attr_s *dtype = &xdma_dist_type[0];
    int consume_left_size = 0;

    for(int i = 0; i < count; i++){
        consume_size += _distributor_analysis(type, mbufs[i], len[i], dtype);
        consume_left_size = len[i]-consume_size;
        if(consume_left_size > 0)
            printf_note("consume size:%ld[%lu], %d\n", consume_size, len[i], consume_left_size);
    }
    //(count > 1){
    //   printf_warn("count:%lu, consume_size:%ld, [%p]len0:%lu, [%p]len1:%lu\n", count,consume_size, mbufs[0], len[0], mbufs[1],len[1]);
    //}

    return consume_size;
}


static void _distributor_wait_consume_over(struct _distributor_type_attr_s *dtype)
{
    queue_ctx_t *pkt_q;
    volatile int not_consume_over = 0;
    int timeout = 0;
    struct timeval start, now;
    
 //   usleep(20);
    _gettime(&start);
    do{
        not_consume_over = 0;
        for(int ch = 0; ch < dtype->idx_num; ch++){
            pkt_q = (queue_ctx_t *)dtype->pkt_queue[ch];
            if(!pkt_q->ops->is_empty(pkt_q)){
                printf_debug("ch: %d is not consume over,  entry:%u\n", ch, pkt_q->ops->get_entry(pkt_q));
                not_consume_over = 1;
                usleep(2);
            }
        }

        _gettime(&now);
        timeout = _tv_diff(&now, &start);
        if(timeout > 2000){
            printf_warn("Wait TimeOut[%dms]!, %d\n",2000,   _tv_diff(&now, &start));
            for(int ch = 0; ch < dtype->idx_num; ch++){
                pkt_q = (queue_ctx_t *)dtype->pkt_queue[ch];
                if(!pkt_q->ops->is_empty(pkt_q)){
                    printf_note("ch:%d is not consume over!\n", ch);
                }
            }
            _distributor_reset(-1);
            break;
        }

        if(not_consume_over == 0)
            break;
    }while(1);
}



static int _distributor_data_uplink_prod(void *s)
{
    struct spm_context *_ctx;
    volatile uint8_t *ptr[2048] = {NULL};
    size_t len[2048] = {0}, prod_size = 0;
    ssize_t count = 0;
    struct _distributor_type_attr_s *disp = s;
    unsigned int consume_len = 0;

    _ctx = get_spm_ctx();
    if(_ctx == NULL)
        return -1;

    if(_ctx->ops->read_raw_data)
        count = _ctx->ops->read_raw_data(-1, STREAM_XDMA_READ, (void **)ptr, len, NULL);

    if(count > 0 && count < 2048){
        consume_len = _distributor_process_v2(STREAM_XDMA_READ, count, (uint8_t **)ptr, len);
    }
    if(consume_len > 0){
        _distributor_wait_consume_over(disp);
    }

    if(_ctx->ops->read_raw_over_deal)
        _ctx->ops->read_raw_over_deal(-1, STREAM_XDMA_READ, &consume_len);
    
    return 0;
}

static int _send_data_header(char *header, int fd, int idx)
{
    /*
        XWKJ 1.0\r\n         数据协议标示 
        TYPE: 0\r\n         数据类型，十进制字符表示
        \r\n                数据头与原始数据分割
    */

    char *ptr;
    char type_str[32] = {0};
    int type = _get_type_by_idx(idx), r = 0;
    printf_note("idx:%d, type：%d\n", idx, type);
    ptr = header;
    
    strcat(ptr, "XWKJ 1.0\r\n");
    snprintf(type_str, sizeof(type_str) -1, "TYPE: %d\r\n", type);
    strcat(ptr, type_str);
    strcat(ptr, "\r\n");

    //printf_note("[%ld] %s", strlen(header), header);
    struct spm_context *pctx = get_spm_ctx();
    if(!pctx)
        return -1;
    if(pctx->ops->send_data_by_fd)
        r = pctx->ops->send_data_by_fd(fd, header, strlen(header), NULL);
    
    return r;
}
static int _distributor_data_uplink_init(void *args)
{
    printf_note("XDMA Start\n");
    io_set_enable_command(XDMA_MODE_ENABLE, -1, 0, 0);
#ifndef DEBUG_TEST
    SET_CHANNEL_SEL(get_fpga_reg(), 0xff);
#endif
    return 0;
}

static void _distributor_data_uplink_exit(void *args)
{
    printf_note("XDMA Stop\n");
    io_set_enable_command(XDMA_MODE_DISABLE, -1, 0, 0); /* close dma */
}



static ssize_t _distributor_data_uplink_consume(int fd, void *args)
{
    int idx = 0, r = 0;
    struct _distributor_pkt_data_s *pkt = NULL;
    uint8_t *ptr = NULL;
    size_t len = 0;
    struct spm_context *pctx = get_spm_ctx();
    if(!pctx)
        return -1;
    
    if(args)
        idx = *(int *)args;

    queue_ctx_t *pkt_q  = (queue_ctx_t *)xdma_dist_type[0].pkt_queue[idx];
    pkt = pkt_q->ops->pop(pkt_q);
    //printf_note("POP,idx: %d, pkt:%p, pkt_q:%p\n", idx, pkt, pkt_q);
    if(pkt == NULL){
        usleep(2);
        return 1;
    }
    ptr = pkt->pkt_buffer_ptr;
    len = pkt->pkt_len;
    //printf_note("idx:%d, ptr: %p, len: %lu\n", idx, ptr, len);

    //print_array(ptr, 8);
    if(pctx->ops->send_data_by_fd)
        r = pctx->ops->send_data_by_fd(fd, ptr, len, NULL);

    _safe_free_(pkt);
    return r;
}



static ssize_t data_uplink_handle(int fd, void *args)
{
     struct spm_context *pctx = get_spm_ctx();
    if(!pctx)
        return -1;

    ssize_t count = 0, r = 0;
    volatile uint8_t *ptr[2048] = {NULL};
    uint32_t len[2048] = {[0 ... 2047] = 0};

    if(pctx->ops->read_raw_data)
        count = pctx->ops->read_raw_data(-1, STREAM_XDMA_READ, (void **)ptr, len, NULL);

    if(count <= 0){
        r = 1;/* loop read */
    }
    for(int i = 0; i < count; i++){
        if(pctx->ops->send_data_by_fd)
            r += pctx->ops->send_data_by_fd(fd, ptr[i], len[i], NULL);
    }
    if(count > 0){
        if(pctx->ops->read_raw_over_deal)
                pctx->ops->read_raw_over_deal(-1, STREAM_XDMA_READ, NULL);
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

int _align_power2(int rc)
{
    int i = 0;
    for(i = 1; i <= 8; i++){
        if(rc <= (1 <<i)){
            break;
        }
    }
    return (ALIGN(rc, (1 <<i)));
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

#define _MAX_PKT_LEN_BYTE 4096
#define _MAX_BUFFER_LEN_BYTE 8192
void *gbuffer = NULL;

static ssize_t data_downlink_handle(int fd, const void *data, size_t len)
{
    struct spm_context *pctx = get_spm_ctx();
    ssize_t consume_len = 0, remain_len = len;
    uint8_t *buffer = NULL, *ptr = NULL;
    int max_data_len = 256, ret = 0;
    int max_pkt_len = _MAX_PKT_LEN_BYTE;

    if(!pctx)
        return -1;

    if(gbuffer)
        buffer = gbuffer;
    else
        buffer = malloc_align(_MAX_BUFFER_LEN_BYTE);
    if(!buffer)
        return -1;
    
    memset(buffer, 0, _MAX_BUFFER_LEN_BYTE);
    if(len > _MAX_BUFFER_LEN_BYTE)
        len = _MAX_BUFFER_LEN_BYTE;
    memcpy(buffer, data, len);
    ptr = buffer;
    do{
        consume_len = min(max_pkt_len, remain_len);
        if(consume_len < max_pkt_len){
            consume_len = max_data_len;
        }
        
        if(pctx->ops->write_data){
            if(pctx->ops->write_data(-1, ptr, consume_len) < 0){
                ret = -1;
                break;
            }
        }
        //printf_note("remain_len:%ld, consume_len:%ld\n", remain_len, consume_len);
        if(remain_len < consume_len){
            break;
        }else {
            remain_len -= consume_len;
            ptr += consume_len;
        }
        if(ptr - buffer > len){
            printf_err("buffer ptr err!!!\n");
            break;
        }
    }while(remain_len > 0);
    if(!gbuffer)
        free(buffer);
    //usleep(600);

    return ret;
}


volatile int is_read = 0, is_write = 0;

static int write_dst_board(int id, void *args)
{
    printf_note("write dst id: %d\n", id);
    xdma_dist_type[0].dst_board = id;
    return 0;
}

static void write_dst_board_reg(void)
{
#ifdef DEBUG_TEST
    return;
#endif
#ifdef SET_SRIO_SRC_DST_ID2
    if(xdma_dist_type[0].dst_board == 1){
        SET_SRIO_SRC_DST_ID2(get_fpga_reg(), 0x00060007);
    }
    else{
        SET_SRIO_SRC_DST_ID2(get_fpga_reg(), 0x00070006); 
    }
#endif
}


static int data_pre_handle(int rw, void *args)
{
    struct spm_context *pctx = get_spm_ctx();
    if(!pctx)
        return -1;
    if(rw == MISC_WRITE){
        is_write = 1;
        write_dst_board_reg();
        gbuffer = malloc_align(_MAX_BUFFER_LEN_BYTE);
    }
    if(rw == MISC_READ){
        if(is_read == 0){
            int ret = -1;
            pthread_t tid = 0;

            if(args){
                ftp_client_t *c = args;
                char buffer[128] = {0};
                _send_data_header(buffer, c->pasv_fd, c->retr_idx);
            }
            
            is_read = 1;
            printf_note("Start Distributor...\n");
            ret =  pthread_create_detach (NULL,_distributor_data_uplink_init, 
                                            _distributor_data_uplink_prod,
                                            _distributor_data_uplink_exit,
                                            "Distributor", &xdma_dist_type[0],
                                            &xdma_dist_type[0], &tid);
            if(ret != 0){
                perror("pthread save file thread!!");
            }
            xdma_dist_type[0].tid = tid;
            printf_note("start thread tid:%lu\n", tid);
        }
    }
     return 0;
}

static int data_post_handle(int rw, void *args)
{
    if(rw == MISC_READ){
        printf_note("Post Handle %p,%ld\n", args, ftp_client_idx_weight(args));
        if((args && ftp_client_idx_weight(args)  == 0) || args == NULL){ 
            /* all or  one client read over */
            printf_note("stop thread tid:%lu\n", xdma_dist_type[0].tid);
            pthread_cancel_by_tid(xdma_dist_type[0].tid);
            is_read = 0;
        }
    }
    if(rw == MISC_WRITE){
        if(gbuffer){
            free(gbuffer);
            gbuffer = NULL;
        }
        is_write = 0;
    }
#ifndef DEBUG_TEST
    if(is_read == 0 && is_write == 0)
        SET_CHANNEL_SEL(get_fpga_reg(), 0);
#endif
    return 0;
}


static const struct misc_ops misc_reg = {
    .post_handle = data_post_handle,
    .pre_handle = data_pre_handle,
    .write_handle = data_downlink_handle,
    .read_handle = _distributor_data_uplink_consume, //data_uplink_handle,
    .usr1_handle = write_dst_board,
};

const struct misc_ops * misc_create_ctx(void)
{
    const struct misc_ops *ctx = calloc(1, sizeof(*ctx));
     if (!ctx)
        return NULL;
     
    ctx = &misc_reg;
    for(int i = 0; i < xdma_dist_type[0].idx_num; i++)
        xdma_dist_type[0].pkt_queue[i] = queue_create_ctx();
    xdma_dist_type[0].buffer.len = 0;
    xdma_dist_type[0].buffer.ptr = calloc(1, 4096);
    xdma_dist_type[0].buffer.tail = xdma_dist_type[0].buffer.ptr;
    xdma_dist_type[0].dst_board = 0;
    xdma_dist_type[0].stat.read_bytes = 0;
    return ctx;
}

