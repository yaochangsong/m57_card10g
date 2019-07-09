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
*  Rev 1.0   06 July 2019   yaochangsong
*  Initial revision.
******************************************************************************/

#include "config.h"

static s_config config;

/**
 * Mutex for the configuration file, used by the auth_servers related
 * functions. */
pthread_mutex_t config_mutex = PTHREAD_MUTEX_INITIALIZER;

/** Sets the default config parameters and initialises the configuration system */
void
config_init(void)
{  
    //config.configfile = safe_strdup(DEFAULT_CONFIGFILE);

    config.daemon = -1;
}


int8_t config_parse_data(uint8_t class_code, uint8_t business_code, void *data)
{
    printf_debug("start to config parse data\n");
    return 0;
}

int8_t config_save_batch(uint8_t class_code, uint8_t business_code, void *data)
{
    printf_debug("save parse data\n");
    return 0;
}



