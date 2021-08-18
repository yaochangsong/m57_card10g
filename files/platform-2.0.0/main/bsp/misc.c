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

const struct misc_ops *misc = NULL;

static bool misc_is_init = false;
const struct misc_ops *misc_get(void)
{
    if(misc_is_init == false){
        misc_init();
        misc_is_init = true;
    }
    return misc;
}

/* 杂项控制初始化，根据不同项目可定义不同控制方法 */
int misc_init(void)
{
    int ret = 0;

    misc = misc_create_ctx();

    misc_is_init = true;
    return ret;
}

