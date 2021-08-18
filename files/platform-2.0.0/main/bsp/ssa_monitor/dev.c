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


static struct _spm_stream spm_stream[] = {
};


struct _spm_stream* spm_dev_get_stream(int *count)
{
    if(count)
        *count = ARRAY_SIZE(spm_stream);
    return spm_stream;
}

static struct gpio_node_info gpio_node[] ={
    /* pin   direction  default gpio value   func_code    func_name    fd */
    {0,      "out",      0,               CONDI_GPIO_0,    "CONDI_GPIO_0",  -1 },
    {1,      "out",      0,               CONDI_GPIO_1,    "CONDI_GPIO_1",  -1 },
    {2,      "out",      0,               CONDI_GPIO_2,    "CONDI_GPIO_2",  -1 },
    {3,      "out",      0,               CONDI_GPIO_3,    "CONDI_GPIO_3",  -1 },
    {4,      "out",      0,               CONDI_GPIO_4,    "CONDI_GPIO_4",  -1 },
    {5,      "out",      0,               CONDI_GPIO_5,    "CONDI_GPIO_5",  -1 },
    {6,      "out",      0,               CONDI_GPIO_6,    "CONDI_GPIO_6",  -1 },
    {7,      "out",      0,               CONDI_GPIO_7,    "CONDI_GPIO_7",  -1 },
    {8,      "out",      0,               CONDI_GPIO_8,    "CONDI_GPIO_8",  -1 },
    {9,      "out",      0,               CONDI_GPIO_9,    "CONDI_GPIO_9",  -1 },
    {10,     "out",      0,               CONDI_GPIO_10,   "CONDI_GPIO_10",  -1 },
    {11,     "out",      0,               CONDI_GPIO_11,   "CONDI_GPIO_11",  -1 },
    {12,     "out",      0,               CONDI_GPIO_12,   "CONDI_GPIO_12",  -1 },
    {13,     "out",      0,               CONDI_GPIO_13,   "CONDI_GPIO_13",  -1 },
    {14,     "out",      0,               CONDI_GPIO_14,   "CONDI_GPIO_14",  -1 },
    {15,     "out",      0,               CONDI_GPIO_15,   "CONDI_GPIO_15",  -1 },
    {16,     "out",      0,               CONDI_GPIO_16,   "CONDI_GPIO_16",  -1 },
    {17,     "out",      0,               CONDI_GPIO_17,   "CONDI_GPIO_17",  -1 },
    {18,     "out",      0,               CONDI_GPIO_18,   "CONDI_GPIO_18",  -1 },
    {19,     "out",      0,               CONDI_GPIO_19,   "CONDI_GPIO_19",  -1 },
    {20,     "out",      0,               CONDI_GPIO_20,   "CONDI_GPIO_20",  -1 },
    {21,     "out",      0,               CONDI_GPIO_21,   "CONDI_GPIO_21",  -1 },
    {22,     "out",      0,               CONDI_GPIO_22,   "CONDI_GPIO_22",  -1 },
    {23,     "out",      0,               CONDI_GPIO_23,   "CONDI_GPIO_23",  -1 },
};

struct gpio_node_info* dev_get_gpio(int *count)
{
    if(count)
        *count = ARRAY_SIZE(gpio_node);
    return gpio_node;
}



