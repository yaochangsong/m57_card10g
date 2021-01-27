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

#define MAX_FFT_SIZE  (64768)

enum stream_type {
    STREAM_IQ = 0,
    STREAM_FFT,
    STREAM_ADC_WRITE,
    STREAM_ADC_READ,
};

#define DMA_IQ_TYPE_BUFFER_SIZE (DMA_IQ_BUFFER_SIZE*2)
enum stream_iq_type {
    STREAM_IQ_TYPE_AUDIO,
    STREAM_IQ_TYPE_RAW,
    STREAM_IQ_TYPE_MAX,
};
#define for_each_iq_type(type, run) \
    for (int i = 0; \
            type = i, run.dis_iq.send_ptr = run.dis_iq.ptr[i],run.dis_iq.send_len = run.dis_iq.offset[i]*sizeof(iq_t), i < STREAM_IQ_TYPE_MAX; \
            i++)

struct spm_backend_ops {
    int (*create)(void);
    ssize_t (*read_iq_data)(void **);
    ssize_t (*read_fft_data)(void **, void*);
    ssize_t (*read_adc_data)(int,void **);
    int (*read_adc_over_deal)(int,void *);
    int (*read_iq_over_deal)(void *);
    fft_t *(*data_order)(fft_t *, size_t,  size_t *, void *);
    int (*send_fft_data)(void *, size_t, void *);
    int (*send_iq_data)(void *, size_t, void *);
    int (*send_cmd)(void *, void *, size_t, void *);
    int (*send_iq_type)(int, char *, size_t, void *);
    int (*iq_dispatcher)(iq_t *, size_t, void *);
    /* AGC自动增益控制*/
    int (*agc_ctrl)(int, void *);
    /* 获取驻留时间是否到达：根据通道驻留策略和是否有信号 */
    bool (*residency_time_arrived)(uint8_t, int, bool);
    /* 获取某通道是否有信号；并返回信号强度 */
    int32_t (*signal_strength)(uint8_t ch,uint8_t subch, uint32_t, bool *is_singal, uint16_t *strength);
    int (*back_running_file)(int, uint8_t, char *);
    int (*stream_start)(int, int, uint32_t ,uint8_t , int);
    int (*stream_stop)(int, int, uint8_t);
    int (*sample_ctrl)(void *);
    int (*spm_scan)(uint64_t *, uint64_t* , uint32_t* , uint32_t *, uint64_t *);
    int (*set_calibration_value)(int);
    void (*set_smooth_time)(int32_t);
    int (*set_flush_trigger)(bool);
    int (*close)(void *);
    
};


#include "platform.h"
struct spm_context {
    struct poal_config *pdata;
    const struct spm_backend_ops *ops;
    struct spm_run_parm *run_args[MAX_RADIO_CHANNEL_NUM];
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

#endif
