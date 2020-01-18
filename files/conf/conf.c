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
    #if 0
    saddr.sin_addr.s_addr=inet_addr("192.168.0.105");
    config.oal_config.network.ipaddress = saddr.sin_addr.s_addr;
    saddr.sin_addr.s_addr=inet_addr("192.168.0.1");
    config.oal_config.network.gateway = saddr.sin_addr.s_addr;
    saddr.sin_addr.s_addr=inet_addr("255.255.255.0");
    config.oal_config.network.netmask = saddr.sin_addr.s_addr;
    config.oal_config.network.port = 1325;
    #endif
    if(get_mac(mac, sizeof(mac)) != -1){
        memcpy(config.oal_config.network.mac, mac, sizeof(config.oal_config.network.mac));
    }
    printf_debug("mac:%x%x%x%x%x%x\n", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    printf_debug("config init\n");
    config.configfile = safe_strdup(DEFAULT_CONFIGFILE);
    config.calibrationfile = safe_strdup(CALIBRATION_FILE);
    config.daemon = -1;
    config.oal_config.work_mode = OAL_NULL_MODE;
    config.oal_config.ctrl_para.wz_threshold_bandwidth = 10000000; /* 万兆默认阀值; >=该值，用万兆传输 */
    dao_read_create_config_file(config.configfile, &config);
}

/** Accessor for the current configuration
@return:  A pointer to the current config.  The pointer isn't opaque, but should be treated as READ-ONLY
 */
s_config *config_get_config(void)
{
    return &config;
}


/*本控 or 远控 查看接口*/
ctrl_mode_param config_get_control_mode(void)
{
#ifdef  SUPPORT_REMOTE_LOCAL_CTRL_EN
    if(config.oal_config.ctrl_para.remote_local == CTRL_MODE_LOCAL){
        return CTRL_MODE_LOCAL;
    }else{
        return CTRL_MODE_REMOTE;
    }
#endif
    return CTRL_MODE_REMOTE;
}

bool config_get_is_internal_clock(void)
{
    if(config.oal_config.ctrl_para.internal_clock == 0){
        /* External clock */
        return false;
    }
    return true;
}

uint32_t  config_get_fft_size(uint8_t ch)
{
    uint32_t fftsize = 0;
    uint8_t mode; 
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    mode = poal_config->work_mode;
    if((mode == OAL_FAST_SCAN_MODE) || (mode == OAL_MULTI_ZONE_SCAN_MODE)){
        fftsize = poal_config->multi_freq_fregment_para[ch].fregment[0].fft_size;
    }else{
        fftsize = poal_config->multi_freq_point_param[ch].points[0].fft_size;
    }
    if(fftsize == 0){
        printf_warn("fftsize %u not set!!!\n",fftsize);
    }
    printf_note("fftsize:%u\n",fftsize);
    return fftsize;
}

int32_t  config_get_fft_calibration_value(uint32_t fft_size)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    int i;
    int32_t cal_value=0,found = 0;
    for(i=0;i<sizeof(poal_config->cal_level.cali_fft.fftsize)/sizeof(uint32_t);i++)
    {
        if(fft_size==poal_config->cal_level.cali_fft.fftsize[i])
        {
            cal_value=poal_config->cal_level.cali_fft.cali_value[i];
            found=1;
            break;

        }
    }
    cal_value += poal_config->cal_level.specturm.global_roughly_power_lever;
    printf_note("cal_value=%d\n",cal_value);
    if(found){
        printf_debug("find the fft_mgc calibration value: %d\n",cal_value);
    }else{
        
        printf_debug("Not find fft_mgc calibration level, use default value\n");
    }
    return cal_value;
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

#ifdef SUPPORT_DAO_XML
     dao_conf_save_batch(cmd,type,config);
        
#elif defined SUPPORT_DAO_JSON
    
#else
    #error "NOT SUPPORT DAO FORMAT"
#endif
    return 0;
}

/******************************************************************************
* FUNCTION:
*     config_write_data
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

int8_t config_write_data(exec_cmd cmd, uint8_t type, uint8_t ch, void *data)
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
                    poal_config->multi_freq_point_param[ch].points[0].noise_en = *(int8_t *)data;
                    break;
                case EX_MUTE_THRE:
                    poal_config->multi_freq_point_param[ch].points[0].noise_thrh = *(int8_t *)data;
                    break;
                case EX_DEC_METHOD:
                    poal_config->multi_freq_point_param[ch].points[0].d_method = *(int8_t *)data;
                    break;
                case EX_DEC_BW:
                    poal_config->multi_freq_point_param[ch].points[0].d_bandwith = *(int32_t *)data;
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
                    poal_config->rf_para[ch].mid_freq = *(int64_t *)data;
                    break;
                case EX_RF_MID_BW:
                    poal_config->rf_para[ch].mid_bw = *(int32_t *)data;
                    break;
                case EX_RF_MODE_CODE:
                    poal_config->rf_para[ch].rf_mode_code = *(int8_t *)data;
                    break;
                case EX_RF_GAIN_MODE:
                    poal_config->rf_para[ch].gain_ctrl_method = *(int8_t *)data;
                    break;
                case EX_RF_MGC_GAIN:
                    poal_config->rf_para[ch].mgc_gain_value = *(int8_t *)data;
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
                    poal_config->network.ipaddress = *(int32_t *)data;
                    break;
                case EX_NETWORK_MASK:
                    poal_config->network.netmask = *(int32_t *)data;
                    break;
                case EX_NETWORK_GW:
                    poal_config->network.gateway = *(int32_t *)data;
                    break;
                case EX_NETWORK_PORT:
                    poal_config->network.port = *(int16_t *)data;
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


int8_t config_read_by_cmd(exec_cmd cmd, uint8_t type, uint8_t ch, void *data, ...)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    int ret=  -1;
    va_list argp;
    va_start (argp, data);
    printf_debug("config_read_by_cmd[%d]\n", cmd);
    if(data == NULL){
        goto exit;
    }
     switch(cmd)
     {
        uint32_t index = va_arg(argp, uint32_t);
        case EX_MID_FREQ_CMD:
        {
            switch(type)
            {
                case EX_MUTE_SW:
                    *(uint8_t *)data = poal_config->multi_freq_point_param[ch].points[index].noise_en;
                    break;
                case EX_MUTE_THRE:
                     *(int8_t *)data = poal_config->multi_freq_point_param[ch].points[index].noise_thrh;
                    break;
                case EX_DEC_METHOD:
                     *(uint8_t *)data = poal_config->multi_freq_point_param[ch].points[index].d_method;
                    break;
                case EX_DEC_BW:
                     *(uint32_t *)data = poal_config->multi_freq_point_param[ch].points[index].d_bandwith;
                    break;
                case EX_AUDIO_SAMPLE_RATE:
                     *(float *)data = poal_config->multi_freq_point_param[ch].audio_sample_rate;
                    break;
                case EX_MID_FREQ:
                    *(uint64_t *)data = poal_config->multi_freq_point_param[ch].points[index].center_freq;
                    break;
                case EX_BANDWITH:
                {
                    *(uint32_t *)data = poal_config->multi_freq_point_param[ch].points[index].bandwidth;
                    break;
                }
                default:
                    printf_err("not surpport type\n");
                    goto exit;
            }
            break;
        }
        case EX_RF_FREQ_CMD:
        {
            switch(type)
            {
                case EX_RF_MID_FREQ:
                    *(uint64_t *)data = poal_config->rf_para[ch].mid_freq;
                    break;
                case EX_RF_MID_BW:{
                    struct scan_bindwidth_info *scanbw;
                    scanbw = &poal_config->ctrl_para.scan_bw; 
                    if(scanbw->work_fixed_bindwidth_flag){
                        *(int32_t *)data = scanbw->work_bindwidth_hz;
                    }
                    else{
                        *(int32_t *)data = poal_config->rf_para[ch].mid_bw;
                    }
                    printf_debug("ch=%d, rf middle bw=%u\n",ch, poal_config->rf_para[ch].mid_bw);
                    if(*(int32_t *)data == 0){
                        goto exit;
                    }
                    /* Update sideband rate based on bandwidth 
                    for(int i = 0; i<sizeof(scanbw->bindwidth_hz)/sizeof(uint32_t); i++){
                        if(*(int32_t *)data == scanbw->bindwidth_hz[i]){
                            scanbw->work_sideband_rate = scanbw->sideband_rate[i];
                            printf_debug("Update sideband rate[%f] based on bandwidth[%u]\n", scanbw->work_sideband_rate, scanbw->bindwidth_hz[i]);
                            break;
                        }
                    }
                    */
                }
                    break;
                case EX_RF_MODE_CODE:
                    *(uint8_t *)data = poal_config->rf_para[ch].rf_mode_code;
                    break;
                case EX_RF_GAIN_MODE:
                    *(uint8_t *)data = poal_config->rf_para[ch].gain_ctrl_method;
                    break;
                case EX_RF_MGC_GAIN:
                    *(int8_t *)data = poal_config->rf_para[ch].mgc_gain_value;
                    break;
                default:
                    printf_err("not surpport type\n");
                    goto exit;
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
                    *(uint32_t *)data = poal_config->network.ipaddress;
                    break;
                case EX_NETWORK_MASK:
                    *(uint32_t *)data = poal_config->network.netmask;
                    break;
                case EX_NETWORK_GW:
                    *(uint32_t *)data = poal_config->network.gateway;
                    break;
                case EX_NETWORK_PORT:
                    *(uint16_t *)data = poal_config->network.port;
                    break;
                default:
                    printf_err("not surpport type\n");
                    goto exit;
            }
            break;
        }
        case EX_CTRL_CMD:
        {
             switch(type)
             {
                case EX_CTRL_LOCAL_REMOTE:
                    break;
                case EX_CTRL_SIDEBAND:
                {
                    struct scan_bindwidth_info *scanbw;
                    scanbw = &poal_config->ctrl_para.scan_bw; 
                    uint32_t bw = va_arg(argp, uint32_t);
                    int found = 0;
                     for(int i = 0; i<sizeof(scanbw->bindwidth_hz)/sizeof(uint32_t); i++){
                        if(scanbw->bindwidth_hz[i] == bw){
                            *(float *)data = scanbw->sideband_rate[i];
                            scanbw->work_sideband_rate = scanbw->sideband_rate[i];
                            printf_note("--find side rate:%f, %f\n",*(float *)data,  scanbw->sideband_rate[i]);
                            found = 1;
                            break;
                        }
                    }
                    if(found == 1){
                        ret = 0;
                        printf_note("find side rate:%f, bw=%u\n",*(float *)data,  bw);
                    }else{
                        *(float *)data = 0.0;
                        printf_warn("not find side rate, bw=%u\n",  bw);
                        goto exit;
                    }
                    break;
                }
                default:
                    printf_err("not surpport type\n");
                    goto exit;
             }
             break;
        }
        default:
            printf_err("invalid set data[%d]\n", cmd);
            goto exit;
     }
     ret = 0;
exit:
    va_end(argp);
    return ret;
}





