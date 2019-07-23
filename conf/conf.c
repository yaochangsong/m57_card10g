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
 * Mutex for the configuration file, used by the related
 * functions. */
pthread_mutex_t config_mutex = PTHREAD_MUTEX_INITIALIZER;

/** Sets the default config parameters and initialises the configuration system */
void config_init(void)
{  
    printf_debug("config init\n");

    /* to test*/
    struct sockaddr_in saddr;
    uint8_t  mac[6];
    
    saddr.sin_addr.s_addr=inet_addr("192.168.0.105");
    config.oal_config.network.ipaddress = saddr.sin_addr.s_addr;
    saddr.sin_addr.s_addr=inet_addr("192.168.0.1");
    config.oal_config.network.gateway = saddr.sin_addr.s_addr;
    saddr.sin_addr.s_addr=inet_addr("255.255.255.0");
    config.oal_config.network.netmask = saddr.sin_addr.s_addr;
    config.oal_config.network.port = 1325;
    if(get_mac(mac, sizeof(mac)) != -1){
        memcpy(config.oal_config.network.mac, mac, sizeof(config.oal_config.network.mac));
    }
    printf_debug("mac:%x%x%x%x%x%x\n", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    printf_debug("config init\n");
    config.configfile = safe_strdup(DEFAULT_CONFIGFILE);
    config.daemon = -1;
    
    dao_read_create_config_file(config.configfile, &config);

}

/** Accessor for the current configuration
@return:  A pointer to the current config.  The pointer isn't opaque, but should be treated as READ-ONLY
 */
s_config *config_get_config(void)
{
    return &config;
}


int8_t config_parse_data(exec_cmd cmd, uint8_t type, void *data)
{
    printf_debug("start to config parse data\n");
    return 0;
}

/******************************************************************************
* FUNCTION:
*     config_save_batch
*
* DESCRIPTION:
*     根据命令批量（单个）保存对应参数
* PARAMETERS
*     cmd:  保存参数对应类型命令; 见executor.h定义
*    type:  子类型 见executor.h定义
*     data: 对应数据结构；默认专递全局配置config结构体指针
* RETURNS
*       -1:  失败
*        0：成功
******************************************************************************/

int8_t config_save_batch(exec_cmd cmd, uint8_t type,s_config *config)
{
    printf_debug(" config_save_batch\n");

#if DAO_XML == 1
     dao_conf_save_batch(EX_NETWORK_CMD,NULL,config);
        
#elif DAO_JSON == 1
    
#else
    #error "NOT SUPPORT DAO FORMAT"
#endif
        return NULL;

    return 0;
}



