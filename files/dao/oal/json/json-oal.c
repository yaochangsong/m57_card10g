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
*  Rev 1.0   28 Feb. 2020   yaochangsong
*  Initial revision.
******************************************************************************/
#include "config.h"
extern char *get_version_string(void);

static cJSON* root_json = NULL;

int json_print(cJSON *root, int do_format)
{
    if(root == NULL){
        return -1;
    }
    char *printed_json = NULL;
    if (do_format)
    {
        printed_json = cJSON_Print(root);
    }
    else
    {
        printed_json = cJSON_PrintUnformatted(root);
    }
        
    if (printed_json == NULL)
    {
        return -1;
    }
    printf("%s\n", printed_json);
    free(printed_json);
    return 0;
}

static int json_write_file(char *filename, cJSON *root)
{
    FILE *fp;
    char *printed_json = NULL;
    int do_format = 1;

    if(filename == NULL || root == NULL){
        return -1;
    }
    
    if (do_format)
    {
        printed_json = cJSON_Print(root);
    }
    else
    {
        printed_json = cJSON_PrintUnformatted(root);
    }
    
    fp = fopen(filename, "w");
    if(!fp){
        printf("Open file error!\n");
        return -1;
    }

    fwrite(printed_json,1,strlen(printed_json),fp);

    free(printed_json);
    fclose(fp);
    
    return 0;
}


static cJSON* json_read_file(const char *filename, cJSON* root)
{
    long len = 0;
    size_t  temp;
    char *JSON_content;

    FILE* fp = fopen(filename, "rb+");
    if(!fp)
    {
        printf_err("open file %s failed.\n", filename);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    if(0 == len)
    {
        fclose(fp);
        return NULL;
    }

    fseek(fp, 0, SEEK_SET);
    JSON_content = (char*) malloc(sizeof(char) * len);
    temp = fread(JSON_content, 1, len, fp);

    fclose(fp);
    root = cJSON_Parse(JSON_content);
    if (root == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return NULL;
    }
    free(JSON_content);

    return root;
}

static inline  int json_write_string_param(const char * const grandfather, const char * const parents, const  char * const child, char *data)
{
    cJSON* root = root_json;
    if(root == NULL || parents == NULL || child == NULL || data == NULL)
        return -1;
    cJSON *grand_object = NULL, *object = NULL;
    if(grandfather != NULL){
        grand_object = cJSON_GetObjectItem(root, grandfather);
        if(grand_object != NULL)
            object = cJSON_GetObjectItem(grand_object, parents);
    }else{
        object = cJSON_GetObjectItem(root, parents);
    }
    if(object != NULL)
        cJSON_ReplaceItemInObject(object, child, cJSON_CreateString((char *)data));
    return 0;
}

static inline  int json_write_double_param(const char * const grandfather, const char * const parents, const char * const child, double data)
{
    cJSON* root = root_json;
    if(root == NULL || parents == NULL || child == NULL)
        return -1;
    cJSON *grand_object = NULL, *object = NULL;
    if(grandfather != NULL){
        grand_object = cJSON_GetObjectItem(root, grandfather);
        object = cJSON_GetObjectItem(grand_object, parents);
    }else{
        object = cJSON_GetObjectItem(root, parents);
    }
    cJSON_GetObjectItem(object,child)->valuedouble = data;

    return 0;
}

static inline  int json_write_int_param(const char * const grandfather, const char * const parents, const char * const child, int data)
{
    cJSON* root = root_json;
    if(root == NULL || parents == NULL || child == NULL)
        return -1;
    cJSON *grand_object = NULL, *object = NULL;
    if(grandfather != NULL){
        grand_object = cJSON_GetObjectItem(root, grandfather);
        if(grand_object != NULL)
            object = cJSON_GetObjectItem(grand_object, parents);
    }else{
        object = cJSON_GetObjectItem(root, parents);
    }
    if(object != NULL)
        cJSON_GetObjectItem(object,child)->valuedouble = data;
    
    return 0;
}




static inline  int json_write_array_string_param(const char * const parents, const char * const array, int index, const char * const name,  char *data)
{
    cJSON* root = root_json;
    cJSON *grand_object = NULL, *object = NULL, *node= NULL;
    if(root == NULL || array == NULL || name == NULL || data == NULL)
        return -1;
    if(parents != NULL){
        grand_object = cJSON_GetObjectItem(root, parents);
        object = cJSON_GetObjectItem(grand_object, array);
    }else{
        object = cJSON_GetObjectItem(root, array);
    }
    node = cJSON_GetArrayItem(object, index);
    cJSON_ReplaceItemInObject(object, node, cJSON_CreateString((char *)data));
    //strcpy(cJSON_GetObjectItem(node,name)->valuestring, data);
    
    return 0;
}


static inline  int json_write_array_int_param(const      char *const parents,const    char *const array,int index,const char * const name,  int data)
{
    cJSON* root = root_json;
    cJSON *grand_object = NULL, *object = NULL, *node= NULL;
    if(root == NULL || array == NULL || name == NULL)
        return -1;
    if(parents != NULL){
        grand_object = cJSON_GetObjectItem(root, parents);
        object = cJSON_GetObjectItem(grand_object, array);
    }else{
        object = cJSON_GetObjectItem(root, array);
    }
    node = cJSON_GetArrayItem(object, index);
    /*  更新不成功 */
    //cJSON_GetObjectItem(node,name)->valueint = data;
    cJSON_GetObjectItem(node,name)->valuedouble = (double)data;

    return 0;
}

static inline  int json_write_array_double_param(const char * const parents, const char * const array, int index, const char * const name,  double data)
{
    cJSON* root = root_json;
    cJSON *grand_object = NULL, *object = NULL, *node= NULL;
    if(root == NULL || array == NULL || name == NULL)
        return -1;
    if(parents != NULL){
        grand_object = cJSON_GetObjectItem(root, parents);
        object = cJSON_GetObjectItem(grand_object, array);
    }else{
        object = cJSON_GetObjectItem(root, array);
    }
    node = cJSON_GetArrayItem(object, index);
    cJSON_GetObjectItem(node,name)->valuedouble = data;
    
    return 0;
}

static int json_write_config_param(cJSON* root, struct poal_config *config)
{

    cJSON* node;
    cJSON* group;

    cJSON *network = NULL;
    struct sockaddr_in saddr;

    
    /*network*/
    /* 1g */
    saddr.sin_addr.s_addr = config->network.gateway;
    json_write_string_param(NULL, "network", "gateway", inet_ntoa(saddr.sin_addr));
    network = cJSON_GetObjectItem(root, "network");
    node = cJSON_GetObjectItem(network, "gateway");
    printf_debug("gatewayis:%s\n",node->valuestring);

    saddr.sin_addr.s_addr = config->network.ipaddress;
    json_write_string_param(NULL, "network", "ipaddress", inet_ntoa(saddr.sin_addr));
    node = cJSON_GetObjectItem(network, "ipaddress");
    printf_debug("ipaddress is:%s\n",node->valuestring);
    
    saddr.sin_addr.s_addr = config->network.netmask;
    json_write_string_param(NULL, "network", "netmask", inet_ntoa(saddr.sin_addr));
    node = cJSON_GetObjectItem(network, "netmask");
    printf_debug("netmask:%s\n",node->valuestring);

    
    json_write_double_param(NULL, "network", "port", config->network.port);
    node = cJSON_GetObjectItem(network, "port");
    printf_debug("port is:%d\n",node->valueint);

    char temp[30]={0};
    sprintf(temp,"%02x:%02x:%02x:%02x:%02x:%02x", config->network.mac[0],config->network.mac[1],
    config->network.mac[2],config->network.mac[3],config->network.mac[4],config->network.mac[5]);
    json_write_int_param(NULL, "network", "mac", temp);
    node = cJSON_GetObjectItem(network, "mac");
    printf_debug("mac is:%s\n",node->valuestring);

#ifdef SUPPORT_NET_WZ
    /* 10g */
    saddr.sin_addr.s_addr = config->network_10g.gateway;
    json_write_string_param(NULL, "network_10g", "gateway", inet_ntoa(saddr.sin_addr));
    network = cJSON_GetObjectItem(root, "network");
    node = cJSON_GetObjectItem(network, "gateway");
    printf_debug("10g gatewayis:%s\n",node->valuestring);

    saddr.sin_addr.s_addr = config->network_10g.ipaddress;
    json_write_string_param(NULL, "network_10g", "ipaddress", inet_ntoa(saddr.sin_addr));
    node = cJSON_GetObjectItem(network, "ipaddress");
    printf_debug("10g ipaddress is:%s\n",node->valuestring);
    
    saddr.sin_addr.s_addr = config->network_10g.netmask;
    json_write_string_param(NULL, "network_10g", "netmask", inet_ntoa(saddr.sin_addr));
    node = cJSON_GetObjectItem(network, "netmask");
    printf_debug("10g netmask:%s\n",node->valuestring);

    
    json_write_double_param(NULL, "network_10g", "port", config->network_10g.port);
    node = cJSON_GetObjectItem(network, "port");
    printf_debug("10g port is:%d\n",node->valueint);

    
    sprintf(temp,"%02x:%02x:%02x:%02x:%02x:%02x", config->network_10g.mac[0],config->network_10g.mac[1],
    config->network.mac[2],config->network_10g.mac[3],config->network_10g.mac[4],config->network_10g.mac[5]);
    json_write_string_param(NULL, "network_10g", "mac", temp);
    node = cJSON_GetObjectItem(network, "mac");
    printf_debug("10g mac is:%s\n",node->valuestring);
#endif
    /*control_parm*/
     json_write_int_param(NULL, "control_parm", "spectrum_time_interval", config->ctrl_para.spectrum_time_interval);
     json_write_int_param(NULL, "control_parm", "bandwidth_remote_ctrl", config->ctrl_para.remote_local);
      json_write_int_param(NULL, "control_parm", "internal_clock", config->ctrl_para.internal_clock);
    /*status_parm*/
    json_write_string_param("status_parm", "soft_version", "app", config->status_para.softVersion.app);

   /*calibration_parm */
   json_write_int_param(NULL, "calibration_parm", "psd_power_global",config->cal_level.specturm.global_roughly_power_lever);
       /* psd_fftsize */
     int i=0;

     for(i=0;i<5;i++)
     {
         json_write_array_int_param("calibration_parm", "psd_fftsize", i, "fftsize", config->cal_level.cali_fft.fftsize[i]);
         json_write_array_int_param("calibration_parm", "psd_fftsize", i, "value", config->cal_level.cali_fft.cali_value[i]);
        
     }

       /* psd_freq */
     for(i=0;i<1;i++)
     {
         json_write_array_int_param("calibration_parm", "psd_freq", i, "start_freq", config->cal_level.specturm.start_freq_khz[i]);
         json_write_array_int_param("calibration_parm", "psd_freq", i, "end_freq", config->cal_level.specturm.end_freq_khz[i]);
         json_write_array_int_param("calibration_parm", "psd_freq", i, "value", config->cal_level.specturm.power_level[i]);
     }

      /* analysis_power_global */
     json_write_int_param(NULL, "calibration_parm", "analysis_power_global",config->cal_level.analysis.global_roughly_power_lever);
     for(i=0;i<1;i++)
     {
         json_write_array_int_param("calibration_parm", "analysis_freq", i, "start_freq", config->cal_level.analysis.start_freq_khz[i]);
         json_write_array_int_param("calibration_parm", "analysis_freq", i, "end_freq", config->cal_level.analysis.end_freq_khz[i]);
         json_write_array_int_param("calibration_parm", "analysis_freq", i, "value", config->cal_level.analysis.power_level[i]);
     }

     /* low_noise_power */
     json_write_int_param(NULL, "calibration_parm", "low_noise_power",config->cal_level.low_noise.global_power_val);
      /* lo_leakage_threshold */
     json_write_int_param(NULL, "calibration_parm", "lo_leakage_threshold",config->cal_level.lo_leakage.global_threshold);
      /* lo_leakage_renew_data_len */
     json_write_int_param(NULL, "calibration_parm", "lo_leakage_renew_data_len",config->cal_level.lo_leakage.global_renew_data_len);

     /* lo_leakage */
     for(i=0;i<2;i++)
     {

         json_write_array_int_param("calibration_parm", "lo_leakage", i, "fftsize", config->cal_level.lo_leakage.fft_size[i]);
         json_write_array_int_param("calibration_parm", "lo_leakage", i, "threshold",config->cal_level.lo_leakage.threshold[i]);
         json_write_array_int_param("calibration_parm", "lo_leakage", i, "renew_data_len",config->cal_level.lo_leakage.renew_data_len[i]);
     }

      /* mgc_gain_global */  
     json_write_int_param(NULL, "calibration_parm", "mgc_gain_global",config->cal_level.mgc.global_gain_val);
      /* mgc_gain_freq */
     for(i=0;i<2;i++)
     {

         json_write_array_int_param("calibration_parm", "mgc_gain_freq", i, "start_freq", config->cal_level.mgc.start_freq_khz[i]);
         json_write_array_int_param("calibration_parm", "mgc_gain_freq", i, "end_freq",config->cal_level.mgc.end_freq_khz[i]);
         json_write_array_int_param("calibration_parm", "mgc_gain_freq", i, "value",config->cal_level.mgc.gain_val[i]);
     }

     /* spectrum_parm */
          /* side_bandrate */
     for(i=0;i<4;i++)
     {
         json_write_array_double_param("spectrum_parm", "side_bandrate", i,"rate",config->ctrl_para.scan_bw.sideband_rate[i]);
         json_write_array_int_param("spectrum_parm", "side_bandrate", i, "bandwidth",config->ctrl_para.scan_bw.bindwidth_hz[i]);
     }

       /*if_parm*/
     for(i=0;i<1;i++)
     {

         json_write_array_int_param("spectrum_parm", "if_parm", i,"channel",config->multi_freq_point_param[i].cid);
         json_write_array_int_param("spectrum_parm", "if_parm", i, "middle_freq",config->multi_freq_point_param[i].points[0].center_freq);
         json_write_array_int_param("spectrum_parm", "if_parm", i, "bandwith",config->multi_freq_point_param[i].points[0].bandwidth);
         json_write_array_int_param("spectrum_parm", "if_parm", i, "dec_bandwith",config->multi_freq_point_param[i].points[0].d_bandwith);
         json_write_array_int_param("spectrum_parm", "if_parm", i, "dec_method",config->multi_freq_point_param[i].points[0].d_method);
         json_write_array_int_param("spectrum_parm", "if_parm", i, "mute_switch",config->multi_freq_point_param[i].points[0].noise_en);
         json_write_array_int_param("spectrum_parm", "if_parm", i, "audio_volume",config->multi_freq_point_param[i].points[0].audio_volume);
     }
     /*rf_parm*/
     for(i=0;i<1;i++)
     {

         json_write_array_int_param("spectrum_parm", "rf_parm", i,"channel",config->rf_para[i].cid);
         json_write_array_int_param("spectrum_parm", "rf_parm", i, "attenuation",config->rf_para[i].attenuation);
         json_write_array_int_param("spectrum_parm", "rf_parm", i,"mode_code",config->rf_para[i].rf_mode_code);
         json_write_array_int_param("spectrum_parm", "rf_parm", i, "gain_mode",config->rf_para[i].gain_ctrl_method);
         json_write_array_int_param("spectrum_parm", "rf_parm", i,"agc_ctrl_time",config->rf_para[i].agc_ctrl_time);
         json_write_array_int_param("spectrum_parm", "rf_parm", i, "agc_output_amp_dbm",config->rf_para[i].agc_mid_freq_out_level);
         json_write_array_int_param("spectrum_parm", "rf_parm", i, "mgc_gain",config->rf_para[i].mgc_gain_value);
         json_write_array_double_param("spectrum_parm", "rf_parm", i, "middle_freq",config->rf_para[i].mid_freq);
         json_write_array_int_param("spectrum_parm", "rf_parm", i, "mid_bw",config->rf_para[i].mid_bw);
     }

    return 0;
}


static int json_parse_config_param(const cJSON* root, struct poal_config *config)
{
    cJSON *value = NULL;
    cJSON *node = NULL;
    
/* network */
    struct sockaddr_in saddr;
    cJSON *network = NULL;

    if(root == NULL || config == NULL){
        return -1;
    }
    
    /* status_parm */
    cJSON *status_parm = NULL;
    status_parm = cJSON_GetObjectItem(root, "status_parm");
    if(status_parm == NULL){
        printf_warn("not found json node[%s]\n","status_parm");
        return -1;
    }

    node = cJSON_GetObjectItem(status_parm, "soft_version");
    if(node != NULL){
        value = cJSON_GetObjectItem(node, "app");
        if(value!= NULL && cJSON_IsString(value)){
            char *version = get_version_string();
            config->status_para.softVersion.app = value->valuestring;
            if(strcmp(version, config->status_para.softVersion.app)){
                config->status_para.softVersion.app = version;
                json_write_string_param("status_parm", "soft_version", "app", config->status_para.softVersion.app);
                json_write_file(config_get_config()->configfile, root);
                printf_note("renew verson: %s\n", version);
            }
            printf_debug("app=>value is:%s, %s\n",value->valuestring, config->status_para.softVersion.app);
        }

        #if 0
        value = cJSON_GetObjectItem(node, "fpga");
        if(value!= NULL && cJSON_IsString(value)){
            char ver[33] = {0};
            sprintf(ver, "%x", get_fpga_version());
            char *version = strdup(ver);
            config->status_para.softVersion.fpga = value->valuestring;
            if(strcmp(version, config->status_para.softVersion.fpga)){
                config->status_para.softVersion.fpga = version;
                json_write_string_param("status_parm", "soft_version", "fpga", config->status_para.softVersion.fpga);
                json_write_file(config_get_config()->configfile, root);
                printf_note("renew fpga verson: %s\n", version);
            }
            printf_note("fpga version=>value is:%s, %s\n",value->valuestring, config->status_para.softVersion.fpga);
        }
        #endif
    }
    value = cJSON_GetObjectItem(status_parm, "device_sn");
    if(value!= NULL && cJSON_IsString(value)){
        config->status_para.device_sn = value->valuestring;
        printf_debug("config->status_para.device_sn=>value is:%s\n",config->status_para.device_sn);
    }else{
        config->status_para.device_sn = NULL;
    }
    
    network = cJSON_GetObjectItem(root, "network");
    if(network == NULL){
        printf_warn("not found json node[%s]\n","network");
        return -1;
    }
    value = cJSON_GetObjectItem(network, "mac");
    if(value!= NULL && cJSON_IsString(value)){
        char mac_buf[128] = {0};
        if(config->network.mac){
            sprintf(mac_buf, "%02X:%02X:%02X:%02X:%02X:%02X", config->network.mac[0], 
                                                            config->network.mac[1],
                                                            config->network.mac[2],
                                                            config->network.mac[3],
                                                            config->network.mac[4],
                                                            config->network.mac[5]);
            if(strcmp(value->valuestring, mac_buf)){
                printf_note("renew mac: %s\n", mac_buf);
                json_write_string_param(NULL, "network", "mac", mac_buf);
                json_write_file(config_get_config()->configfile, root);
            }
        }
    }
    value = cJSON_GetObjectItem(network, "gateway");
    if(value!= NULL && cJSON_IsString(value)){
        saddr.sin_addr.s_addr = inet_addr(value->valuestring);
        printf_debug("gateway=>value is:%s, %s\n",value->valuestring, inet_ntoa(saddr.sin_addr));
        config->network.gateway = saddr.sin_addr.s_addr;
    }
    value = cJSON_GetObjectItem(network, "port");
    if(value!= NULL && cJSON_IsNumber(value)){
        config->network.port = value->valueint;
        printf_debug("port=>value is:%d\n",config->network.port);
    }
    value = cJSON_GetObjectItem(network, "ipaddress");
    if(value!= NULL && cJSON_IsString(value)){
        saddr.sin_addr.s_addr = inet_addr(value->valuestring);
        printf_debug("ipaddress=>value is:%s, %s\n",value->valuestring, inet_ntoa(saddr.sin_addr));
        config->network.ipaddress = saddr.sin_addr.s_addr;
    }
    value = cJSON_GetObjectItem(network, "netmask");
    if(value!= NULL && cJSON_IsString(value)){
        saddr.sin_addr.s_addr = inet_addr(value->valuestring);
        printf_debug("netmask=>value is:%s, %s\n",value->valuestring, inet_ntoa(saddr.sin_addr));
        config->network.netmask = saddr.sin_addr.s_addr;
    }
    value = cJSON_GetObjectItem(network,"mac");
    if(value!=NULL&&cJSON_IsString(value)){
    char temp[30];
    char mactemp[30]={0}; 
    char *p=mactemp;
    char buff[20]={0};
    int j=0,k=0,i=0;
    
   //sprintf(temp,"%s",value->valuestring);
     for(i=0;i<30;i++)
    {
        if(value->valuestring[i]!=':')
        {
           *p=value->valuestring[i];
           p++;
        }
    }
    // printf_debug("mactemp=%s\n",mactemp);
    for(i=0;i<6;i++)
    {
       buff[0] = mactemp[k++];
       buff[1] = mactemp[k++];
       config->network.mac[i] = strtol(buff, NULL, 16);
    }

   // printf_debug("mactemp=%s\n",mactemp);
    printf_debug("mac=%02x%02x%02x%02x%02x%02x\n",config->network.mac[0],config->network.mac[1],config->network.mac[2],
    config->network.mac[3],config->network.mac[4],config->network.mac[5]);
    }
    #ifdef SUPPORT_NET_WZ
    /*10g net*/
    network = cJSON_GetObjectItem(root, "network_10g");
    if(network == NULL){
        printf_warn("not found json node[%s]\n","network_10g");
    }else{
        value = cJSON_GetObjectItem(network, "ipaddress");
        if(value!= NULL && cJSON_IsString(value)){
            saddr.sin_addr.s_addr = inet_addr(value->valuestring);
            printf_debug("10g ipaddr=>value is:%s, %s\n",value->valuestring, inet_ntoa(saddr.sin_addr));
            config->network_10g.ipaddress = saddr.sin_addr.s_addr;
        }
        value = cJSON_GetObjectItem(network, "netmask");
        if(value!= NULL && cJSON_IsString(value)){
            saddr.sin_addr.s_addr = inet_addr(value->valuestring);
            printf_debug("10g netmask=>value is:%s, %s\n",value->valuestring, inet_ntoa(saddr.sin_addr));
            config->network_10g.netmask = saddr.sin_addr.s_addr;
        }
        value = cJSON_GetObjectItem(network, "gateway");
        if(value!= NULL && cJSON_IsString(value)){
            saddr.sin_addr.s_addr = inet_addr(value->valuestring);
            printf_debug("10g gateway=>value is:%s, %s\n",value->valuestring, inet_ntoa(saddr.sin_addr));
            config->network_10g.gateway = saddr.sin_addr.s_addr;
        }
        value = cJSON_GetObjectItem(network, "port");
        if(value!= NULL && cJSON_IsNumber(value)){
            config->network_10g.port = value->valueint;
            printf_debug("10g port=>value is:%d\n",config->network_10g.port);
        }
    }
    #endif
    /*control_parm*/
    cJSON *control_parm = NULL;
    control_parm = cJSON_GetObjectItem(root, "control_parm");
    if(control_parm == NULL){
        printf_warn("not found json node[%s]\n","network");
        return -1;
    }
    value = cJSON_GetObjectItem(control_parm, "spectrum_time_interval");
    if(value!= NULL && cJSON_IsNumber(value)){
        config->ctrl_para.spectrum_time_interval=value->valueint;
        printf_debug("aspectrum_time_intervalis:%d, \n",config->ctrl_para.spectrum_time_interval);
    }
    value = cJSON_GetObjectItem(control_parm, "bandwidth_remote_ctrl");
    if(value!= NULL && cJSON_IsNumber(value)){
        config->ctrl_para.remote_local=value->valueint;
        printf_debug("aspectrum_time_intervalis:%d, \n",config->ctrl_para.remote_local);
    }
    value = cJSON_GetObjectItem(control_parm, "internal_clock");
    if(value!= NULL && cJSON_IsNumber(value)){
        config->ctrl_para.internal_clock=value->valueint;
        printf_debug("aspectrum_time_intervalis:%d, \n",config->ctrl_para.internal_clock);
    }
#ifdef SUPPORT_NET_WZ
    value = cJSON_GetObjectItem(control_parm, "net10g_threshold_bandwidth");
    if(value!= NULL && cJSON_IsNumber(value)){
        config->ctrl_para.wz_threshold_bandwidth=value->valuedouble;
        printf_debug("net 10g threshold bandwidth:%uHz, \n",config->ctrl_para.wz_threshold_bandwidth);
    }
#endif
    value = cJSON_GetObjectItem(control_parm, "signal_threshold");
    if(value!= NULL && cJSON_IsNumber(value)){
        config->ctrl_para.signal.threshold=value->valueint;
        printf_debug("signal.threshold:%d, \n",config->ctrl_para.signal.threshold);
    }
	value = cJSON_GetObjectItem(control_parm, "iq_data_length");
    if(value!= NULL && cJSON_IsNumber(value)){
        config->ctrl_para.iq_data_length=value->valueint;
        printf_debug("iq_data_length:%d, \n",config->ctrl_para.iq_data_length);
    }
    /*
    value = cJSON_GetObjectItem(control_parm, "agc_ref_val_0dbm");
    if(value!= NULL && cJSON_IsNumber(value)){
        config->ctrl_para.agc_ref_val_0dbm=value->valueint;
        printf_debug("agc_ref_val_0dbm:%d, \n",config->ctrl_para.agc_ref_val_0dbm);
    } */
    
/* calibration_parm */
    cJSON *calibration = NULL;
    calibration = cJSON_GetObjectItem(root, "calibration_parm");
    if(calibration == NULL){
        printf_warn("not found json node[%s]\n","calibration_parm");
        return -1;
    }
    value = cJSON_GetObjectItem(calibration, "psd_power_global");
    if(value!= NULL && cJSON_IsNumber(value)){
        config->cal_level.specturm.global_roughly_power_lever = value->valueint;
        printf_debug("psd_power_global=>value is:%d\n",config->cal_level.specturm.global_roughly_power_lever);
    }
    /* psd_fftsize */
    cJSON *psd_fftsize = NULL;
    psd_fftsize = cJSON_GetObjectItem(calibration, "psd_fftsize");
    if(psd_fftsize != NULL){
        printf_debug("psd_fftsize:\n");
        for(int i = 0; i < cJSON_GetArraySize(psd_fftsize); i++){
            printfd("index:%d ", i);
            node = cJSON_GetArrayItem(psd_fftsize, i);
            value = cJSON_GetObjectItem(node, "fftsize");
            if(cJSON_IsNumber(value))
            {
                //printfd("fftsize:%d, ", value->valueint);
                config->cal_level.cali_fft.fftsize[i]=value->valueint;
                printfd("fftsize:%d, ", config->cal_level.cali_fft.fftsize[i]);
            }
            
            value = cJSON_GetObjectItem(node, "value");
            if(cJSON_IsNumber(value)){
               // printfd("value:%d", value->valueint);
                config->cal_level.cali_fft.cali_value[i]=value->valueint;
                printfd("value:%d, ", config->cal_level.cali_fft.cali_value[i]);
            }
             printfd("\n");
        }
    }
    /* psd_freq */
   cJSON  *psd_freq = NULL;
   psd_freq = cJSON_GetObjectItem(calibration, "psd_freq");
   if(psd_freq!=NULL){
        printf_debug("psd_freq:\n");
        for(int i = 0; i < cJSON_GetArraySize(psd_freq); i++){
            printfd("index:%d ", i);
            node = cJSON_GetArrayItem(psd_freq, i);            
            value = cJSON_GetObjectItem(node, "fftsize");
            if(cJSON_IsNumber(value)){
                config->cal_level.spm_level.cal_node[i].fft =value->valueint;
                printfd("fft:%u, ", config->cal_level.spm_level.cal_node[i].fft);
            }
            value = cJSON_GetObjectItem(node, "start_freq");
            if(cJSON_IsNumber(value)){
                config->cal_level.spm_level.cal_node[i].start_freq_khz =value->valueint;
                printfd("start freq:%u, ", config->cal_level.spm_level.cal_node[i].start_freq_khz);
            }
            value = cJSON_GetObjectItem(node, "end_freq");
            if(cJSON_IsNumber(value)){
                config->cal_level.spm_level.cal_node[i].end_freq_khz =value->valueint;
                printfd("end freq:%u, ", config->cal_level.spm_level.cal_node[i].end_freq_khz);
            } 
            value = cJSON_GetObjectItem(node, "value");
             if(cJSON_IsNumber(value)){
                config->cal_level.spm_level.cal_node[i].power_level =value->valueint;
                printfd("power level:%d, ", config->cal_level.spm_level.cal_node[i].power_level);
            } 
            printfd("\n");
        }
    
   }
    
    /* analysis_power_global */
    value = cJSON_GetObjectItem(calibration, "analysis_power_global");
    if(value!= NULL && cJSON_IsNumber(value)){
        config->cal_level.analysis.global_roughly_power_lever = value->valueint;
        printf_debug("analysis_power_global=>value is:%d\n",config->cal_level.analysis.global_roughly_power_lever );
    }
    /* analysis_freq */
    cJSON  *analysis_freq = NULL;
    analysis_freq = cJSON_GetObjectItem(calibration, "analysis_freq");
    if(analysis_freq!=NULL){
         printf_debug("analysis_freq:\n");
         for(int i = 0; i < cJSON_GetArraySize(analysis_freq); i++){
             printfd("index:%d ", i);
             node = cJSON_GetArrayItem(analysis_freq, i);            
             value = cJSON_GetObjectItem(node, "start_freq");
             if(cJSON_IsNumber(value)){
                 //printfd("start_freq:%d, ", value->valueint);
                 config->cal_level.analysis.start_freq_khz[i]=value->valueint;
                 printfd("start_freq:%u, ", config->cal_level.analysis.start_freq_khz[i]);
             }
             value = cJSON_GetObjectItem(node, "end_freq");
             if(cJSON_IsNumber(value)){
                // printfd("end_freq:%d, ", value->valueint);
                 config->cal_level.analysis.end_freq_khz[i]=value->valueint;
                 printfd("end_freq:%u, ", config->cal_level.analysis.end_freq_khz[i]);
             } 
             value = cJSON_GetObjectItem(node, "value");
              if(cJSON_IsNumber(value)){
                // printfd("value:%d, ", value->valueint);
                 config->cal_level.analysis.power_level[i]=value->valueint;
                 printfd("value:%d, ",config->cal_level.analysis.power_level[i]);
             } 
              printfd("\n");
         }
     
    }        
    /* low_noise_power */
    value = cJSON_GetObjectItem(calibration, "low_noise_power");
    if(value!= NULL && cJSON_IsNumber(value)){
        config->cal_level.low_noise.global_power_val = value->valueint;
        printf_debug("low_noise_power=>value is:%d\n",config->cal_level.low_noise.global_power_val);
    }
    /* lo_leakage_threshold */
    value = cJSON_GetObjectItem(calibration, "lo_leakage_threshold");
    if(value!= NULL && cJSON_IsNumber(value)){
        config->cal_level.lo_leakage.global_threshold = value->valueint;
        printf_debug("lo_leakage_threshold=>value is:%d\n",config->cal_level.lo_leakage.global_threshold);
    } 
    /* lo_leakage_renew_data_len */
    value = cJSON_GetObjectItem(calibration, "lo_leakage_renew_data_len");
    if(value!= NULL && cJSON_IsNumber(value)){
        config->cal_level.lo_leakage.global_renew_data_len = value->valueint;
        printf_debug("lo_leakage_renew_data_len=>value is:%d\n",config->cal_level.lo_leakage.global_renew_data_len);
    }     
    /* lo_leakage */
    cJSON *lo_leakage=NULL;
    lo_leakage = cJSON_GetObjectItem(calibration, "lo_leakage");
    if(lo_leakage!=NULL){
        printf_debug("lo_leakage:\n");
        for(int i = 0; i < cJSON_GetArraySize(lo_leakage); i++){
            printfd("index:%d ", i);
            node = cJSON_GetArrayItem(lo_leakage, i);            
            value = cJSON_GetObjectItem(node, "fftsize");
            if(cJSON_IsNumber(value)){
              //  printfd("fftsize:%d, ", value->valueint);
                config->cal_level.lo_leakage.fft_size[i]=value->valueint;
                printfd("fftsize:%d, ", config->cal_level.lo_leakage.fft_size[i]);
            }
            value = cJSON_GetObjectItem(node, "threshold");
            if(cJSON_IsNumber(value)){
               // printfd("threshold:%d, ", value->valueint);
                config->cal_level.lo_leakage.threshold[i]=value->valueint;
                printfd("threshold:%d, ", config->cal_level.lo_leakage.threshold[i]);
            } 
            value = cJSON_GetObjectItem(node, "renew_data_len");
             if(cJSON_IsNumber(value)){
              //  printfd("renew_data_len:%d, ", value->valueint);
                config->cal_level.lo_leakage.renew_data_len[i]=value->valueint;
                printfd("renew_data_len:%d, ", config->cal_level.lo_leakage.renew_data_len[i]);
            } 
             printfd("\n");
        }
    }
     
    /* mgc_gain_global */
    value = cJSON_GetObjectItem(calibration, "mgc_gain_global");
    if(value!= NULL && cJSON_IsNumber(value)){
        config->cal_level.mgc.global_gain_val = value->valueint;
        printf_debug("mgc_gain_global=>value is:%d\n",value->valueint);
    } 
    /* mgc_gain_freq */
    cJSON *mgc_gain_freq=NULL;
    mgc_gain_freq = cJSON_GetObjectItem(calibration, "mgc_gain_freq");
    if(mgc_gain_freq!=NULL){
        printf_debug("mgc_gain_freq:\n");
        for(int i = 0; i < cJSON_GetArraySize(mgc_gain_freq); i++){
            printfd("index:%d ", i);
            node = cJSON_GetArrayItem(mgc_gain_freq, i);            
            value = cJSON_GetObjectItem(node, "start_freq");
            if(cJSON_IsNumber(value)){
               // printfd("start_freq:%d, ", value->valueint);
                config->cal_level.mgc.start_freq_khz[i]=value->valueint;
                 printfd("start_freq:%u, ", config->cal_level.mgc.start_freq_khz[i]);
            }
            value = cJSON_GetObjectItem(node, "end_freq");
            if(cJSON_IsNumber(value)){
              //  printfd("end_freq:%d, ", value->valueint);
                config->cal_level.mgc.end_freq_khz[i]=value->valueint;
                 printfd("end_freq:%u, ", config->cal_level.mgc.end_freq_khz[i]);
            } 
            value = cJSON_GetObjectItem(node, "value");
             if(cJSON_IsNumber(value)){
               //printfd("value:%d, ", value->valueint);
                config->cal_level.mgc.gain_val[i]=value->valueint;
                printfd("value:%d, ", config->cal_level.mgc.gain_val[i]);
            } 
             printfd("\n");
        }
    }

/* spectrum_parm */
    cJSON *spectrum_parm = NULL;
    spectrum_parm = cJSON_GetObjectItem(root, "spectrum_parm");
    if(spectrum_parm == NULL){
        printf_warn("not found json node[%s]\n","spectrum_parm");
        return -1;
    }
    /* side_bandrate */
    cJSON *side_bandrate = NULL;
    side_bandrate = cJSON_GetObjectItem(spectrum_parm, "side_bandrate");
    if(side_bandrate!=NULL){
        printf_debug("side_bandrate:\n");
        for(int i = 0; i < cJSON_GetArraySize(side_bandrate); i++){
            printfd("index:%d ", i);
            node = cJSON_GetArrayItem(side_bandrate, i);            
            value = cJSON_GetObjectItem(node, "rate");
            if(cJSON_IsNumber(value)){
               // printfd("rate:%d, ", value->valueint);
                config->ctrl_para.scan_bw.sideband_rate[i]=value->valuedouble;
                printfd("rate:%f, ", config->ctrl_para.scan_bw.sideband_rate[i]);
            }
            value = cJSON_GetObjectItem(node, "bandwidth");
            if(cJSON_IsNumber(value)){
               // printfd("bandwidth:%d, ", config->ctrl_para.scan_bw.bindwidth_hz[i]);
                config->ctrl_para.scan_bw.bindwidth_hz[i]=value->valueint;
                printfd("bandwidth:%u, ", config->ctrl_para.scan_bw.bindwidth_hz[i]);
            } 
             printfd("\n");
        }
    }
    /* if_parm */
    cJSON *if_parm = NULL;
    if_parm = cJSON_GetObjectItem(spectrum_parm, "if_parm");
    if(if_parm!=NULL){
        printf_debug("if_parm:\n");
        for(int i = 0; i < cJSON_GetArraySize(if_parm); i++){
            printfd("index:%d ", i);
            node = cJSON_GetArrayItem(if_parm, i);            
            value = cJSON_GetObjectItem(node, "channel");
            if(cJSON_IsNumber(value)){
               // printfd("channel:%d, ", value->valueint);
                config->multi_freq_point_param[i].points[0].index=value->valueint;
                 printfd("channel:%d, ",config->multi_freq_point_param[i].points[0].index);
            }
            value = cJSON_GetObjectItem(node, "middle_freq");
            if(cJSON_IsNumber(value)){
               // printfd("middle_freq:%d, ", value->valueint);
                config->multi_freq_point_param[i].points[0].center_freq=value->valuedouble;
                printfd("middle_freq:%d, ",config->multi_freq_point_param[i].points[0].center_freq);
            } 
            value = cJSON_GetObjectItem(node, "bandwith");
            if(cJSON_IsNumber(value)){
               // printfd("bandwidth:%d, ", value->valueint);
                config->multi_freq_point_param[i].points[0].d_bandwith=value->valueint;
                 printfd("bandwidth:%d, ", config->multi_freq_point_param[i].points[0].d_bandwith);
            } 
            value = cJSON_GetObjectItem(node, "mute_switch");
            if(cJSON_IsNumber(value)){
                config->multi_freq_point_param[i].points[0].noise_en=value->valueint;
                printfd("noise_en[mute_switch]:%d, ", config->multi_freq_point_param[i].points[0].noise_en);
            } 
            value = cJSON_GetObjectItem(node, "dec_method");
            if(cJSON_IsNumber(value)){
                config->multi_freq_point_param[i].points[0].d_method=value->valueint;
                printfd("dec_method:%d, ", config->multi_freq_point_param[i].points[0].d_method);
            }
            value = cJSON_GetObjectItem(node, "dec_bandwith");
            if(cJSON_IsNumber(value)){
                config->multi_freq_point_param[i].points[0].d_bandwith=value->valueint;
                printfd("dec_bandwith:%u, ", config->multi_freq_point_param[i].points[0].d_bandwith);
            } 
            value = cJSON_GetObjectItem(node, "audio_volume");
            if(cJSON_IsNumber(value)){
                config->multi_freq_point_param[i].points[0].audio_volume=value->valueint;
                printfd("audio_volume:%d, ", config->multi_freq_point_param[i].points[0].audio_volume);
            } 

            printfd("\n");
        }
    }
    
    /* rf_parm */
    #if 0
    cJSON *rf_parm = NULL;
    rf_parm = cJSON_GetObjectItemCaseSensitive(spectrum_parm, "rf_parm");
    printf_debug("rf_parm:\n");
    cJSON_ArrayForEach(node, rf_parm){
        value = cJSON_GetObjectItemCaseSensitive(node, "channel");
        if(cJSON_IsNumber(value)){
           // printfd(" channel:%d, ", value->valueint);
            config->rf_para[0].cid=value->valueint;
            printfd(" channel:%d, ", config->rf_para[0].cid);
        }
        value = cJSON_GetObjectItemCaseSensitive(node, "attenuation");
        if(cJSON_IsNumber(value)){
           // printfd(" attenuation:%d,", value->valueint);
            config->rf_para[0].attenuation=value->valueint;
             printfd(" attenuation:%d,", config->rf_para[0].attenuation);
        }
        value = cJSON_GetObjectItemCaseSensitive(node, "mode_code");
        if(cJSON_IsNumber(value)){
            //printfd(" mode_code:%d,", value->valueint);
            config->rf_para[0].rf_mode_code=value->valueint;
             printfd(" mode_code:%d,", config->rf_para[0].rf_mode_code);
        }
        value = cJSON_GetObjectItemCaseSensitive(node, "gain_mode");
        if(cJSON_IsNumber(value)){
           // printfd(" gain_mode:%s,", value->valuestring);
            config->rf_para[0].gain_ctrl_method=value->valueint;
            printfd(" gain_mode:%d,", config->rf_para[0].gain_ctrl_method);
        }
        value = cJSON_GetObjectItemCaseSensitive(node, "agc_ctrl_time");
        if(cJSON_IsNumber(value)){
           // printfd(" agc_ctrl_time:%d,", value->valueint);
            config->rf_para[0].agc_ctrl_time=value->valueint;
             printfd(" agc_ctrl_time:%d,", config->rf_para[0].agc_ctrl_time);
        }
        value = cJSON_GetObjectItemCaseSensitive(node, "agc_output_amp_dbm");
        if(cJSON_IsNumber(value)){
          //  printfd(" agc_output_amp_dbm:%d", value->valueint);
            config->rf_para[0].agc_mid_freq_out_level=value->valueint;
            printfd(" agc_output_amp_dbm:%d", config->rf_para[0].agc_mid_freq_out_level);
        }
        printfd("\n");
    }
    #endif

   
    cJSON *rf_parm = NULL;
    rf_parm = cJSON_GetObjectItem(spectrum_parm, "rf_parm");
    if(rf_parm != NULL){
        printf_debug("rf_parm:\n");
        for(int i = 0; i < cJSON_GetArraySize(rf_parm); i++){
            printfd("index:%d ", i);
            node = cJSON_GetArrayItem(rf_parm, i);
            value = cJSON_GetObjectItem(node, "channel");
            if(cJSON_IsNumber(value)){
                // printfd(" channel:%d, ", value->valueint);
                 config->rf_para[i].cid=value->valueint;
                 printfd(" channel:%d, ", config->rf_para[i].cid);
            }
            value = cJSON_GetObjectItem(node, "attenuation");
            if(cJSON_IsNumber(value)){
                // printfd(" attenuation:%d,", value->valueint);
                 config->rf_para[i].attenuation=value->valueint;
                 printfd(" attenuation:%d,", config->rf_para[i].attenuation);                   
            }
            value = cJSON_GetObjectItem(node, "mode_code");
            if(cJSON_IsNumber(value)){
                 //printfd(" mode_code:%d,", value->valueint);
                 config->rf_para[i].rf_mode_code=value->valueint;
                 printfd(" mode_code:%d,", config->rf_para[i].rf_mode_code);                   
            }
            value = cJSON_GetObjectItem(node, "gain_mode");
            if(cJSON_IsNumber(value)){
                // printfd(" gain_mode:%s,", value->valuestring);
                 config->rf_para[i].gain_ctrl_method=value->valueint;
                 printfd(" gain_mode:%d,", config->rf_para[i].gain_ctrl_method);                   
            }
            value = cJSON_GetObjectItem(node, "agc_ctrl_time");
            if(cJSON_IsNumber(value)){
                // printfd(" agc_ctrl_time:%d,", value->valueint);
                 config->rf_para[i].agc_ctrl_time=value->valueint;
                 printfd(" agc_ctrl_time:%d,", config->rf_para[i].agc_ctrl_time);                   
            }
            value = cJSON_GetObjectItem(node, "agc_ref_val_0dbm");
            if(cJSON_IsNumber(value)){
                config->ctrl_para.agc_ref_val_0dbm=value->valueint;
                printf_debug("agc_ref_val_0dbm:%d, \n",config->ctrl_para.agc_ref_val_0dbm);
            } 
            value = cJSON_GetObjectItem(node, "agc_output_amp_dbm");
            if(cJSON_IsNumber(value)){
                config->rf_para[i].agc_mid_freq_out_level=value->valueint;
                printfd(" agc_output_amp_dbm:%d", config->rf_para[i].agc_mid_freq_out_level);
            }
            value = cJSON_GetObjectItem(node, "middle_freq");
            if(cJSON_IsNumber(value)){
                config->rf_para[i].mid_freq=value->valuedouble;
                printfd(" middle_freq:%llu", config->rf_para[i].mid_freq);
            }
            value = cJSON_GetObjectItem(node, "mid_bw");
            if(cJSON_IsNumber(value)){
                config->rf_para[i].mid_bw=value->valueint;
                printfd(" mid_bw:%u", config->rf_para[i].mid_bw);
            }
            value = cJSON_GetObjectItem(node, "mgc_gain");
            if(cJSON_IsNumber(value)){
                config->rf_para[i].mgc_gain_value=value->valueint;
                printfd(" mgc_gain:%d", config->rf_para[i].mgc_gain_value);
            }
            printfd("\n");
        }
        
    }
    return 0;
}

int json_read_config_file(const void *config)
{
    char *file;
    s_config *conf = (struct poal_config *)config;
    
    file = conf->configfile;
    if(file == NULL || config == NULL)
        exit(1);
    
    root_json = json_read_file(file, root_json);
    if(root_json == NULL){
        printf_err("json read error\n", file);
        exit(1);
    }
    //json_print(root_json, 1);
    if(json_parse_config_param(root_json, &conf->oal_config) == -1){
        exit(1);
    }
    return 0;
}
int json_write_config_file(void *config)
{
    int ret = -1;
    char *file;
    s_config *conf = (struct s_config *)config;
    file = conf->configfile;
    if(file == NULL || config == NULL || root_json == NULL)
        return -1;
     json_write_config_param(root_json, &conf->oal_config);
    //json_print(root_json, 1);
    ret = json_write_file(file, root_json);

    return ret;
}



