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
*  Rev 1.0   29 Feb 2020   yaochangsong
*  Initial revision.
******************************************************************************/
#include "config.h"
#include "parse_cmd.h"

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
         return true;
    }
       
    return ((*_check)(value));
}



int parse_json_client_net(int ch, const char * const body)
{
    struct poal_config *config = &(config_get_config()->oal_config);
    cJSON *node, *value;
    cJSON *root = cJSON_Parse(body);
    if (root == NULL){
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL){
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return RESP_CODE_PARSE_ERR;
    }

    cJSON *client = NULL;
    struct sockaddr_in sclient;
    client = cJSON_GetObjectItem(root, "array");
    if(client!=NULL){
        for(int i = 0; i < cJSON_GetArraySize(client); i++){
            node = cJSON_GetArrayItem(client, i);
            value = cJSON_GetObjectItem(node, "ipaddr");
            if(value!=NULL&& cJSON_IsString(value)){
                sclient.sin_addr.s_addr = inet_addr(value->valuestring);
                printf_note("client addr: %s, 0x%x\n", value->valuestring, sclient.sin_addr.s_addr);
            }
            value = cJSON_GetObjectItem(node, "port");
            if(value!=NULL&& cJSON_IsNumber(value)){
                sclient.sin_port = ntohs(value->valueint);
                printf_note("port: %d, 0x%x\n", value->valueint, value->valueint);
            }
            udp_add_client_to_list(&sclient, ch);
        }
    }
    
    return RESP_CODE_OK;
}


int parse_json_net(const char * const body)
{
    struct poal_config *config = &(config_get_config()->oal_config);
    cJSON *root = cJSON_Parse(body);
    cJSON *node, *value;
    if (root == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return RESP_CODE_PARSE_ERR;
    }
    value = cJSON_GetObjectItem(root, "ipaddr");
    if(value!=NULL&&cJSON_IsString(value)){
        config->network.ipaddress = inet_addr(value->valuestring);
        printf_note("set ipaddr: %s, 0x%x\n", value->valuestring, config->network.ipaddress);
    }
    value = cJSON_GetObjectItem(root, "netmask");
    if(value!=NULL&&cJSON_IsString(value)){
        config->network.netmask = inet_addr(value->valuestring);
        printf_note("set netmask: %s, 0x%x\n", value->valuestring, config->network.netmask);
    }
    value = cJSON_GetObjectItem(root, "gateway");
    if(value!=NULL&&cJSON_IsString(value)){
        config->network.gateway = inet_addr(value->valuestring);
        printf_note("set gateway: %s, 0x%x\n", value->valuestring, config->network.gateway);
    }
    executor_set_command(EX_NETWORK_CMD, 0, 0, NULL);
    
    return RESP_CODE_OK;
}

int parse_json_rf_multi_value(const char * const body)
{
    return RESP_CODE_OK;
}

int parse_json_if_multi_value(const char * const body)
{  
    return RESP_CODE_OK;

}

int parse_json_multi_band(const char * const body,uint8_t cid)
{
     printfd(" \n*************开始解析的body多频段数据消息*****************\n");
     cJSON *node, *value;
     uint32_t subcid;
     int code = RESP_CODE_OK;
     struct poal_config *config = &(config_get_config()->oal_config);
     cJSON *root = cJSON_Parse(body);
    if (root == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return RESP_CODE_PARSE_ERR;
    }
    config->multi_freq_fregment_para[cid].cid=cid;
     printfd("cid:%d,\n", config->multi_freq_fregment_para[cid].cid);
    value = cJSON_GetObjectItem(root, "windowType");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->multi_freq_fregment_para[cid].window_type=value->valueint;
         printfd("window_type:%d,\n", config->multi_freq_fregment_para[cid].window_type);

    }
    value = cJSON_GetObjectItem(root, "frameDropCnt");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->multi_freq_fregment_para[cid].frame_drop_cnt=value->valueint;
         printfd("frameDropCnt:%d,\n", config->multi_freq_fregment_para[0].frame_drop_cnt);

    }
    value = cJSON_GetObjectItem(root, "smoothTimes");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->multi_freq_fregment_para[cid].smooth_time=value->valueint;
         printfd("smoothTimes:%d,\n", config->multi_freq_fregment_para[cid].smooth_time);

    }
    value = cJSON_GetObjectItem(root, "freqSegmentCnt");
    if(value!=NULL&&cJSON_IsNumber(value)){
            
         config->multi_freq_fregment_para[cid].freq_segment_cnt=value->valueint;
         printfd("freqSegmentCnt:%d,\n", config->multi_freq_fregment_para[cid].freq_segment_cnt);

    }
    cJSON *multi_band = NULL;
    multi_band = cJSON_GetObjectItem(root, "array");
    if(multi_band!=NULL){
        for(int i = 0; i < cJSON_GetArraySize(multi_band); i++){
            node = cJSON_GetArrayItem(multi_band, i);
            value = cJSON_GetObjectItem(node, "index");
            if(cJSON_IsNumber(value)){
                subcid=value->valueint;
                config->multi_freq_fregment_para[cid].fregment[value->valueint].index=value->valueint;
                 printfd("index:%d,subcid=%d,",config->multi_freq_fregment_para[cid].fregment[subcid].index,subcid);
            }
            value = cJSON_GetObjectItem(node, "startFrequency");
            if(cJSON_IsString(value)){
                uint64_t startFrequency = 0;
                if(str_to_u64(value->valuestring, &startFrequency, NULL) == false){
                    code = RESP_CODE_PARSE_ERR;
                }else{
                    config->multi_freq_fregment_para[cid].fregment[subcid].start_freq=startFrequency;
                    printfd("startFrequency:%llu, ",config->multi_freq_fregment_para[cid].fregment[subcid].start_freq);
                }
            } 
            value = cJSON_GetObjectItem(node, "endFrequency");
            if(cJSON_IsString(value)){
                uint64_t endFrequency = 0;
                if(str_to_u64(value->valuestring, &endFrequency, NULL) == false){
                    code = RESP_CODE_PARSE_ERR;
                }else{
                    config->multi_freq_fregment_para[cid].fregment[subcid].end_freq=endFrequency;
                    printfd("endFrequency:%llu, ", config->multi_freq_fregment_para[cid].fregment[subcid].end_freq);
                }
            } 
            value = cJSON_GetObjectItem(node, "step");
            if(cJSON_IsNumber(value)){
                config->multi_freq_fregment_para[cid].fregment[subcid].step=value->valueint;
                 printfd("step:%d, ",config->multi_freq_fregment_para[cid].fregment[subcid].step);
            }
            value = cJSON_GetObjectItem(node, "freqResolution");
            if(cJSON_IsString(value)){
                uint64_t freqResolution = 0;
                if(str_to_u64(value->valuestring, &freqResolution, NULL) == false){
                    code = RESP_CODE_PARSE_ERR;
                }else{
                     config->multi_freq_fregment_para[cid].fregment[subcid].freq_resolution=freqResolution;
                     printfd("freqResolution:%f, ",config->multi_freq_fregment_para[cid].fregment[subcid].freq_resolution);
                }
            }
            value = cJSON_GetObjectItem(node, "fftSize");
            if(cJSON_IsNumber(value)){
                config->multi_freq_fregment_para[cid].fregment[subcid].fft_size=value->valueint;
                 printfd("fftSize:%d, ",config->multi_freq_fregment_para[cid].fregment[subcid].fft_size);
            }
            
            printfd("\n");
           
        }
         
    }    
    if(config->multi_freq_fregment_para[cid].freq_segment_cnt == 0){
        config->work_mode = OAL_NULL_MODE;
        printf_warn("Unknown Work Mode\n");
    }else if(config->multi_freq_fregment_para[cid].freq_segment_cnt == 1){
        config->work_mode = OAL_FAST_SCAN_MODE;
    }else{
        config->work_mode = OAL_MULTI_ZONE_SCAN_MODE;
    }
    printfd("\n*****************解析完成************\n");
    return code;
}

int parse_json_muti_point(const char * const body,uint8_t cid)
{
    printfn(" \n*************开始解析的body多频点数据消息*****************\n");
    cJSON *node, *value;
    int code = RESP_CODE_OK;
    struct poal_config *config = &(config_get_config()->oal_config);
    cJSON *root = cJSON_Parse(body);
    if (root == NULL){
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL){
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return RESP_CODE_PARSE_ERR;
    }
    config->multi_freq_point_param[cid].cid=cid;
    printfd("channel:%d,\n", config->multi_freq_point_param[cid].cid);
    value = cJSON_GetObjectItem(root, "windowType");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->multi_freq_point_param[cid].window_type=value->valueint;
         printfd("window_type:%d,\n", config->multi_freq_point_param[cid].window_type);

    }
    value = cJSON_GetObjectItem(root, "frameDropCnt");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->multi_freq_point_param[cid].frame_drop_cnt=value->valueint;
         printfd("frameDropCnt:%d,\n", config->multi_freq_point_param[cid].frame_drop_cnt);

    }
    value = cJSON_GetObjectItem(root, "smoothTimes");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->multi_freq_point_param[cid].smooth_time=value->valueint;
         printfd("smoothTimes:%d,\n", config->multi_freq_point_param[cid].smooth_time);

    }
    value = cJSON_GetObjectItem(root, "residenceTime");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->multi_freq_point_param[cid].residence_time=value->valueint;
         printfd("residenceTime:%d,\n", config->multi_freq_point_param[cid].residence_time);

    }
    value = cJSON_GetObjectItem(root, "residencePolicy");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->multi_freq_point_param[cid].residence_policy=value->valueint;
         printfd("residencePolicy:%d,\n",config->multi_freq_point_param[cid].residence_policy);

    }
    value = cJSON_GetObjectItem(root, "audioSampleRate");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->multi_freq_point_param[cid].audio_sample_rate=value->valuedouble;
         printfd("audio_sample_rate:%f,\n", config->multi_freq_point_param[cid].audio_sample_rate);

    }
    cJSON *muti_point = NULL;
    uint32_t subcid;
    muti_point = cJSON_GetObjectItem(root, "array");
    if(muti_point!=NULL){
        config->multi_freq_point_param[cid].freq_point_cnt = cJSON_GetArraySize(muti_point);
        for(int i = 0; i < cJSON_GetArraySize(muti_point); i++){
            node = cJSON_GetArrayItem(muti_point, i);            
            value = cJSON_GetObjectItem(node, "index");
            if(cJSON_IsNumber(value)){
                config->multi_freq_point_param[cid].points[value->valueint].index=value->valueint;
                subcid=value->valueint;
                printfn("index:%d, subcid=%d ",config->multi_freq_point_param[cid].points[subcid].index,subcid);
            }
            value = cJSON_GetObjectItem(node, "centerFreq");
            if(cJSON_IsString(value)){
                uint64_t center_freq = 0;
                if(str_to_u64(value->valuestring, &center_freq, NULL) == false){
                    code = RESP_CODE_PARSE_ERR;
                }else{
                    config->multi_freq_point_param[cid].points[subcid].center_freq=center_freq;
                    printfn("middle_freq:%d, ",config->multi_freq_point_param[cid].points[subcid].center_freq);
                }
            } 
            value = cJSON_GetObjectItem(node, "bandwith");
            if(cJSON_IsString(value)){
                uint32_t bandwith = 0;
                if(str_to_uint(value->valuestring, &bandwith, NULL) == false){
                    code = RESP_CODE_PARSE_ERR;
                }else{
                    config->multi_freq_point_param[cid].points[subcid].bandwidth=bandwith;
                    printfn("bind_width:%u, ", config->multi_freq_point_param[cid].points[subcid].bandwidth);
                }
               
            } 
            value = cJSON_GetObjectItem(node, "freqResolution");
            if(cJSON_IsString(value)){
                uint32_t resolution = 0;
                if(str_to_uint(value->valuestring, &resolution, NULL) == false){
                    code = RESP_CODE_PARSE_ERR;
                }else{
                    config->multi_freq_point_param[cid].points[subcid].freq_resolution=(float)resolution;
                    printfn("freqResolution:%f, ",config->multi_freq_point_param[cid].points[subcid].freq_resolution);
                }
            }
             value = cJSON_GetObjectItem(node, "fftSize");
             if(cJSON_IsNumber(value)){
                 config->multi_freq_point_param[cid].points[subcid].fft_size=value->valueint;
                  printfn("fftSize:%u, ",config->multi_freq_point_param[cid].points[subcid].fft_size);
             }
             value = cJSON_GetObjectItem(node, "decMethodId");
             if(cJSON_IsNumber(value)){
                // printfd("channel:%d, ", value->valueint);
                 config->multi_freq_point_param[cid].points[subcid].d_method=value->valueint;
                  printfn("decMethodId:%d, ",config->multi_freq_point_param[cid].points[subcid].d_method);
             }
             value = cJSON_GetObjectItem(node, "decBandwidth");
            if(cJSON_IsString(value)){
                uint32_t decBandwidth = 0;
                if(str_to_uint(value->valuestring, &decBandwidth, NULL) == false){
                    code = RESP_CODE_PARSE_ERR;
                }else{
                    config->multi_freq_point_param[cid].points[subcid].d_bandwith=decBandwidth;
                    printfn("decBandwidth:%u, ",config->multi_freq_point_param[cid].points[subcid].d_bandwith);
                }
            }
             value = cJSON_GetObjectItem(node, "muteSwitch");
             if(cJSON_IsNumber(value)){
                 config->multi_freq_point_param[cid].points[subcid].noise_en=value->valueint;
                  printfn("muteSwitch:%d, ",config->multi_freq_point_param[cid].points[subcid].noise_en);
             }
             value = cJSON_GetObjectItem(node, "muteThreshold");
             if(cJSON_IsNumber(value)){
                 config->multi_freq_point_param[cid].points[subcid].noise_thrh=value->valueint;
                  printfn("muteThreshold:%d, ",config->multi_freq_point_param[cid].points[subcid].noise_thrh);
             }
             printfn("\n");
                  
        }
         
    }  
    if(config->multi_freq_point_param[cid].freq_point_cnt == 0){
        config->work_mode = OAL_NULL_MODE;
        printf_warn("Unknown Work Mode\n");
    }else if(config->multi_freq_point_param[cid].freq_point_cnt == 1){
        config->work_mode = OAL_FIXED_FREQ_ANYS_MODE;
    }else{
        config->work_mode = OAL_MULTI_POINT_SCAN_MODE;
    }
    printfd("\n*****************解析完成************\n");
    return code;
}
int parse_json_demodulation(const char * const body,uint8_t cid,uint8_t subid )
{
     printfd(" \n*************开始解析的body解调数据消息*****************\n");
     cJSON *node, *value;
     int code = RESP_CODE_OK;
     struct poal_config *config = &(config_get_config()->oal_config);
     cJSON *root = cJSON_Parse(body);
    if (root == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return RESP_CODE_PARSE_ERR;
    }
    config->sub_channel_para[cid].cid=cid;
     printfd("cid:%d, subid:%d\n", config->sub_channel_para[cid].cid,subid);
    
    value = cJSON_GetObjectItem(root, "audioSampleRate");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->sub_channel_para[cid].audio_sample_rate=value->valueint;
         printfd("audioSampleRate:%d,\n", config->sub_channel_para[cid].audio_sample_rate);

    }
    value = cJSON_GetObjectItem(root, "centerFreq");
    if(value!=NULL&&cJSON_IsString(value)){
        uint64_t center_freq = 0;
        if(str_to_u64(value->valuestring, &center_freq, NULL) == false){
            code = RESP_CODE_PARSE_ERR;
        }else{
            config->sub_channel_para[cid].sub_ch[subid].center_freq=center_freq;
            printfd("centerFreq:%d,\n", config->sub_channel_para[cid].sub_ch[subid].center_freq);
        }
    }
    value = cJSON_GetObjectItem(root, "decBandwidth");
    if(value!=NULL&&cJSON_IsString(value)){
        uint32_t decBandwidth = 0;
        if(str_to_uint(value->valuestring, &decBandwidth, NULL) == false){
            code = RESP_CODE_PARSE_ERR;
        }else{
            config->sub_channel_para[cid].sub_ch[subid].d_bandwith=decBandwidth;
            printfd("decBandwidth:%d,\n", config->sub_channel_para[cid].sub_ch[subid].d_bandwith);
        }
    }
    value = cJSON_GetObjectItem(root, "fftSize");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->sub_channel_para[cid].sub_ch[subid].fft_size=value->valueint;
         printfd("fftSize:%d,\n", config->sub_channel_para[cid].sub_ch[subid].fft_size);

    }
    value = cJSON_GetObjectItem(root, "decMethodId");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->sub_channel_para[cid].sub_ch[subid].d_method=value->valueint;
         printfd("decMethodId:%d,\n",config->sub_channel_para[cid].sub_ch[subid].d_method);

    }
    value = cJSON_GetObjectItem(root, "muteSwitch");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->sub_channel_para[cid].sub_ch[subid].noise_en=value->valueint;
         printfd("muteSwitch:%d,\n", config->sub_channel_para[cid].sub_ch[subid].noise_en);

    }
    value = cJSON_GetObjectItem(root, "muteThreshold");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->sub_channel_para[cid].sub_ch[subid].noise_thrh=value->valueint;
         printfd("muteThreshold:%d,\n", config->sub_channel_para[cid].sub_ch[subid].noise_thrh);

    }
    
    struct sub_channel_freq_para_st *sub_channel_array;
    sub_channel_array = &config->sub_channel_para[cid];
    /* 解调中心频率需要工作中心频率计算 */
    executor_set_command(EX_MID_FREQ_CMD, EX_SUB_CH_MID_FREQ, subid,
        &sub_channel_array->sub_ch[subid].center_freq,/* 解调频率*/
        config->multi_freq_point_param[cid].points[subid].center_freq); /* 频点工作频率 */
    /* 解调带宽, 不同解调方式，带宽系数表不一样*/
    executor_set_command(EX_MID_FREQ_CMD, EX_SUB_CH_DEC_BW, subid, 
        &sub_channel_array->sub_ch[subid].d_bandwith,
        sub_channel_array->sub_ch[subid].d_method);
    /* 子通道解调方式 */
    executor_set_command(EX_MID_FREQ_CMD, EX_SUB_CH_DEC_METHOD, subid, 
        &sub_channel_array->sub_ch[subid].d_method);

    return code;
}

int parse_json_file_backtrace(const char * const body, uint8_t ch,  uint8_t enable, char  *filename)
{
    struct poal_config *config = &(config_get_config()->oal_config);
    uint32_t bandwidth;
    int ret = 0;
    cJSON *node, *value;
    cJSON *root = cJSON_Parse(body);
    if (root == NULL){
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL){
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return RESP_CODE_PARSE_ERR;
    }

    value = cJSON_GetObjectItem(root, "bandwith");
    if(value!=NULL&&cJSON_IsNumber(value)){
         bandwidth=value->valuedouble;
         printf_debug("store bandwidth:%u\n", bandwidth);

    }
#if defined(SUPPORT_XWFS)
    if(enable)
        ret = xwfs_start_backtrace(filename);
    else
        ret = xwfs_stop_backtrace(filename);
#endif
    if(ret != 0)
        return RESP_CODE_EXECMD_ERR;
    else
        return RESP_CODE_OK;
}

int parse_json_file_store(const char * const body, uint8_t ch,  uint8_t enable, char  *filename)
{
    struct poal_config *config = &(config_get_config()->oal_config);
    uint32_t bandwidth;
    int ret = 0;
    cJSON *node, *value;
    cJSON *root = cJSON_Parse(body);
    if (root == NULL){
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL){
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return RESP_CODE_PARSE_ERR;
    }

    value = cJSON_GetObjectItem(root, "bandwith");
    if(value!=NULL&&cJSON_IsNumber(value)){
         bandwidth=value->valuedouble;
         printf_debug("store bandwidth:%u\n", bandwidth);

    }
    executor_set_command(EX_MID_FREQ_CMD, EX_BANDWITH, ch, &bandwidth);
#if defined(SUPPORT_XWFS)
    if(enable)
        ret = xwfs_start_save_file(filename);
    else
        ret = xwfs_stop_save_file(filename);
#endif
    if(ret != 0)
        return RESP_CODE_EXECMD_ERR;
    else
        return RESP_CODE_OK;
}

char *assemble_json_file_list(void)
{
    char *str_json = NULL;
    int i;
    struct file_list{
        char *filename;
        size_t size;
        time_t ctime;
    };
    struct file_list fl[] = {
        {"test1.wav", "1201MByte", 14684468},
        {"test2.wav", "120MByte",  14684461},
    };
    cJSON *root = cJSON_CreateObject();
    cJSON *array = cJSON_CreateArray();
    cJSON* item = NULL;

    cJSON_AddNumberToObject(root, "number", ARRAY_SIZE(fl));
    for(i = 0; i < ARRAY_SIZE(fl); i++){
        cJSON_AddItemToArray(array, item = cJSON_CreateObject());
        cJSON_AddStringToObject(item, "filename", fl[i].filename);
        cJSON_AddStringToObject(item, "size", fl[i].size);
        cJSON_AddNumberToObject(item, "createTime", fl[i].ctime);
    }
    cJSON_AddItemToObject(root, "list", array);
    json_print(root, 1);
    str_json = cJSON_PrintUnformatted(root);
    
    return str_json;
}

char *assemble_json_find_file(char *filename)
{
    char *str_json = NULL;
    int i;
    struct file_list{
        char *filename;
        size_t size;
        time_t ctime;
    };
    struct file_list fl[] = {
        {"test1.wav", "1201MByte", 14684468},
    };
    cJSON *root = cJSON_CreateObject();
    cJSON* item = cJSON_CreateObject();
    
    cJSON_AddStringToObject(root, "filename", filename);
    cJSON_AddStringToObject(root, "size", fl[i].size);
    cJSON_AddNumberToObject(root, "createTime", fl[i].ctime);
    
    json_print(root, 1);
    str_json = cJSON_PrintUnformatted(root);
    
    return str_json;
}




/* NOTE: 调用该函数后，需要free返回指针 */
char *assemble_json_response(int err_code, const char *message)
{
    char *str_json = NULL;
    cJSON *root = cJSON_CreateObject();
    
    cJSON_AddNumberToObject(root, "code", err_code);
    cJSON_AddStringToObject(root, "message", message);
    
    str_json = cJSON_PrintUnformatted(root);
    return str_json;
}


/* NOTE: 调用该函数后，需要free返回指针 */
char *assemble_json_data_response(int err_code, const char *message, const char * const data)
{
    char *str_json = NULL, *body = NULL;
    cJSON *root, *node;

    str_json = assemble_json_response(err_code, message);
    root = cJSON_Parse(str_json);
    node = cJSON_Parse(data);
    if (root != NULL){
        cJSON_AddItemToObject(root, "data", node);
    }
    body = cJSON_PrintUnformatted(root);
    return body;
}




