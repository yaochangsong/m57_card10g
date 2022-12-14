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
    {RESP_CODE_EXECMD_REBOOT,               "wait! reboot application to take effect!"},
};

/* ?????????????????? */
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

/* ?????????????????? */
static const char *const if_types[] = {
    [EX_MUTE_SW] = "muteSwitch",
    [EX_MUTE_THRE] = "muteThreshold",              /*????????????*/
    [EX_MID_FREQ]  = "midFreq",            /*????????????*/
    [EX_BANDWITH] = "bandwidth",               /*??????*/
    [EX_DEC_METHOD] = "decMethodId"    ,        /*????????????*/
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

static inline bool check_valid_slot_id(int id)
{
    return ((id > 5 || id <2) ? false : true);
}

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

static inline bool str_to_hex(char *str, int *ivalue, bool(*_check)(int))
{
    char *end;
    int value;
    
    if(str == NULL || ivalue == NULL)
        return false;
    
    value = (int) strtol(str, &end, 16);
    if (str == end){
        return false;
    }
    *ivalue = value;
    if(*_check == NULL){
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

int cmd_bddc(struct uh_client *cl, void **arg, void **content)
{
    char *s_ch;
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
    ????????????????????????
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
    ????????????????????????
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
    ????????????????????????
    @type??? ??????????????????; 
        mode:??????????????????
        gain:????????????
        
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
    ????????????????????????
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
    code = parse_json_client_net(ch, cl->dispatch.body, NULL);
error:
    *arg = get_resp_message(code);
    return code;
}

/*
    ???????????????????????????????????????
    "/net/@ch/client/@type"
    @type: fft,biq,niq
*/
int cmd_net_client_type(struct uh_client *cl, void **arg, void **content)
{
    char *s_ch, *type;
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
    ???????????????
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

    printf_info("enable ch=%d, enable=%d\n", ch, enable);
    poal_config->channel[ch].enable.cid = ch;
    if(!strcmp(s_type, "psd")){
        if(enable)
            poal_config->channel[ch].enable.psd_en = 1;
        else
            poal_config->channel[ch].enable.psd_en = 0;
        INTERNEL_ENABLE_BIT_SET(poal_config->channel[ch].enable.bit_en,poal_config->channel[ch].enable);
        executor_set_command(EX_FFT_ENABLE_CMD, -1, ch, &enable);
    }else if(!strcmp(s_type, "iq")||!strcmp(s_type, "biq")){
        if(enable)
            poal_config->channel[ch].enable.iq_en = 1;
        else
            poal_config->channel[ch].enable.iq_en = 0;
        INTERNEL_ENABLE_BIT_SET(poal_config->channel[ch].enable.bit_en,poal_config->channel[ch].enable);
        executor_set_command(EX_BIQ_ENABLE_CMD, -1, ch, &enable);
    }else if(!strcmp(s_type, "audio")){
        if(enable)
            poal_config->channel[ch].enable.audio_en = 1;
        else
            poal_config->channel[ch].enable.audio_en = 0;
        INTERNEL_ENABLE_BIT_SET(poal_config->channel[ch].enable.bit_en,poal_config->channel[ch].enable);
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
    ???????????????
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


/* "POST",     "/file/store/@ch/@enable/@filename" 
    ??????????????????/??????
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
    ??????????????????
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
    ??????????????????
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
#else
    filename = filename;
#endif
    *arg = get_resp_message(code);
    return code;
}

/* "POST",     "/file/backtrace/@ch/@enable/@filename" 
    ????????????/??????????????????
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
    ??????????????????????????????
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
    ??????????????????????????????
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

int cmd_get_selfcheck_info(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
    *content = assemble_json_selfcheck_info();
    if(*content == NULL)
        code = RESP_CODE_EXECMD_ERR;
    
    *arg = get_resp_message(code);
    return code;
}

int cmd_get_status_uplink_info(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
    *content = assemble_json_net_uplink_info(0);
    if(*content == NULL)
        code = RESP_CODE_EXECMD_ERR;
    
    *arg = get_resp_message(code);
    return code;
}

int cmd_get_status_downlink_info(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
    *content = assemble_json_net_downlink_info();
    if(*content == NULL)
        code = RESP_CODE_EXECMD_ERR;
    
    *arg = get_resp_message(code);
    return code;
}

int cmd_get_status_slot_info(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
    *content = assemble_json_slot_info();
    if(*content == NULL)
        code = RESP_CODE_EXECMD_ERR;
    
    *arg = get_resp_message(code);
    return code;
}

int cmd_get_status_client_info(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
    *content = assemble_json_statistics_client_info();
    if(*content == NULL)
        code = RESP_CODE_EXECMD_ERR;
    
    *arg = get_resp_message(code);
    return code;
}

int cmd_get_status_client_sub_info(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
    *content = assemble_json_client_sub_info();
    if(*content == NULL)
        code = RESP_CODE_EXECMD_ERR;
    *arg = get_resp_message(code);
    return code;
}

int cmd_get_status_spm_hash_info(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
    *content = assemble_json_spm_hash_info();
    if(*content == NULL)
        code = RESP_CODE_EXECMD_ERR;
    *arg = get_resp_message(code);
    return code;
}


int cmd_get_statistics_info(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
    *content = assemble_json_statistics_all_info();
    if(*content == NULL)
        code = RESP_CODE_EXECMD_ERR;
    
    *arg = get_resp_message(code);
    return code;
}

int cmd_get_sys_info(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
    *content = assemble_json_sys_info();
    if(*content == NULL)
        code = RESP_CODE_EXECMD_ERR;

    *arg = get_resp_message(code);
    return code;
}

int cmd_get_rf_identify_info(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
    *content = assemble_json_rf_identify_info();
    if(*content == NULL)
        code = RESP_CODE_EXECMD_ERR;

    *arg = get_resp_message(code);
    return code;
}

int cmd_get_device_status(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
    *content = assemble_json_device_status_info();
    if(*content == NULL)
        code = RESP_CODE_EXECMD_ERR;
    
    *arg = get_resp_message(code);
    return code;
}


int cmd_get_temperature(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
    *content = assemble_json_device_temperature_info();
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

/* gps: gps??????  irib???irib?????? */
int cmd_timesource_set(struct uh_client *cl, void **arg, void **content)
{
    char *s_value;
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
    char *s_value;
    int code = RESP_CODE_OK;

    s_value = cl->get_restful_var(cl, "value");
    if(!strcmp(s_value, "on")){
        printf_note("RF Power On\n");
        RF_POWER_ON();
    }else if(!strcmp(s_value, "off")){
        printf_note("RF Power Off\n");
        RF_POWER_OFF();
    } else
        code = RESP_CODE_PARSE_ERR;

    *arg = get_resp_message(code);
    return code;
}


/* AGC  ???????????? 0: ??????AGC???????????? 1???????????????AGC */
int cmd_rf_agcmode(struct uh_client *cl, void **arg, void **content)
{
    struct poal_config *config = &(config_get_config()->oal_config);
    char *s_value;
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

int cmd_route_reload(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
    printf_note("route reload\n");
    system("/usr/bin/sroute -c /etc/nr_config.json");
    *arg = get_resp_message(code);
    return code;
}

int cmd_linkcheck_set(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
    const char *slotid, *enable;
    int _sid, _enable;

    slotid = cl->get_restful_var(cl, "slotId");
    enable = cl->get_restful_var(cl, "enable");
    if(str_to_int(slotid, &_sid, check_valid_slot_id) == false){
        code = RESP_CODE_CHANNEL_ERR;
        goto error;
    }
    if(str_to_int(enable, &_enable, check_valid_enable) == false){
        code = RESP_CODE_INVALID_PARAM;
        goto error;
    }
    printf_note("link status set: slotid: %d, enable: %d\n", _sid, _enable);
    config_set_link_switch(_sid, _enable);
error:
    *arg = get_resp_message(code);
    return code;
}

int cmd_unload_fpga_bit(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
    const char *fpga_id;
    int  _fpga_id;

    fpga_id = cl->get_restful_var(cl, "fpga_id");
    if(str_to_hex(fpga_id, &_fpga_id, NULL) == false){
        code = RESP_CODE_PARSE_ERR;
        goto error;
    }

    printf_note("unload fpga bit set: fpga_id: 0x%x\n", _fpga_id);
    if(m57_unload_bitfile_from_fpga(_fpga_id) == -1){
        code = RESP_CODE_INVALID_PARAM;
        goto error;
    }
error:
    *arg = get_resp_message(code);
    return code;
}

int cmd_unload_fpga_bit_by_id(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
    const char *slotid, *cardid;
    int  _slotid, _cardid;
    uint16_t fpga_id;

    slotid = cl->get_restful_var(cl, "slotid");
    if(str_to_hex(slotid, &_slotid, NULL) == false){
        code = RESP_CODE_PARSE_ERR;
        goto error;
    }

    cardid = cl->get_restful_var(cl, "chipid");
    if(str_to_hex(cardid, &_cardid, NULL) == false){
        code = RESP_CODE_PARSE_ERR;
        goto error;
    }
    fpga_id = ((_slotid << 8)&0xff00) | (_cardid &0xff);
    printf_note("unload fpga bit set: slotid: 0x%x, cardid: 0x%x, fpga_id:  0x%x\n", _slotid, _cardid, fpga_id);
    if(m57_unload_bitfile_from_fpga(fpga_id) == -1){
        code = RESP_CODE_INVALID_PARAM;
        goto error;
    }
error:
    *arg = get_resp_message(code);
    return code;
}

int cmd_qa_xdmaloop(struct uh_client *cl, void **arg, void **content)
{
    int code = RESP_CODE_OK;
    const char  *enable, *s_ch;
    int _enable, ch;

    s_ch = cl->get_restful_var(cl, "ch");
    if(str_to_int(s_ch, &ch, check_valid_ch) == false){
        code = RESP_CODE_CHANNEL_ERR;
        goto error;
    }

    enable = cl->get_restful_var(cl, "enable");
    if(str_to_int(enable, &_enable, check_valid_enable) == false){
        code = RESP_CODE_INVALID_PARAM;
        goto error;
    }
    //printf_note("ch:%d, enable: %d\n", ch, _enable);
    if(_enable){
        if(io_srio_data_loop_qa(ch) == false){
            code = RESP_CODE_EXECMD_ERR;
        }
    }
error:
    *arg = get_resp_message(code);
    return code;
}


