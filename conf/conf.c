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

/*本控 or 远控 查看接口*/
ctrl_mode_param config_get_control_mode(void)
{
#if  (CONTROL_MODE_SUPPORT == 1)
    if(config.oal_config->ctrl_para.remote_local == CTRL_MODE_LOCAL){
        return CTRL_MODE_LOCAL;
    }else{
        return CTRL_MODE_REMOTE;
    }
#endif
    return CTRL_MODE_REMOTE;
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
    printf_info(" config_save_batch\n");

#if DAO_XML == 1
     dao_conf_save_batch(cmd,type,config);
        
#elif DAO_JSON == 1
    
#else
    #error "NOT SUPPORT DAO FORMAT"
#endif
    return 0;
}

/******************************************************************************
* FUNCTION:
*     config_refresh_from_data
*
* DESCRIPTION:
*     根据命令和参数保存数据到config结构体; 中频参数默认保存到频点为1的参数
* PARAMETERS
*     cmd:  保存参数对应类型命令; 见executor.h定义
*     type:  子类型 见executor.h定义
*     data:   数据
* RETURNS
*       -1:  失败
*        0：成功
******************************************************************************/

int8_t config_refresh_data(exec_cmd cmd, uint8_t type, uint8_t ch, void *data)
{
    printf_info(" config_load_from_data\n");

    struct poal_config *poal_config = &(config_get_config()->oal_config);

    if(data ==NULL)
        return -1;
    
     switch(cmd)
     {
        case EX_MID_FREQ_CMD:
        {
            switch(type)
            {
                case EX_MUTE_SW:
                    if(*(uint8_t *)data > 1){
                        return -1;
                    }
                    poal_config->multi_freq_point_param[ch].points[0].noise_en = *(uint8_t *)data;
                    break;
                case EX_MUTE_THRE:
                    poal_config->multi_freq_point_param[ch].points[0].noise_thrh = *(uint8_t *)data;
                    break;
                case EX_DEC_METHOD:
                    poal_config->multi_freq_point_param[ch].points[0].d_method = *(uint8_t *)data;
                    break;
                case EX_DEC_BW:
                    poal_config->multi_freq_point_param[ch].points[0].d_bandwith = *(uint32_t *)data;
                    break;
                case EX_AUDIO_SAMPLE_RATE:
                    poal_config->multi_freq_point_param[ch].audio_sample_rate = *(float *)data;
                    break;
                default:
                    printf_err("not surpport type\n");
                    return -1;
            }
            break;
        }
        case EX_RF_FREQ_CMD:
        {
            switch(type)
            {
                case EX_RF_MID_FREQ:
                    poal_config->rf_para[ch].mid_freq = *(uint64_t *)data;
                    break;
                case EX_RF_MID_BW:
                    poal_config->rf_para[ch].mid_bw = *(uint32_t *)data;
                    break;
                case EX_RF_MODE_CODE:
                    poal_config->rf_para[ch].rf_mode_code = *(uint8_t *)data;
                    break;
                case EX_RF_GAIN_MODE:
                    poal_config->rf_para[ch].gain_ctrl_method = *(uint8_t *)data;
                    break;
                case EX_RF_MGC_GAIN:
                    poal_config->rf_para[ch].mgc_gain_value = *(float *)data;
                    break;
                default:
                    printf_err("not surpport type\n");
                    return -1;
            }
            break;
        }
        case EX_ENABLE_CMD:
        {
            break;
        }
        case EX_NETWORK_CMD:
        {
            switch(type)
            {
                case EX_NETWORK_IP:
                    poal_config->network.ipaddress = *(uint32_t *)data;
                    break;
                case EX_NETWORK_MASK:
                    poal_config->network.netmask = *(uint32_t *)data;
                    break;
                case EX_NETWORK_GW:
                    poal_config->network.gateway = *(uint32_t *)data;
                    break;
                case EX_NETWORK_PORT:
                    poal_config->network.port = *(uint16_t *)data;
                    break;
                default:
                    printf_err("not surpport type\n");
                    return -1;
            }
            break;

        }
        case EX_CTRL_CMD:
        {
             switch(type)
             {
                case EX_CTRL_LOCAL_REMOTE:
                    break;
                default:
                    printf_err("not surpport type\n");
                    return -1;
             }
        }
            break;
        default:
            printf_err("invalid set data[%d]\n", cmd);
            return -1;
     }
     
    config_save_batch(cmd, type, config_get_config());
    return 0;
}


int8_t config_read_by_cmd(exec_cmd cmd, uint8_t type, uint8_t ch, void *data)
{
    printf_info("config_read_by_cmd\n");
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    
    if(data == NULL){
        return -1;
    }
     switch(cmd)
     {
        case EX_MID_FREQ_CMD:
        {
            switch(type)
            {
                case EX_MUTE_SW:
                    data = (void *)&poal_config->multi_freq_point_param[ch].points[0].noise_en;
                    break;
                case EX_MUTE_THRE:
                     data = (void *)&poal_config->multi_freq_point_param[ch].points[0].noise_thrh;
                    break;
                case EX_DEC_METHOD:
                     data = (void *)&poal_config->multi_freq_point_param[ch].points[0].d_method;
                    break;
                case EX_DEC_BW:
                     data = (void *)&poal_config->multi_freq_point_param[ch].points[0].d_bandwith;
                    break;
                case EX_AUDIO_SAMPLE_RATE:
                     data = (void *)&poal_config->multi_freq_point_param[ch].audio_sample_rate;
                    break;
                default:
                    printf_err("not surpport type\n");
                    return -1;
            }
            break;
        }
        case EX_RF_FREQ_CMD:
        {
            switch(type)
            {
                case EX_RF_MID_FREQ:
                    data = (void *)&poal_config->rf_para[ch].mid_freq;
                    break;
                case EX_RF_MID_BW:
                    data = (void *)&poal_config->rf_para[ch].mid_bw;
                    break;
                case EX_RF_MODE_CODE:
                    data = (void *)&poal_config->rf_para[ch].rf_mode_code;
                    break;
                case EX_RF_GAIN_MODE:
                    data = (void *)&poal_config->rf_para[ch].gain_ctrl_method;
                    break;
                case EX_RF_MGC_GAIN:
                    data = (void *)&poal_config->rf_para[ch].mgc_gain_value;
                    break;
                default:
                    printf_err("not surpport type\n");
                    return -1;
            }
            break;
        }
        case EX_ENABLE_CMD:
        {
            break;
        }
        case EX_NETWORK_CMD:
        {
            switch(type)
            {
                case EX_NETWORK_IP:
                    data = (void *)&poal_config->network.ipaddress;
                    break;
                case EX_NETWORK_MASK:
                    data = (void *)&poal_config->network.netmask;
                    break;
                case EX_NETWORK_GW:
                    data = (void *)&poal_config->network.gateway;
                    break;
                case EX_NETWORK_PORT:
                    data = (void *)&poal_config->network.port;
                    break;
                default:
                    printf_err("not surpport type\n");
                    return -1;
            }
            break;
        }
        case EX_CTRL_CMD:
        {
             switch(type)
             {
                case EX_CTRL_LOCAL_REMOTE:
                    break;
                default:
                    printf_err("not surpport type\n");
                    return -1;
             }
        }
        default:
            printf_err("invalid set data[%d]\n", cmd);
            return -1;
     }
     
    return 0;
}




