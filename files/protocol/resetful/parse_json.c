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
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/mman.h>
#include "config.h"
#include "parse_cmd.h"
#include "parse_json.h"
#include "../../dao/json/cJSON.h"
#include "../../bsp/io.h"
#include "../../net/net_thread.h"
#include "../../utils/bitops.h"

void safe_cJson_AddItemToObject(cJSON *object, const char *string, char *assemble_info)
{
    cJSON_AddItemToObject(object, string, cJSON_Parse(assemble_info));
    _safe_free_(assemble_info);
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

static int8_t xw_decode_method_convert(uint8_t method)
{
    uint8_t d_method;
    
    if(method == DC_MODE_AM){
        d_method = IO_DQ_MODE_AM;
    }else if(method == DC_MODE_FM) {
        d_method = IO_DQ_MODE_FM;
    }else if(method == DC_MODE_LSB || method == DC_MODE_USB) {
        d_method = IO_DQ_MODE_LSB;
    }else if(method == DC_MODE_CW) {
        d_method = IO_DQ_MODE_CW;
    }else if(method == DC_MODE_IQ) {
        d_method = IO_DQ_MODE_IQ;
    }else{
        printf_err("decode method not support:%d, use iq\n",method);
        return IO_DQ_MODE_IQ;
    }
    printf_note("method convert:%d ===> %d\n",method, d_method);
    return d_method;
}



int parse_json_client_net(int ch, const char * const body, char *type)
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
            if(type != NULL){
                if(!strcmp(type, "biq")){
                    net_data_add_client(&sclient, ch, NET_DATA_TYPE_BIQ);
                } else if(!strcmp(type, "niq")){
                    net_data_add_client(&sclient, ch, NET_DATA_TYPE_NIQ);
                    net_data_add_client(&sclient, ch, NET_DATA_TYPE_AUDIO);
                } else if(!strcmp(type, "fft")){
                    net_data_add_client(&sclient, ch, NET_DATA_TYPE_FFT);
                }
                
            }else{
                net_data_add_client(&sclient, ch, NET_DATA_TYPE_BIQ);
                net_data_add_client(&sclient, ch, NET_DATA_TYPE_NIQ);
                net_data_add_client(&sclient, ch, NET_DATA_TYPE_FFT);
                net_data_add_client(&sclient, ch, NET_DATA_TYPE_AUDIO);
            }
        }
    }
    cJSON_Delete(root);
    return RESP_CODE_OK;
}

static int _check_element_has_equal(uint32_t *data,uint32_t datalen)
{
   int i=0,j=0,ret=0;
   
   for(i=0; i < datalen -1; i++)
   {
      for(j=i+1; j < datalen; j++)
        {
           if(data[i] == data[j])
            {
                 printf_err("there are equal data, please check\n");
                 return  ret = -1;
            }
        }
   }
   return ret;
}

int parse_json_net(const char * const body)
{
    cJSON *root = cJSON_Parse(body);
    cJSON *node, *value;
    char ifname[IFNAMSIZ];
    uint32_t ipaddr, netmask, gw;
    struct in_addr addr;
    int r, is_reboot = 0;
    if (root == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return RESP_CODE_PARSE_ERR;
    }
    value = cJSON_GetObjectItem(root, "ifname");
    if(value != NULL&&cJSON_IsString(value) && (strlen(value->valuestring) != 0)){
        strcpy(ifname, value->valuestring);
        printf_info("ifname: %s, %ld\n", ifname, strlen(value->valuestring));
    }else{
        cJSON_Delete(root);
        return RESP_CODE_PARSE_ERR;
    }
    value = cJSON_GetObjectItem(root, "ipaddr");
    if(value!=NULL&&cJSON_IsString(value)){
        //ipaddr = inet_addr(value->valuestring);
        r = inet_pton(AF_INET, value->valuestring, &addr);
        if(r <= 0){
            printf_warn("invalid addr: %s\n", value->valuestring);
            cJSON_Delete(root);
            return RESP_CODE_PARSE_ERR;
        }
        ipaddr = addr.s_addr;
        if(config_match_ipaddr_addr(ifname, ipaddr)== false){
            printf_info("set ipaddr: %s, 0x%x\n", value->valuestring, ipaddr);
            if(config_set_ip(ifname, ipaddr) != 0){
            cJSON_Delete(root);
            return RESP_CODE_EXECMD_ERR;
            }
            is_reboot = 1;
        }
    }

    value = cJSON_GetObjectItem(root, "netmask");
    if(value!=NULL&&cJSON_IsString(value)){
        //netmask = inet_addr(value->valuestring);
        r = inet_pton(AF_INET, value->valuestring, &addr);
        if(r <= 0){
            printf_warn("invalid netmask: %s\n", value->valuestring);
            cJSON_Delete(root);
            return RESP_CODE_PARSE_ERR;
        }
        netmask = addr.s_addr;
        if(config_match_netmask_addr(ifname, netmask)== false){
            printf_info("set netmask: %s, 0x%x\n", value->valuestring, netmask);
            if(config_set_netmask(ifname, netmask) != 0){
            	cJSON_Delete(root);
            	return RESP_CODE_EXECMD_ERR;
            }
        }
    }
    value = cJSON_GetObjectItem(root, "gateway");
    if(value!=NULL&&cJSON_IsString(value)){
        
        //gw = inet_addr(value->valuestring);
        r = inet_pton(AF_INET, value->valuestring, &addr);
        if(r <= 0){
            printf_warn("invalid gateway: %s\n", value->valuestring);
            return RESP_CODE_PARSE_ERR;
        }
        gw = addr.s_addr;
        if(config_match_gateway_addr(ifname, gw)== false){
            printf_info("set gateway: %s, 0x%x\n", value->valuestring, gw);
            if(config_set_gateway(ifname, gw) != 0){
	            cJSON_Delete(root);
	            return RESP_CODE_EXECMD_ERR;
            }
        }
    }
    cJSON *port;
    port = cJSON_GetObjectItem(root, "port");
    if(port!=NULL&&cJSON_IsNumber(port)){
        if(config_set_if_cmd_port(ifname, (uint16_t)port->valueint) == 0)
            is_reboot = 1;
    }
    
    cJSON_Delete(root);
    if(is_reboot == 1)
        return RESP_CODE_EXECMD_REBOOT;

    return RESP_CODE_OK;
}

int parse_json_if_multi_value(const char * const body, uint8_t cid)
{
    return RESP_CODE_OK;
}

int parse_json_rf_multi_value(const char * const body, uint8_t cid)
{  
    struct poal_config *config = &(config_get_config()->oal_config);
    cJSON *node, *value;
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
    config->channel[cid].rf_para.cid = cid;
    value = cJSON_GetObjectItem(root, "modeCode");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->channel[cid].rf_para.rf_mode_code=value->valueint;
         printfd("rf_mode_code:%d,\n", config->channel[cid].rf_para.rf_mode_code);
         executor_set_command(EX_MID_FREQ_CMD, EX_RF_MODE_CODE, cid, &config->channel[cid].rf_para.rf_mode_code);
    }
    value = cJSON_GetObjectItem(root, "gainMode");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->channel[cid].rf_para.gain_ctrl_method=value->valueint;
         printfd("gain_ctrl_method:%d,\n", config->channel[cid].rf_para.gain_ctrl_method);
    }
    if(config->channel[cid].rf_para.gain_ctrl_method == POAL_MGC_MODE){
    value = cJSON_GetObjectItem(root, "mgcGain");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->channel[cid].rf_para.mgc_gain_value=value->valueint;
             printfd("mgc_gain_value:%d,\n", config->channel[cid].rf_para.mgc_gain_value);
             executor_set_command(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, cid, &config->channel[cid].rf_para.mgc_gain_value);
        }
        value = cJSON_GetObjectItem(root, "rfAttenuation");
        if(value!=NULL&&cJSON_IsNumber(value)){
             config->channel[cid].rf_para.attenuation=value->valueint;
             printfd("attenuation:%d,\n", config->channel[cid].rf_para.attenuation);
             executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, cid, &config->channel[cid].rf_para.attenuation);
        }
    }
    value = cJSON_GetObjectItem(root, "agcCtrlTime");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->channel[cid].rf_para.agc_ctrl_time=value->valueint;
         printfd("agc_ctrl_time:%d,\n", config->channel[cid].rf_para.agc_ctrl_time);
    }
    value = cJSON_GetObjectItem(root, "agcOutPutAmp");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->channel[cid].rf_para.agc_mid_freq_out_level=value->valueint;
         printfd("agc_mid_freq_out_level:%d,\n", config->channel[cid].rf_para.agc_mid_freq_out_level);
    }
    value = cJSON_GetObjectItem(root, "bandwidth");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->channel[cid].rf_para.mid_bw=value->valueint;
         printfd("rf mid_bw:%d,\n", config->channel[cid].rf_para.mid_bw);
    }
    printfd("\n");
    cJSON_Delete(root);
    return RESP_CODE_OK;

}

int parse_json_multi_band(const char * const body,uint8_t cid)
{
     printfd(" \n*************multi band set*****************\n");
     cJSON *node, *value;
     uint32_t subcid;
     int code = RESP_CODE_OK;
     struct poal_config *config = &(config_get_config()->oal_config);
     struct multi_freq_point_para_st *point;
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
    config->channel[cid].multi_freq_fregment_para.cid=cid;
     printfd("cid:%d,\n", config->channel[cid].multi_freq_fregment_para.cid);
    point = &config->channel[cid].multi_freq_point_param;
    value = cJSON_GetObjectItem(root, "windowType");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->channel[cid].multi_freq_fregment_para.window_type=value->valueint;
         printfd("window_type:%d,\n", config->channel[cid].multi_freq_fregment_para.window_type);

    }
    value = cJSON_GetObjectItem(root, "frameDropCnt");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->channel[cid].multi_freq_fregment_para.frame_drop_cnt=value->valueint;
         printfd("frameDropCnt:%d,\n", config->channel[cid].multi_freq_fregment_para.frame_drop_cnt);

    }
    value = cJSON_GetObjectItem(root, "smoothTimes");
    if(value!=NULL&&cJSON_IsNumber(value)){
         point->smooth_time=value->valueint;
         printfd("smoothTimes:%d,\n", point->smooth_time);
         executor_set_command(EX_MID_FREQ_CMD, EX_SMOOTH_TIME, cid, &point->smooth_time);
    }
    value = cJSON_GetObjectItem(root, "freqSegmentCnt");
    if(value!=NULL&&cJSON_IsNumber(value)){
            
         config->channel[cid].multi_freq_fregment_para.freq_segment_cnt=value->valueint;
         printfd("freqSegmentCnt:%d,\n", config->channel[cid].multi_freq_fregment_para.freq_segment_cnt);

    }
    cJSON *multi_band = NULL;
    multi_band = cJSON_GetObjectItem(root, "array");
    if(multi_band!=NULL){
        for(int i = 0; i < cJSON_GetArraySize(multi_band); i++){
            node = cJSON_GetArrayItem(multi_band, i);
            value = cJSON_GetObjectItem(node, "index");
            if(cJSON_IsNumber(value)){
                subcid=value->valueint;
                config->channel[cid].multi_freq_fregment_para.fregment[value->valueint].index=value->valueint;
                 printfd("index:%d,subcid=%d,",config->channel[cid].multi_freq_fregment_para.fregment[subcid].index,subcid);
            }
            value = cJSON_GetObjectItem(node, "startFrequency");
            if(cJSON_IsNumber(value)){
                config->channel[cid].multi_freq_fregment_para.fregment[subcid].start_freq = value->valuedouble;
                printfd("startFrequency:%"PRIu64", ",config->channel[cid].multi_freq_fregment_para.fregment[subcid].start_freq);
            } 
            value = cJSON_GetObjectItem(node, "endFrequency");
            if(cJSON_IsNumber(value)){
                config->channel[cid].multi_freq_fregment_para.fregment[subcid].end_freq = value->valuedouble;
                printfd("endFrequency:%"PRIu64", ",config->channel[cid].multi_freq_fregment_para.fregment[subcid].end_freq);
            }
            value = cJSON_GetObjectItem(node, "step");
            if(cJSON_IsNumber(value)){
                config->channel[cid].multi_freq_fregment_para.fregment[subcid].step=value->valueint;
                 printfd("step:%d, ",config->channel[cid].multi_freq_fregment_para.fregment[subcid].step);
            }
            value = cJSON_GetObjectItem(node, "freqResolution");
            if(cJSON_IsNumber(value)){
                config->channel[cid].multi_freq_fregment_para.fregment[subcid].freq_resolution = value->valuedouble;
                printfd("freq_resolution:%f, ",config->channel[cid].multi_freq_fregment_para.fregment[subcid].freq_resolution);
                config->channel[cid].multi_freq_fregment_para.fregment[subcid].fft_size = config_get_fft_by_resolution((uint32_t)value->valuedouble);
            }else{
            value = cJSON_GetObjectItem(node, "fftSize");
            if(cJSON_IsNumber(value)){
                config->channel[cid].multi_freq_fregment_para.fregment[subcid].fft_size=value->valueint;
                 printfd("fftSize:%d, ",config->channel[cid].multi_freq_fregment_para.fregment[subcid].fft_size);
                }
            }
            printfd("ch:%d, fft_size=%u\n", cid, config->channel[cid].multi_freq_fregment_para.fregment[subcid].fft_size);
            printfd("\n");
           
        }
         
    }    
    if(config->channel[cid].multi_freq_fregment_para.freq_segment_cnt == 0){
        config->channel[cid].work_mode = OAL_NULL_MODE;
        printf_warn("Unknown Work Mode\n");
    }else if(config->channel[cid].multi_freq_fregment_para.freq_segment_cnt == 1){
        config->channel[cid].work_mode = OAL_FAST_SCAN_MODE;
    }else{
        config->channel[cid].work_mode = OAL_MULTI_ZONE_SCAN_MODE;
    }
    cJSON_Delete(root);
    return code;
}

int parse_json_muti_point(const char * const body,uint8_t cid)
{
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
    config->channel[cid].multi_freq_point_param.cid=cid;
    printfd("channel:%d,\n", config->channel[cid].multi_freq_point_param.cid);
    value = cJSON_GetObjectItem(root, "windowType");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->channel[cid].multi_freq_point_param.window_type=value->valueint;
         printfd("window_type:%d,\n", config->channel[cid].multi_freq_point_param.window_type);

    }
    value = cJSON_GetObjectItem(root, "frameDropCnt");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->channel[cid].multi_freq_point_param.frame_drop_cnt=value->valueint;
         printfd("frameDropCnt:%d,\n", config->channel[cid].multi_freq_point_param.frame_drop_cnt);

    }
    value = cJSON_GetObjectItem(root, "smoothTimes");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->channel[cid].multi_freq_point_param.smooth_time=value->valueint;
         printfd("smoothTimes:%d,\n", config->channel[cid].multi_freq_point_param.smooth_time);

    }
    value = cJSON_GetObjectItem(root, "residenceTime");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->channel[cid].multi_freq_point_param.residence_time=value->valueint;
         printfd("residenceTime:%d,\n", config->channel[cid].multi_freq_point_param.residence_time);

    }
    value = cJSON_GetObjectItem(root, "residencePolicy");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->channel[cid].multi_freq_point_param.residence_policy=value->valueint;
         printfd("residencePolicy:%d,\n",config->channel[cid].multi_freq_point_param.residence_policy);

    }
    value = cJSON_GetObjectItem(root, "audioSampleRate");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->channel[cid].multi_freq_point_param.audio_sample_rate=value->valuedouble;
         printfd("audio_sample_rate:%f,\n", config->channel[cid].multi_freq_point_param.audio_sample_rate);

    }
    value = cJSON_GetObjectItem(root, "startFrequency");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->channel[cid].multi_freq_point_param.start_freq = value->valuedouble;
         printfd("start_freq:%"PRIu64" ",config->channel[cid].multi_freq_point_param.start_freq);

    }
    value = cJSON_GetObjectItem(root, "endFrequency");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->channel[cid].multi_freq_point_param.end_freq = value->valuedouble;
         printfd("end_freq:%"PRIu64" ",config->channel[cid].multi_freq_point_param.end_freq);

    }
    value = cJSON_GetObjectItem(root, "smoothMode");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->channel[cid].multi_freq_point_param.smooth_mode = value->valueint;
         printfd("smooth_mode:%d ",config->channel[cid].multi_freq_point_param.smooth_mode);
    }
    value = cJSON_GetObjectItem(root, "smoothThreshold");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->channel[cid].multi_freq_point_param.smooth_threshold = value->valueint;
         printfd("smooth_threshold:%d ",config->channel[cid].multi_freq_point_param.smooth_threshold);
    }
    cJSON *muti_point = NULL;
    uint32_t subcid = 0;
    muti_point = cJSON_GetObjectItem(root, "array");
    if(muti_point!=NULL){
        config->channel[cid].multi_freq_point_param.freq_point_cnt = cJSON_GetArraySize(muti_point);
        for(int i = 0; i < cJSON_GetArraySize(muti_point); i++){
            node = cJSON_GetArrayItem(muti_point, i);            
            value = cJSON_GetObjectItem(node, "index");
            if(cJSON_IsNumber(value)){
                config->channel[cid].multi_freq_point_param.points[value->valueint].index=value->valueint;
                subcid=value->valueint;
                printfd("index:%d, subcid=%d ",config->channel[cid].multi_freq_point_param.points[subcid].index,subcid);
            }
            value = cJSON_GetObjectItem(node, "centerFreq");
            if(cJSON_IsNumber(value)){
                config->channel[cid].multi_freq_point_param.points[subcid].center_freq=value->valuedouble;
                printfd("middle_freq:%"PRIu64" ",config->channel[cid].multi_freq_point_param.points[subcid].center_freq);
            }
            value = cJSON_GetObjectItem(node, "bandwidth");
            if(cJSON_IsNumber(value)){
                config->channel[cid].multi_freq_point_param.points[subcid].bandwidth=value->valuedouble;
                printfd("bandwidth:%"PRIu64" ",config->channel[cid].multi_freq_point_param.points[subcid].bandwidth);
            } 
            value = cJSON_GetObjectItem(node, "freqResolution");
            if(cJSON_IsNumber(value)){
                config->channel[cid].multi_freq_point_param.points[subcid].freq_resolution=value->valueint;
                config->channel[cid].multi_freq_point_param.points[subcid].fft_size = config_get_fft_by_resolution((uint32_t)value->valuedouble);
            } else {
                 value = cJSON_GetObjectItem(node, "fftSize");
                 if(cJSON_IsNumber(value)){
                     config->channel[cid].multi_freq_point_param.points[subcid].fft_size=value->valueint;
                     printfd("fftSize:%u, ",config->channel[cid].multi_freq_point_param.points[subcid].fft_size);
                 }
            }
            if(config->channel[cid].multi_freq_point_param.points[subcid].freq_resolution == 0){
                printf_warn("%d, %d, freq_resolution is 0\n", cid, subcid);
            }
            
             value = cJSON_GetObjectItem(node, "decMethodId");
             if(cJSON_IsNumber(value)){
                 config->channel[cid].multi_freq_point_param.points[subcid].d_method=xw_decode_method_convert(value->valueint);
                 printfd("decMethodId:%d, ",config->channel[cid].multi_freq_point_param.points[subcid].d_method);
             }
             value = cJSON_GetObjectItem(node, "decBandwidth");
             if(cJSON_IsNumber(value)){
                config->channel[cid].multi_freq_point_param.points[subcid].d_bandwith = value->valueint;
                printfd("decBandwidth:%u, ",config->channel[cid].multi_freq_point_param.points[subcid].d_bandwith);
             }
             value = cJSON_GetObjectItem(node, "muteSwitch");
             if(cJSON_IsNumber(value)){
                 config->channel[cid].multi_freq_point_param.points[subcid].noise_en=value->valueint;
                 printfd("muteSwitch:%d, ",config->channel[cid].multi_freq_point_param.points[subcid].noise_en);
             }
             value = cJSON_GetObjectItem(node, "muteThreshold");
             if(cJSON_IsNumber(value)){
                 config->channel[cid].multi_freq_point_param.points[subcid].noise_thrh=value->valueint;
                 printfd("muteThreshold:%d, ",config->channel[cid].multi_freq_point_param.points[subcid].noise_thrh);
             }
             printfd("\n");
                  
        }
         
    }  
    cJSON_Delete(root);
    if(config->channel[cid].multi_freq_point_param.freq_point_cnt == 0){
        config->channel[cid].work_mode = OAL_NULL_MODE;
        printf_warn("Unknown Work Mode\n");
    }else if(config->channel[cid].multi_freq_point_param.freq_point_cnt == 1){
        config->channel[cid].work_mode = OAL_FIXED_FREQ_ANYS_MODE;
    }else{
        config->channel[cid].work_mode = OAL_MULTI_POINT_SCAN_MODE;
    }
    return code;
}

/* ??????ddc???????????? */
int parse_json_bddc(const char * const body,uint8_t ch)
{
    int code = RESP_CODE_OK;
    struct poal_config *config = &(config_get_config()->oal_config);
    cJSON *node, *value;
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
    value = cJSON_GetObjectItem(root, "centerFreq");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->channel[ch].multi_freq_point_param.ddc.middle_freq = value->valuedouble;
         printf_note("ch:%d,dec middle_freq:%"PRIu64", middlefreq=%"PRIu64"\n", ch, config->channel[ch].multi_freq_point_param.ddc.middle_freq, 
                                      _get_middle_freq(ch,value->valuedouble, &config->channel[ch].multi_freq_point_param.points[0].center_freq));
    }
    value = cJSON_GetObjectItem(root, "decBandwidth");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->channel[ch].multi_freq_point_param.ddc.bandwidth = value->valuedouble;
         printf_note("decBandwidth:%"PRIu64",\n", config->channel[ch].multi_freq_point_param.ddc.bandwidth);
    }
    executor_set_command(EX_MID_FREQ_CMD, EX_DEC_BW, ch, &config->channel[ch].multi_freq_point_param.ddc.bandwidth);
    executor_set_command(EX_MID_FREQ_CMD, EX_DEC_MID_FREQ, ch,&config->channel[ch].multi_freq_point_param.ddc.middle_freq,
                    _get_middle_freq(ch,config->channel[ch].multi_freq_point_param.ddc.middle_freq, &config->channel[ch].multi_freq_point_param.points[0].center_freq));
    cJSON_Delete(root);
    return code;
}

int parse_json_demodulation(const char * const body,uint8_t cid,uint8_t subid )
{
     printfd(" \n*************demodulation set*****************\n");
     cJSON *node, *value;
     int code = RESP_CODE_OK;
     uint8_t ch = cid;
     struct poal_config *config = &(config_get_config()->oal_config);
     struct sub_channel_freq_para_st *sub_channel_array;
     sub_channel_array = &(config->channel[ch].sub_channel_para);
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
    config->channel[ch].sub_channel_para.cid=cid;
     printfd("cid:%d, subid:%d\n", config->channel[ch].sub_channel_para.cid,subid);
    
    value = cJSON_GetObjectItem(root, "audioSampleRate");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->channel[ch].sub_channel_para.sub_ch[subid].audio_sample_rate_khz=value->valueint;
         printfd("audioSampleRate:%d,\n", value->valueint);
         executor_set_command(EX_MID_FREQ_CMD, EX_SUB_CH_AUDIO_SAMPLE_RATE, subid,  &sub_channel_array->sub_ch[subid].audio_sample_rate_khz);
    }
    value = cJSON_GetObjectItem(root, "centerFreq");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->channel[ch].sub_channel_para.sub_ch[subid].center_freq=value->valuedouble;
         printfd("center_freq:%"PRIu64",\n", config->channel[ch].sub_channel_para.sub_ch[subid].center_freq);
    }
    value = cJSON_GetObjectItem(root, "decBandwidth");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->channel[ch].sub_channel_para.sub_ch[subid].d_bandwith=value->valueint;
         printfd("d_bandwith:%u\n", config->channel[ch].sub_channel_para.sub_ch[subid].d_bandwith);
    }
    value = cJSON_GetObjectItem(root, "fftSize");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->channel[ch].sub_channel_para.sub_ch[subid].fft_size=value->valueint;
         printfd("fftSize:%d,\n", config->channel[ch].sub_channel_para.sub_ch[subid].fft_size);

    }
    value = cJSON_GetObjectItem(root, "decMethodId");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->channel[ch].sub_channel_para.sub_ch[subid].d_method = xw_decode_method_convert(value->valueint);
         config->channel[ch].sub_channel_para.sub_ch[subid].raw_d_method = value->valueint;
         printfd("cid=%d, subid=%d,decMethodId:%d,raw_d_method=%d\n",cid, subid, 
            config->channel[ch].sub_channel_para.sub_ch[subid].d_method, 
            config->channel[ch].sub_channel_para.sub_ch[subid].raw_d_method);
    }
    value = cJSON_GetObjectItem(root, "muteSwitch");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->channel[ch].sub_channel_para.sub_ch[subid].noise_en=value->valueint;
         printfd("muteSwitch:%d,\n", config->channel[ch].sub_channel_para.sub_ch[subid].noise_en);

    }
    value = cJSON_GetObjectItem(root, "muteThreshold");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->channel[ch].sub_channel_para.sub_ch[subid].noise_thrh=value->valueint;
         printfd("muteThreshold:%d,\n", config->channel[ch].sub_channel_para.sub_ch[subid].noise_thrh);
    }
    value = cJSON_GetObjectItem(root, "gainMode");
    if(value!=NULL&&cJSON_IsNumber(value)){
         sub_channel_array->sub_ch[subid].gain_mode=value->valueint;
         printfn("gain_mode:%s,\n", sub_channel_array->sub_ch[subid].gain_mode == POAL_MGC_MODE ? "MGC" : "AGC");
         executor_set_command(EX_MID_FREQ_CMD, EX_SUB_CH_GAIN_MODE, subid,  &sub_channel_array->sub_ch[subid].gain_mode);
    }
    if(sub_channel_array->sub_ch[subid].gain_mode == POAL_MGC_MODE){
        value = cJSON_GetObjectItem(root, "mgcGain");
        if(value!=NULL&&cJSON_IsNumber(value)){
             config->channel[ch].sub_channel_para.sub_ch[subid].mgc_gain=value->valueint;
             printfd("mgc_gain:%d,\n", config->channel[ch].sub_channel_para.sub_ch[subid].mgc_gain);
             executor_set_command(EX_MID_FREQ_CMD, EX_SUB_CH_MGC_GAIN, subid,  &sub_channel_array->sub_ch[subid].mgc_gain);
        }
    }
    value = cJSON_GetObjectItem(root, "agcCtrlTime");
    if(value!=NULL&&cJSON_IsNumber(value)){
         config->channel[ch].sub_channel_para.sub_ch[subid].agc_ctrl_time=value->valueint;
         printfd("agc_ctrl_time:%d,\n", config->channel[ch].sub_channel_para.sub_ch[subid].agc_ctrl_time);
    }
    /* ???????????????????????????????????????????????? */
    executor_set_command(EX_MID_FREQ_CMD, EX_SUB_CH_MID_FREQ, subid,
        &sub_channel_array->sub_ch[subid].center_freq,/* ????????????*/
        _get_middle_freq(subid,sub_channel_array->sub_ch[subid].center_freq, &config->channel[ch].multi_freq_point_param.points[0].center_freq)); /* ?????????????????? */
    /* ????????????, ?????????????????????????????????????????????*/
    executor_set_command(EX_MID_FREQ_CMD, EX_SUB_CH_DEC_BW, subid, 
        &sub_channel_array->sub_ch[subid].d_bandwith,
        sub_channel_array->sub_ch[subid].d_method);
    /* ????????????????????? */
    executor_set_command(EX_MID_FREQ_CMD, EX_SUB_CH_DEC_METHOD, subid, 
        &sub_channel_array->sub_ch[subid].d_method);

    cJSON_Delete(root);
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

    value = cJSON_GetObjectItem(root, "bandwidth");
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
        return RESP_CODE_DISK_DETECTED_ERR;
    }
    if(enable)
        ret = fs_ctx->ops->fs_start_read_raw_file(ch, filename);
    else
        ret = fs_ctx->ops->fs_stop_read_raw_file(ch, filename);
#endif
    cJSON_Delete(root);
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

    value = cJSON_GetObjectItem(root, "bandwidth");
    if(value!=NULL&&cJSON_IsNumber(value)){
         bandwidth=value->valuedouble;
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
            fs_ctx->ops->fs_start_save_file(ch, filename, &bandwidth);
        else
            fs_ctx->ops->fs_stop_save_file(ch, filename);
#endif
    cJSON_Delete(root);
    if(ret != 0)
        return RESP_CODE_EXECMD_ERR;
    else
        return RESP_CODE_OK;
}

static void creat_file_list(char *filename, struct stat *stats, void *_array)
{
    cJSON* item = NULL;
    cJSON *array = (cJSON *)_array;
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
        char *size;
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
    fs_ctx->ops->fs_dir(NULL, creat_file_list, (void *)array);
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
    cJSON_Delete(root);
    return str_json;
}

static int find_file_list(char *filename, struct stat *stats, void *node)
{
    cJSON* item = NULL;
    cJSON *root = (cJSON *)node;
    if(stats == NULL || filename == NULL || root == NULL)
        return -1;
    cJSON_AddStringToObject(root, "filename", filename);
    cJSON_AddNumberToObject(root, "size", stats->st_size);
    cJSON_AddNumberToObject(root, "createTime", stats->st_mtime);
    return 0;
}
char *assemble_json_find_file(char *filename)
{
    char *str_json = NULL;
    int i = 0;
    
    cJSON *root = cJSON_CreateObject();
#if defined(SUPPORT_FS)
    struct fs_context *fs_ctx;
    fs_ctx = get_fs_ctx();
    if(fs_ctx == NULL)
        return NULL;
    fs_ctx->ops->fs_find(filename, find_file_list, (void *)root);
#else
    struct file_list{
        char *filename;
        char * size;
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
    cJSON_Delete(root);
    
    return str_json;
}


char *assemble_json_build_info(void)
{
    char *str_json = NULL;
    cJSON *root = cJSON_CreateObject();
    struct poal_compile_Info *pinfo;
    pinfo = (struct poal_compile_Info *)get_compile_info();
    if(pinfo){
        if(pinfo->build_jenkins_id)
            cJSON_AddStringToObject(root, "build_id", pinfo->build_jenkins_id);
        if(pinfo->build_name)
        cJSON_AddStringToObject(root, "build_name", pinfo->build_name);
        if(pinfo->build_time)
            cJSON_AddStringToObject(root, "build_time", pinfo->build_time);
        if(pinfo->build_version)
            cJSON_AddStringToObject(root, "build_version", pinfo->build_version);
        if(pinfo->build_jenkins_url)
            cJSON_AddStringToObject(root, "build_jenkins_url", pinfo->build_jenkins_url);
        if(pinfo->code_branch)
            cJSON_AddStringToObject(root, "code_branch", pinfo->code_branch);
        if(pinfo->code_hash)
            cJSON_AddStringToObject(root, "code_hash", pinfo->code_hash); 
        if(pinfo->code_url)
            cJSON_AddStringToObject(root, "code_url", pinfo->code_url);
        if(pinfo->release_debug)
            cJSON_AddStringToObject(root, "release_debug", pinfo->release_debug);
    }
    json_print(root, 1);
    str_json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
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
    cJSON_AddStringToObject(root, "kernelversion", get_kernel_version());
    cJSON_AddStringToObject(root, "fpgaversion", version);
    json_print(root, 1);
    str_json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return str_json;
}

char *assemble_json_net_uplink_info(int ch)
{
    char *str_json = NULL;
    cJSON *root = cJSON_CreateObject();
    uint64_t bytes = 0;
    char buffer[128] = {0};

    for(int i = 0; i < MAX_XDMA_NUM; i++)
        bytes += ns_uplink_get_read_bytes(i);
    snprintf(buffer, sizeof(buffer) - 1, "%" PRIu64, bytes);
    cJSON_AddStringToObject(root, "readBytes", buffer);

    bytes = 0;
    for(int i = 0; i < MAX_XDMA_NUM; i++)
        bytes += ns_uplink_get_route_bytes(i);
    snprintf(buffer, sizeof(buffer) - 1, "%" PRIu64, bytes);
    cJSON_AddStringToObject(root, "routeBytes", buffer);

    snprintf(buffer, sizeof(buffer) - 1, "%" PRIu64, statistics_get_all_client_send_ok());
    cJSON_AddStringToObject(root, "sendBytes", buffer);
    snprintf(buffer, sizeof(buffer) - 1, "%" PRIu64, statistics_get_all_client_send_err());
    cJSON_AddStringToObject(root, "sendErrBytes", buffer);

    bytes = 0;
    for(int i = 0; i < MAX_XDMA_NUM; i++)
        bytes += ns_uplink_get_forward_pkgs(i);
    snprintf(buffer, sizeof(buffer) - 1, "%" PRIu64, bytes);
    cJSON_AddStringToObject(root, "forwardPackages", buffer);

    bytes = 0;
    for(int i = 0; i < MAX_XDMA_NUM; i++)
        bytes += ns_downlink_get_route_err_pkgs(i);
    snprintf(buffer, sizeof(buffer) - 1, "%" PRIu64, bytes);
    cJSON_AddStringToObject(root, "routeErrPackages", buffer);

    bytes = 0;
    for(int i = 0; i < MAX_XDMA_NUM; i++)
        bytes += ns_downlink_get_over_run_cnt(i);
    snprintf(buffer, sizeof(buffer) - 1, "%" PRIu64, bytes);
    cJSON_AddStringToObject(root, "overrun", buffer);

    
    bytes = 0;
    for(int i = 0; i < MAX_XDMA_NUM; i++)
        bytes += io_xdma_get_overflow(i);
    snprintf(buffer, sizeof(buffer) - 1, "%" PRIu64, bytes);
    cJSON_AddStringToObject(root, "FpgaOverrun", buffer);

    bytes = 0;
    for(int i = 0; i < MAX_XDMA_NUM; i++)
        bytes += ns_uplink_get_speed(i);
    snprintf(buffer, sizeof(buffer) - 1, "%" PRIu64, bytes);
    cJSON_AddStringToObject(root, "readSpeed", buffer);

    json_print(root, 1);
    str_json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return str_json;
}

char *assemble_json_net_downlink_info(void)
{
    char *str_json = NULL;
    cJSON *root = cJSON_CreateObject();

    char buffer[128] = {0};
    snprintf(buffer, sizeof(buffer) - 1, "%" PRIu64, ns_downlink_get_cmd_pkgs());
    cJSON_AddStringToObject(root, "cmdPackages", buffer);
    snprintf(buffer, sizeof(buffer) - 1, "%" PRIu64, ns_downlink_get_data_pkgs());
    cJSON_AddStringToObject(root, "dataPackages", buffer);
    snprintf(buffer, sizeof(buffer) - 1, "%" PRIu64, ns_downlink_get_err_pkgs());
    cJSON_AddStringToObject(root, "errPackages", buffer);

    json_print(root, 1);
    str_json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return str_json;
}

char *assemble_json_slot_info(void)
{
    char *str_json = NULL;
    cJSON* item = NULL;
    cJSON *array = cJSON_CreateArray();
    int bit = 0;
    char buffer[128] = {0};
    uint64_t bytes = 0;

#ifdef DEBUG_TEST
       cJSON_AddItemToArray(array, item = cJSON_CreateObject());
       cJSON_AddNumberToObject(item, "id", 5);
       cJSON_AddStringToObject(item, "status", "ok");
       cJSON_AddNumberToObject(item, "type", 1);
       cJSON_AddStringToObject(item, "loadStatus",   "OK");
       cJSON_AddStringToObject(item, "uplinkBytes", "0");
       cJSON_AddStringToObject(item, "softVersion", "1.0");
       cJSON_AddNumberToObject(item, "linkSwitch", 1);
       cJSON_AddStringToObject(item, "linkStatus", "OK");
#else
    for_each_set_bit(bit, cards_status_get_bitmap(), MAX_FPGA_CARD_SLOT_NUM){
        cJSON_AddItemToArray(array, item = cJSON_CreateObject());
        cJSON_AddNumberToObject(item, "id", bit);
        snprintf(buffer, sizeof(buffer) - 1, "%s", (io_xdma_is_valid_addr(bit) == true ? "OK" : "False"));
        cJSON_AddStringToObject(item, "status", buffer);
        cJSON_AddNumberToObject(item, "type", 1);
        cJSON_AddStringToObject(item, "loadStatus",  ns_downlink_get_loadbit_str_result(bit));
        bytes = 0;
        for(int i = 0; i < MAX_XDMA_NUM; i++)
            bytes += get_send_bytes_by_type(i, HASHMAP_TYPE_SLOT, bit);
        snprintf(buffer, sizeof(buffer) - 1, "%" PRIu64, bytes);
        cJSON_AddStringToObject(item, "uplinkBytes", buffer);
        snprintf(buffer, sizeof(buffer) - 1, "%x", io_xdma_get_slot_version(bit));
        cJSON_AddStringToObject(item, "softVersion", buffer);
        cJSON_AddNumberToObject(item, "linkSwitch", config_get_link_switch(bit));
        cJSON_AddStringToObject(item, "linkStatus", config_get_link_switch(bit) == 0 ? "??????" : ns_downlink_get_link_str_result(bit));
        cJSON_AddNumberToObject(item, "fmcStatus", io_xdma_get_fmc_status(bit));
    }
#endif
    json_print(array, 1);
    str_json = cJSON_PrintUnformatted(array);
    cJSON_Delete(array);
    return str_json;
}

int _assemble_statistics_client_info(struct net_tcp_client *cl, void* array)
{
    cJSON* item = NULL;
    short port;
    char buffer[128];
    cJSON* _array = array;

    cJSON_AddItemToArray(_array, item = cJSON_CreateObject());
    cJSON_AddStringToObject(item, "ipaddr", cl->get_peer_addr(cl));
    cJSON_AddNumberToObject(item, "port",   cl->get_peer_port(cl));
    snprintf(buffer, sizeof(buffer) - 1, "%" PRIu64, statistics_client_get_send_ok(cl));
    cJSON_AddStringToObject(item, "sendOk", buffer);
    snprintf(buffer, sizeof(buffer) - 1, "%" PRIu64, statistics_client_get_send_err(cl));
    cJSON_AddStringToObject(item, "sendErr",buffer);
    return 0;
}

static int  _sub_info(void *args, int hid, int prio)
{
    cJSON* item = args;
    char buffer[256] = {0};
    if(item == NULL)
        return -1;
    
    snprintf(buffer, sizeof(buffer) - 1, "hid:0x%x, SlotId:0x%x, ChipId:0x%x, FuncId:0x%x, PortId:0x%x, ProiId:0x%x",hid,
                GET_SLOTID_BY_HASHID(hid), GET_CHIPID_BY_HASHID(hid), GET_FUNCID_BY_HASHID(hid),
                GET_PORTID_BY_HASHID(hid),GET_PROIID_BY_HASHID(hid));
    cJSON_AddStringToObject(item, "hashId", buffer);

    return 0;
}


int _assemble_client_sub_info(struct net_tcp_client *cl, void* array)
{
    cJSON* item = NULL;
    short port;
    cJSON* _array = array;

    cJSON_AddItemToArray(_array, item = cJSON_CreateObject());
    cJSON_AddStringToObject(item, "ipaddr", cl->get_peer_addr(cl));
    cJSON_AddNumberToObject(item, "port",   cl->get_peer_port(cl));
    hash_do_for_each(cl->section.hash, _sub_info, item);
    return 0;
}

char *assemble_json_statistics_all_info(void)
{
    char *str_json = NULL;
    cJSON *root = cJSON_CreateObject();
    safe_cJson_AddItemToObject(root, "uplink", assemble_json_net_uplink_info(0));
    safe_cJson_AddItemToObject(root, "downlink", assemble_json_net_downlink_info());
    safe_cJson_AddItemToObject(root, "softversion", assemble_json_softversion());
    safe_cJson_AddItemToObject(root, "device", assemble_json_device_status_info());
    safe_cJson_AddItemToObject(root, "temperature", assemble_json_device_temperature_info());
    safe_cJson_AddItemToObject(root, "sysinfo", assemble_json_sys_info());
    json_print(root, 1);
    str_json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return str_json;
}

char *assemble_json_statistics_client_info(void)
{
    char *str_json = NULL;
    cJSON *array = cJSON_CreateArray();
    for(int i = 0; i < MAX_XDMA_NUM; i++)
        tcp_client_do_for_each(_assemble_statistics_client_info, NULL, i, array);
    str_json = cJSON_PrintUnformatted(array);
    cJSON_Delete(array);
    return str_json;
}

char *assemble_json_client_sub_info(void)
{
    char *str_json = NULL;
    cJSON *array = cJSON_CreateArray();
    for(int i = 0; i < MAX_XDMA_NUM; i++)
        tcp_client_do_for_each(_assemble_client_sub_info, NULL, i, array);
    str_json = cJSON_PrintUnformatted(array);
    cJSON_Delete(array);
    return str_json;
}

char *assemble_json_spm_hash_info(void)
{
    char *str_json = NULL;
    struct spm_context *_ctx;
    void *hash;
    cJSON* item = NULL;
    
    cJSON *array = cJSON_CreateArray();
    for(int i = 0; i < MAX_XDMA_NUM; i++){
        cJSON_AddItemToArray(array, item = cJSON_CreateObject());
        cJSON_AddNumberToObject(item, "ch",   i);
        _ctx = get_spm_ctx();
        if(_ctx && _ctx->run_args[i]){
            hash = _ctx->run_args[i]->hash;
            hash_do_for_each(hash, _sub_info, item);
        }
    }
    str_json = cJSON_PrintUnformatted(array);
    cJSON_Delete(array);
    return str_json;
}


char *assemble_json_sys_info(void)
{
    char *str_json = NULL;
    cJSON *root = cJSON_CreateObject();

    struct sysinfo info;
    if(sysinfo(&info)){
        fprintf(stderr, "Failed to get sysinfo, errno: %u[%s]\n", errno, strerror(errno));
        return NULL;
    }

    char buffer[128] = {0};
    snprintf(buffer, sizeof(buffer) - 1, "%" PRIu64, info.uptime);
    cJSON_AddStringToObject(root, "uptime", buffer);
    snprintf(buffer, sizeof(buffer) - 1, "1min:%lu,5min:%lu,15min:%lu", info.loads[0], info.loads[1], info.loads[2]);
    cJSON_AddStringToObject(root, "loads", buffer);
    snprintf(buffer, sizeof(buffer) - 1, "%" PRIu64, info.totalram);
    cJSON_AddStringToObject(root, "totalram", buffer);
    snprintf(buffer, sizeof(buffer) - 1, "%" PRIu64, info.freeram);
    cJSON_AddStringToObject(root, "freeram", buffer);
    snprintf(buffer, sizeof(buffer) - 1, "%" PRIu64, info.totalswap);
    cJSON_AddStringToObject(root, "totalswap", buffer);
    snprintf(buffer, sizeof(buffer) - 1, "%" PRIu64, info.freeswap);
    cJSON_AddStringToObject(root, "freeswap", buffer);
    snprintf(buffer, sizeof(buffer) - 1, "%u", info.procs);
    cJSON_AddStringToObject(root, "procs", buffer);

    json_print(root, 1);
    str_json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return str_json;

}



char *assemble_json_device_status_info(void)
{
    char *str_json = NULL;
    cJSON *root = cJSON_CreateObject();
    char *info = NULL;
    int code = 0;

    code = config_get_device_status(&info);
    cJSON_AddNumberToObject(root, "status", code);
    if(info)
        cJSON_AddStringToObject(root, "info", info);
    else
        cJSON_AddStringToObject(root, "info", "");
    json_print(root, 1);
    str_json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return str_json;

}

char *assemble_json_device_temperature_info(void)
{
    char *str_json = NULL;
    cJSON *root = cJSON_CreateObject();
    int16_t temp = 0;
    int ret = io_get_adc_temperature_ex(&temp);
    if(ret == 0)
        cJSON_AddNumberToObject(root, "boardTemperature", temp);

    json_print(root, 1);
    str_json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
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
    char version[32] = {0}, _status[32];
    cJSON *root = cJSON_CreateObject();
    io_get_fpga_status(&args);
    snprintf(version, sizeof(version), "%x", get_fpga_version());
    version[sizeof(version) - 1] = 0;
    cJSON_AddStringToObject(root, "fpgaVersion", version);
    cJSON_AddNumberToObject(root, "temperature", args.temp);
    cJSON_AddNumberToObject(root, "fpgaIntVol", args.vol);
    cJSON_AddNumberToObject(root, "fpgaBramVol", args.current);
    cJSON_AddNumberToObject(root, "fpgaResources", args.resources);
    snprintf(_status, sizeof(_status), "0x%x", get_cards_status());
    cJSON_AddStringToObject(root, "status", _status);
    json_print(root, 1);
    str_json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return str_json;
}

#if defined (SUPPORT_GPS)
char *assemble_json_gps_info(void)
{
    char *str_json = NULL;
    bool is_ok = false;
    cJSON *root = cJSON_CreateObject();
    struct gps_info *gpsi;
    char buffer[32];
    is_ok = gps_location_is_valid();
    if(is_ok == true){
        gpsi = gps_get_info();
        cJSON_AddStringToObject(root, "status",  "ok");
        cJSON_AddNumberToObject(root, "utc_hms", gpsi->utc_hms);
        cJSON_AddNumberToObject(root, "utc_ymd", gpsi->utc_ymd);
        cJSON_AddNumberToObject(root, "latitude", gpsi->latitude);
        cJSON_AddNumberToObject(root, "longitude", gpsi->longitude);
        cJSON_AddNumberToObject(root, "altitude", gpsi->altitude);
        snprintf(buffer, sizeof(buffer) - 1, "%c", gpsi->ns);
        cJSON_AddStringToObject(root, "ns", buffer);
        snprintf(buffer, sizeof(buffer) - 1, "%c", gpsi->ew);
        cJSON_AddStringToObject(root, "ew", buffer);
        cJSON_AddNumberToObject(root, "horizontal_speed", gpsi->horizontal_speed);
        cJSON_AddNumberToObject(root, "vertical_speed", gpsi->vertical_speed);
        cJSON_AddNumberToObject(root, "location_type", gpsi->location_type);
        cJSON_AddNumberToObject(root, "true_north_angle", gpsi->true_north_angle);
        cJSON_AddNumberToObject(root, "magnetic_angle", gpsi->magnetic_angle);
    } else{
        cJSON_AddStringToObject(root, "status",  "no");
    }
    cJSON_AddNumberToObject(root, "code", get_gps_status_code(is_ok));
    json_print(root, 1);
    str_json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return str_json;
}
#endif

char *assemble_json_clock_info(void)
{
    char *str_json = NULL;
    uint8_t external_clk = 0;
    bool lock_ok = false;
    cJSON *root = cJSON_CreateObject();
    lock_ok =  io_get_inout_clock_status(&external_clk);
    cJSON_AddStringToObject(root, "inout", (external_clk == 1 ? "out" : "in"));
    cJSON_AddStringToObject(root, "status", (lock_ok == false ? "no":"ok"));
    cJSON_AddNumberToObject(root, "code", get_adc_status_code(lock_ok));
    cJSON_AddNumberToObject(root, "frequency", get_clock_frequency());
    json_print(root, 1);
    str_json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return str_json;
}
char *assemble_json_board_info(void)
{
    char *str_json = NULL;
    int16_t temp;
    struct arg_s{
        float v;
        float i;
    };
    struct arg_s power;
    cJSON *root = cJSON_CreateObject();
    io_get_board_power(&power);
    temp = io_get_adc_temperature();
    cJSON_AddNumberToObject(root, "temperature", temp);
    if(config_is_temperature_warning(temp))
        cJSON_AddStringToObject(root, "warning", "temperature");
    cJSON_AddNumberToObject(root, "voltage", power.v);
    cJSON_AddNumberToObject(root, "current", power.i);
    json_print(root, 1);
    str_json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
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
    int32_t link = get_netlink_status(NETWORK_EHTHERNET_POINT);
    int32_t speed = get_ifname_speed(NETWORK_EHTHERNET_POINT);
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
    cJSON_Delete(root);
    return str_json;
}

char *assemble_json_net_list_info(void)
{
    #define NULL_STR ""
    char *str_json = NULL, *s_speed = "", *s_link = "error";
    int i;
    struct speed_table{
        int32_t i_speed;
        char *s_speed;
    };
    char if_name[IF_NAMESIZE] = {0}, *name;
    struct speed_table spt[] = {
        {SPEED_10, "10Mbps"},
        {SPEED_100, "100Mbps"},
        {SPEED_1000, "1Gbps"},
        {SPEED_2500, "2.5Gbps"},
        {SPEED_10000, "10Gbps"},
    };
    int32_t link, speed;
    struct in_addr ipaddr;
    int num = get_ifname_number();
    uint16_t port = 0;
    cJSON *array = cJSON_CreateArray();
    cJSON* item = NULL;
    for(int index = 1; index <= num; index++){
        name = if_indextoname(index, if_name);
        if(name == NULL)
            continue;
        if(!strcmp(if_name, "lo") || !strcmp(if_name, "docker0")){
            continue;
        }
        link = get_netlink_status(if_name);
        if(link >= 0)
            s_link = (link == 0 ? "down" : "up");
        else
            continue;
        cJSON_AddItemToArray(array, item = cJSON_CreateObject());
        speed = get_ifname_speed(if_name);
        if(speed != 0){
            if(!strcmp(if_name, NETWORK_10G_EHTHERNET_POINT)){
                speed = SPEED_10000;
            }
        }
        s_speed = NULL_STR; 
        for(i = 0; i < ARRAY_SIZE(spt); i++){
            if(speed == spt[i].i_speed){
                s_speed = spt[i].s_speed;
                break;
            }
        }
        cJSON_AddStringToObject(item, "ifname", if_name);
        cJSON_AddStringToObject(item, "link", s_link);
        cJSON_AddStringToObject(item, "speed", s_speed);
        if(get_ipaddress(if_name, &ipaddr) != -1){
            cJSON_AddStringToObject(item, "ipaddr", inet_ntoa(ipaddr));
        }else{
            cJSON_AddStringToObject(item, "ipaddr", "0.0.0.0");
        }
        memset(&ipaddr, 0, sizeof(struct in_addr));
        if(get_netmask(if_name, &ipaddr) != -1){
            cJSON_AddStringToObject(item, "netmask", inet_ntoa(ipaddr));
        }else{
            cJSON_AddStringToObject(item, "netmask", "0.0.0.0");
        }
        memset(&ipaddr, 0, sizeof(struct in_addr));
        if(get_gateway(if_name, &ipaddr) != -1){
            cJSON_AddStringToObject(item, "gateway", inet_ntoa(ipaddr));
        }else{
            cJSON_AddStringToObject(item, "gateway", "0.0.0.0");
        }
        if(config_get_if_cmd_port(if_name, &port) != -1){
            cJSON_AddNumberToObject(item, "port", port);
        }else{
            cJSON_AddNumberToObject(item, "port", 0);
        }
    }
    
    str_json = cJSON_PrintUnformatted(array);
    cJSON_Delete(array);
    return str_json;
}

char *assemble_json_rf_identify_info(void)
{
    char *str_json = NULL;
#ifdef SUPPORT_RF_SPI_M57
    if(get_rfctx() == NULL || get_rfctx()->ops->get_info == NULL){
        return NULL;
    }

    struct rf_info_t *info;
    int i = 0, j = 0;
    cJSON *array = cJSON_CreateArray();
    cJSON* item = NULL;
    info = get_rfctx()->ops->get_info();
    for(i = 0; i < MAX_XDMA_RF_CARD_NUM; i++){
        if(info[i].is_load_info == false)
            continue;
        cJSON_AddItemToArray(array, item = cJSON_CreateObject());
        cJSON_AddNumberToObject(item, "addr", info[i].addr);
        cJSON_AddNumberToObject(item, "serial_num", info[i].serial_num);
        cJSON_AddNumberToObject(item, "vender", info[i].vender);
        cJSON_AddNumberToObject(item, "max_clk_mhz", info[i].max_clk_mhz);
        cJSON_AddNumberToObject(item, "module", info[i].module);
        cJSON_AddStringToObject(item, "fw_version", info[i].fw_version);
        cJSON_AddStringToObject(item, "father_version", info[i].father_version);
        cJSON_AddStringToObject(item, "protocol_version", info[i].protocol_version);
    }
    str_json = cJSON_PrintUnformatted(array);
    cJSON_Delete(array);
#endif
    return str_json;
}

char *assemble_json_rf_info(void)
{
    char *str_json = NULL;
    int i = 0, j = 0;
    cJSON *array = cJSON_CreateArray();
    cJSON* item = NULL;
    int16_t  rf_temp = 0;
    int8_t  rfAttenuation = 0;
    int8_t  mgcGain = 0;
    int8_t  modeCode = 0;
    for(i = 0; i <MAX_RF_NUM; i++){
        cJSON_AddItemToArray(array, item = cJSON_CreateObject());
        cJSON_AddNumberToObject(item, "index", i);
        executor_get_command(EX_RF_FREQ_CMD, EX_RF_STATUS_TEMPERAT, i,  &rf_temp);
        if(rf_temp <= 0){
            cJSON_AddStringToObject(item, "status", "no");
            cJSON_AddNumberToObject(item, "code", get_rf_status_code(true));
        }
        else{
            cJSON_AddStringToObject(item, "status", "ok");
            cJSON_AddNumberToObject(item, "code", get_rf_status_code(false));
            cJSON_AddNumberToObject(item, "temperature", rf_temp);
            #if 0
            if(executor_get_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, i,  &rfAttenuation) == 0){
                cJSON_AddNumberToObject(item, "rfAttenuation", rfAttenuation); 
            }
            if(executor_get_command(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, i,  &mgcGain) == 0){
                cJSON_AddNumberToObject(item, "mgcGain", mgcGain);
            }
            if(executor_get_command(EX_RF_FREQ_CMD, EX_RF_MODE_CODE, i,  &modeCode) == 0){
                cJSON_AddNumberToObject(item, "modeCode", modeCode);
            }
            #endif
        }
    }
   str_json = cJSON_PrintUnformatted(array);
   cJSON_Delete(array);
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
    struct statfs diskInfo;
    memset(&diskInfo, 0, sizeof(diskInfo));
#if defined(SUPPORT_FS)
    struct fs_context *fs_ctx;
    fs_ctx = get_fs_ctx_ex();
    b_valid = fs_ctx->ops->fs_disk_info(&diskInfo);
#endif
    for(i = 0; i < DISK_NUM; i++){
        cJSON_AddItemToArray(array, item = cJSON_CreateObject());
        cJSON_AddNumberToObject(item, "index", i);
        cJSON_AddStringToObject(item, "status", (b_valid == false ? "no":"ok"));
        cJSON_AddNumberToObject(item, "code", get_gps_disk_code(b_valid, &diskInfo));
        if(b_valid){
            cJSON_AddNumberToObject(item, "totalSpace", diskInfo.f_bsize * diskInfo.f_blocks);
            cJSON_AddNumberToObject(item, "freeSpace", diskInfo.f_bsize * diskInfo.f_bfree);
        }
    }
   str_json = cJSON_PrintUnformatted(array);
   cJSON_Delete(array);
   return str_json;
}
char *assemble_json_all_info(void)
{
    char *str_json = NULL;
    cJSON *root = cJSON_CreateObject();
    safe_cJson_AddItemToObject(root, "versionInfo", assemble_json_softversion());
    safe_cJson_AddItemToObject(root, "diskInfo", assemble_json_disk_info());
    safe_cJson_AddItemToObject(root, "clockInfo", assemble_json_clock_info());
    //safe_cJson_AddItemToObject(root, "rfInfo", assemble_json_rf_info());
    safe_cJson_AddItemToObject(root, "boardInfo", assemble_json_board_info());
    safe_cJson_AddItemToObject(root, "fpgaInfo", assemble_json_fpag_info());
#if defined (SUPPORT_GPS)
    safe_cJson_AddItemToObject(root, "gpsInfo", assemble_json_gps_info());
#endif
    safe_cJson_AddItemToObject(root, "netInfo", assemble_json_net_list_info());
    safe_cJson_AddItemToObject(root, "buildInfo", assemble_json_build_info());
    safe_cJson_AddItemToObject(root, "slotInfo", assemble_json_slot_info());
    json_print(root, 1);
    str_json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return str_json;
}
char *assemble_json_selfcheck_info(void)
{
    char *str_json = NULL;
    cJSON *root = cJSON_CreateObject();
    safe_cJson_AddItemToObject(root, "diskInfo", assemble_json_disk_info());
    safe_cJson_AddItemToObject(root, "clockInfo", assemble_json_clock_info());
    safe_cJson_AddItemToObject(root, "boardInfo", assemble_json_board_info());
    safe_cJson_AddItemToObject(root, "fpgaInfo", assemble_json_fpag_info());
#if defined (SUPPORT_GPS)
    safe_cJson_AddItemToObject(root, "gpsInfo", assemble_json_gps_info());
#endif
    json_print(root, 1);
    str_json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return str_json;
}
char *assemble_json_netlist_info(void)
{
    char *str_json = NULL;
    str_json = assemble_json_net_list_info();
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
    if(rf_temp > 0)
        cJSON_AddNumberToObject(root, "rfTemp", rf_temp);
    json_print(root, 1);
    str_json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return str_json;
}


/* NOTE: ???????????????????????????free???????????? */
char *assemble_json_response(int err_code, const char *message)
{
    char *str_json = NULL;
    cJSON *root = cJSON_CreateObject();
    
    cJSON_AddNumberToObject(root, "code", err_code);
    cJSON_AddStringToObject(root, "message", message);
    
    str_json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return str_json;
}


/* NOTE: ???????????????????????????free???????????? */
char *assemble_json_data_response(int err_code, const char *message,const char * data)
{
    char *str_json = NULL, *body = NULL;
    cJSON *root, *node;

    str_json = assemble_json_response(err_code, message);
    root = cJSON_Parse(str_json);
    node = cJSON_Parse(data);
    _safe_free_(str_json);
    _safe_free_(data);
    if (root != NULL){
        cJSON_AddItemToObject(root, "data", node);
    }
    body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return body;
}

