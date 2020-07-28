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
#include <sys/types.h>
#include <dirent.h>
#include "parse_json.h"
#include "../../dao/json/cJSON.h"


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
    config_save_all();
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
     printfd(" \n*************multi band set*****************\n");
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
    printfn(" \n*************multi point set*****************\n");
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
    return code;
}
int parse_json_demodulation(const char * const body,uint8_t cid,uint8_t subid )
{
     printfd(" \n*************demodulation set*****************\n");
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
#elif defined(SUPPORT_FS)
    struct fs_context *fs_ctx;
    fs_ctx = get_fs_ctx();
    if(fs_ctx == NULL){
        printf_warn("NOT FOUND DISK!!\n");
        return RESP_CODE_EXECMD_ERR;
    }
    if(enable)
        fs_ctx->ops->fs_start_read_raw_file(filename);
    else
        fs_ctx->ops->fs_stop_read_raw_file(filename);
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
#elif defined(SUPPORT_FS)
        struct fs_context *fs_ctx;
        fs_ctx = get_fs_ctx();
        if(fs_ctx == NULL){
            printf_warn("NOT FOUND DISK!!\n");
            return RESP_CODE_EXECMD_ERR;
        }
        if(enable)
            fs_ctx->ops->fs_start_save_file(filename);
        else
            fs_ctx->ops->fs_stop_save_file(filename);
#endif
    if(ret != 0)
        return RESP_CODE_EXECMD_ERR;
    else
        return RESP_CODE_OK;
}

static void creat_file_list(char *filename, struct stat *stats, cJSON *array)
{
    cJSON* item = NULL;
    if(stats == NULL || filename == NULL || array == NULL)
        return;
    cJSON_AddItemToArray(array, item = cJSON_CreateObject());
    cJSON_AddStringToObject(item, "filename", filename);
    cJSON_AddNumberToObject(item, "size", (off_t)stats->st_size);
    cJSON_AddNumberToObject(item, "createTime", stats->st_mtime);
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
#if defined(SUPPORT_FS)
    struct fs_context *fs_ctx;
    fs_ctx = get_fs_ctx();
    if(fs_ctx == NULL)
        return NULL;
    fs_ctx->ops->fs_dir(NULL, creat_file_list, array);
#else
    cJSON_AddNumberToObject(root, "number", ARRAY_SIZE(fl));
    for(i = 0; i < ARRAY_SIZE(fl); i++){
        cJSON_AddItemToArray(array, item = cJSON_CreateObject());
        cJSON_AddStringToObject(item, "filename", fl[i].filename);
        cJSON_AddStringToObject(item, "size", fl[i].size);
        cJSON_AddNumberToObject(item, "createTime", fl[i].ctime);
    }
#endif
    cJSON_AddItemToObject(root, "list", array);
    json_print(root, 1);
    str_json = cJSON_PrintUnformatted(root);
    
    return str_json;
}

static void find_file_list(char *filename, struct stat *stats, cJSON *root)
{
    cJSON* item = NULL;
    if(stats == NULL || filename == NULL || root == NULL)
        return;
    cJSON_AddStringToObject(root, "filename", filename);
    cJSON_AddNumberToObject(root, "size", stats->st_size);
    cJSON_AddNumberToObject(root, "createTime", stats->st_mtime);
}
char *assemble_json_find_file(char *filename)
{
    char *str_json = NULL;
    int i;
    
    cJSON *root = cJSON_CreateObject();
#if defined(SUPPORT_FS)
    struct fs_context *fs_ctx;
    fs_ctx = get_fs_ctx();
    if(fs_ctx == NULL)
        return NULL;
    fs_ctx->ops->fs_find(filename, find_file_list, root);
#else
    struct file_list{
        char *filename;
        size_t size;
        time_t ctime;
    };
    struct file_list fl[] = {
        {"test1.wav", "1201MByte", 14684468},
    };
    cJSON_AddStringToObject(root, "filename", filename);
    cJSON_AddStringToObject(root, "size", fl[i].size);
    cJSON_AddNumberToObject(root, "createTime", fl[i].ctime);
#endif
    json_print(root, 1);
    str_json = cJSON_PrintUnformatted(root);
    
    return str_json;
}


char *assemble_json_softversion(void)
{
    char *str_json = NULL;
    cJSON *root = cJSON_CreateObject();
    char version[32] = {0};
    snprintf(version, sizeof(version), "%x", get_fpga_version());
    version[sizeof(version) - 1] = 0;
    cJSON_AddStringToObject(root, "appversion", get_version_string());
    cJSON_AddStringToObject(root, "kernelversion", "1.0.0");
    cJSON_AddStringToObject(root, "fpgaversion", version);
    json_print(root, 1);
    str_json = cJSON_PrintUnformatted(root);
    return str_json;
}
char *assemble_json_fpag_info(void)
{
    char *str_json = NULL;
    struct arg_s{
        uint32_t temp;
        float vol;
        float current;
        uint32_t resources;
    };
    struct arg_s args;
    char version[32] = {0};
    cJSON *root = cJSON_CreateObject();
    io_get_fpga_status(&args);
    snprintf(version, sizeof(version), "%x", get_fpga_version());
    version[sizeof(version) - 1] = 0;
    cJSON_AddStringToObject(root, "fpgaVersion", version);
    cJSON_AddNumberToObject(root, "temperature", args.temp);
    cJSON_AddNumberToObject(root, "fpgaIntVol", args.vol);
    cJSON_AddNumberToObject(root, "fpgaBramVol", args.current);
    cJSON_AddNumberToObject(root, "fpgaResources", args.resources);
    json_print(root, 1);
    str_json = cJSON_PrintUnformatted(root);
    return str_json;
}
char *assemble_json_gps_info(void)
{
    char *str_json = NULL;
    struct arg_s{
        uint32_t status;
    };
    struct arg_s args;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", 
#if defined (SUPPORT_GPS)
        (gps_location_is_valid() == true ? "ok" : "no")
#else
        "no"
#endif
        );
    json_print(root, 1);
    str_json = cJSON_PrintUnformatted(root);
    return str_json;
}
char *assemble_json_clock_info(void)
{
    char *str_json = NULL;
    uint8_t external_clk = 0;
    bool lock_ok = false;
    cJSON *root = cJSON_CreateObject();
    lock_ok = io_get_clock_status(&external_clk);
    cJSON_AddStringToObject(root, "inout", (external_clk == 1 ? "out" : "in"));
    cJSON_AddStringToObject(root, "status", (lock_ok == false ? "no":"ok"));
    cJSON_AddNumberToObject(root, "frequency", 512000000);
    json_print(root, 1);
    str_json = cJSON_PrintUnformatted(root);
    return str_json;
}
char *assemble_json_board_info(void)
{
    char *str_json = NULL;
    struct arg_s{
        float v;
        float i;
    };
    struct arg_s power;
    cJSON *root = cJSON_CreateObject();
    io_get_board_power(&power);
    cJSON_AddNumberToObject(root, "temperature", io_get_adc_temperature());
    cJSON_AddNumberToObject(root, "voltage", power.v);
    cJSON_AddNumberToObject(root, "current", power.i);
    json_print(root, 1);
    str_json = cJSON_PrintUnformatted(root);
    return str_json;
}
char *assemble_json_net_info(void)
{
    char *str_json = NULL, *s_speed = "error", *s_link = "error";
    int i;
    struct speed_table{
        int32_t i_speed;
        char *s_speed;
    };

    struct speed_table spt[] = {
        {SPEED_10, "10Mbps"},
        {SPEED_100, "100Mbps"},
        {SPEED_1000, "1Gbps"},
        {SPEED_2500, "2.5Gbps"},
        {SPEED_10000, "10Gbps"},
    };
    cJSON *root = cJSON_CreateObject();
    int32_t link = get_netlink_status("eth1");
    int32_t speed = get_ifname_speed("eth1");
    if(link >= 0)
        s_link = (link == 0 ? "down" : "up");
    for(i = 0; i < ARRAY_SIZE(spt); i++){
        if(speed == spt[i].i_speed){
            s_speed = spt[i].s_speed;
            break;
        }
    }
    cJSON_AddStringToObject(root, "ifname", "eth1");
    cJSON_AddStringToObject(root, "link", s_link);
    cJSON_AddStringToObject(root, "speed", s_speed);
    json_print(root, 1);
    str_json = cJSON_PrintUnformatted(root);
    return str_json;
}
char *assemble_json_rf_info(void)
{
    char *str_json = NULL;
    int i;
    cJSON *array = cJSON_CreateArray();
    cJSON* item = NULL;
    int16_t  rf_temp = 0;
    for(i = 0; i < MAX_RADIO_CHANNEL_NUM; i++){
        cJSON_AddItemToArray(array, item = cJSON_CreateObject());
        cJSON_AddNumberToObject(item, "index", i);
        executor_get_command(EX_RF_FREQ_CMD, EX_RF_STATUS_TEMPERAT, i,  &rf_temp);
        if(rf_temp > 200 || rf_temp < -100 || rf_temp == 0)
            cJSON_AddStringToObject(item, "status", "no");
        else
            cJSON_AddStringToObject(item, "status", "ok");
        cJSON_AddNumberToObject(item, "temperature", rf_temp);
    }
   str_json = cJSON_PrintUnformatted(array);
   return str_json;
}
char *assemble_json_disk_info(void)
{
    #define DISK_NUM 1
    char *str_json = NULL;
    int i;
    cJSON *array = cJSON_CreateArray();
    cJSON* item = NULL;
    bool b_valid = false;
#if defined(SUPPORT_FS)
    struct fs_context *fs_ctx;
    struct statfs diskInfo;
    fs_ctx = get_fs_ctx_ex();
    b_valid = fs_ctx->ops->fs_disk_info(&diskInfo);
#endif
    for(i = 0; i < DISK_NUM; i++){
        cJSON_AddItemToArray(array, item = cJSON_CreateObject());
        cJSON_AddNumberToObject(item, "index", i);
        cJSON_AddStringToObject(item, "status", (b_valid == false ? "no":"ok"));
        if(b_valid){
            cJSON_AddNumberToObject(item, "totalSpace", diskInfo.f_bsize * diskInfo.f_blocks);
            cJSON_AddNumberToObject(item, "freeSpace", diskInfo.f_bsize * diskInfo.f_bfree);
        }
    }
   str_json = cJSON_PrintUnformatted(array);
   return str_json;
}
char *assemble_json_all_info(void)
{
    char *str_json = NULL;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "versionInfo", cJSON_Parse(assemble_json_softversion()));
    cJSON_AddItemToObject(root, "diskInfo", cJSON_Parse(assemble_json_disk_info()));
    cJSON_AddItemToObject(root, "clockInfo", cJSON_Parse(assemble_json_clock_info()));
    cJSON_AddItemToObject(root, "rfInfo", cJSON_Parse(assemble_json_rf_info()));
    cJSON_AddItemToObject(root, "boardInfo", cJSON_Parse(assemble_json_board_info()));
    cJSON_AddItemToObject(root, "fpgaInfo", cJSON_Parse(assemble_json_fpag_info()));
    cJSON_AddItemToObject(root, "gpsInfo", cJSON_Parse(assemble_json_gps_info()));
    cJSON_AddItemToObject(root, "netInfo", cJSON_Parse(assemble_json_net_info()));
    json_print(root, 1);
    str_json = cJSON_PrintUnformatted(root);
    return str_json;
}
char *assemble_json_temp_info(void)
{
    char *str_json = NULL;
    uint16_t ps_temp, rf_temp;
    ps_temp = io_get_adc_temperature();
    executor_get_command(EX_RF_FREQ_CMD, EX_RF_STATUS_TEMPERAT, 0,  &rf_temp);
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "dbTemp", ps_temp);
    cJSON_AddNumberToObject(root, "rfTemp", rf_temp);
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




