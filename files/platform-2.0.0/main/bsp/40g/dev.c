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

#define DMA_FFT_DEV   "/dev/dma_fft"
#define DMA_BIQ_DEV  "/dev/dma_biq"
#define DMA_NIQ_DEV   "/dev/dma_niq"


static struct _spm_stream spm_stream[] = {
    {DMA_BIQ_DEV,     - 1,-1, NULL, DMA_IQ_BUFFER_SIZE,  "BIQ0 Stream",      DMA_READ, STREAM_BIQ},
    {DMA_FFT_DEV,      -1,-1, NULL, DMA_BUFFER_16M_SIZE, "FFT0 Stream",    DMA_READ, STREAM_FFT},
    {DMA_NIQ_DEV,      -1,-1, NULL, DMA_IQ_BUFFER_SIZE,  "NIQ Stream",     DMA_READ, STREAM_NIQ},
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
    {985040, 0, 0, 5000},
    {198208, 0, 0, 25000},
    {197408, 0, 0, 50000},
    {197008, 0, 0, 100000},
    {196874, 0, 0, 150000},
    {196768, 0, 0, 250000},
    {196688, 0, 0, 500000},
    {196648, 0, 0, 1000000},
    {196624, 0, 0, 2500000},
    {196616, 0, 0, 5000000},
    {196612, 0, 0, 10000000},
    {458752, 0, 0, 20000000},
    {196608, 0, 0, 40000000},
}; 

/* 窄带表：解调方式为非IQ系数表 */
static struct  band_table_t nbandtable[] ={
    {198208, 128,   0, 600},
    {198208, 128,   0, 1500},
    {198208, 129,   0, 2400},
    {198208, 131,   0, 6000},
    {198208, 0,     0, 9000},
    {198208, 0,     0, 12000},
    {198208, 0,     0, 15000},
    {197408, 0,     0, 30000},
    {197008, 0,     0, 50000},
    {196808, 0,     0, 100000},
    {196708, 0,     0, 150000},
}; 
/* 宽带系数表 */
static struct  band_table_t bandtable[] ={
    {196648, 0,  0, 1000000},
    {196628, 0,  0, 2000000},
    {196616, 0,  0, 5000000},
    {196612, 0,  0, 10000000},
    {458752, 0,  0, 20000000},
    {196608, 0,  0, 40000000},
    {65536,  0,  0, 60000000},
    {65536,  0,  0, 80000000},
    {65536,  0,  0, 120000000},
    {0,      0,  0, 160000000},
    {0,      0,  0, 175000000},
    {0,      0,  0, 200000000},

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
