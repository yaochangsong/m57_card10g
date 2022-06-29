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


#define XDMA_R_DEV0 "/dev/xdma0_h2c_0"  //下行
#define XDMA_R_DEV1 "/dev/xdma0_c2h_0"  //上行
#define XDMA_R_DEV2 "/dev/xdma0_c2h_1"
#define XDMA_R_DEV3 "/dev/xdma0_c2h_3"


static struct _spm_xstream spm_stream[] = {
    {XDMA_R_DEV1,      -1, 0, XDMA_BUFFER_SIZE, XDMA_BLOCK_SIZE,  "FFT Stream", DMA_READ, STREAM_FFT, -1},
    {XDMA_R_DEV2,      -1, 1, XDMA_BUFFER_SIZE, XDMA_BLOCK_SIZE,  "NIQ Stream", DMA_READ, STREAM_NIQ, -1},
};

struct _spm_xstream* spm_dev_get_stream(int *count)
{
    if(count)
        *count = ARRAY_SIZE(spm_stream);
    return spm_stream;
}

static struct gpio_node_info gpio_node[] ={
    /* pin   direction  default gpio value   func_code    func_name    fd */
//    {3,      "out",        0,               GPIO_LOCAL_CTR,   "GPIO LOCAL REMOTE",                 -1 },  //通道射频电源控制
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
 //  {UART_LCD_CODE, "/dev/ttyPS1", 115200,   "LCD",   true, -1},
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
    {196688, 0,  0, 500000},
    {196648, 0,  0, 1000000},
    {196628, 0,  0, 2000000},
    {196618, 0,  0, 4000000},
    {196612, 0,  0, 10000000},
    {458753, 0,  0, 20000000},
    {196609, 0,  0, 40000000},
    {65537, 0,  0, 80000000},
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


