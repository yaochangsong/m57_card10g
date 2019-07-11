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
}exec_cmd;

/* executor: mid frequency paramter */
enum {
    EX_MUTE_SW,                /*静噪开关*/
    EX_MUTE_THRE,              /*静噪门限*/
    EX_MID_FREQ,               /*中心频率*/
    EX_BANDWITH,               /*中心频率*/
    EX_DEC_METHOD,             /*解调方式*/
    EX_DEC_BW,                 /*解调带宽*/
    EX_SMOOTH_TIME,            /*平滑次数*/
    EX_RESIDENCE_TIME,         /*驻留时间*/
    EX_RESIDENCE_POLICY,       /*驻留策略*/
    EX_AUDIO_SAMPLE_RATE,      /*音频采样速率*/
    EX_FFT_SIZE,               /*FFT 点数*/
    EX_FRAME_DROP_CNT,         /*丢帧次数*/
    EX_CHANNEL_SELECT,         /*通道选择*/
};

/* executor: RF paramter */
enum {
    EX_RF_MODE_CODE,              /*模式码 0x00：低失真 0x01：常规 0x02：低噪声*/
    EX_RF_GAIN_MODE,              /*增益模式*/
    EX_RF_MGC_GAIN,               /*MGC 增益值*/
    EX_RF_AGC_CTRL_TIME,          /*AGC 控制时间*/
    EX_RF_AGC_OUTPUT_AMP,         /*AGC 中频输出幅度*/
    EX_RF_MID_FREQ,               /*中频带宽*/
    EX_RF_ANTENNA_SELECT,         /*天线选择*/
    EX_RF_ATTENUATION,            /*射频衰减*/
};

/* executor: enable paramter */
enum {
    PSD_MODE_ENABLE,             /*PSD 数据使能*/
    PSD_MODE_DISABLE,
    AUDIO_MODE_ENABLE,           /*音频数据使能*/
    AUDIO_MODE_DISABLE,
    IQ_MODE_ENABLE,              /*IQ 数据使能*/
    IQ_MODE_DISABLE,
    SPCTRUM_MODE_ANALYSIS_ENABLE,/*频谱分析使能*/
    SPCTRUM_MODE_ANALYSIS_DISABLE,
    DIRECTION_MODE_ENABLE,       /*测向使能位*/
    DIRECTION_MODE_ENABLE_DISABLE,
};

/* executor: status paramter */
enum {
    EX_SW_VERSION,             /*软件版本*/
    EX_DISK_INFO,              /*磁盘信息*/
    EX_CLK_INFO,               /*时钟信息*/
    EX_AD_INFO,                /*AD 信息*/
    EX_RF_INFO,                /*射频信息*/
    EX_FPGA_INFO,              /*fpga 信息*/
};


extern int8_t executor_set_command(exec_cmd cmd, uint8_t type,  void *data);

#endif
