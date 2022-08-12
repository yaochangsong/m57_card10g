/******************************************************************************
*  Copyright 2021, Showay Technology Dev Co.,Ltd.
*  ---------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Showay Technology Dev Co.,Ltd. (C) 2019
******************************************************************************/
/*****************************************************************************     
*  Rev 1.0   2 June 2021   ycs
*  Initial revision.
******************************************************************************/
#include "config.h"

struct cond_freq_info{
    uint64_t start_freq;
    uint64_t end_freq;
    char gpio_val[32];
    size_t val_num;
};

struct cond_attenuation_info{
    float attenuation;
    char gpio_val[32];
    size_t val_num;
};


struct cond_freq_info cond_freq_node[] = {
    {MHZ(50),   MHZ(120),  {1,1,1,0,0,0,0,1}, 8},
    {MHZ(120),  MHZ(210),  {1,1,0,1,1,1,0,1}, 8},
    {MHZ(210),  MHZ(420),  {1,1,1,1,0,1,0,1}, 8},
    {MHZ(420),  MHZ(680),  {1,1,0,0,1,0,0,1}, 8},
    {MHZ(680),  MHZ(1240), {1,0,1,0,0,0,1,1}, 8},
    {MHZ(1240), MHZ(2180), {1,0,0,1,1,1,1,1}, 8},
    {MHZ(2180), MHZ(3340), {1,0,1,1,0,1,1,1}, 8},
    {MHZ(3340), MHZ(5880), {1,0,0,0,1,0,1,1}, 8},
};

struct cond_attenuation_info cond_atten_node_pre[] = {
    {0,     {1,1,1,1,1,1}, 6},
    {0.5,   {0,1,1,1,1,1}, 6},
    {1,     {1,0,1,1,1,1}, 6},
    {2,     {1,1,0,1,1,1}, 6},
    {4,     {1,1,1,0,1,1}, 6},
    {8,     {1,1,1,1,0,1}, 6},
    {16,    {1,1,1,1,1,0}, 6},
    {31.5,  {0,0,0,0,0,0}, 6},
};

struct cond_attenuation_info cond_atten_node_post[] = {
    {0,     {0,1,1,1,1,1}, 6},
    {0.5,   {1,0,1,1,1,1}, 6},
    {1,     {1,0,1,1,1,1}, 6},
    {2,     {1,1,0,1,1,1}, 6},
    {4,     {1,1,1,0,1,1}, 6},
    {8,     {1,1,1,1,0,1}, 6},
    {16,    {1,1,1,1,1,0}, 6},
    {31.5,  {0,0,0,0,0,0}, 6},
};

int  condition_attenuation(int val_db)
{
    struct cond_attenuation_info *ptr_pre = cond_atten_node_pre;
    struct cond_attenuation_info *ptr_post = cond_atten_node_post;
    size_t len_pre  = ARRAY_SIZE(cond_atten_node_pre);
    size_t len_post = ARRAY_SIZE(cond_atten_node_pre);
    int found = 0;
    
    for(int i = 0; i< len_pre; i++){
        for(int j = 0; j< len_post; j++){
            if(val_db == (int)(ptr_pre[i].attenuation + ptr_post[j].attenuation)){
                GPIO_SET_ATTENUATION_1(ptr_pre[i].gpio_val);
                GPIO_SET_ATTENUATION_2(ptr_post[j].gpio_val);
                found = 1;
                
                #if 1
                printf_note("[%d, %d]atten: %d, pre:%f, post:%f\n", i, j, val_db, ptr_pre[i].attenuation, ptr_post[j].attenuation);
                printf_note("pre gpio: ");
                for(int k = 0; k < ptr_pre[i].val_num; k++){
                    printfn("%d ", ptr_pre[i].gpio_val[k]);
                }
                printfn("\n");
                printf_note("post gpio: ");
                for(int k = 0; k < ptr_post[j].val_num; k++){
                    printfn("%d ", ptr_post[j].gpio_val[k]);
                }
                printfn("\n");
                #endif
                
                i = len_pre;
                break;
            }
        }
    }
    if(found == 0){
        /* default 32db */
        GPIO_SET_ATTENUATION_1(ptr_pre[1].gpio_val);
        GPIO_SET_ATTENUATION_2(ptr_post[7].gpio_val);
        printf_note("Not find, use atten: pre:%f, post:%f\n", ptr_pre[1].attenuation, ptr_post[7].attenuation);
    }
    
    return 0;
}
int  condition_freq(uint64_t freq_hz)
{
    struct cond_freq_info *ptr = cond_freq_node;
    size_t len = ARRAY_SIZE(cond_freq_node);
    int found = 0;
    
    for(int i = 0; i< len; i++){
       if((freq_hz >= ptr[i].start_freq) && (freq_hz <= ptr[i].end_freq)){
            if(i < 4){
                GPIO_SET_FREQ_1(ptr[i].gpio_val);
            }
            else{
                GPIO_SET_FREQ_2(ptr[i].gpio_val);
            }
            printf_note("[%d]freq: %"PRIu64", start:%"PRIu64", end:%"PRIu64"\n", i, freq_hz, ptr[i].start_freq, ptr[i].end_freq);
            printf_note("freq gpio: ");
            for(int k = 0; k < ptr[i].val_num; k++){
                printfn("%d ", ptr[i].gpio_val[k]);
            }
            printfn("\n");
            found = 1;
            break;
        }
    }
    if(found == 0){
        GPIO_SET_FREQ_1(ptr[0].gpio_val);
        printf_note("Not find, use freq: start:%"PRIu64", end:%"PRIu64"\n", ptr[0].start_freq, ptr[0].end_freq);
        printf_note("freq gpio: ");
        for(int k = 0; k < ptr[0].val_num; k++){
            printfn("%d ", ptr[0].gpio_val[k]);
        }
        printfn("\n");
    }
    return 0;
}


