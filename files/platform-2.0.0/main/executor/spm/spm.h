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
*  Rev 1.0   21 Feb. 2020   yaochangsong
*  Initial revision.
******************************************************************************/
#ifndef _SPM_H_
#define _SPM_H_

typedef int16_t fft_t;
typedef int16_t iq_t;

#define MAX_FFT_SIZE  (65536)

enum stream_type {
    STREAM_NIQ = 0,
    STREAM_BIQ,
    STREAM_FFT,
    STREAM_ADC_WRITE,
    STREAM_ADC_READ,
};

#define DMA_IQ_TYPE_BUFFER_SIZE (DMA_IQ_BUFFER_SIZE*2)

enum stream_iq_type {
    STREAM_NIQ_TYPE_AUDIO,
    STREAM_NIQ_TYPE_RAW,
    STREAM_NIQ_TYPE_MAX,
    STREAM_BIQ_TYPE_RAW,
};

struct _spm_stream {
    char *devname;      /* 流设备节点名称 */
    int id;             /* 频谱流类型描述符 */
    int ch;             /* 主通道，如果没有写-1 */
    uint8_t *ptr;       /* 频谱流数据buffer指针 */
    uint32_t len;       /* 频谱流数据buffer长度 */
    char *name;         /* 频谱流名称 */
    int rd_wr;
    enum stream_type type;
};

#define for_each_niq_type(type, run) \
    for (int i = 0; \
            type = i, run.dis_iq.send_ptr = run.dis_iq.ptr[i],run.dis_iq.send_len = run.dis_iq.offset[i]*sizeof(iq_t), i < STREAM_NIQ_TYPE_MAX; \
            i++)


struct spm_backend_ops {
    int (*create)(void);
    ssize_t (*read_niq_data)(void **);
    ssize_t (*read_biq_data)(int ch, void **data);
    ssize_t (*read_fft_data)(int, void **, void*);
    ssize_t (*read_adc_data)(int,void **);
    int (*read_adc_over_deal)(int,void *);
    int (*read_niq_over_deal)(void *);
    fft_t *(*data_order)(fft_t *, size_t,  size_t *, void *);
    int (*send_fft_data)(void *, size_t, void *);
    int (*send_biq_data)(int, void *, size_t, void *);
    int (*send_niq_data)(void *, size_t, void *);
    int (*send_niq_type)(enum stream_iq_type, iq_t *, size_t, void *);
    int (*niq_dispatcher)(iq_t *, size_t, void *);
    int (*agc_ctrl)(int, void *);
    bool (*residency_time_arrived)(uint8_t, int, bool);
    int32_t (*signal_strength)(uint8_t ch,uint8_t subch, uint32_t, bool *is_singal, uint16_t *strength);
    int (*back_running_file)(int, enum stream_type, int);
    int (*stream_start)(int, int, uint32_t ,uint8_t , enum stream_type);
    int (*stream_stop)(int, int, enum stream_type);
    int (*sample_ctrl)(void *);
    int (*spm_scan)(uint64_t *, uint64_t* , uint32_t* , uint32_t *, uint64_t *);
    int (*set_calibration_value)(int);
    void (*set_smooth_time)(int32_t);
    int (*set_flush_trigger)(bool);
    int (*close)(void *);
    
};


#include "config.h"
struct spm_context {
    struct poal_config *pdata;
    const struct spm_backend_ops *ops;
    struct spm_run_parm *run_args[MAX_RADIO_CHANNEL_NUM];
    void *pool_buffer;
};

extern pthread_mutex_t send_fft_mutex;
extern pthread_mutex_t send_fft2_mutex;
extern pthread_mutex_t send_iq_mutex;

#define __lock_fft_send__() do {           \
        pthread_mutex_lock(&send_fft_mutex); \
} while (0)

#define __unlock_fft_send__() do { \
    pthread_mutex_unlock(&send_fft_mutex); \
} while (0)

#define __lock_fft2_send__() do { \
    pthread_mutex_lock(&send_fft2_mutex); \
} while (0)

#define __unlock_fft2_send__() do { \
    pthread_mutex_unlock(&send_fft2_mutex); \
} while (0)


#define __lock_iq_send__() do { \
    pthread_mutex_lock(&send_iq_mutex); \
} while (0)

#define __unlock_iq_send__() do { \
    pthread_mutex_unlock(&send_iq_mutex); \
} while (0)

#define __lock_send__() do { \
    __lock_fft_send__(); \
    __lock_fft2_send__(); \
    __lock_iq_send__(); \
} while (0)

#define __unlock_send__() do { \
    __unlock_fft_send__(); \
    __unlock_fft2_send__(); \
    __unlock_iq_send__(); \
} while (0)
extern void *spm_init(void);
extern struct spm_context *get_spm_ctx(void);
extern void spm_deal(struct spm_context *ctx, void *args, int ch);
extern int spm_close(void);
extern void spm_niq_deal_notify(void *arg);
extern void spm_biq_deal_notify(void *arg);


#endif
