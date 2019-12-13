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
    EX_FPGA_RESET,             /*FPGA复位*/
    EX_FPGA_CALIBRATE,         /*FPGA校准*/
    EX_SUB_CH_DEC_BW,          /* 子通道解调带宽 */
    EX_SUB_CH_MID_FREQ,        /* 子通道解调中心频率 */
    EX_SUB_CH_ONOFF            /* 子通道开关 */
};

/* executor: work mode  */
typedef enum {
    EX_NULL_MODE            =0x00,
    EX_FIXED_FREQ_ANYS_MODE =0x01,      /*定频模式*/
    EX_FAST_SCAN_MODE       =0x02,      /*快速扫描模式*/
    EX_MULTI_ZONE_SCAN_MODE =0x03,      /*多频段模式*/
    EX_MULTI_POINT_SCAN_MODE=0x04,      /*多频点模式*/
}work_mode;


/* executor: enable paramter */
enum {
    PSD_MODE_ENABLE=50,          /*PSD 数据使能*/
    PSD_MODE_DISABLE,
    AUDIO_MODE_ENABLE,           /*音频数据使能*/
    AUDIO_MODE_DISABLE,
    IQ_MODE_ENABLE,              /*IQ 数据使能*/
    IQ_MODE_DISABLE,
    SPCTRUM_MODE_ANALYSIS_ENABLE,/*频谱分析使能*/
    SPCTRUM_MODE_ANALYSIS_DISABLE,
    DIRECTION_MODE_ENABLE,       /*测向使能位*/
    DIRECTION_MODE_ENABLE_DISABLE,
    FREQUENCY_BAND_ENABLE_DISABLE,
};

/* executor: status paramter */
enum {
    EX_SW_VERSION=70,          /*软件版本*/
    EX_DISK_INFO,              /*磁盘信息*/
    EX_CLK_INFO,               /*时钟信息*/
    EX_AD_INFO,                /*AD 信息*/
    EX_RF_INFO,                /*射频信息*/
    EX_FPGA_INFO,              /*fpga 信息*/
    EX_FPGA_TEMPERATURE        /*fpga 温度*/
};

/* executor: RF paramter */
enum {
    EX_RF_MID_FREQ,               /* 射频中心频率 */
    EX_RF_MID_BW,                 /*中频带宽*/
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
};

/* network paramters */
enum {
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
};

struct sem_st{
    sem_t   notify_deal;        /* 开启或关闭使能后，通知线程处理相关逻辑 */
    sem_t   kernel_sysn;        /* 频谱分析时，内核处理完数据后，异步消息通知应用层 */
};

/* 频谱或IQ数据部分变量参数结构体，用作变化的参数传递 */
struct spectrum_header_param{
    uint32_t scan_bw;
    uint32_t bandwidth;
    uint32_t fft_size;
    uint32_t fft_sn;
    uint32_t total_fft;
    uint64_t s_freq;             /* 开始频率 */
    uint64_t e_freq;             /* 截止频率 */
    uint64_t m_freq;             /* 中心频率 */
    float freq_resolution;       /* 分辨率 */
    uint8_t ch;
    uint8_t datum_type;          /* 0x00：字符型 0x01：短整型 0x02 浮点型 */
    work_mode mode;
    uint32_t data_len;
    uint8_t d_method;           /* 解调类型 */
    uint8_t type;               /* 数据类型： 频谱数据/IQ数据/音频数据 */
    uint8_t ex_type;            /* 扩展帧类型： 频谱帧/解调帧 */
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
#ifdef SUPPORT_PROTOCAL_AKT
    #if defined(SUPPORT_SPECTRUM_KERNEL)
    /* use kernel space to deal data(fft, iq) and send to client */
    #define executor_assamble_header_response_data_cb  akt_assamble_data_extend_frame_header_data
    /* use user space to send basic config parameter (TCP) */
    #define executor_send_data_to_clent_cb             executor_send_config_data_to_clent
    #else  
    /* use user space to deal data(fft, iq) and send to client */
    #define executor_assamble_header_response_data_cb  akt_assamble_data_frame_header_data
    /* */
    #define executor_send_data_to_clent_cb              executor_send_config_data_to_clent
    #endif
#else 
#define executor_assamble_header_response_data_cb   
#define executor_send_data_to_clent_cb              
#endif


extern struct sem_st work_sem;
extern void executor_init(void);
//extern int8_t executor_set_command(exec_cmd cmd, uint8_t type, uint8_t ch,  void *data);
extern int8_t executor_get_command(exec_cmd cmd, uint8_t type, uint8_t ch,  void *data);
extern int8_t executor_set_enable_command(uint8_t ch);
extern int8_t executor_set_command(exec_cmd cmd, uint8_t type, uint8_t ch,  void *data, ...);

#endif
