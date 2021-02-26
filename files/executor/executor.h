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
*  Rev 1.0   09 July 2019   yaochangsong
*  Initial revision.
******************************************************************************/
#ifndef _EXECUTOR_H_H
#define _EXECUTOR_H_H

/* executor:command*/
typedef enum {
    EX_MID_FREQ_CMD,           /*中频参数命令*/
    EX_RF_FREQ_CMD,            /*射频参数命令*/
    EX_ENABLE_CMD,             /*使能命令*/
    EX_FFT_ENABLE_CMD,         /*FFT工作使能命令*/
    EX_BIQ_ENABLE_CMD,         /*宽带IQ工作使能命令*/
    EX_NIQ_ENABLE_CMD,         /*窄带IQ工作使能命令*/
    EX_STATUS_CMD,             /*状态参数命令*/
    EX_WORK_MODE_CMD,          /*工作模式命令*/
    EX_NETWORK_CMD,            /*网络参数命令*/
    EX_CTRL_CMD,               /*控制参数命令*/
}exec_cmd;

/* executor: mid frequency paramter */
enum {
    EX_MUTE_SW=11,             /*静噪开关*/
    EX_MUTE_THRE,              /*静噪门限*/
    EX_MID_FREQ,               /*中心频率*/
    EX_BANDWITH,               /*带宽*/
    EX_DEC_METHOD,             /*解调方式*/
    EX_DEC_BW,                 /*解调带宽*/
    EX_DEC_MID_FREQ,           /* 解调中心频率 */
    EX_DEC_RAW_DATA,           /* 解调原始参数 */
    EX_SMOOTH_TIME,            /*平滑次数*/
    EX_RESIDENCE_TIME,         /*驻留时间*/
    EX_RESIDENCE_POLICY,       /*驻留策略*/
    EX_AUDIO_SAMPLE_RATE,      /*音频采样速率*/
    EX_FFT_SIZE,               /*FFT 点数*/
    EX_FRAME_DROP_CNT,         /*丢帧次数*/
    EX_CHANNEL_SELECT,         /*通道选择*/
    EX_FILL_RF_PARAM,          /*填充中频参数*/
    EX_FPGA_RESET,             /*FPGA复位*/
    EX_FPGA_CALIBRATE,         /*FPGA校准*/
    EX_SUB_CH_DEC_BW,          /* 子通道解调带宽 */
    EX_SUB_CH_MID_FREQ,        /* 子通道解调中心频率 */
    EX_SUB_CH_DEC_METHOD,      /* 子通道解调方式 */
    EX_SUB_CH_MUTE_THRE,       /* 子通道静噪门限 */
    EX_SUB_CH_ONOFF,           /* 子通道开关 */
    EX_SAMPLE_CTRL,            /*  0关闭直采，1开启直采*/
    EX_AUDIO_VOL_CTRL          /*  音频音量控制*/
};

/* executor: enable paramter */
enum {
    PSD_MODE_ENABLE=50,          /*PSD 数据使能*/
    PSD_MODE_DISABLE,
    AUDIO_MODE_ENABLE,           /*音频数据使能*/
    AUDIO_MODE_DISABLE,
    IQ_MODE_ENABLE,              /*IQ 数据使能*/
    IQ_MODE_DISABLE,
    BIQ_MODE_ENABLE,             /*宽带IQ 数据使能*/
    BIQ_MODE_DISABLE,
    SPCTRUM_MODE_ANALYSIS_ENABLE,/*频谱分析使能*/
    SPCTRUM_MODE_ANALYSIS_DISABLE,
    DIRECTION_MODE_ENABLE,       /*测向使能位*/
    DIRECTION_MODE_ENABLE_DISABLE,
    FREQUENCY_BAND_ENABLE_DISABLE,
    ADC_MODE_ENABLE,
    ADC_MODE_DISABLE,
};

/* executor: status paramter */
enum {
    EX_SW_VERSION=70,          /*软件版本*/
    EX_DISK_INFO,              /*磁盘信息*/
    EX_CLK_INFO,               /*时钟信息*/
    EX_AD_INFO,                /*AD 信息*/
    EX_RF_INFO,                /*射频信息*/
    EX_FPGA_INFO,              /*fpga 信息*/
    EX_FPGA_TEMPERATURE,       /*fpga 温度*/
    EX_CLK_STATUS,             /*时钟状态 */
    EX_AD_STATUS,              /*AD状态 */
    EX_DISK_STATUS,            /*DISK状态 */
    EX_CHECK_STATUS,           /*自检状态 */
};

/* executor: RF paramter */
enum {
    EX_RF_MID_FREQ,               /* 射频中心频率 */
    EX_RF_MID_BW,                 /*中频模拟带宽*/
    EX_RF_MODE_CODE,              /*模式码 0x00：低失真 0x01：常规 0x02：低噪声*/
    EX_RF_GAIN_MODE,              /*增益模式*/
    EX_RF_MGC_GAIN,               /*MGC 增益值*/
    EX_RF_AGC_CTRL_TIME,          /*AGC 控制时间*/
    EX_RF_AGC_OUTPUT_AMP,         /*AGC 中频输出幅度*/
    EX_RF_ANTENNA_SELECT,         /*天线选择*/
    EX_RF_ATTENUATION,            /*射频衰减*/
    EX_RF_STATUS_TEMPERAT,         /*射频温度*/
    EX_RF_AGC_FREQUENCY,          /*AGC 频率设置*/
    EX_RF_AGC_BW,                 /*AGC 带宽设置*/
    EX_RF_CALIBRATE,              /*校正*/
    EX_RF_SAMPLE_CTRL,             /* 直采控制 0关闭直采，1开启直采 */
    EX_RF_LOW_NOISE,              /* 低噪放 */
};

/* network paramters */
enum {
    EX_NETWORK,
    EX_NETWORK_1G,
    EX_NETWORK_10G,
    EX_NETWORK_IP,               /* IP地址 */
    EX_NETWORK_MASK,
    EX_NETWORK_GW,
    EX_NETWORK_PORT,
};

/* control paramters */
enum {
    EX_CTRL_LOCAL_REMOTE,        /* 本控远控 */
    EX_CTRL_CALIBRATION,         /* 校准控制 */
    EX_CTRL_TIMESET,             /* 时间设置 */
    EX_CTRL_SIDEBAND,            /* 边带率 */
    EX_CTRL_CALI_SIGAL_THRE,     /* 有无信号门限值 */
    EX_CTRL_IQ_DATA_LENGTH       /* IQ数据长度 */
};

#include "config.h"
struct sem_st{
    sem_t   notify_deal[MAX_RADIO_CHANNEL_NUM];        /* 开启或关闭使能后，通知线程处理相关逻辑 */
    sem_t   kernel_sysn;        /* 频谱分析时，内核处理完数据后，异步消息通知应用层 */
};

struct spm_iq_parm{
    uint64_t m_freq_hz;
    uint32_t bandwidth_hz;
    uint32_t sample_rate;
    uint8_t d_method;
    uint8_t resv1;
    uint8_t resv2;
    uint8_t resv3;
};
struct spm_dispatcher_iq{
    uint32_t len[STREAM_IQ_TYPE_MAX];
    uint32_t offset[STREAM_IQ_TYPE_MAX];
    iq_t *ptr[STREAM_IQ_TYPE_MAX];
    iq_t *send_ptr;
    size_t send_len;
};
/* 频谱运行中，可能变化的参数 */
struct spm_run_parm{
    uint32_t scan_bw;
    uint32_t bandwidth;         /*显示频谱带宽*/
    uint32_t fft_size;
    uint32_t fft_sn;
    uint32_t total_fft;
    uint32_t fregment_num;
    uint32_t sample_rate;
    uint32_t data_len;
    uint64_t s_freq;             /* 开始频率 */
    uint64_t s_freq_offset;      /* 开始频率偏移 */
    uint64_t e_freq;             /* 截止频率 */
    uint64_t m_freq;             /* 频谱显示中心频率 */
    uint64_t m_freq_s;           /* 频谱扫描实际中心频率 */
    float freq_resolution;       /* 分辨率 */
    uint8_t ch;
    uint8_t datum_type;          /* 0x00：字符型 0x01：短整型 0x02 浮点型 */
    uint8_t mode;
    uint8_t d_method;           /* 解调类型 */
    uint8_t type;               /* 数据类型： 频谱数据/IQ数据/音频数据 */
    uint8_t ex_type;            /* 扩展帧类型： 频谱帧/解调帧 */
    uint8_t gain_mode;
    int8_t gain_value;
    int8_t rf_mode;
    uint32_t sub_ch_index;      /* 子通道索引 */
    uint32_t audio_points;
    struct spm_iq_parm sub_ch_para; /* 子通道参数 */
    void *fft_ptr;              /* fft数据缓冲区 */
    void *fft_ptr_swap;         /* fft数据交换缓冲区,dma存储区存在拷贝数据对齐问题,需要将dma数据拷贝到交换区后再处理 */
    struct spm_dispatcher_iq dis_iq;
};




/*  define the command setting lock */
#define LOCK_SET_COMMAND() do { \
    pthread_mutex_lock(&set_cmd_mutex); \
} while (0)

#define UNLOCK_SET_COMMAND() do { \
    pthread_mutex_unlock(&set_cmd_mutex); \
} while (0)

#define delta_bw  MHZ(30)
#define middle_freq_resetting(bw, mfreq)    ((bw/2) >= (mfreq) ? (bw/2+delta_bw) : (mfreq))

#include "config.h"

extern void executor_init(void);
extern int executor_tcp_disconnect_notify(void *cl);
//extern int8_t executor_set_command(exec_cmd cmd, uint8_t type, uint8_t ch,  void *data);
extern int8_t executor_get_command(exec_cmd cmd, uint8_t type, uint8_t ch,  void *data, ...);
extern int8_t executor_set_enable_command(uint8_t ch);
extern int8_t executor_set_command(exec_cmd cmd, uint8_t type, uint8_t ch,  void *data, ...);
extern uint32_t executor_get_audio_point(uint8_t ch);
extern uint64_t executor_get_mid_freq(uint8_t ch);
extern void executor_close(void);
extern void udp_free(struct net_udp_client *cl);
extern int8_t  executor_serial_points_scan(uint8_t ch, work_mode_type mode, void *args);
extern int8_t  executor_points_scan(uint8_t ch, work_mode_type mode, void *args);
extern int8_t  executor_fragment_scan(uint32_t fregment_num,uint8_t ch, work_mode_type mode, void *args);

#endif
