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
    EX_NETWORK_CMD             /*网络参数命令*/
}exec_cmd;

/* executor: mid frequency paramter */
enum {
    EX_MUTE_SW=11,                /*静噪开关*/
    EX_MUTE_THRE=12,              /*静噪门限*/
    EX_MID_FREQ=13,               /*中心频率*/
    EX_BANDWITH=14,               /*带宽*/
    EX_DEC_METHOD=15,             /*解调方式*/
    EX_DEC_BW=16,                 /*解调带宽*/
    EX_SMOOTH_TIME=17,            /*平滑次数*/
    EX_RESIDENCE_TIME=18,         /*驻留时间*/
    EX_RESIDENCE_POLICY=19,       /*驻留策略*/
    EX_AUDIO_SAMPLE_RATE=20,     /*音频采样速率*/
    EX_FFT_SIZE=21,              /*FFT 点数*/
    EX_FRAME_DROP_CNT=22,        /*丢帧次数*/
    EX_CHANNEL_SELECT=23,        /*通道选择*/
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
    PSD_MODE_ENABLE=50,             /*PSD 数据使能*/
    PSD_MODE_DISABLE=51,
    AUDIO_MODE_ENABLE=52,           /*音频数据使能*/
    AUDIO_MODE_DISABLE=53,
    IQ_MODE_ENABLE=54,              /*IQ 数据使能*/
    IQ_MODE_DISABLE=55,
    SPCTRUM_MODE_ANALYSIS_ENABLE=56,/*频谱分析使能*/
    SPCTRUM_MODE_ANALYSIS_DISABLE=57,
    DIRECTION_MODE_ENABLE=58,       /*测向使能位*/
    DIRECTION_MODE_ENABLE_DISABLE=59,
};

/* executor: status paramter */
enum {
    EX_SW_VERSION=70,             /*软件版本*/
    EX_DISK_INFO=71,              /*磁盘信息*/
    EX_CLK_INFO=72,               /*时钟信息*/
    EX_AD_INFO=73,                /*AD 信息*/
    EX_RF_INFO=74,                /*射频信息*/
    EX_FPGA_INFO=75,              /*fpga 信息*/
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
};

struct sem_st{
    sem_t   notify_deal;
    sem_t   kernel_sysn;
};

extern void executor_init(void);
extern int8_t executor_set_command(exec_cmd cmd, uint8_t type,  void *data);

#endif
