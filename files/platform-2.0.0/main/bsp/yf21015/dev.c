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
*  Rev 1.0   24 June. 2021   yaochangsong
*  Initial revision.
******************************************************************************/
#include "dev.h"

#define DMA_FFT0_DEV   "/dev/dma_fft0"
#define DMA_FFT1_DEV   "/dev/dma_fft1"
#define DMA_BIQ_DEV    "/dev/dma_biq"
#define DMA_NIQ_DEV    "/dev/dma_niq"

static struct _spm_stream spm_stream[] = {
    {DMA_BIQ_DEV,     -1,-1, NULL, DMA_IQ_BUFFER_SIZE,  "BIQ0 Stream",    DMA_READ, STREAM_BIQ},
    {DMA_FFT0_DEV,    -1, 0, NULL, DMA_BUFFER_16M_SIZE, "FFT0 Stream",    DMA_READ, STREAM_FFT},
    {DMA_FFT1_DEV,    -1, 1, NULL, DMA_BUFFER_16M_SIZE, "FFT1 Stream",    DMA_READ, STREAM_FFT},
    {DMA_NIQ_DEV,     -1,-1, NULL, DMA_IQ_BUFFER_SIZE,  "NIQ Stream",     DMA_READ, STREAM_NIQ},
};


struct _spm_stream* spm_dev_get_stream(int *count)
{
    if(count)
        *count = ARRAY_SIZE(spm_stream);
    return spm_stream;
}


static struct gpio_node_info gpio_node[] ={
    /* pin   direction  default gpio value   func_code    func_name    fd */
    {24,     "out",        1,               GPIO_RF_POWER_ONOFF,    "Rf Power On/Off  gpio ctrl",   -1 },
    {25,     "out",        0,               GPIO_GPS_LOCK,          "GPS Locked gpio ctrl",         -1 },
};

struct gpio_node_info* dev_get_gpio(int *count)
{
    if(count)
        *count = ARRAY_SIZE(gpio_node);
    return gpio_node;
}

struct uart_info_t uartinfo[] = {
    /* NOTE: ttyUL*为PL侧控制，波特率由PL侧设置（默认为9600，不可更改），需要更改FPGA */
   {UART_GPS_CODE, "/dev/ttyUL1", 9600,   "gps",   false},
};

struct uart_info_t* dev_get_uart(int *count)
{
    if(count)
        *count = ARRAY_SIZE(uartinfo);
    return uartinfo;
}

/* 窄带表：解调方式为IQ系数表 */
static struct  band_table_t iq_nbandtable[] ={
    {986373,  0,  0,   3000},
    {462085,  0,  0,   6000},
    {460974,  0,  0,   9000},
    {199941,  0,  0,   12000},
    {199274,  0,  0,   15000},
    {197941,  0,  0,   30000},
    {197408,  0,  0,   50000},
    {197008,  0,  0,   100000},
    {196874,  0,  0,   150000},
    {196808,  0,  0,   200000},
    {196688,  0,  0,   500000},

}; 

/* 窄带表：解调方式为非IQ系数表 */
static struct  band_table_t nbandtable[] ={
    {198208, 129,  0,   3000},
    {198208, 131,  0,   6000},
    {198208, 133,  0,   9000},
    {198208, 135,  0,   12000},
    {198208, 0,    0,   15000},
    {197408, 0,    6,   30000},
    {197008, 135,  5,   50000},
    {196808, 135,  3,   100000},
    {196708, 133,  2,   150000},
    {196708, 135,  2,   200000},
    {196658, 0,    1,   500000},
}; 

/* 宽带系数表 */
static struct  band_table_t bandtable[] ={
    {196632, 0,  0,   1250000},
    {196620, 0,  0,   2500000},
    {196614, 0,  0,   5000000},
    {65542,  0,  0,   10000000},
    {1114112, 0, 0,   20000000},
    {1048576, 0, 0,   40000000},
    {65536, 0,  0,   60000000},
}; 

struct band_table_t* broadband_factor_table(int *count)
{
    if(count)
        *count = ARRAY_SIZE(bandtable);
    return bandtable;
}

struct band_table_t* narrowband_demo_factor_table(int *count)
{
    if(count)
        *count = ARRAY_SIZE(nbandtable);
    return nbandtable;
}

struct band_table_t* narrowband_iq_factor_table(int *count)
{
    if(count)
        *count = ARRAY_SIZE(iq_nbandtable);
    return iq_nbandtable;
}



