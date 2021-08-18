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
*  Rev 1.0   30 July 2021   yaochangsong
*  Initial revision.
******************************************************************************/
#include "config.h"

/*   
    {EMIO:[8:9:10]}=>C1 C2 C3 }
    C3  C2  C1  控制状态
    x   x   0   J1[30m~1350mhz]-CH1
    x   0   x   J1[30m~1350mhz]-CH2
    x   x   1   J2[1G~8Ghz]-CH1
    0   1   x   J2[1G~8Ghz]-CH2
    1   1   x   J3[18G~40Ghz]-CH2
*/
static   void _ctrl_freq(void *args)
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
    if((mid_freq[0] <= MHZ(1350) || mid_freq[0] == 0)  && 
        (mid_freq[1] == 0 || (mid_freq[1] >= GHZ(1) && mid_freq[1] <= GHZ(18)))){
        changed_value[ch] = 1;
        if(changed_value[ch] !=changed_value_dup[ch]){
            printf_info("[ch1:%"PRIu64", ch2:%"PRIu64"]EMIO: 0 ,0, 0\n", mid_freq[0], mid_freq[1]);
            //SET_GPIO_ARRAY(0,0,0);
        }
    } else if((mid_freq[0] <= MHZ(1350) || mid_freq[0] == 0)  &&
               mid_freq[1] >= GHZ(18)){
        changed_value[ch] = 2;
        if(changed_value[ch] !=changed_value_dup[ch]){
            printf_info("[ch1:%"PRIu64", ch2:%"PRIu64"]EMIO: 0 ,0, 1\n", mid_freq[0], mid_freq[1]);
           // SET_GPIO_ARRAY(1,0,0);
        }
    } else if(((mid_freq[0] >= GHZ(1) && mid_freq[0] <= GHZ(18)) ||  mid_freq[0] == 0) && 
              (mid_freq[1] <= MHZ(1350) || mid_freq[1] == 0)){
        changed_value[ch] = 3;
        if(changed_value[ch] !=changed_value_dup[ch]){
            printf_info("[ch1:%"PRIu64", ch2:%"PRIu64"]EMIO: 0 ,1, 0\n", mid_freq[0], mid_freq[1]);
           // SET_GPIO_ARRAY(0,1,0);
        }
    } else if((mid_freq[0] >= GHZ(1) && mid_freq[0] <= GHZ(18)) &&  
                mid_freq[1] >= GHZ(18)){
        changed_value[ch] = 4;
        if(changed_value[ch] !=changed_value_dup[ch]){
            printf_info("[ch1:%"PRIu64", ch2:%"PRIu64"]EMIO: 0 ,1, 1\n", mid_freq[0], mid_freq[1]);
            //SET_GPIO_ARRAY(1,1,0);
        }
    }  else{
        changed_value[ch] = 5;
        if(changed_value[ch] !=changed_value_dup[ch]){
            printf_info("Not support freq: ch0:%"PRIu64", ch1:%"PRIu64"\n", mid_freq[0], mid_freq[1]);
        }
    }
    changed_value_dup[ch] = changed_value[ch];
}

static const struct misc_ops misc_reg = {
    .freq_ctrl = _ctrl_freq,
};


const struct misc_ops * misc_create_ctx(void)
{
    const struct misc_ops *ctx = calloc(1, sizeof(*ctx));
     if (!ctx)
        return NULL;
     
    ctx = &misc_reg;
    return ctx;
}

