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
*  Rev 1.0   16 Feb 2020   yaochangsong
*  Initial revision.
******************************************************************************/
#include "config.h"
#include "../http/client.h"
#include "../../log/log.h"
#include "../../conf/conf.h"
#include "parse_cmd.h"
#include "parse_json.h"
#include "../../bsp/io.h"
#include "../../utils/bitops.h"


static struct response_err_code {
    int code;
    char *message;
} resp_code[] ={
    {RESP_CODE_OK,                          "ok"},
    {RESP_CODE_UNKNOWN_OPS_MODE,            "unknown operating mode"},
    {RESP_CODE_PARSE_ERR,                   "parameter parsing failed"},
    {RESP_CODE_INTERNAL_ERR,                "device internal error"},
    {RESP_CODE_INVALID_PARAM,               "invalid parameter"},
    {RESP_CODE_CHANNEL_ERR,                 "channel error"},
    {RESP_CODE_BUSY,                        "device is busy"},
    {RESP_CODE_FILE_NOT_EXIST,              "file does not exist"},
    {RESP_CODE_FILE_ALREADY_EXIST,          "file already exists"},
    {RESP_CODE_DISK_FORMAT_ERR,             "disk format failed"},
    {RESP_CODE_DISK_DETECTED_ERR,           "disk not detected"},
    {RESP_CODE_PATH_PARAM_ERR,              "path parameter error"},
    {RESP_CODE_EXECMD_ERR,                  "execute command error"},
    {RESP_CODE_EXECMD_REBOOT,               "reboot,waiting..."},
};

/* 射频参数类型 */
static const char *const rf_types[] = {
    [EX_RF_MID_FREQ] = "midFreq",
    [EX_RF_MID_BW] = "bandwidth",
    [EX_RF_MODE_CODE] = "modeCode",
    [EX_RF_GAIN_MODE] = "gainMode",
    [EX_RF_MGC_GAIN] = "mgcGain",
    [EX_RF_AGC_CTRL_TIME]="agcCtrlTime",          
    [EX_RF_AGC_OUTPUT_AMP]="agcOutPutAmp",          
    [EX_RF_ANTENNA_SELECT]="antennaSelect",          
    [EX_RF_ATTENUATION] = "rfAttenuation",
    [EX_RF_STATUS_TEMPERAT] = "temperature",         
    [EX_RF_CALIBRATE] = "calibrate",
};

/* 中频参数类型 */
static const char *const if_types[] = {
    [EX_MUTE_SW] = "muteSwitch",
    [EX_MUTE_THRE] = "muteThreshold",              /*静噪门限*/
    [EX_MID_FREQ]  = "midFreq",            /*中心频率*/
    [EX_BANDWITH] = "bandwidth",               /*带宽*/
    [EX_DEC_METHOD] = "decMethodId"    ,        /*解调方式*/
    [EX_DEC_BW] = "decBandwidth", 
    [EX_DEC_MID_FREQ] = "decMidFreq", 
    [EX_SMOOTH_TIME] = "smoothTimes", 
    [EX_SMOOTH_TYPE] = "smoothMode",
    [EX_SMOOTH_THRE] = "smoothThreshold",
    [EX_AUDIO_SAMPLE_RATE] = "audioSampleRate", 
    [EX_FFT_SIZE] = "fftSize", 
    [EX_RESOLUTION] = "freqResolution",
    [EX_AUDIO_VOL_CTRL] = "audioVolume",
    [EX_WINDOW_TYPE] = "windowType",
};



static inline bool check_valid_ch(int ch)
{
    return (ch >= MAX_RADIO_CHANNEL_NUM ? false : true);
}

static inline bool check_valid_subch(int subch)
{
    return (subch >= MAX_SIGNAL_CHANNEL_NUM ? false : true);
}

static inline bool check_valid_enable(int enable)
{
    if(enable != 0 && enable != 1){
        return false;
    }
    return true;
}

static inline int find_idx_safe(const char *const *list, int max, const char *str)
{
    int i;

    for (i = 0; i < max; i++){
        if(list[i] == NULL)
            continue;
        if (!strcmp(list[i], str))
            return i;
    }

    return -1;
}

static inline char *get_resp_message(int code)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(resp_code); i++){
        if(resp_code[i].code == code)
            return resp_code[i].message;
    }

    return "undefined err";
}

static inline bool str_to_int(const char *str, int *ivalue, bool(*_check)(int))
{
    char *end;
    int value;
    
    if(str == NULL || ivalue == NULL)
        return false;
    
    value = (int) strtol(str, &end, 10);
    if (str == end){
        return false;
    }
    *ivalue = value;
    if(*_check == NULL){
         return true;
    }
       
    return ((*_check)(value));
}

static inline bool str_to_uint(const char *str, uint32_t *ivalue, bool(*_check)(int))
{
    char *end;
    uint32_t value;
    
    if(str == NULL || ivalue == NULL)
        return false;
    
    value = (uint32_t) strtoul(str, &end, 10);
    if (str == end){
        return false;
    }
    *ivalue = value;
    if(*_check == NULL){
         return true;
    }
       
    return ((*_check)(value));
}


 

static inline bool str_to_u64(const char *str, uint64_t *ivalue, bool(*_check)(int))
{
    char *end;
    uint64_t value;
    
    if(str == NULL || ivalue == NULL)
        return false;
    value = (uint64_t) strtoull(str, &end, 10);
    if (str == end){
        return false;
    }
    *ivalue = value;
    if(*_check == NULL){
         return true;
    }
       
    return ((*_check)(value));
}

static inline bool str_to_s64(const char *str, int64_t *ivalue, bool(*_check)(int))
{
    char *end;
    int64_t value;
    
    if(str == NULL || ivalue == NULL)
        return false;
    value = (int64_t) strtoll(str, &end, 10);
    if (str == end){
        return false;
    }
    *ivalue = value;
    if(*_check == NULL){
         printf_debug("null check func\n");
         return true;
    }
       
    return ((*_check)(value));
}

int cmd_set_if_string(int ch, const char *s_type, void *val)
{
    int itype;
    
    if(s_type == NULL)
        return -1;
    itype = find_idx_safe(if_types, ARRAY_SIZE(if_types), s_type);
    if(itype >= 0){
        config_write_data(EX_MID_FREQ_CMD, itype, ch, val);
        executor_set_command(EX_MID_FREQ_CMD, itype, ch, val);
    }
    return 0;
}

/* POST /mode/mutiPoint/@ch */
int cmd_muti_point(struct uh_client *cl, void **arg, void **content)
{
    const char *s_ch;
    int ch;
    int code = RESP_CODE_OK;
    
    s_ch = cl->get_restful_var(cl, "ch");
    printf_note("ch = %s\n", s_ch);
    if(str_to_int(s_ch, &ch, check_valid_ch) == false){
        code = RESP_CODE_CHANNEL_ERR;
        goto error;
    }
    printf_note("%s", cl->dispatch.body);
    code =parse_json_muti_point(cl->dispatch.body,ch);
error:
    *arg = get_resp_message(code);
    return code;

}


/* "POST","/mode/multiBand/@ch" */
int cmd_multi_band(struct uh_client *cl, void **arg, void **content)
{
    const char *s_ch;
    int ch;
    int code = RESP_CODE_OK;
    
    s_ch = cl->get_restful_var(cl, "ch");
    printf_note("ch = %s\n", s_ch);
    if(str_to_int(s_ch, &ch, check_valid_ch) == false){
        code = RESP_CODE_CHANNEL_ERR;
        goto error;
    }
    printf_note("%s\n", cl->dispatch.body);
    code = parse_json_multi_band(cl->dispatch.body,ch);
error:
    *arg = get_resp_message(code);
    return code;

}

/* "POST","/demodulation/@ch/@subch" */
/* "POST","/nddc/@ch/@subch" */
int cmd_demodulation(struct uh_client *cl, void **arg, void **content)
{
    const char *s_ch, *s_subch;
    int ch, subch;
    int code = RESP_CODE_OK;
    
    s_ch = cl->get_restful_var(cl, "ch");
    s_subch = cl->get_restful_var(cl, "subch");
    printf_debug("ch = %s, subch=%s\n", s_ch, s_subch);
    if(str_to_int(s_ch, &ch, check_valid_ch) == false){
        code = RESP_CODE_CHANNEL_ERR;
        goto error;
    }
    if(s_subch != NULL){
        if(str_to_int(s_subch, &subch, check_valid_subch) == false){
            code = RESP_CODE_CHANNEL_ERR;
            goto error;
        }
    }else{
        code = RESP_CODE_CHANNEL_ERR;
        goto error;
    }
    printf_info("%s\n", cl->dispatch.body);
    code = parse_json_demodulation(cl->dispatch.body,ch,subch);
error:
    *arg = get_resp_message(code);
    return code;

}

int cmd_bddc(struct uh_client *cl, void **arg, void **content)
{
    const char *s_ch;
    int ch;
    int code = RESP_CODE_OK;
    
    s_ch = cl->get_restful_var(cl, "ch");

    printf_note("bddc ch = %s\n", s_ch);
    if(str_to_int(s_ch, &ch, check_valid_ch) == false){
        code = RESP_CODE_CHANNEL_ERR;
        goto error;
    }
    printf_note("%s\n", cl->dispatch.body);
    code = parse_json_bddc(cl->dispatch.body,ch);
error:
    *arg = get_resp_message(code);
    return code;

}




/*"PUT",     /if/@ch/@subch/@type/@value"
    中频单个参数设置
*/
int cmd_if_single_value_set(struct uh_client *cl, void **arg, void **content)
{
    const char *s_type, *s_ch, *s_subch, *s_value, *s_value_2;
    int ch, itype, subch;
    int64_t value = 0, value2=0;
    int code = RESP_CODE_OK;
    
    s_type = cl->get_restful_var(cl, "type");
    s_ch = cl->get_restful_var(cl, "ch");
    s_subch = cl->get_restful_var(cl, "subch");
    s_value = cl->get_restful_var(cl, "value");
    s_value_2 = cl->get_restful_var(cl, "value2");
    printf_note("if type=%s,ch = %s, subch=%s, value=%s, s_value_2=%s\n", s_type, s_ch, s_subch, s_value, s_value_2);
    if(str_to_int(s_ch, &ch, check_valid_ch) == false){
        code = RESP_CODE_CHANNEL_ERR;
        goto error;
    }
    if(s_subch != NULL){
        if(str_to_int(s_subch, &subch, check_valid_subch) == false){
            code = RESP_CODE_CHANNEL_ERR;
            goto error;
        }
    }else{
        subch = -1;
    }

    if(s_value == NULL || str_to_s64(s_value, &value, NULL) == false){
        code = RESP_CODE_PATH_PARAM_ERR;
        goto error;
    }

    if(s_value_2 != NULL){
        if(str_to_s64(s_value_2, &value2, NULL) == false){
            code = RESP_CODE_PATH_PARAM_ERR;
             goto error;
        }
    }
    itype = find_idx_safe(if_types, ARRAY_SIZE(if_types), s_type);
    if(s_value_2 != NULL){
        config_write_data(EX_MID_FREQ_CMD, itype, ch,&value);
        if(executor_set_command(EX_MID_FREQ_CMD, itype, ch,&value, value2) != 0){
            code = RESP_CODE_EXECMD_ERR;
            goto error;
        }
    }else{
        config_write_data(EX_MID_FREQ_CMD, itype, ch,&value);
        if(executor_set_command(EX_MID_FREQ_CMD, itype, ch,&value) != 0){
            code = RESP_CODE_EXECMD_ERR;
            goto error;
        }
    }

error:
    *arg = get_resp_message(code);
    return code;

}

/*"POST",     /if/@ch/@subch"
    中频参数批量设置
*/
int cmd_if_multi_value_set(struct uh_client *cl, void **arg, void **content)
{
    const char *s_ch, *subch;
    int itype, ch;
    int code = RESP_CODE_OK;
    
    s_ch = cl->get_restful_var(cl, "ch");
    subch = cl->get_restful_var(cl, "subch");
    printf_note("rf ch = %s, subch=%s\n", s_ch, subch);
    printf_note("%s\n", cl->dispatch.body);
    if(str_to_int(s_ch, &ch, check_valid_ch) == false){
        code = RESP_CODE_CHANNEL_ERR;
        goto error;
    }
    code = parse_json_if_multi_value(cl->dispatch.body, ch);
    
error:
    *arg = get_resp_message(code);
    return code;
}

/*"PUT",     /rf/@ch/@subch/@type/@value"
    射频单个参数设置
    @type： 设置参数类型; 
        mode:射频工作模式
        gain:增益模式
        
*/
int cmd_rf_single_value_set(struct uh_client *cl, void **arg, void **content)
{
    const char *s_type, *s_ch, *s_subch, *s_value;
    int ch, subch, itype;
    int64_t value = 0;
    int code = RESP_CODE_OK;
    struct poal_config *config = &(config_get_config()->oal_config);
    s_type = cl->get_restful_var(cl, "type");
    s_ch = cl->get_restful_var(cl, "ch");
    s_subch = cl->get_restful_var(cl, "subch");
    s_value = cl->get_restful_var(cl, "value");
    printf_note("rf type=%s,ch = %s, subch=%s, value=%s\n", s_type, s_ch, s_subch, s_value);
    if(str_to_int(s_ch, &ch, check_valid_ch) == false){
        code = RESP_CODE_CHANNEL_ERR;
        goto error;
    }
    if(s_subch != NULL){
        if(str_to_int(s_subch, &subch, check_valid_subch) == false){
            code = RESP_CODE_CHANNEL_ERR;
            goto error;
        }
    }else{
        subch = -1;
    }
        
    if(s_value == NULL || str_to_s64(s_value, &value, NULL) == false){
        code = RESP_CODE_PATH_PARAM_ERR;
        goto error;
    }

    itype = find_idx_safe(rf_types, ARRAY_SIZE(rf_types), s_type);
    if (itype == EX_RF_MGC_GAIN || itype == EX_RF_ATTENUATION) {
        if (config->channel[ch].rf_para.gain_ctrl_method == POAL_AGC_MODE) {
             goto error;
        }
    }
    config_write_data(EX_RF_FREQ_CMD, itype, ch,&value);
    if(executor_set_command(EX_RF_FREQ_CMD, itype, ch,&value) != 0){
        code = RESP_CODE_EXECMD_ERR;
        goto error;
    }
    
error:
    *arg = get_resp_message(code);
    return code;
}

/*"POST",     /rf/@ch/@subch"
    射频参数批量设置
*/
int cmd_rf_multi_value_set(struct uh_client *cl, void **arg, void **content)
{
    const char *s_ch, *s_subch;
    int  ch, subch;
    int code = RESP_CODE_OK;
    
    s_ch = cl->get_restful_var(cl, "ch");
    s_subch = cl->get_restful_var(cl, "subch");
    printf_note("rf ch = %s, subch=%s\n", s_ch, s_subch);

    if(str_to_int(s_ch, &ch, check_valid_ch) == false){
        code = RESP_CODE_CHANNEL_ERR;
        goto error;
    }
    if(s_subch != NULL){
        if(str_to_int(s_subch, &subch, check_valid_subch) == false){
            code = RESP_CODE_CHANNEL_ERR;
            goto error;
        }
    }else{
        subch = -1;
    }
    printf_note("%s\n", cl->dispatch.body);
    code = parse_json_rf_multi_value(cl->dispatch.body, ch);
    
error:
    *arg = get_resp_message(code);
    return code;
}


int cmd_netset(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
    printf_note("%s\n", cl->dispatch.body);
    code = parse_json_net(cl->dispatch.body);
    *arg = get_resp_message(code);
    return code;
}

int cmd_net_client(struct uh_client *cl, void **arg, void **content)
{
    const char *s_ch;
    int  ch;
    int code = RESP_CODE_OK;
    
    s_ch = cl->get_restful_var(cl, "ch");
    if(str_to_int(s_ch, &ch, check_valid_ch) == false){
        code = RESP_CODE_CHANNEL_ERR;
        goto error;
    }
    printf_note("ch:%d, %s\n", ch, cl->dispatch.body);
    code = parse_json_client_net(ch, cl->dispatch.body, NULL);
error:
    *arg = get_resp_message(code);
    return code;
}

/*
    接收数据客户端网络参数设置
    "/net/@ch/client/@type"
    @type: fft,biq,niq
*/
int cmd_net_client_type(struct uh_client *cl, void **arg, void **content)
{
    const char *s_ch, *type;
    int  ch;
    int code = RESP_CODE_OK;
    
    s_ch = cl->get_restful_var(cl, "ch");
    type = cl->get_restful_var(cl, "type");
    if(str_to_int(s_ch, &ch, check_valid_ch) == false){
        code = RESP_CODE_CHANNEL_ERR;
        goto error;
    }
    printf_note("ch:%d, %s\n", ch, cl->dispatch.body);
    code = parse_json_client_net(ch, cl->dispatch.body, type);
error:
    *arg = get_resp_message(code);
    return code;
}


/* "PUT",     "/enable/@ch/@type/@value"
    主通道使能
    @type: "psd", "iq", "audio"
    @value: 1->enable, 0->disable
*/
int cmd_ch_enable_set(struct uh_client *cl, void **arg, void **content)
{
    const char *s_type, *s_ch, *s_enable;
    int ch, enable;
    int code = RESP_CODE_OK;
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    
    s_type = cl->get_restful_var(cl, "type");
    s_ch = cl->get_restful_var(cl, "ch");
    s_enable = cl->get_restful_var(cl, "value");
    printf_note("enable type=%s,ch = %s, value=%s\n", s_type, s_ch, s_enable);
    if(str_to_int(s_ch, &ch, check_valid_ch) == false){
        code = RESP_CODE_CHANNEL_ERR;
        goto error;
    }
    if(str_to_int(s_enable, &enable, check_valid_enable) == false){
        code = RESP_CODE_CHANNEL_ERR;
        goto error;
    }
    printf_info("enable ch=%d, enable=%d\n", ch, enable);
    poal_config->channel[ch].enable.cid = ch;
    if(!strcmp(s_type, "psd")){
        if(enable)
            poal_config->channel[ch].enable.map.bit.fft = 1;
        else
            poal_config->channel[ch].enable.map.bit.fft = 0;
        executor_set_command(EX_FFT_ENABLE_CMD, -1, ch, &enable);
    }else if(!strcmp(s_type, "iq")||!strcmp(s_type, "biq")){
        if(enable)
            poal_config->channel[ch].enable.map.bit.iq = 1;
        else
            poal_config->channel[ch].enable.map.bit.iq = 0;
        executor_set_command(EX_BIQ_ENABLE_CMD, -1, ch, &enable);
    }else if(!strcmp(s_type, "audio")){
        if(enable)
            poal_config->channel[ch].enable.map.bit.audio = 1;
        else
            poal_config->channel[ch].enable.map.bit.audio = 0;
        executor_set_command(EX_FFT_ENABLE_CMD, -1, ch, &enable);
    }else{
        code = RESP_CODE_PATH_PARAM_ERR;
        goto error;
    }
error:
    *arg = get_resp_message(code);
    return code;

}

/* "PUT",     "/enable/@ch/@subch/@type/@value" 
    子通道使能
    @type: "psd", "iq", "audio"
    @value: 1->enable, 0->disable
*/
int cmd_subch_enable_set(struct uh_client *cl, void **arg, void **content)
{
    const char *s_type, *s_ch, *s_subch, *s_enable;
    int ch, subch,enable;
    int code = RESP_CODE_OK;
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    
    s_type = cl->get_restful_var(cl, "type");
    s_ch = cl->get_restful_var(cl, "ch");
    s_subch = cl->get_restful_var(cl, "subch");
    s_enable = cl->get_restful_var(cl, "value");
    if(str_to_int(s_ch, &ch, check_valid_ch) == false){
        code = RESP_CODE_CHANNEL_ERR;
        goto error;
    }
    if(str_to_int(s_enable, &enable, check_valid_enable) == false){
        code = RESP_CODE_PATH_PARAM_ERR;
        goto error;
    }
    if(str_to_int(s_subch, &subch, check_valid_subch) == false){
        code = RESP_CODE_CHANNEL_ERR;
        goto error;
    }
    poal_config->channel[ch].sub_channel_para.sub_ch_enable[subch].cid = ch;
    poal_config->channel[ch].sub_channel_para.sub_ch_enable[subch].sub_id = subch;
    if(!strcmp(s_type, "iq")||!strcmp(s_type, "niq")){
        executor_set_command(EX_NIQ_ENABLE_CMD, -1, ch, &enable, subch);
    }else if(!strcmp(s_type, "audio")){
        executor_set_command(EX_NIQ_ENABLE_CMD, -1, ch, &enable, subch);
    }else{
        code = RESP_CODE_PATH_PARAM_ERR;
        goto error;
    }
    printf_debug("enable type=%s,ch = %d, subch=%d, enable=%d\n", s_type, ch, subch, enable);

error:
    *arg = get_resp_message(code);
    return code;
}

/* "GET",     "/file/autostore/@ch/@enable?time=5&unitsize=2000000&cacheTime=4" 
    文件自动存储开始/停止
    @value: 1->enable, 0->disable
*/
int cmd_file_auto_store(struct uh_client *cl, void **arg, void **content)
{
    const char *s_ch, *s_enable;
    int ch, enable;
    int code = RESP_CODE_OK;
    const char *path, *time,*unitsize,*cacheTime;
    int ret = -1;
    int itime = 0, ictime = 0;
    uint64_t usize = 0;
    struct poal_config *config = &(config_get_config()->oal_config);

    s_ch = cl->get_restful_var(cl, "ch");
    s_enable = cl->get_restful_var(cl, "enable");
    path = cl->get_var(cl, "path");
    time = cl->get_var(cl, "time");
    unitsize = cl->get_var(cl, "unitsize");
    cacheTime = cl->get_var(cl, "cacheTime");
    printf_note("path=%s, time=%s, unitsize=%s, cacheTime=%s\n", path, time, unitsize, cacheTime);
    if(str_to_int(s_ch, &ch, check_valid_ch) == false){
        code = RESP_CODE_CHANNEL_ERR;
        goto error;
    }
    if(str_to_int(s_enable, &enable, check_valid_enable) == false){
        code = RESP_CODE_PATH_PARAM_ERR;
        goto error;
    }
    if(enable){
        if(time && str_to_int(time, &itime, NULL) == false){
            code = RESP_CODE_PATH_PARAM_ERR;
            goto error;
        }
        if(cacheTime && str_to_int(cacheTime, &ictime, NULL) == false){
            code = RESP_CODE_PATH_PARAM_ERR;
            goto error;
        }
        if(unitsize && str_to_u64(unitsize, &usize, NULL) == false){
            code = RESP_CODE_PATH_PARAM_ERR;
            goto error;
        }
    }
#if defined(CONFIG_FS)
    if(enable){
        memset(&config->ctrl_para.fs_save_args, 0, sizeof(struct fs_save_sample_args));
        config->ctrl_para.fs_save_args.cache_time = ictime;
        config->ctrl_para.fs_save_args.sample_time = itime;
        config_set_split_file_threshold(usize);
        config_set_file_auto_save_mode(true);
    }
    else
        config_set_file_auto_save_mode(false);
#endif
    if(ret == -1)
        code = RESP_CODE_EXECMD_ERR;

error:
    *arg = get_resp_message(code);
    return code;
}


int cmd_file_pause_backtrace(struct uh_client *cl, void **arg, void **content)
{
    const char *s_ch, *s_enable;
    int ch, enable = 0, ret = -1;
    int code = RESP_CODE_OK;

    s_ch = cl->get_restful_var(cl, "ch");
    s_enable = cl->get_restful_var(cl, "enable");
    if(str_to_int(s_ch, &ch, check_valid_ch) == false){
        code = RESP_CODE_CHANNEL_ERR;
        goto error;
    }
    if(str_to_int(s_enable, &enable, check_valid_enable) == false){
        code = RESP_CODE_PATH_PARAM_ERR;
        goto error;
    }
#if defined(CONFIG_FS)
    if(cl->fsctx && cl->fsctx->ops->fs_pause_read_raw_file){
        ret = cl->fsctx->ops->fs_pause_read_raw_file(ch, enable, cl->fsctx);
    }
#endif
    if(ret == -1)
        code = RESP_CODE_EXECMD_ERR;
error:
    *arg = get_resp_message(code);
    return code;
}

int cmd_file_pause_store(struct uh_client *cl, void **arg, void **content)
{
    const char *s_ch, *s_enable;
    int ch, enable = 0, ret = -1;
    int code = RESP_CODE_OK;

    s_ch = cl->get_restful_var(cl, "ch");
    s_enable = cl->get_restful_var(cl, "enable");
    if(str_to_int(s_ch, &ch, check_valid_ch) == false){
        code = RESP_CODE_CHANNEL_ERR;
        goto error;
    }
    if(str_to_int(s_enable, &enable, check_valid_enable) == false){
        code = RESP_CODE_PATH_PARAM_ERR;
        goto error;
    }
#if defined(CONFIG_FS)
    if(cl->fsctx && cl->fsctx->ops->fs_pause_save_file){
        ret = cl->fsctx->ops->fs_pause_save_file(ch, enable, cl->fsctx);
    }
#endif
    if(ret == -1)
        code = RESP_CODE_EXECMD_ERR;
error:
    *arg = get_resp_message(code);
    return code;
}
/* "GET",     "/file/store/@ch/@enable?time=5&unitsize=20000000&size=1000000" 
    文件存储开始/停止
    @value: 1->enable, 0->disable
*/
int cmd_file_store(struct uh_client *cl, void **arg, void **content)
{
    const char *s_ch, *s_enable;
    int ch, enable;
    int code = RESP_CODE_OK;
    const char *path, *time,*unitsize,*cacheTime;
    int ret = -1;
    int itime = 0, ictime = 0;
    uint64_t usize = 0;

    s_ch = cl->get_restful_var(cl, "ch");
    s_enable = cl->get_restful_var(cl, "enable");
    path = cl->get_var(cl, "path");
    time = cl->get_var(cl, "time");
    unitsize = cl->get_var(cl, "unitsize");
    cacheTime = cl->get_var(cl, "cacheTime");
    printf_note("path=%s, time=%s, unitsize=%s, cacheTime=%s\n", path, time, unitsize, cacheTime);

    if(config_get_file_auto_save_mode() == true){
        code = RESP_CODE_BUSY;
        goto error;
    }
    if(str_to_int(s_ch, &ch, check_valid_ch) == false){
        code = RESP_CODE_CHANNEL_ERR;
        goto error;
    }
    if(str_to_int(s_enable, &enable, check_valid_enable) == false){
        code = RESP_CODE_PATH_PARAM_ERR;
        goto error;
    }
    if(enable){
        if(time && str_to_int(time, &itime, NULL) == false){
            code = RESP_CODE_PATH_PARAM_ERR;
            goto error;
        }
        if(cacheTime && str_to_int(cacheTime, &ictime, NULL) == false){
            code = RESP_CODE_PATH_PARAM_ERR;
            goto error;
        }
        if(unitsize && str_to_u64(unitsize, &usize, NULL) == false){
            code = RESP_CODE_PATH_PARAM_ERR;
            goto error;
        }
    }
 
#if defined(CONFIG_FS)
    char default_path[PATH_MAX];
    //struct fs_context *fs_ctx = get_fs_ctx();
    if(cl->fsctx){
        if(path == NULL){
            if(cl->fsctx->ops->fs_create_default_path){
                if(cl->fsctx->ops->fs_create_default_path(ch, default_path, PATH_MAX,  NULL) != 0){
                    code = RESP_CODE_EXECMD_ERR;
                    goto error;
                }
                path = default_path;
            }else{
                code = RESP_CODE_EXECMD_ERR;
                goto error;
            }
        }
        if(enable){
            struct fs_save_sample_args args;
            memset(&args, 0, sizeof(args));
            if(itime > 0)
                args.sample_time = itime;
            if(ictime > 0)
                args.cache_time = ictime;
            if(usize > 0)
                config_set_split_file_threshold(usize);
            ret = cl->fsctx->ops->fs_start_save_file(ch, path, &args, cl->fsctx);
        }
        else
            ret = cl->fsctx->ops->fs_stop_save_file(ch, path, cl->fsctx);
    }
#endif
    if(ret == -1)
        code = RESP_CODE_EXECMD_ERR;
error:
    *arg = get_resp_message(code);
    return code;

}

/* "GET",     "/file/@filename" 
    文件下载命令
    @value: 1->enable, 0->disable
*/
int cmd_file_download(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
#if defined(CONFIG_FS_XW)
    const char *filename;
    filename = cl->get_restful_var(cl, "filename");
    file_download(cl, filename);
#endif

    *arg = get_resp_message(code);
    return code;

}


/* "DELETE",     "/file?path=/path/name" 
    单文件删除命令
*/
int cmd_file_delete(struct uh_client *cl, void **arg, void **content)
{
    const char  *filename;
    int code = RESP_CODE_OK;
    int ret = 0;

    filename = cl->get_var(cl, "path");
#if defined(CONFIG_FS_XW)
    xwfs_delete_file(filename);
#endif
#if defined(CONFIG_FS)
    struct fs_context *fs_ctx;
    fs_ctx = get_fs_ctx();
    if(fs_ctx == NULL){
        code = RESP_CODE_DISK_DETECTED_ERR;
        goto error;
    }
    ret = fs_ctx->ops->fs_delete(filename);
    if(ret != 0)
        code = RESP_CODE_EXECMD_ERR;
    error:
#else
    filename = filename;
#endif
    *arg = get_resp_message(code);
    return code;
}

/* "POST",     "/file/delete" 
    批量文件删除命令
*/
int cmd_file_batch_delete(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
    
    code = parse_json_batch_delete(cl->dispatch.body);
    *arg = get_resp_message(code);
    return code;
}

/*
    指定文件位置回溯
    /file/backtrace/offset/@ch/@value?path=/xxx/path/file.dat
*/
int cmd_file_backtrace_offset(struct uh_client *cl, void **arg, void **content)
{
    const char *s_ch, *s_value, *path;
    int ch, ret = -1;
    size_t offset;
    int code = RESP_CODE_OK;
    
    s_ch = cl->get_restful_var(cl, "ch");
    s_value = cl->get_restful_var(cl, "value");
    path = cl->get_var(cl, "path");

    if(str_to_int(s_ch, &ch, check_valid_ch) == false){
        code = RESP_CODE_CHANNEL_ERR;
        goto error;
    }
    if(str_to_u64(s_value, &offset, NULL) == false){
        code = RESP_CODE_PATH_PARAM_ERR;
        goto error;
    }
    printf_note("ch:%s, offset:%s, path:%s\n", s_ch, s_value, path);
#if defined(CONFIG_FS)
    if(cl->fsctx && cl->fsctx->ops->fs_set_raw_file_offset){
        ret = cl->fsctx->ops->fs_set_raw_file_offset(ch, path, offset, cl->fsctx);
    }
#endif

    if(ret == -1)
        code = RESP_CODE_EXECMD_ERR;
error:
    *arg = get_resp_message(code);
    return code;
}

/* "GET",     "/file/backtrace/@ch/@enable?path=/xxx/path/file.dat" 
    文件开始/停止回溯命令
    @value: 1->enable, 0->disable
*/
int cmd_file_backtrace(struct uh_client *cl, void **arg, void **content)
{
    const char *s_ch, *s_enable, *path;
    int ch, enable, ret = -1;
    int code = RESP_CODE_OK;

    s_ch = cl->get_restful_var(cl, "ch");
    s_enable = cl->get_restful_var(cl, "enable");
    path = cl->get_var(cl, "path");
    
    printf_note("ch=%s, en=%s, path=%s\n", s_ch,s_enable, path);
    if(str_to_int(s_ch, &ch, check_valid_ch) == false){
        code = RESP_CODE_CHANNEL_ERR;
        goto error;
    }
    if(str_to_int(s_enable, &enable, check_valid_enable) == false){
        code = RESP_CODE_PATH_PARAM_ERR;
        goto error;
    }
#if defined(CONFIG_FS)
    struct fs_context *fs_ctx = get_fs_ctx();
    if(fs_ctx){
#if defined(CONFIG_ARCH_ARM)
        if(fs_ctx->ops->fs_get_xattr){
            uint64_t freq = 0;
            uint32_t bw = 0;
            struct poal_config *config = &(config_get_config()->oal_config);
            cmd_set_if_string(ch, "midFreq", fs_ctx->ops->fs_get_xattr(ch, path, "midFreq",  (void *)&freq, sizeof(freq)));
            cmd_set_if_string(ch, "bandwidth", fs_ctx->ops->fs_get_xattr(ch, path, "bandwidth",  (void *)&bw, sizeof(bw)));
            printf_note("freq=%"PRIu64", bw=%uhz\n", freq, bw);
            if(freq == 0 || bw == 0){
                printf_err("freq=%"PRIu64"Hz, bw=%uHz error\n", freq, bw);
                code = RESP_CODE_EXECMD_ERR;
                goto error;
            }
            if(enable)
                config->channel[ch].enable.map.bit.fft = 1;
            else
                config->channel[ch].enable.map.bit.fft = 0;
            //executor_set_command(EX_FFT_ENABLE_CMD, -1, ch, &enable);
        }
#endif
        if(enable)
            ret = fs_ctx->ops->fs_start_read_raw_file(ch, path, cl->fsctx);
        else
            ret = fs_ctx->ops->fs_stop_read_raw_file(ch, path, cl->fsctx);
    }
#endif
    if(ret == -1)
        code = RESP_CODE_EXECMD_ERR;
error:
    *arg = get_resp_message(code);
    return code;
}

/* "GET",     "/file/list" 
    获取设备文件列表命令
*/
int cmd_file_list(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
    *content = assemble_json_file_list();
    if(*content == NULL)
        code = RESP_CODE_DISK_DETECTED_ERR;

    *arg = get_resp_message(code);
    return code;
}

/* "GET",     "/file/findrec/@filename?path=." 
    递归查找文件
*/
int cmd_file_find_rec(struct uh_client *cl, void **arg, void **content)
{
    const char  *filename;
    int code = RESP_CODE_OK;
    const char *path;
    
    filename = cl->get_restful_var(cl, "filename");
    path = cl->get_var(cl, "path");
    *content = assemble_json_find_rec_file(path, filename);
    if(*content == NULL)
        code = RESP_CODE_DISK_DETECTED_ERR;

    *arg = get_resp_message(code);
    return code;
}

/* "GET",     "/file/find/@filename?path=." 
    当前目录查找文件
*/
int cmd_file_find(struct uh_client *cl, void **arg, void **content)
{
    const char  *filename;
    int code = RESP_CODE_OK;
    const char *path;
    
    filename = cl->get_restful_var(cl, "filename");
    path = cl->get_var(cl, "path");
    *content = assemble_json_find_file(path, filename);
    if(*content == NULL)
        code = RESP_CODE_DISK_DETECTED_ERR;

    *arg = get_resp_message(code);
    return code;
}

/*
    "GET",     "/file/dir?dir=." 
    根据当前目录显示当前目录和文件信息
*/
int cmd_file_dir(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
    const char *dir = cl->get_var(cl, "dir");
    if(dir == NULL){
        code = RESP_CODE_PATH_PARAM_ERR;
        goto exit;
    }
    *content = assemble_json_file_dir(dir);
    if(*content == NULL)
        code = RESP_CODE_EXECMD_ERR;
exit:
    *arg = get_resp_message(code);
    return code;
}

/*
    根据文件名称获取文件大小
    GET /file/size?path=./f.v
*/
int cmd_file_size(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
    const char *path = cl->get_var(cl, "path");
    if(path == NULL){
        code = RESP_CODE_PATH_PARAM_ERR;
        goto exit;
    }
    *content = assemble_json_file_size(path);
    if(*content == NULL)
        code = RESP_CODE_EXECMD_ERR;
exit:
    *arg = get_resp_message(code);
    return code;
}


int cmd_file_download_cb(struct uh_client *cl, void **arg, void **content)
{
    char  *filename;
    int code = RESP_CODE_OK;
    const char *path;
    int ret = -1;
    
    path = cl->get_var(cl, "path");
    if(path == NULL){
        code = RESP_CODE_PATH_PARAM_ERR;
        goto exit;
    }
    
#if defined(CONFIG_FS)
    struct fs_context *fs_ctx = get_fs_ctx();
    if(fs_ctx)
        ret = fs_ctx->ops->fs_file_download_cb(path);
#endif
    if(ret == -1){
        code = RESP_CODE_EXECMD_ERR;
    }
exit:
    *arg = get_resp_message(code);
    return code;

}

/*
    文件夹新建
    GET /folder?dir=data
*/
int cmd_folder_create(struct uh_client *cl, void **arg, void **content)
{
    const char *dir;
    int code = RESP_CODE_OK;
    int ret = -1;
    
    dir = cl->get_var(cl, "dir");
    if(dir == NULL){
        code = RESP_CODE_PATH_PARAM_ERR;
        goto exit;
    }
#if defined(CONFIG_FS)
    struct fs_context *fs_ctx = get_fs_ctx();
    if(fs_ctx  && fs_ctx->ops->fs_mkdir){
        ret = fs_ctx->ops->fs_mkdir(dir);
        if(ret == -1)
            code = RESP_CODE_EXECMD_ERR;
    }
#endif
    exit:
    *arg = get_resp_message(code);
    return code;
}

/* 文件夹删除
    DELETE /folder?dir=data/test1
*/
int cmd_folder_delete(struct uh_client *cl, void **arg, void **content)
{
    const char *dir;
    int code = RESP_CODE_OK;
    char path[PATH_MAX];
    char cmd[PATH_MAX];
    int ret = -1;
    
    dir = cl->get_var(cl, "dir");
    if(dir == NULL){
        code = RESP_CODE_PATH_PARAM_ERR;
        goto exit;
    }
#if defined(CONFIG_FS)
    struct fs_context *fs_ctx = get_fs_ctx();
    if(fs_ctx  && fs_ctx->ops->fs_root_dir){
        snprintf(path, PATH_MAX - 1, "%s/%s", fs_ctx->ops->fs_root_dir(), dir);
        printf_note("remove: %s\n", path);
        //ret = remove(path);
        snprintf(cmd, PATH_MAX - 1, "rm -r %s", path);
        ret = safe_system(cmd);
        if(ret == 0){
            printf_note("remove %s OK\n", path);
        }else{
            printf_note("remove %s Failed[%d, %s]\n", path, errno, strerror(errno));
            code = RESP_CODE_FILE_NOT_EXIST;
            *arg = strerror(errno);
            return code;
        }
    }
#endif
    exit:
    *arg = get_resp_message(code);
    return code;
}

int cmd_disk_format(struct uh_client *cl, void **arg, void **content)
{
    char  *filename;
    int code = RESP_CODE_OK, ret = 0;
    
#if defined(CONFIG_FS)
    struct fs_context *fs_ctx;
    fs_ctx = get_fs_ctx();
    if(fs_ctx == NULL){
        code = RESP_CODE_DISK_DETECTED_ERR;
        goto error;
    }
    ret = fs_ctx->ops->fs_format();
    if(ret != 0)
        code = RESP_CODE_DISK_FORMAT_ERR;
    error:
#endif
    *arg = get_resp_message(code);
    return code;
}

int cmd_ping(struct uh_client *cl, void **arg, void **content)
{
    *arg = get_resp_message(RESP_CODE_OK);
    return RESP_CODE_OK;
}

int cmd_get_softversion(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
    *content = assemble_json_softversion();
    if(*content == NULL)
        code = RESP_CODE_EXECMD_ERR;

    *arg = get_resp_message(code);
    return code;
}

int cmd_get_fpga_info(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
    *content = assemble_json_fpag_info();
    if(*content == NULL)
        code = RESP_CODE_EXECMD_ERR;

    *arg = get_resp_message(code);
    return code;
}
int cmd_get_temp_info(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
    *content = assemble_json_temp_info();
    if(*content == NULL)
        code = RESP_CODE_EXECMD_ERR;

    *arg = get_resp_message(code);
    return code;
}
int cmd_get_all_info(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
    *content = assemble_json_all_info();
    if(*content == NULL)
        code = RESP_CODE_EXECMD_ERR;
    
    *arg = get_resp_message(code);
    return code;
}

int cmd_get_selfcheck_info(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
    *content = assemble_json_selfcheck_info();
    if(*content == NULL)
        code = RESP_CODE_EXECMD_ERR;
    
    *arg = get_resp_message(code);
    return code;
}

int cmd_netget(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
    *content = assemble_json_netlist_info();
    if(*content == NULL)
        code = RESP_CODE_EXECMD_ERR;
    *arg = get_resp_message(code);
    return code;
}
/* gps: gps授时  irib：irib授时 */
int cmd_timesource_set(struct uh_client *cl, void **arg, void **content)
{
    const char *s_value;
    int code = RESP_CODE_OK;

    s_value = cl->get_restful_var(cl, "value");
    if(!strcmp(s_value, "gps")){
        printf_note("timesource: GPS\n");
    }else if(!strcmp(s_value, "irib")){
        printf_note("timesource: irib\n");
    } else
        code = RESP_CODE_PARSE_ERR;

    *arg = get_resp_message(code);
    return code;
}
int cmd_rf_power_onoff(struct uh_client *cl, void **arg, void **content)
{
    const char *s_value;
    int code = RESP_CODE_OK;

    s_value = cl->get_restful_var(cl, "value");
    if(!strcmp(s_value, "on")){
        printf_note("RF Power On\n");
#ifdef RF_POWER_ON
        RF_POWER_ON();
#endif
    }else if(!strcmp(s_value, "off")){
        printf_note("RF Power Off\n");
#ifdef RF_POWER_OFF
        RF_POWER_OFF();
#endif
    } else
        code = RESP_CODE_PARSE_ERR;

    *arg = get_resp_message(code);
    return code;
}

/* AGC  控制模式 0: 分段AGC（默认） 1：整个频段AGC */
int cmd_rf_agcmode(struct uh_client *cl, void **arg, void **content)
{
    struct poal_config *config = &(config_get_config()->oal_config);
    const char *s_value;
    int code = RESP_CODE_OK;

    s_value = cl->get_restful_var(cl, "value");
    if(!strcmp(s_value, "1")){
        printf_note("AGC mode: range ctrl!\n");
        config->ctrl_para.agc_ctrl_mode = 1;
    }else if(!strcmp(s_value, "0")){
        printf_note("AGC mode: segment ctrl!\n");
        config->ctrl_para.agc_ctrl_mode = 0;
    } else
     code = RESP_CODE_PARSE_ERR;

    *arg = get_resp_message(code);
    return code;

}

int cmd_set_devicecode(struct uh_client *cl, void **arg, void **content)
{
    struct poal_config *config = &(config_get_config()->oal_config);
    const char *s_value;
    int code = RESP_CODE_OK;
    s_value = cl->get_restful_var(cl, "value");
    if(s_value && strlen(s_value) > 0){
        if(config->ctrl_para.device_code != NULL){
            free(config->ctrl_para.device_code);
            config->ctrl_para.device_code = NULL;
        }
        config->ctrl_para.device_code = strdup(s_value);
    }else{
        code = RESP_CODE_EXECMD_ERR;
    }
    *arg = get_resp_message(code);
    return code;
}