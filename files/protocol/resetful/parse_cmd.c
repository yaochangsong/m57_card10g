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
#include "protocol/http/client.h"
#include "log/log.h"
#include "conf/conf.h"
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
    [EX_AUDIO_SAMPLE_RATE] = "audioSampleRate", 
    [EX_FFT_SIZE] = "fftPoints", 
    [EX_AUDIO_VOL_CTRL] = "audioVolume",
};



static inline bool check_valid_ch(int ch)
{
    return (ch > MAX_RADIO_CHANNEL_NUM ? false : true);
}

static inline bool check_valid_subch(int subch)
{
    return (subch > MAX_SIGNAL_CHANNEL_NUM ? false : true);
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

static inline bool str_to_int(char *str, int *ivalue, bool(*_check)(int))
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
         printf_note("null func\n");
         return true;
    }
       
    return ((*_check)(value));
}

static inline bool str_to_uint(char *str, uint32_t *ivalue, bool(*_check)(int))
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
         printf_note("null func\n");
         return true;
    }
       
    return ((*_check)(value));
}


 

static inline bool str_to_u64(char *str, uint64_t *ivalue, bool(*_check)(int))
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
         printf_debug("null check func\n");
         return true;
    }
       
    return ((*_check)(value));
}

static inline bool str_to_s64(char *str, int64_t *ivalue, bool(*_check)(int))
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


/* POST /mode/mutiPoint/@ch */
int cmd_muti_point(struct uh_client *cl, void **arg, void **content)
{
    char *s_ch;
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
    char *s_ch;
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
    char *s_ch, *s_subch;
    int ch, subch;
    int code = RESP_CODE_OK;
    
    s_ch = cl->get_restful_var(cl, "ch");
    s_subch = cl->get_restful_var(cl, "subch");
    printf_note("ch = %s, subch=%s\n", s_ch, s_subch);
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
    printf_note("%s\n", cl->dispatch.body);
    code = parse_json_demodulation(cl->dispatch.body,ch,subch);
error:
    *arg = get_resp_message(code);
    return code;

}

/*"PUT",     /if/@ch/@subch/@type/@value"
    中频单个参数设置
*/
int cmd_if_single_value_set(struct uh_client *cl, void **arg, void **content)
{
    char *s_type, *s_ch, *s_subch, *s_value, *s_value_2;
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
    char *s_ch, *subch;
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
    char *s_type, *s_ch, *s_subch, *s_value;
    int ch, subch, itype;
    int64_t value = 0;
    int code = RESP_CODE_OK;
    
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
    char *s_ch, *s_subch;
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
    char *s_ch;
    int  ch;
    int code = RESP_CODE_OK;
    
    s_ch = cl->get_restful_var(cl, "ch");
    if(str_to_int(s_ch, &ch, check_valid_ch) == false){
        code = RESP_CODE_CHANNEL_ERR;
        goto error;
    }
    printf_note("ch:%d, %s\n", ch, cl->dispatch.body);
    code = parse_json_client_net(ch, cl->dispatch.body);
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
    char *s_type, *s_ch, *s_enable;
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

    printf_note("enable ch=%d, enable=%d\n", ch, enable);
    poal_config->channel[ch].enable.cid = ch;
    if(!strcmp(s_type, "psd")){
        executor_set_command(EX_FFT_ENABLE_CMD, -1, ch, &enable);
    }else if(!strcmp(s_type, "iq")){
        executor_set_command(EX_BIQ_ENABLE_CMD, -1, ch, &enable);
    }else if(!strcmp(s_type, "audio")){
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
    char *s_type, *s_ch, *s_subch, *s_enable;
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
    if(!strcmp(s_type, "iq")){
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


/* "POST",     "/file/store/@ch/@enable/@filename" 
    文件存储开始/停止
    @value: 1->enable, 0->disable
*/
int cmd_file_store(struct uh_client *cl, void **arg, void **content)
{
    char *s_ch, *s_enable, *filename;
    int ch, enable;
    int code = RESP_CODE_OK;

    s_ch = cl->get_restful_var(cl, "ch");
    s_enable = cl->get_restful_var(cl, "enable");
    filename = cl->get_restful_var(cl, "filename");
    if(str_to_int(s_ch, &ch, check_valid_ch) == false){
        code = RESP_CODE_CHANNEL_ERR;
        goto error;
    }
    if(str_to_int(s_enable, &enable, check_valid_enable) == false){
        code = RESP_CODE_PATH_PARAM_ERR;
        goto error;
    }
    code = parse_json_file_store(cl->dispatch.body, ch, enable, filename);
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
#if defined(SUPPORT_XWFS)
    char *filename;
    filename = cl->get_restful_var(cl, "filename");
    file_download(cl, filename);
#endif

    *arg = get_resp_message(code);
    return code;

}


/* "DELETE",     "/file/@filename" 
    文件删除命令
    @value: 1->enable, 0->disable
*/
int cmd_file_delete(struct uh_client *cl, void **arg, void **content)
{
    char  *filename;
    int code = RESP_CODE_OK;
    int ret = 0;
    
    filename = cl->get_restful_var(cl, "filename");
#if defined(SUPPORT_XWFS)
    xwfs_delete_file(filename);
#endif
#if defined(SUPPORT_FS)
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
#endif
    *arg = get_resp_message(code);
    return code;
}

/* "POST",     "/file/backtrace/@ch/@enable/@filename" 
    文件开始/停止回溯命令
    @value: 1->enable, 0->disable
*/
int cmd_file_backtrace(struct uh_client *cl, void **arg, void **content)
{
    char *s_ch, *s_enable, *filename;
    int ch, enable;
    int code = RESP_CODE_OK;

    s_ch = cl->get_restful_var(cl, "ch");
    s_enable = cl->get_restful_var(cl, "enable");
    filename = cl->get_restful_var(cl, "filename");
    if(str_to_int(s_ch, &ch, check_valid_ch) == false){
        code = RESP_CODE_CHANNEL_ERR;
        goto error;
    }
    if(str_to_int(s_enable, &enable, check_valid_enable) == false){
        code = RESP_CODE_PATH_PARAM_ERR;
        goto error;
    }
    code = parse_json_file_backtrace(cl->dispatch.body, ch, enable, filename);
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

/* "GET",     "/file/find/@filename" 
    获取设备文件列表命令
*/
int cmd_file_find(struct uh_client *cl, void **arg, void **content)
{
    char  *filename;
    int code = RESP_CODE_OK;
    
    filename = cl->get_restful_var(cl, "filename");
    *content = assemble_json_find_file(filename);
    if(*content == NULL)
        code = RESP_CODE_DISK_DETECTED_ERR;

    *arg = get_resp_message(code);
    return code;
}

int cmd_disk_format(struct uh_client *cl, void **arg, void **content)
{
    char  *filename;
    int code = RESP_CODE_OK, ret = 0;
    
#if defined(SUPPORT_FS)
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
