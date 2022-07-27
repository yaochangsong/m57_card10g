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
#ifndef _SPM_DISTRIBUTOR_H_
#define _SPM_DISTRIBUTOR_H_

#define MAX_FRAME_BUFFER_LEN (65536)
/* Type of distribution */
enum spm_distributor_type {
    SPM_DIST_FFT = 0,
    SPM_DIST_IQ,
    SPM_DIST_MAX,
};

/* 分发数据统计 */
typedef struct spm_distributor_statistics{
    volatile uint64_t read_bytes;           /* 读取字节数 */
    volatile uint64_t read_ok_pkts;         /* 读取成功包数 */
    volatile uint64_t read_ok_frame;        /* 读取成功帧数 */
    volatile uint32_t read_ok_speed_fps;    /* 读取帧速度，帧/s */
    volatile uint32_t read_speed_bps;       /* 读取字节速度bps */
    volatile float loss_rate;               /* 转发失败率 */
    volatile uint64_t loss_bytes;           /* 转发失败字节数 */
}spm_dist_statistics_t;


struct spm_distributor_ops {
    int (*init)(void*);
    int (*reset)(int, int);
    int (*fft_prod)(void);
    int (*fft_consume)(int, void **, size_t);
    void *(*get_fft_statistics)(void);
    int (*close)(void *);
};

typedef struct spm_distributor_ctx {
    const struct spm_distributor_ops *ops;
}spm_distributor_ctx_t;

extern spm_distributor_ctx_t * spm_create_distributor_ctx(void);
extern int spm_distributor_fft_data_frame_producer(int ch, void **data, size_t len);

#endif
