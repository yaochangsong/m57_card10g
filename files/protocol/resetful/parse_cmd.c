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


#define RESP_CODE_OK                0
#define RESP_CODE_CHANNEL_ERR       -1
#define RESP_CODE_PATH_PARAM_ERR    -2
#define RESP_CODE_PARSE_ERR         -3
#define RESP_CODE_EXECMD_ERR        -4


static struct response_err_code {
    int code;
    char *message;
};


static struct response_err_code resp_code[] ={
    {RESP_CODE_OK,                  "ok"},
    {RESP_CODE_CHANNEL_ERR,         "channel error"},
    {RESP_CODE_PATH_PARAM_ERR,      "path param error"},
    {RESP_CODE_PARSE_ERR,           "parse json error"},
    {RESP_CODE_EXECMD_ERR,          "execute cmd error"}
};


/* 射频参数类型 */
static const char *const rf_types[] = {
    [EX_RF_MID_FREQ] = "middleFreq",
    [EX_RF_MID_BW] = "middleBw",
    [EX_RF_MODE_CODE] = "mode",
    [EX_RF_GAIN_MODE] = "gain",
    [EX_RF_MGC_GAIN] = "mgc",
    [EX_RF_AGC_CTRL_TIME]="agctime",          
    [EX_RF_AGC_OUTPUT_AMP]="agcamp",          
    [EX_RF_ANTENNA_SELECT]="actenna",          
    [EX_RF_ATTENUATION] = "attenuation",
    [EX_RF_STATUS_TEMPERAT] = "temperature",         
    [EX_RF_CALIBRATE] = "calibrate",
};

/* 中频参数类型 */
static const char *const if_types[] = {
    [EX_MUTE_SW] = "middleFreq",
    [EX_MUTE_THRE] = "muteThre",              /*静噪门限*/
    [EX_MID_FREQ]  = "middleFreq",            /*中心频率*/
    [EX_BANDWITH] = "bandWidth",               /*带宽*/
    [EX_DEC_METHOD] = "decMethod"    ,        /*解调方式*/

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
         printf_note("null func\n");
         return true;
    }
       
    return ((*_check)(value));
}

bool parse_if_cmd(int cmd, char *value, int ch, int subch)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    struct multi_freq_point_para_st *point;
    point = &poal_config->multi_freq_point_param[ch];
    printf_note("if cmd:%d\n", cmd);
    switch(cmd){
        case EX_MID_FREQ:
        {
            uint64_t ivalue;
            if(str_to_u64(value, &ivalue, NULL) == false){
                return false;
            }
            executor_set_command(EX_MID_FREQ_CMD, cmd, ch,&ivalue,  ivalue);
            break;
        }
        default:
            break;
    }
    return true;
}

bool parse_rf_cmd(int cmd, char *value, int ch, int subch)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    struct multi_freq_point_para_st *point;
    point = &poal_config->multi_freq_point_param[ch];
    switch(cmd){
        case EX_RF_MID_FREQ:
        {
            uint64_t ivalue;
            if(str_to_u64(value, &ivalue, NULL) == false){
                return false;
            }
            executor_set_command(EX_RF_FREQ_CMD, cmd, ch,&ivalue,  ivalue);
            break;
        }
        case EX_RF_MODE_CODE:
        {
            int mode;
            if(str_to_int(value, &mode, NULL) == false){
                return false;
            }
            poal_config->rf_para[ch].rf_mode_code = (uint8_t)mode;
            executor_set_command(EX_RF_FREQ_CMD, EX_RF_MODE_CODE, ch, &poal_config->rf_para[ch].rf_mode_code);
            break;
        }
        case EX_RF_MID_BW:
        {
            uint32_t bw;
            if(str_to_uint(value, &bw, NULL) == false){
                return false;
            }
            poal_config->rf_para[ch].mid_bw = bw;
            executor_set_command(EX_RF_FREQ_CMD, EX_RF_MID_BW, ch, &bw);
            break;
        }
        default:
            break;
    }
}


/* POST /mode/mutiPoint/@ch */
int cmd_muti_point(struct uh_client *cl, void **arg)
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
    if(parse_json_muti_point(cl->dispatch.body)){
        code = RESP_CODE_PARSE_ERR;
        goto error;
    }

error:
    *arg = get_resp_message(code);
    return code;

}


/* "POST","/mode/multiBand/@ch" */
int cmd_multi_band(struct uh_client *cl, void **arg)
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
    if(parse_json_multi_band(cl->dispatch.body) != 0){
        code = RESP_CODE_PARSE_ERR;
        goto error;
    }
error:
    *arg = get_resp_message(code);
    return code;

}

/* "POST","/demodulation/@ch/@subch" */
int cmd_demodulation(struct uh_client *cl, void **arg)
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
    if(parse_json_demodulation(cl->dispatch.body) != 0){
        code = RESP_CODE_PARSE_ERR;
        goto error;
    }
    
error:
    *arg = get_resp_message(code);
    return code;

}

/*"PUT",     /if/@type/@ch/@subch/@value"
    中频单个参数设置
*/
int cmd_if_single_value_set(struct uh_client *cl, void **arg)
{
    char *s_type, *s_ch, *s_subch, *s_value;
    int ch, itype, subch;
    int code = RESP_CODE_OK;
    
    s_type = cl->get_restful_var(cl, "type");
    s_ch = cl->get_restful_var(cl, "ch");
    s_subch = cl->get_restful_var(cl, "subch");
    s_value = cl->get_restful_var(cl, "value");
    printf_note("if type=%s,ch = %s, subch=%s, value=%s\n", s_type, s_ch, s_subch, s_value);
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

    itype = find_idx_safe(if_types, ARRAY_SIZE(if_types), s_type);
    if(parse_if_cmd(itype, s_value, ch, subch) == false){
        code = RESP_CODE_EXECMD_ERR;
    }
    printf_note("...%d\n", code);
error:
    *arg = get_resp_message(code);
    printf_note("...%s\n", arg);
    return code;

}

/*"POST",     /if/@ch/@subch"
    中频参数批量设置
*/
int cmd_if_multi_value_set(struct uh_client *cl, void **arg)
{
    char *ch, *subch;
    int itype;
    int code = RESP_CODE_OK;
    
    ch = cl->get_restful_var(cl, "ch");
    subch = cl->get_restful_var(cl, "subch");
    printf_note("rf ch = %s, subch=%s\n", ch, subch);
    printf_note("%s\n", cl->dispatch.body);
    if(parse_json_if_multi_value(cl->dispatch.body) != 0){
        code = RESP_CODE_PARSE_ERR;
    }
    
error:
    *arg = get_resp_message(code);
    return code;
}

/*"PUT",     /rf/@type/@ch/@subch/@value"
    射频单个参数设置
    @type： 设置参数类型; 
        mode:射频工作模式
        gain:增益模式
        
*/
int cmd_rf_single_value_set(struct uh_client *cl, void **arg)
{
    char *s_type, *s_ch, *s_subch, *s_value;
    int ch, subch, itype;
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

    itype = find_idx_safe(rf_types, ARRAY_SIZE(rf_types), s_type);
    if(parse_rf_cmd(itype, s_value, ch, subch) == false){
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
int cmd_rf_multi_value_set(struct uh_client *cl, void **arg)
{
    char *s_ch, *s_subch;
    int  ch, subch;
    int code = RESP_CODE_OK;
    
    s_ch = cl->get_restful_var(cl, "ch");
    s_subch = cl->get_restful_var(cl, "subch");
    printf_note("rf ch = %s, subch=%s\n", s_subch, s_subch);

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
    if(parse_json_rf_multi_value(cl->dispatch.body) != 0){
        code = RESP_CODE_PARSE_ERR;
    }
    
error:
    *arg = get_resp_message(code);
    return code;
}


/* "PUT",     "/enable/@ch/@type/@value"
    主通道使能
    @type: "psd", "iq", "audio"
    @value: 1->enable, 0->disable
*/
int cmd_ch_enable_set(struct uh_client *cl, void **arg)
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
    poal_config->enable.cid = ch;
    
    if(!strcmp(s_type, "psd")){
        poal_config->enable.psd_en = enable;
    }else if(!strcmp(s_type, "iq")){
        poal_config->enable.iq_en = enable;
    }else if(!strcmp(s_type, "audio")){
        poal_config->enable.audio_en = enable;
    }else{
        code = RESP_CODE_PATH_PARAM_ERR;
        goto error;
    }
    INTERNEL_ENABLE_BIT_SET(poal_config->enable.bit_en,poal_config->enable);
    executor_set_enable_command(ch);
    
error:
    *arg = get_resp_message(code);
    return code;

}

/* "PUT",     "/enable/@ch/@subch/@type/@value" 
    子通道使能
    @type: "psd", "iq", "audio"
    @value: 1->enable, 0->disable
*/
int cmd_subch_enable_set(struct uh_client *cl, void **arg)
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
    poal_config->sub_ch_enable.cid = ch;
    poal_config->sub_ch_enable.sub_id = subch;
    
    if(!strcmp(s_type, "psd")){
        poal_config->sub_ch_enable.psd_en = enable;
    }else if(!strcmp(s_type, "iq")){
        
        poal_config->sub_ch_enable.iq_en = enable;
        /* 先关闭 */
        io_set_enable_command(IQ_MODE_DISABLE, ch, 0);
        executor_set_command(EX_MID_FREQ_CMD, EX_SUB_CH_ONOFF, subch, &enable);
        /* 通道IQ使能 */
        if(enable){
            /* NOTE:The parameter must be a MAIN channel, not a subchannel */
            io_set_enable_command(IQ_MODE_ENABLE, ch, 0);
        }else{
            io_set_enable_command(IQ_MODE_DISABLE, ch, 0);
        }
    }else if(!strcmp(s_type, "audio")){
        poal_config->sub_ch_enable.audio_en = enable;
    }else{
        code = RESP_CODE_PATH_PARAM_ERR;
        goto error;
    }

    printf_note("enable type=%s,ch = %d, subch=%d, enable=%d\n", s_type, ch, subch, enable);

error:
    *arg = get_resp_message(code);
    return code;
}


