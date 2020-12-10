/******************************************************************************
*  Copyright 2020, Showay Technology Dev Co.,Ltd.
*  ---------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Showay Technology Dev Co.,Ltd. (C) 2019
******************************************************************************/
/*****************************************************************************     
*  Rev 1.0   8 Dec 2020    yaochangsong
*  Initial revision.
******************************************************************************/
#ifndef _CTRL_H
#define _CTRL_H

#include "platform.h"
#include "../../device/gpio/gpio_raw.h"
#include "../../executor/executor.h"
#include "../../log/log.h"



/*   
    {EMIO:[8:9:10]}=>C1 C2 C3 }
    0 0 0  => CH1:30_1350MHZ    CH2:1_18GHZ  
    0 0 1  => CH1:30_1350MHZ    CH2:18_40GHZ
    0 1 0  => CH1:1_18GHZ       CH2:30_1350MHZ
    0 1 1  => CH1:1_18GHZ       CH2:18_40GHZ
*/
static inline  void _ctrl_freq(void *args)
{
    uint64_t mid_freq[MAX_RADIO_CHANNEL_NUM];
    int i, ch;
    static bool has_init = false;
    static int changed_value_dup[MAX_RADIO_CHANNEL_NUM] = {-1, -1};
    int changed_value[MAX_RADIO_CHANNEL_NUM] = {0};
    struct spm_run_parm *r_args = args;
    ch = r_args->ch;

    if(has_init == false){
        for(i = 0; i < MAX_RADIO_CHANNEL_NUM; i++)
            changed_value_dup[i] = -1;
        has_init = true;
    }
    
    for(i = 0; i < MAX_RADIO_CHANNEL_NUM; i++){
        mid_freq[i] = executor_get_mid_freq(i);
    }
    if(mid_freq[0] <= MHZ(1350) && 
    (mid_freq[1] >= GHZ(1) && mid_freq[1] <= GHZ(18))){
        changed_value[ch] = 1;
        if(changed_value[ch] !=changed_value_dup[ch]){
            printf_info("[ch1:%llu, ch2:%llu]EMIO: 0 ,0, 0\n", mid_freq[0], mid_freq[1]);
            SET_GPIO_ARRAY(0,0,0);
        }
    } else if(mid_freq[0] <= MHZ(1350) && 
            mid_freq[1] >= GHZ(18)){
        changed_value[ch] = 2;
        if(changed_value[ch] !=changed_value_dup[ch]){
            printf_info("[ch1:%llu, ch2:%llu]EMIO: 0 ,0, 1\n", mid_freq[0], mid_freq[1]);
            SET_GPIO_ARRAY(0,0,1);
        }
    } else if((mid_freq[0] >= GHZ(1) && mid_freq[0] <= GHZ(18)) && 
            mid_freq[1] <= MHZ(1350)){
        changed_value[ch] = 3;
        if(changed_value[ch] !=changed_value_dup[ch]){
            printf_info("[ch1:%llu, ch2:%llu]EMIO: 0 ,1, 0\n", mid_freq[0], mid_freq[1]);
            SET_GPIO_ARRAY(0,1,0);
        }
    } else if((mid_freq[0] >= GHZ(1) && mid_freq[0] <= GHZ(18)) && 
             mid_freq[1] >= GHZ(18)){
        changed_value[ch] = 4;
        if(changed_value[ch] !=changed_value_dup[ch]){
            printf_info("[ch1:%llu, ch2:%llu]EMIO: 0 ,1, 1\n", mid_freq[0], mid_freq[1]);
            SET_GPIO_ARRAY(0,1,1);
        }
    } else{
        changed_value[ch] = 5;
        if(changed_value[ch] !=changed_value_dup[ch]){
            printf_info("Not support freq: ch0:%llu, ch1:%llu\n", mid_freq[0], mid_freq[1]);
        }
    }
   // changed_value_dup[ch] = changed_value[ch];
}
#endif
