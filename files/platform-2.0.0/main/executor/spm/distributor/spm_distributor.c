/******************************************************************************
*  Copyright 2019, Showay Technology Dev Co.,Ltd.
*  ---------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Showay Technology Dev Co.,Ltd. (C) 2019
******************************************************************************/
/*****************************************************************************     
*  Rev 1.0   30 June. 2022   yaochangsong
*  Initial revision.
******************************************************************************/
//#include "config.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <poll.h>
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
#include <semaphore.h>
#include <net/if.h>
#include <sys/stat.h>
#include <termios.h>
#include <errno.h>
#include <linux/spi/spidev.h>
#include <time.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <math.h>
#include <sys/mman.h>
#include <mqueue.h>
#include <assert.h>
#include "bsp.h"
#include "spm_distributor.h"
#include "../../../main/log/log.h"
#include "../../../main/executor/spm/spm.h"
#include "../../../main/executor/executor.h"
#include "../../../main/utils/queue.h"
#include "../../../main/utils/lqueue.h"
#include "../../../main/utils/thread.h"
#include "../../../main/utils/utils.h"
#include "../../../main/conf/conf.h"


#define DIST_MAX_COUNT 2048
#define DIST_MAX_FFT_CHANNEL MAX_RADIO_CHANNEL_NUM
#define DIST_MAX_IQ_CHANNEL  8
#define SPM_DIST_TIMEOUT_MS (10000)

/* The max number of channel 48 is considered OK at present*/
#define FRAME_MAX_CHANNEL 48

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))
#endif

struct spm_distributor_pkt_data_s{
    int ch;
    uint32_t sn;
    void *pkt_buffer_ptr;
    size_t pkt_len;
    int frame_pkt_num;
    uint64_t frame_idx;
};

static int _spm_distributor_iq_thread_loop(void *s);
static int _spm_distributor_fft_thread_loop(void *s);


struct spm_distributor_type_attr_s{
    char *name;
    /* The max number of channel each type*/
    int channel_num;
    int type;
    int pkt_len;
    int pkt_header_len;
    uint16_t pkt_header_flags;
    int (*distributor_loop)(void *);
    void *pkt_queue[FRAME_MAX_CHANNEL];
    void *frame_buffer[FRAME_MAX_CHANNEL];
    volatile bool is_reset[FRAME_MAX_CHANNEL];
} dist_type[] = {
    {"FFT", 1, SPM_DIST_FFT, 528, 16, 0xaa55, _spm_distributor_fft_thread_loop},
//    {"IQ",  DIST_MAX_IQ_CHANNEL,  SPM_DIST_IQ,  512, 16, 0xaa55, _spm_distributor_iq_thread_loop},
};

struct spm_data_pkt_s{
    uint16_t header_flags;
    uint8_t serial_num;
    uint8_t  channel;
    uint16_t pkt_len;
};


struct frame_devider_s{
    bool frame_start;
    int old_pkt_idx;
    uint64_t frame_idx;
}frame_devider[SPM_DIST_MAX][FRAME_MAX_CHANNEL];

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



static int _frame_devider_gen_idx(int type, int ch, int pkt_index, uint64_t *f_idx)
{
    if(ch  >= FRAME_MAX_CHANNEL)
        return -1;
    struct frame_devider_s *frd = &frame_devider[type][ch];
    //printf_note("old_pkt_idx=%d,frame_start=%d,frame_idx=%"PRIu64",pkt_index=%d\n", frd->old_pkt_idx, frd->frame_start,frd->frame_idx, pkt_index);
    if(pkt_index == 0){
        frd->frame_start = true;
        frd->frame_idx++;
    } else if(frd->old_pkt_idx + 1 != pkt_index || frd->frame_start != true){
        frd->frame_start = false;
        return -1;
    }
    frd->old_pkt_idx = pkt_index;
    *f_idx = frd->frame_idx;
    return 0;
}

static int _frame_devider_init(struct frame_devider_s *fds)
{
    struct frame_devider_s *frd = fds;
    frd->frame_start = false;
    frd->old_pkt_idx = 0;
    frd->frame_idx = 0;
    return 0;
}

static int _data_dump(void *args)
{
    struct spm_distributor_pkt_data_s *pkt = args;
    printf("pkt:%p, frameId:%"PRIu64", ptr:%p, len:%lu, sn:%u\n", pkt,
        pkt->frame_idx, pkt->pkt_buffer_ptr, pkt->pkt_len, pkt->sn);
    return 0;
}

static ssize_t _spm_find_header(uint8_t *ptr, uint16_t header, size_t len)
{
    size_t offset = 0;
    do{
        if(ptr != NULL && *(uint16_t *)ptr != header){
            ptr += 2;
            offset += 2;
        }else{
            break;
        }
    }while(offset < len);

    if(offset >= len || ptr == NULL)
        return -1;

    return offset;
}


int _spm_distributor_analysis(int type, uint8_t *mbufs, size_t len, struct spm_distributor_type_attr_s *dtype)
{
    uint8_t *ptr = mbufs;
    size_t offset = 0;
    ssize_t dirty_offset = 0;
    int ret = -1;
    uint64_t fidx = 0;
    struct spm_data_pkt_s *header = NULL;

    do{
        header= (struct spm_data_pkt_s *)ptr;
        if(!header)
            break;
        if(header->header_flags != dtype->pkt_header_flags){
           // printf("fft header[0x%x] error\n", header->header_flags);
            dirty_offset = _spm_find_header(ptr, dtype->pkt_header_flags, dtype->pkt_len);
            if(dirty_offset > 0){
                ptr += dirty_offset;
                header= (struct spm_data_pkt_s *)ptr;
               // printf_note("find header, offset:%ld\n", dirty_offset);
            }else{
                break;
            }
        }
        if(header->channel >= dtype->channel_num){
            printf("fft channel[%d] too big\n", header->channel);
            break;
        }
        if(config_get_fft_work_enable(header->channel) == false){
            printf("fft channel[%d] not work\n", header->channel);
            break;
        }
        if(header->pkt_len != dtype->pkt_len){
            printf("fft pkt_len[%d] error[%d]\n", header->pkt_len, dtype->pkt_len);
            break;
        }

        if(type == SPM_DIST_FFT){
            if(_frame_devider_gen_idx(type, header->channel, header->serial_num, &fidx) == -1){
                printf_note("frame sn[%d] error\n", header->serial_num);
                break;
            }
        }

        struct spm_distributor_pkt_data_s *pkt;
        pkt = calloc(1, sizeof(*pkt));
        if(!pkt){
            break;
        }
        pkt->ch = header->channel;
        pkt->sn = header->serial_num;
        pkt->frame_idx = fidx;
        if(header->pkt_len > dtype->pkt_header_len){
            pkt->pkt_len = header->pkt_len - dtype->pkt_header_len;
            pkt->pkt_buffer_ptr = ptr + dtype->pkt_header_len;
        }
        else
            printf_warn("header len[%d] is bigger than pkt len[%d]\n", dtype->pkt_header_len, header->pkt_len);

        queue_ctx_t *qctx = dtype->pkt_queue[pkt->ch];
        printf_info("PUSH pkt:  ch:%d, sn: %d, pkt_len:%lu, frame_idx:%"PRIu64"\n", pkt->ch, pkt->sn, pkt->pkt_len, pkt->frame_idx);
        qctx->ops->push(qctx, pkt);
        //qctx->ops->foreach(qctx, _data_dump);
        offset += header->pkt_len;
        if(offset + dirty_offset >= len){
            ret = 0;
            break;
        }
        ptr += header->pkt_len;
    }while(offset + dirty_offset < len);
    return ret;
}

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

static int _spm_distributor_iq_data_producer(int ch, void **data, size_t len)
{
    queue_ctx_t *pkt_q  = (queue_ctx_t *)dist_type[SPM_DIST_IQ].pkt_queue[ch];
    uint8_t *mbuf_header = (uint8_t *)dist_type[SPM_DIST_IQ].frame_buffer[ch];
    struct spm_distributor_pkt_data_s *pkt = NULL;
    struct timeval start, now;
    uint8_t *ptr = mbuf_header;
    
    _gettime(&start);
    do{
        _gettime(&now);
        if(_tv_diff(&now, &start) > SPM_DIST_TIMEOUT_MS){
            printf_warn("IQ Read TimeOut!\n");
            return -1;
        }
        pkt = pkt_q->ops->pop(pkt_q);
        if(pkt == NULL)
            continue;
        memcpy(ptr, pkt->pkt_buffer_ptr, pkt->pkt_len);
        _safe_free_(pkt);
        break;
    }while(1);

    *data = ptr;
    return 0;
}

static int _spm_distributor_data_frame_producer(int ch, int type, void **data, size_t len)
{
    queue_ctx_t *pkt_q  = (queue_ctx_t *)dist_type[type].pkt_queue[ch];
    uint8_t *mbuf_header = (uint8_t *)dist_type[type].frame_buffer[ch];
    if(!pkt_q || !mbuf_header)
        return -1;
    struct spm_distributor_pkt_data_s *pkt = NULL;
    size_t frame_len = 0, frame_pkt_num = 0, pkt_len = 0;
    int frame_1st_idx = -1, ret = 0;
    bool start_frame = false;
    uint8_t *ptr = mbuf_header;
    struct timeval start, now;
    _gettime(&start);
    do{
        _gettime(&now);
        if(_tv_diff(&now, &start) > SPM_DIST_TIMEOUT_MS){
            printf_warn("Read TimeOut!\n");
            ret = -1;
            break;
        }
        if(dist_type[type].is_reset[ch] == true){
            dist_type[type].is_reset[ch] = false;
            break;
        }
        //printf_note("POP,type:%d, ch:%d\n", type, ch);
        pkt = pkt_q->ops->pop(pkt_q);
        if(pkt == NULL)
            continue;
        printf_info("POP ch:%d, sn:%u, fidx:%"PRIu64", pkt_len:%lu\n", ch, pkt->sn, pkt->frame_idx, pkt->pkt_len);
        /* 流水号为0说明是第一包数据开始，初始化相关变量 */
        if(pkt->sn == 0){
            start_frame = true;
            frame_1st_idx = pkt->frame_idx;
            frame_len = pkt->pkt_len;
            pkt_len = pkt->pkt_len;
            frame_pkt_num = 1;
            ptr = mbuf_header;
        }
        /* 未收到从0开始的数据包，说明起始包错误，丢弃,重新找流水号为0的包*/
        if(start_frame == false){
            printf_warn("Not receive start pkt[sn:%d], Discard pkt\n", pkt->sn);
            _safe_free_(pkt);
            continue;
        }
        if(frame_pkt_num > 1)
            frame_len += pkt_len;
        if(frame_len >= MAX_FRAME_BUFFER_LEN){
            printf_err("Frame pkt[%lu] is too long!\n", frame_pkt_num);
            _safe_free_(pkt);
            break;
        }
        /* 接受到中间包帧号和第一包帧号不一致，说明不是同一帧，丢弃，重新查找流水号为0的包 */
        if(frame_1st_idx != pkt->frame_idx && frame_len < len){
            printf_warn("Frame idx:%"PRIu64" err, Discard pkt\n", pkt->frame_idx);
            start_frame = false;
            _safe_free_(pkt);
            continue;
        }
        /* 将包数据逐步拷贝到通道缓存区，直到完成一帧的数据长度 */
        memcpy(ptr, pkt->pkt_buffer_ptr, pkt_len);
       // printf_note("mbuf_header:%p, ch data:%d\n", mbuf_header, ch);
        //print_array(ptr, 512);
        _safe_free_(pkt);
        if(frame_len == len){
            printf_info("ch=%d, frame_len=%lu, len=%lu, Frame OK!\n", ch, frame_len, len);
            break;
        }
        ptr += pkt_len;
        frame_pkt_num++;
    }while(frame_len < len);
    if(frame_len > 0 && frame_len != len){
        printf_warn("Read len[%lu] is not eq expect len[%lu], Discard pkt\n", frame_len, len);
        ret = -1;
    }
    *data = mbuf_header;
    return ret;
}

int spm_distributor_fft_data_frame_producer(int ch, void **data, size_t len)
{
    return _spm_distributor_data_frame_producer(ch, SPM_DIST_FFT, data, len);
}

static int spm_distributor_process(int type, size_t count, uint8_t **mbufs, size_t len[])
{
    int ret = 0;
    for(int index = 0; index < count; index++){
        ret = _spm_distributor_analysis(type, mbufs[index], len[index], &dist_type[type]);
        if(ret == -1)
            continue;
    }
    return ret;
}

int _spm_distributor_wait_fft_read_over(void)
{
    #define WAIT_FFT_CONSUME_TIMEOUT_MS 10
    queue_ctx_t *pkt_q;
    struct timeval start, now;
    int ret = 0;
    _gettime(&start);
    do{
        for(int i = 0; i < ARRAY_SIZE(dist_type); i++){
            for(int ch = 0; ch < dist_type[i].channel_num; ch++){
                pkt_q = (queue_ctx_t *)dist_type[i].pkt_queue[ch];
                if(!pkt_q->ops->is_empty(pkt_q)){
                    _gettime(&now);
                    if(_tv_diff(&now, &start) > WAIT_FFT_CONSUME_TIMEOUT_MS){
                        printf_warn("Wait TimeOut!\n");
                        ret = -1;
                        break;
                    }
                    usleep(5);
                    continue;
                }
            }
        }
        /* consumer over */
        break;
    }while(1);
    return ret;
}

static int _spm_distributor_fft_thread_loop(void *s)
{
    struct spm_context *_ctx;
    volatile uint8_t *ptr[DIST_MAX_COUNT] = {NULL};
    size_t len[DIST_MAX_COUNT] = {0};
    ssize_t count = 0;

    _ctx = get_spm_ctx();
    if(_ctx == NULL)
        return -1;

    if(_ctx->ops->read_fft_vec_data)
        count = _ctx->ops->read_fft_vec_data(-1, (void **)ptr, len, NULL);

    if(count > 0 && count < DIST_MAX_COUNT){
        spm_distributor_process(SPM_DIST_FFT, count, (uint8_t **)ptr, len);
    }
    _spm_distributor_wait_fft_read_over();
    
    if(_ctx->ops->read_fft_over_deal)
        _ctx->ops->read_fft_over_deal(-1, NULL);
    
    return 0;
}

static int _spm_distributor_iq_thread_loop(void *s)
{
    struct spm_context *_ctx;
    volatile uint8_t *ptr[DIST_MAX_COUNT] = {NULL};
    size_t len[DIST_MAX_COUNT] = {0}, count = 0;

    _ctx = get_spm_ctx();
    if(_ctx == NULL)
        return 0;

    if(_ctx->ops->read_iq_vec_data)
        count = _ctx->ops->read_iq_vec_data(-1, (void **)&ptr, len, NULL);
    if(count > 0 && count < DIST_MAX_COUNT){
        spm_distributor_process(SPM_DIST_IQ, count, (uint8_t **)ptr, len);
    }
    return 0;
}

int spm_distributor_thread(void *dtype)
{
    pthread_t tid;
    int ret;
    struct spm_distributor_type_attr_s *disp_type = dtype;
    ret =  pthread_create_detach (NULL,
                NULL, 
                disp_type->distributor_loop, 
                NULL,  
                disp_type->name ,disp_type, disp_type, &tid
    );
    if(ret != 0){
        perror("pthread err");
        exit(1);
    }

    return ret;
}

int spm_consumer_thread(void *arg)
{
    int ch = *(int *)arg;

    if(ch > DIST_MAX_FFT_CHANNEL)
        return 0;
    struct spm_context *spmctx = get_spm_ctx();
    struct spm_run_parm *args = spmctx->run_args[ch];
    args->fft_size = 8192;
    args->ch = ch;
    spm_deal(spmctx, args, ch);
    //sleep(1);
    return 0;
}

int spm_distributor_fft_consumer_thread(int ch, int type)
{
    pthread_t tid;
    int ret;
    char name[128];
    static int s_ch = 0;
    s_ch = ch;
    printf_note("ch:%d\n", ch);
    snprintf(name, sizeof(name)-1, "FFT_ch%d_%s", s_ch, type == SPM_DIST_FFT ? "FFT" : "IQ");
    printf_note("thread name:%s\n", name);

    ret =  pthread_create_detach (NULL,
                NULL, 
                spm_consumer_thread,
                NULL, 
                name ,&s_ch, &s_ch, &tid
    );
    if(ret != 0){
        perror("pthread err");
        exit(1);
    }
    return ret;
}


static int _spm_distributor_init(void *args)
{
    queue_ctx_t *qctx = NULL;
    int type;
    for(int i = 0; i < ARRAY_SIZE(dist_type); i++){
        printf_note("type:%d, %s\n", i,  dist_type[i].name);
        for(int ch = 0; ch < dist_type[i].channel_num; ch++){
            qctx = queue_create_ctx();
            if(qctx){
                dist_type[i].pkt_queue[ch] = (void *)qctx;
            }
            _frame_devider_init(&frame_devider[i][ch]);
            dist_type[i].frame_buffer[ch] = calloc(1, MAX_FRAME_BUFFER_LEN);
            if(!dist_type[i].frame_buffer[ch]){
                printf_warn("calloc memory faild!\n");
                continue;
            }
            dist_type[i].is_reset[ch] = false;
        }
    }
    for(int i = 0; i < ARRAY_SIZE(dist_type); i++)
        spm_distributor_thread(&dist_type[i]);
    return 0;
}

static int _spm_distributor_reset(int type, int ch)
{
    if(type > SPM_DIST_MAX)
        return -1;
    
    if(ch > dist_type[type].channel_num)
        return -1;

    dist_type[type].is_reset[ch] = true;
    queue_ctx_t *qctx =  dist_type[type].pkt_queue[ch];
    if(qctx->ops->clear)
        qctx->ops->clear(qctx);
    _frame_devider_init(&frame_devider[type][ch]);
    printf_note("Reset, type:%d, ch:%d\n", type, ch);
    return 0;
}

static const struct spm_distributor_ops spm_disp_ops = {
    .init = _spm_distributor_init,
    .reset = _spm_distributor_reset,
};

spm_distributor_ctx_t * spm_create_distributor_ctx(void)
{
    int ret = -ENOMEM, ch;
    unsigned int len;
    spm_distributor_ctx_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx)
        goto err_set_errno;

    ctx->ops = &spm_disp_ops;
    ctx->ops->init(NULL);
err_set_errno:
    errno = -ret;
    return ctx;

}

