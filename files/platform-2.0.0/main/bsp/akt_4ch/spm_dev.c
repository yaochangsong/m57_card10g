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
#include "spm_dev.h"

#define DMA_FFT0_DEV "/dev/dma_fft0"
#define DMA_FFT1_DEV "/dev/dma_fft1"
#define DMA_FFT2_DEV "/dev/dma_fft2"
#define DMA_FFT3_DEV "/dev/dma_fft3"
#define DMA_NIQ_DEV  "/dev/dma_iq"


static struct _spm_stream spm_stream[] = {
    {DMA_FFT0_DEV,      -1,0, NULL, DMA_BUFFER_16M_SIZE, "FFT0 Stream",    DMA_READ, STREAM_FFT},
    {DMA_FFT1_DEV,      -1,1, NULL, DMA_BUFFER_16M_SIZE, "FFT1 Stream",    DMA_READ, STREAM_FFT},
    {DMA_FFT2_DEV,      -1,2, NULL, DMA_BUFFER_16M_SIZE, "FFT2 Stream",    DMA_READ, STREAM_FFT},
    {DMA_FFT3_DEV,      -1,3, NULL, DMA_BUFFER_16M_SIZE, "FFT3 Stream",    DMA_READ, STREAM_FFT},
    {DMA_NIQ_DEV,      -1,-1, NULL, DMA_BUFFER_32M_SIZE,  "NIQ Stream",     DMA_READ, STREAM_NIQ},
};



struct _spm_stream* spm_dev_get_stream(int *count)
{
    if(count)
        *count = ARRAY_SIZE(spm_stream);
    return spm_stream;
}

static struct gpio_node_info gpio_node[] ={
    /* pin   direction  default gpio value   func_code    func_name    fd */
    {24,      "out",        1,               GPIO_POWER_CH0,   "GPIO POWER 0",                 -1 },  //通道射频电源控制
    {25,      "out",        1,               GPIO_POWER_CH1,   "GPIO POWER 1",                 -1 },
    {26,      "out",        1,               GPIO_POWER_CH2,   "GPIO POWER 2",                 -1 },
    {27,      "out",        1,               GPIO_POWER_CH3,   "GPIO POWER 3",                 -1 },
};

struct gpio_node_info* dev_get_gpio(int *count)
{
    if(count)
        *count = ARRAY_SIZE(gpio_node);
    return gpio_node;
}


struct uart_info_t uartinfo[] = {
    /* NOTE: ttyUL*为PL侧控制，波特率由PL侧设置（默认为9600，不可更改），需要更改FPGA */
   {UART_LCD_CODE, "/dev/ttyPS1", 115200,   "LCD",   true},
};

struct uart_info_t* dev_get_uart(int *count)
{
    if(count)
        *count = ARRAY_SIZE(uartinfo);
    return uartinfo;
}


