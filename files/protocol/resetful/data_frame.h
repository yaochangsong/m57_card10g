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
*  Rev 1.0   30 march 2020   yaochangsong
*  Initial revision.
******************************************************************************/
#ifndef _DATA_FRAME_H
#define _DATA_FRAME_H

struct data_frame_header{
    uint32_t sysn;
    //time_t timestamp;
    uint32_t timestamp;
    uint32_t time_ns;
    #define XW_PROTOCAL_VERSION 1
    uint32_t version    : 8;
    uint32_t resv       : 24;
    //uint32_t time_counter;
    uint16_t ex_frame_type;
#define  DFH_EX_TYPE_PSD   0x00
#define  DFH_EX_TYPE_DEMO  0x01
    uint16_t ex_frame_header_len;
}__attribute__ ((packed));

struct data_ex_frame_psd_head{
    uint8_t ch;
    uint8_t mode;
    uint8_t gain_mode;  /* 0: 手动; 1:自动 */
    int8_t   gain_value; 
    uint32_t power    : 8;
    uint32_t resv     : 24;
    uint64_t start_freq_hz;
    uint64_t end_freq_hz;
    uint64_t mid_freq_hz;
    uint32_t bandwidth;
    uint32_t freq_resolution;
    uint32_t sn;
    uint16_t fft_len;
    uint16_t data_type;
#define  DEFH_DTYPE_CHAR   0x00
#define  DEFH_DTYPE_SHORT  0x02
#define  DEFH_DTYPE_FLOAT  0x04
    uint32_t data_len;
}__attribute__ ((packed));

struct data_ex_frame_demodulation_head{
    uint8_t ch;
    uint8_t mode;
    uint8_t gain_mode;  /* 0: 手动; 1:自动 */
    int8_t   gain_value; 
    uint32_t power    : 8;
    uint32_t resv     : 24;
    uint64_t mid_freq_hz;
    uint32_t bandwidth;
    uint32_t sn;
    uint16_t demodulate_type;
    uint16_t data_type;
#define  DEFH_DTYPE_BB_IQ   0x04
#define  DEFH_DTYPE_CH_IQ   0x06
#define  DEFH_DTYPE_AUDIO   0x08
    uint32_t data_len;
}__attribute__ ((packed));

extern uint8_t * xw_assamble_frame_data(uint32_t *len, void *args);
#endif

