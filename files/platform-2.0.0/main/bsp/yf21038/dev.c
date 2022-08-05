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
*  Rev 1.0   29 June. 2022   yaochangsong
*  Initial revision.
******************************************************************************/
#include "dev.h"

#define DMA_FFT0_DEV    "/dev/dma_fft0"
#define DMA_BIQ_RX_DEV  "/dev/dma_biq_rx"
#define DMA_NIQ_DEV     "/dev/dma_niq"



static struct _spm_stream spm_stream[] = {
    {0, DMA_FFT0_DEV, DMA_READ, NULL, DMA_BUFFER_64M_SIZE, "FFT0 Stream", -1, STREAM_FFT},   //共用一个dma
    {1, DMA_NIQ_DEV,  DMA_READ, NULL, DMA_BUFFER_16M_SIZE, "NIQ Stream",  -1, STREAM_NIQ},
};

struct _spm_stream* spm_dev_get_stream(int *count)
{
    if(count)
        *count = ARRAY_SIZE(spm_stream);
    return spm_stream;
}

static struct gpio_node_info gpio_node[] ={
    /* pin   direction  default gpio value   func_code    func_name    fd */
    {52,      "out",        1,               -1,   "GPIO DIR1",                 -1 },  
    {53,      "out",        0,               -1,   "GPIO_OE_N1",                 -1 },  
    {54,      "out",        1,               -1,   "GPIO_DIR2",                 -1 },  
    {55,      "out",        0,               -1,   "GPIO_OE_N2",                 -1 }, 
    {56,      "out",        0,               -1,   "GPIO_DIR3",                 -1 },  
    {57,      "out",        0,               -1,   "GPIO_OE_N3",                 -1 },

};

struct gpio_node_info* dev_get_gpio(int *count)
{
    if(count)
        *count = ARRAY_SIZE(gpio_node);
    return gpio_node;
}


struct uart_info_t uartinfo[] = {
    /* NOTE: ttyUL*为PL侧控制，波特率由PL侧设置（默认为9600，不可更改），需要更改FPGA */
 //  {UART_GPS_CODE, "/dev/ttyUL2", 9600,   "gps",   false},
   {UART_LCD_CODE, "/dev/ttyPS1", 115200,   "LCD",   true, -1},
};

struct uart_info_t* dev_get_uart(int *count)
{
    if(count)
        *count = ARRAY_SIZE(uartinfo);
    return uartinfo;
}


/* 窄带表：解调方式为IQ系数表 */
static struct  band_table_t iq_nbandtable[] ={
    {986240,  0,   0, 3125},
    {461952,  0,   0, 6250},
    {199808,  0,   0, 12500},
    {198208,  0,   0, 25000},
    {197408,  0,   0, 50000},
    {197008,  0,   0, 100000},
    {196768,  0,   0, 250000},
    {196688,  0,   0, 500000},
    {458772,  0,   0, 1000000},
    {196628,  0,   0, 2000000},
    {65552,  0,   0, 5000000},
    {65544,  0,   0, 10000000},
    {8,  0,   0, 20000000},
    {4,  0,   0, 40000000},
}; 


/* 窄带表：解调方式为非IQ系数表 */
static struct  band_table_t nbandtable[] ={
    {198208,  128,   0, 600},
    {198208,  128,   0, 1500},
    {198208,  129,   0, 2400},
    {198208,  131,   0, 6000},
    {198208,  133,   0, 9000},
    {198208,  0,   0, 15000},
    {197408,  0,   6, 25000},
    {197008,  0,   5, 50000},
    {196808,  0,   3, 100000},
    {196768,  0,   3, 150000},
}; 

/* 宽带系数表 */
static struct  band_table_t bandtable[] ={
    {0x70014, 0,  0, 500000},
    {0x7000a, 0,  0, 1000000},
    {0x70005, 0,  0, 2000000},
    {0x30004, 0,  0, 5000000},
    {0x70001, 0,  0, 10000000},
    {0x30001, 0,  0, 20000000},

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
        *count = ARRAY_SIZE(nbandtable);
    return nbandtable;
}


