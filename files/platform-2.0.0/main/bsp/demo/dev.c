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

#ifdef CONFIG_SPM_SOURCE_XDMA
#define XDMA_R_DEV0 "/dev/xdma0_h2c_0"  //下行
#define XDMA_R_DEV1 "/dev/xdma0_c2h_0"  //上行
#define XDMA_R_DEV2 "/dev/xdma0_c2h_1"
#define XDMA_R_DEV3 "/dev/xdma0_c2h_3"
static struct _spm_xstream spm_stream[] = {
    {XDMA_R_DEV1,      -1, 0, XDMA_BUFFER_SIZE, XDMA_BLOCK_SIZE,  "FFT Stream", DMA_READ, STREAM_FFT, -1},
    {XDMA_R_DEV2,      -1, 1, XDMA_BUFFER_SIZE, XDMA_BLOCK_SIZE,  "NIQ Stream", DMA_READ, STREAM_NIQ, -1},
    {XDMA_R_DEV0,      -1, 1, XDMA_BUFFER_SIZE, XDMA_BLOCK_SIZE,  "XDMA Stream", DMA_WRITE, STREAM_NIQ, -1},
};
#else
#define DMA_FFT0_DEV "/dev/dma_fft"
#define DMA_FFT1_DEV "/dev/dma_fft2"
#define DMA_NIQ_DEV  "/dev/dma_iq"
#define DMA_ADC_TX0_DEV "/dev/dma_adc_tx"
#define DMA_ADC_RX0_DEV "/dev/dma_adc_rx"
#define DMA_ADC_TX1_DEV "/dev/dma_adc_tx2"
#define DMA_ADC_RX1_DEV "/dev/dma_adc_rx2"

static struct _spm_stream spm_stream[] = {
    {DMA_NIQ_DEV,       -1,-1, NULL, DMA_IQ_BUFFER_SIZE, "NIQ Stream",      DMA_READ, STREAM_NIQ},
    {DMA_FFT0_DEV,      -1, 0, NULL, DMA_BUFFER_16M_SIZE, "FFT0 Stream",    DMA_READ, STREAM_FFT},
    {DMA_FFT1_DEV,      -1, 1, NULL, DMA_BUFFER_16M_SIZE, "FFT1 Stream",    DMA_READ, STREAM_FFT},
    {DMA_ADC_TX0_DEV,   -1, 0, NULL, DMA_BUFFER_64M_SIZE, "ADC Tx0 Stream", DMA_WRITE, STREAM_ADC_WRITE},
    {DMA_ADC_TX1_DEV,   -1, 1, NULL, DMA_BUFFER_64M_SIZE, "ADC Tx1 Stream", DMA_WRITE, STREAM_ADC_WRITE},
    {DMA_ADC_RX0_DEV,   -1, 0, NULL, DMA_BUFFER_64M_SIZE, "ADC Rx0 Stream", DMA_READ, STREAM_ADC_READ},
    {DMA_ADC_RX1_DEV,   -1, 1, NULL, DMA_BUFFER_64M_SIZE, "ADC Rx1 Stream", DMA_READ, STREAM_ADC_READ},
};
#endif

#ifdef CONFIG_SPM_SOURCE_XDMA
struct _spm_xstream* spm_dev_get_stream(int *count)
#else
struct _spm_stream* spm_dev_get_stream(int *count)
#endif
{
    if(count)
        *count = ARRAY_SIZE(spm_stream);
    return spm_stream;
}


static struct gpio_node_info gpio_node[] ={
    /* pin   direction  default gpio value   func_code    func_name    fd */
    {24,     "out",        1,               GPIO_RF_POWER_ONOFF,    "Rf Power On/Off  gpio ctrl",   -1 },
    {25,     "out",        0,               GPIO_GPS_LOCK,          "GPS Locked gpio ctrl",         -1 },
    {4,      "out",        0,               GPIO_FUNC_LOW_NOISER,   "RS485 0 ctrl",                 -1 },
    {5,      "out",        0,               GPIO_FUNC_COMPASS2,     "RS485 1 ctrl",                 -1 },
    {6,      "out",        0,               GPIO_FUNC_COMPASS1,     "RS485 2 ctrl",                 -1 },
};

struct gpio_node_info* dev_get_gpio(int *count)
{
    if(count)
        *count = ARRAY_SIZE(gpio_node);
    return gpio_node;
}

#ifdef CONFIG_DEVICE_UART
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
#endif

/* 窄带表：解调方式为IQ系数表 */
static struct  band_table_t iq_nbandtable[] ={
    {985540, 0, 0, 5000},
    {198608, 0, 0, 25000},
    {197608, 0, 0, 50000},
    {197108, 0, 0, 100000},
    {196941, 0, 0, 150000},
    {196808, 0, 0, 250000},
    {196708, 0, 0, 500000},
    {196658, 0, 0, 1000000},
    {196628, 0, 0, 2500000},
    {196618, 0, 0, 5000000},
    {196613, 0, 0, 10000000},
    {65541,  0, 0, 20000000},
    {5,      0, 0, 40000000},
}; 

/* 窄带表：解调方式为非IQ系数表 */
static struct  band_table_t nbandtable[] ={
    {198608, 128,   0, 600},
    {198608, 128,   0, 1500},
    {198608, 129,   0, 2400},
    {198608, 131,   0, 6000},
    {198608, 133,   0, 9000},
    {198608, 135,   0, 12000},
    {198608, 137,   0, 15000},
    {197608, 0,     0, 30000},
    {197108, 0,     0, 50000},
    {196858, 0,     0, 100000},
    {196733, 0,     0, 150000},
}; 
/* 宽带系数表 */
static struct  band_table_t bandtable[] ={
    {196658, 0,  0, 1000000},
    {196633, 0,  0, 2000000},
    {196618, 0,  0, 50000008},
    {196613, 0,  0, 10000000},
    {65541,  0,  0, 20000000},
    {196608, 0,  0, 40000000},
    {196608, 0,  0, 60000000},
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
        *count = ARRAY_SIZE(nbandtable);
    return nbandtable;
}

