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


#define DIST_MAX_COUNT 2048
#define DIST_MAX_FFT_CHANNEL MAX_RADIO_CHANNEL_NUM

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
    int channel_num;
    int type;
    int pkt_len;
    int pkt_header_len;
    uint16_t pkt_header_flags;
    int (*distributor_loop)(void *);
    void *pkt_queue[8];
    void *frame_buffer[8];
} dist_type[] = {
    {"FFT", DIST_MAX_FFT_CHANNEL, SPM_DIST_FFT, 528, 16, 0xaa55, _spm_distributor_fft_thread_loop},
//    {"IQ", 8, SPM_DIST_IQ, 528, 16, 0xaa55, _spm_distributor_iq_thread_loop},
};

struct spm_data_pkt_s{
    uint16_t header_flags;
    uint8_t serial_num;
    uint8_t  channel;
    uint16_t pkt_len;
};

#define FRAME_MAX_CHANNEL 32
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
        printf("%02x ", *ptr++);
    }
    printf("\n");
}


static int _frame_devider_gen_idx(int type, int ch, int pkt_index, uint64_t *f_idx)
{
    if(ch  >= FRAME_MAX_CHANNEL)
        return -1;
    struct frame_devider_s *frd = &frame_devider[type][ch];
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

int _spm_distributor_analysis(int type, uint8_t *mbufs, size_t len, struct spm_distributor_type_attr_s *dtype)
{
    uint8_t *ptr = mbufs;
    struct spm_data_pkt_s *fft_header = (struct spm_data_pkt_s *)mbufs;
    size_t offset = 0;
    int ret = -1;
    uint64_t fidx = 0;
    //printf_note("Type:%d,%p\n", type, ptr);
    do{
        if(!fft_header)
            break;
        if(fft_header->header_flags != dtype->pkt_header_flags){
            printf("fft header[0x%x] error\n", fft_header->header_flags);
            break;
        }
        if(fft_header->channel >= dtype->channel_num){
            printf("fft channel[%d] too big\n", fft_header->channel);
            break;
        }
        if(fft_header->pkt_len != dtype->pkt_len){
            printf("fft pkt_len[%d] error\n", fft_header->pkt_len);
            break;
        }

        if(_frame_devider_gen_idx(type, fft_header->channel, fft_header->serial_num, &fidx) == -1){
            printf_note("frame sn[%d] error\n", fft_header->serial_num);
            break;
        }
        //printf_note("frame idx:%"PRIu64"\n", fidx);
        struct spm_distributor_pkt_data_s *pkt;
        pkt = calloc(1, sizeof(*pkt));
        if(!pkt){
            break;
        }
        pkt->ch = fft_header->channel;
        pkt->sn = fft_header->serial_num;
        pkt->frame_idx = fidx;
        if(fft_header->pkt_len > dtype->pkt_header_len){
            pkt->pkt_len = fft_header->pkt_len - dtype->pkt_header_len;
            pkt->pkt_buffer_ptr = ptr + dtype->pkt_header_len;
        }
        else
            printf_warn("header len[%d] is bigger than pkt len[%d]\n", dtype->pkt_header_len, fft_header->pkt_len);
        
        queue_ctx_t *qctx = dtype->pkt_queue[pkt->ch];
        printf_note("push pkt:  ch:%d, sn: %d, pkt_len:%lu, frame_idx:%"PRIu64"\n", pkt->ch, pkt->sn, pkt->pkt_len, pkt->frame_idx);
        qctx->ops->push(qctx, pkt);
        //qctx->ops->foreach(qctx, _data_dump);
        offset += fft_header->pkt_len;
        if(offset >= len){
            ret = 0;
            break;
        }
        ptr += fft_header->pkt_len;
    }while(offset < len);
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

static int _spm_distributor_data_frame_producer(int ch, int type, void **data, size_t len)
{
    #define _SPM_DIST_TIMEOUT_MS (1000)
    queue_ctx_t *pkt_q  = (queue_ctx_t *)dist_type[type].pkt_queue[ch];
    uint8_t *mbuf_header = (uint8_t *)dist_type[type].frame_buffer[ch];
    struct spm_distributor_pkt_data_s *pkt = NULL;
    size_t frame_len = 0, frame_pkt_num = 0, pkt_len = 0;
    int frame_1st_idx = -1, ret = 0;
    bool start_frame = false;
    uint8_t *ptr = mbuf_header;
    struct timeval start, now;
    _gettime(&start);
    do{
        _gettime(&now);
        //printf_note(">>>name:%s, %p, frame_buffer:%p\n",   dist_type[type].name, &dist_type[type], mbuf_header);
        if(_tv_diff(&now, &start) > _SPM_DIST_TIMEOUT_MS){
            printf_warn("Read TimeOut!\n");
            ret = -1;
            break;
        }
       // pkt_q->ops->foreach(pkt_q, _data_dump);
        pkt = pkt_q->ops->pop(pkt_q);
        if(pkt == NULL)
            continue;
        //printf_note(">>>ch:%d, frame_buffer:%p,%p, pkt:%p, len:%d, pkt_buffer_ptr:%p, len:%lu\n", ch, mbuf_header, ptr, pkt, pkt->pkt_len, pkt->pkt_buffer_ptr, len);
        //print_array(pkt->pkt_buffer_ptr, pkt->pkt_len);
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
        /* 接受到中间包帧号和第一包帧号不一致，说明不是同一帧，丢弃，重新查找流水号为0的包 */
        if(frame_1st_idx != pkt->frame_idx && frame_len < len){
            printf_warn("Frame idx:%d err, Discard pkt\n", pkt->frame_idx);
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
            printf_note("ch=%d, frame_len=%lu, len=%lu, Frame OK!\n", ch, frame_len, len);
            break;
        }
        ptr += pkt_len;
        frame_pkt_num++;
    }while(frame_len < len);
    if(frame_len > 0 && frame_len != len){
        printf_warn("Read len[%lu] is not eq expect len[%lu], Discard pkt\n", frame_len, len);
        ret = -1;
    }
    *data = ptr;
    return ret;
}

int spm_distributor_fft_data_frame_producer(int ch, void **data, size_t len)
{
    return _spm_distributor_data_frame_producer(ch, SPM_DIST_FFT, data, len);
}

static int spm_distributor_process(int type, int count, uint8_t *mbufs, size_t *len)
{
    int ret = 0;
    //print_array(mbufs, 512);
    for(int index = 0; index < count; index++){
        ret = _spm_distributor_analysis(type, mbufs, len[index], &dist_type[type]);
        if(ret == -1)
            break;
    }
    return ret;
}
static int _assamble_header_debug(int ch, int type,  void **data, size_t *len, void *args)
{
    void *mbuf = NULL;
    int count = 0;
    static int cn = 0, sn = 0;
    //if(mbuf == NULL){
    //    mbuf = calloc(1, 528);
    //}
    
    if(cn++ < 4){
        mbuf = calloc(1, 528);
        count = 1;
    }else
        return -1;
    if(cn == 4)
        sn = 0;
        
    uint8_t *ptr = (void *)mbuf;
    struct _header_pkt{
        uint16_t start_flags;
        uint8_t sn;
        uint8_t ch;
        uint16_t len;
        uint8_t resv[10];
        uint8_t data[512];
    }*hpkt;
    hpkt = (struct _header_pkt *)ptr;
    hpkt->start_flags = 0xaa55;
    
    hpkt->sn = sn++;
    hpkt->ch = ch;
    hpkt->len = 528;
    for(int i = 0; i < 512; i++){
        hpkt->data[i] = i;
    }
   // printf_note("hpkt->data:%p\n", hpkt->data);
   // print_array(hpkt->data, 512);
    data[0] = hpkt;
    len[0] = 528;
    return count;
}
static int _spm_distributor_fft_thread_loop(void *s)
{
    struct spm_context *_ctx;
    volatile uint8_t *ptr = NULL;
    size_t len[DIST_MAX_COUNT] = {0}, count = 0;
    static int dma_ch = -1;

    #if 0
    _ctx = get_spm_ctx();
    if(_ctx == NULL)
        return 0;

    if(_ctx->ops->read_fft_vec_data)
        count = _ctx->ops->read_fft_vec_data(dma_ch, (void **)&ptr, len, NULL);
    if(count > 0 && count < DIST_MAX_COUNT){
        spm_distributor_process(SPM_DIST_FFT, count, ptr, len);
    }
    #else
    for(int i = 0; i < DIST_MAX_FFT_CHANNEL; i++){
        count = _assamble_header_debug(i, SPM_DIST_FFT, (void **)&ptr, len, NULL);
        if(count > 0 && count < DIST_MAX_COUNT)
            spm_distributor_process(SPM_DIST_FFT, count, ptr, len);
    }
    #endif
    return 0;
}

static int _spm_distributor_iq_thread_loop(void *s)
{
    return 0;
}

static void _spm_distributor_thread_exit(void *arg)
{

}

static int _spm_distributor_thread_init(void *args)
{
    struct spm_distributor_type_attr_s *disp_type = args;
    queue_ctx_t *qctx = NULL;
    sleep(2);
    printf_note("disp_type:%p, name:%s, chnum:%d, %d\n", disp_type, disp_type->name, disp_type->channel_num, DIST_MAX_FFT_CHANNEL);
    for(int i = 0; i < disp_type->channel_num; i++){
        qctx = queue_create_ctx();
        if(qctx){
            disp_type->pkt_queue[i] = (void *)qctx;
        }
        _frame_devider_init(&frame_devider[disp_type->type][i]);
        disp_type->frame_buffer[i] = calloc(1, 16384);
        printf_note("frame buffer: %p\n", disp_type->frame_buffer[i]);
    }

    return 0;
}


int spm_distributor_thread(void *dtype)
{
    pthread_t tid;
    int ret;
    struct spm_distributor_type_attr_s *disp_type = dtype;
    ret =  pthread_create_detach (NULL,
                _spm_distributor_thread_init, 
                disp_type->distributor_loop, 
                _spm_distributor_thread_exit,  
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
    args->fft_size = 512;
    args->ch = ch;
    spm_deal(spmctx, args, ch);
    sleep(1);
}

static int _spm_consumer_thread_init(void *args)
{
    sleep(3);
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
                _spm_consumer_thread_init, 
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


int spm_distributor_create(void)
{
    int ret = 0;
    for(int i = 0; i < ARRAY_SIZE(dist_type); i++){
        printf_note("type:%d, %p, %s\n", i, &dist_type[i], dist_type[i].name);
        spm_distributor_thread(&dist_type[i]);
        for(int ch = 0; ch < dist_type[i].channel_num; ch++){
            printf_note("ch:%d\n", ch);
            spm_distributor_fft_consumer_thread(ch, i);
        }
    }
    return ret;
}
