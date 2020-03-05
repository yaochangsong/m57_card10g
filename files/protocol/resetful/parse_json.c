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

int parse_json_rf_multi_value(const char * const body)
{
    return 0;
}

int parse_json_if_multi_value(const char * const body)
{  
    return 0;

}

int parse_json_multi_band(const char * const body,uint8_t cid)
{
     printfd(" \n*************开始解析的body多频段数据消息*****************\n");
     cJSON *node, *value;
     uint32_t subcid;
     struct poal_config *config = &(config_get_config()->oal_config);
     cJSON *root = cJSON_Parse(body);
    if (root == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return -1;
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
            printfd("value:%d, ",cJSON_IsNumber(value));
            if(cJSON_IsNumber(value)){
                config->multi_freq_fregment_para[cid].fregment[subcid].start_freq=value->valueint;
                printfd("startFrequency:%d, ",config->multi_freq_fregment_para[cid].fregment[subcid].start_freq);
            } 
            value = cJSON_GetObjectItem(node, "endFrequency");
            if(cJSON_IsNumber(value)){
                config->multi_freq_fregment_para[cid].fregment[subcid].end_freq=value->valueint;
                 printfd("endFrequency:%d, ", config->multi_freq_fregment_para[cid].fregment[subcid].end_freq);
            } 
            value = cJSON_GetObjectItem(node, "step");
            if(cJSON_IsNumber(value)){
                config->multi_freq_fregment_para[cid].fregment[subcid].step=value->valueint;
                 printfd("step:%d, ",config->multi_freq_fregment_para[cid].fregment[subcid].step);
            }
            value = cJSON_GetObjectItem(node, "freqResolution");
            if(cJSON_IsNumber(value)){
                config->multi_freq_fregment_para[cid].fregment[subcid].freq_resolution=value->valuedouble;
                 printfd("freqResolution:%f, ",config->multi_freq_fregment_para[cid].fregment[subcid].freq_resolution);
            }
            value = cJSON_GetObjectItem(node, "fftSize");
            if(cJSON_IsNumber(value)){
                config->multi_freq_fregment_para[cid].fregment[subcid].fft_size=value->valueint;
                 printfd("fftSize:%d, ",config->multi_freq_fregment_para[cid].fregment[subcid].fft_size);
            }
            
            printfd("\n");
           
        }
         
    }    
    printfd("\n*****************解析完成************\n");
    return 0;
}

int parse_json_muti_point(const char * const body,uint8_t cid)
{
     printfd(" \n*************开始解析的body多频点数据消息*****************\n");
     cJSON *node, *value;
     struct poal_config *config = &(config_get_config()->oal_config);
     cJSON *root = cJSON_Parse(body);
    if (root == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return -1;
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
        for(int i = 0; i < cJSON_GetArraySize(muti_point); i++){
            node = cJSON_GetArrayItem(muti_point, i);            
            value = cJSON_GetObjectItem(node, "index");
            

                 if(cJSON_IsNumber(value)){
                     config->multi_freq_point_param[cid].points[value->valueint].index=value->valueint;
                     subcid=value->valueint;
                     
                      printfd("index:%d, subcid=%d ",config->multi_freq_point_param[cid].points[subcid].index,subcid);
                 }
                 value = cJSON_GetObjectItem(node, "middle_freq");
                 if(cJSON_IsNumber(value)){
                     config->multi_freq_point_param[cid].points[subcid].center_freq=value->valueint;
                     printfd("middle_freq:%d, ",config->multi_freq_point_param[cid].points[subcid].center_freq);
                 } 
                 value = cJSON_GetObjectItem(node, "bandwith");
                 if(cJSON_IsNumber(value)){
                     config->multi_freq_point_param[cid].points[subcid].d_bandwith=value->valueint;
                      printfd("bind_width:%d, ", config->multi_freq_point_param[cid].points[subcid].d_bandwith);
                 } 
                 value = cJSON_GetObjectItem(node, "freqResolution");
                 if(cJSON_IsNumber(value)){
                     config->multi_freq_point_param[cid].points[subcid].freq_resolution=value->valuedouble;
                      printfd("freqResolution:%f, ",config->multi_freq_point_param[cid].points[subcid].freq_resolution);
                 }
                 value = cJSON_GetObjectItem(node, "fftSize");
                 if(cJSON_IsNumber(value)){
                     config->multi_freq_point_param[cid].points[subcid].fft_size=value->valueint;
                      printfd("fftSize:%d, ",config->multi_freq_point_param[cid].points[subcid].fft_size);
                 }
                 value = cJSON_GetObjectItem(node, "decMethodId");
                 if(cJSON_IsNumber(value)){
                    // printfd("channel:%d, ", value->valueint);
                     config->multi_freq_point_param[cid].points[subcid].d_method=value->valueint;
                      printfd("decMethodId:%d, ",config->multi_freq_point_param[cid].points[subcid].d_method);
                 }
                 value = cJSON_GetObjectItem(node, "decBandwidth");
                 if(cJSON_IsNumber(value)){
                    // printfd("channel:%d, ", value->valueint);
                     config->multi_freq_point_param[cid].points[subcid].d_bandwith=value->valueint;
                      printfd("decBandwidth:%d, ",config->multi_freq_point_param[cid].points[subcid].d_bandwith);
                 }
                 value = cJSON_GetObjectItem(node, "muteSwitch");
                 if(cJSON_IsNumber(value)){
                    // printfd("channel:%d, ", value->valueint);
                     config->multi_freq_point_param[cid].points[subcid].noise_en=value->valueint;
                      printfd("muteSwitch:%d, ",config->multi_freq_point_param[cid].points[subcid].noise_en);
                 }
                 value = cJSON_GetObjectItem(node, "muteThreshold");
                 if(cJSON_IsNumber(value)){
                    // printfd("channel:%d, ", value->valueint);
                     config->multi_freq_point_param[cid].points[subcid].noise_thrh=value->valueint;
                      printfd("muteThreshold:%d, ",config->multi_freq_point_param[cid].points[subcid].noise_thrh);
                 }
                 printfd("\n");
                  
        }
         
    }  
    printfd("\n*****************解析完成************\n");
    return 0;
}
int parse_json_demodulation(const char * const body,uint8_t cid,uint8_t subid )
{
     printfd(" \n*************开始解析的body解调数据消息*****************\n");
     cJSON *node, *value;
     struct poal_config *config = &(config_get_config()->oal_config);
     cJSON *root = cJSON_Parse(body);
    if (root == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return -1;
    }
    config->sub_channel_para[cid].cid=cid;
     printfd("cid:%d, subid:%d\n", config->sub_channel_para[cid].cid,subid);
    
    value = cJSON_GetObjectItem(root, "audioSampleRate");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->sub_channel_para[cid].audio_sample_rate=value->valueint;
         printfd("audioSampleRate:%d,\n", config->sub_channel_para[cid].audio_sample_rate);

    }
    value = cJSON_GetObjectItem(root, "centerFreq");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->sub_channel_para[cid].sub_ch[subid].center_freq=value->valueint;
         printfd("centerFreq:%d,\n", config->sub_channel_para[cid].sub_ch[subid].center_freq);

    }
    value = cJSON_GetObjectItem(root, "decBandwidth");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->sub_channel_para[cid].sub_ch[subid].d_bandwith=value->valueint;
         printfd("decBandwidth:%d,\n", config->sub_channel_para[cid].sub_ch[subid].d_bandwith);

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

    return 0;
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




