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
   // printf("%s\n", printed_json);
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
        printf_err("Open file error!\n");
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
    fread(JSON_content, 1, len, fp);

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



#if 0
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
#endif


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

static int json_write_config_param(cJSON* root, s_config *sconfig)
{

    cJSON* node;
    cJSON* group;
    struct poal_config *config = &sconfig->oal_config;

    cJSON *network = NULL;
    struct sockaddr_in saddr;

    if((sconfig->version != NULL) && !strcmp(sconfig->version, "1.0")){
        json_write_double_param(NULL, "network", "port", config->network[0].port);
        json_write_double_param(NULL, "network_10g", "port", config->network[0].port);
    } else{
        cJSON* ifname, *port, *value,*portnode;
        network = cJSON_GetObjectItem(root, "network");
        for(int i = 0; i < cJSON_GetArraySize(network); i++){
            node = cJSON_GetArrayItem(network, i);
            ifname = cJSON_GetObjectItem(node, "ifname");
            if(cJSON_IsString(ifname)){
               int index = config_get_if_nametoindex(ifname->valuestring);
                value = cJSON_GetObjectItem(node, "port");
                if(value != NULL){
                    port = cJSON_GetObjectItem(value, "command");
                    if(cJSON_IsNumber(port)){
                            port->valuedouble = config->network[index].port;
                    }
                    port = cJSON_GetObjectItem(value, "data");
                    if(port != NULL){
                        for(int j = 0; j < cJSON_GetArraySize(port); j++){
                            portnode = cJSON_GetArrayItem(port, j);
                            if(cJSON_IsNumber(portnode)){
                                    portnode->valuedouble = config->network[index].data_port[j];
                            }
                        }
                    }
                }
            }
        }
    }

    cJSON *control_parm = NULL;
    control_parm = cJSON_GetObjectItem(root, "control_parm");
    if(control_parm == NULL){
        printf_warn("not found json node[%s]\n","network");
        return -1;
    }
    /* link switch */
    cJSON *sid;
    cJSON *linkSwitch = NULL;
    linkSwitch = cJSON_GetObjectItem(control_parm, "linkSwitch");
    if(linkSwitch != NULL){
        printf_note("save linkSwitch:\n");
        for(int i = 0; i < cJSON_GetArraySize(linkSwitch); i++){
            printfn("index:%d ", i);
            node = cJSON_GetArrayItem(linkSwitch, i);
            sid = cJSON_GetObjectItem(node, "slotId");
            if(sid->valueint >= MAX_FPGA_CARD_SLOT_NUM)
                continue;
            
            printfn("node: %d, ", sid->valueint);
            if(cJSON_IsNumber(sid))
            {
                if(config->ctrl_para.link_switch[sid->valueint].slod_id == sid->valueint)
                    cJSON_GetObjectItem(node, "on")->valuedouble = (double)config->ctrl_para.link_switch[sid->valueint].onoff;
            }
             printfn("\n");
        }
    }
    
#if 0
    cJSON* if_parm, *rf_parm, *cjch;
    int ch;
    /*if_parm*/
    group = cJSON_GetObjectItem(root, "spectrum_parm");
    if(group == NULL)
        return -1;
    
    if_parm = cJSON_GetObjectItem(group, "if_parm");
    for(int i = 0; i < cJSON_GetArraySize(if_parm); i++){
        node = cJSON_GetArrayItem(if_parm, i);
        cjch = cJSON_GetObjectItem(node, "channel");
        ch = cjch->valueint;
        if(ch >= MAX_RF_NUM)
            break;
        json_write_array_int_param("spectrum_parm", "if_parm", i, "middle_freq",config->channel[ch].multi_freq_point_param.points[0].center_freq);
        json_write_array_int_param("spectrum_parm", "if_parm", i, "bandwith",config->channel[ch].multi_freq_point_param.points[0].bandwidth);
        json_write_array_int_param("spectrum_parm", "if_parm", i, "dec_bandwith",config->channel[ch].multi_freq_point_param.points[0].d_bandwith);
        json_write_array_int_param("spectrum_parm", "if_parm", i, "dec_method",config->channel[ch].multi_freq_point_param.points[0].d_method);
        json_write_array_int_param("spectrum_parm", "if_parm", i, "mute_switch",config->channel[ch].multi_freq_point_param.points[0].noise_en);
        json_write_array_int_param("spectrum_parm", "if_parm", i, "audio_volume",config->channel[ch].multi_freq_point_param.points[0].audio_volume);
    }

    /*rf_parm*/
    rf_parm = cJSON_GetObjectItem(group, "rf_parm");
    for(int i = 0; i < cJSON_GetArraySize(rf_parm); i++){
        node = cJSON_GetArrayItem(rf_parm, i);
        cjch = cJSON_GetObjectItem(node, "channel");
        ch = cjch->valueint;
        if(ch >= MAX_RF_NUM)
            break;
        json_write_array_int_param("spectrum_parm", "rf_parm", i, "attenuation",config->channel[ch].rf_para.attenuation);
        json_write_array_int_param("spectrum_parm", "rf_parm", i,"mode_code",config->channel[ch].rf_para.rf_mode_code);
        json_write_array_int_param("spectrum_parm", "rf_parm", i, "gain_mode",config->channel[ch].rf_para.gain_ctrl_method);
        json_write_array_int_param("spectrum_parm", "rf_parm", i,"agc_ctrl_time",config->channel[ch].rf_para.agc_ctrl_time);
        json_write_array_int_param("spectrum_parm", "rf_parm", i, "agc_output_amp_dbm",config->channel[ch].rf_para.agc_mid_freq_out_level);
        json_write_array_int_param("spectrum_parm", "rf_parm", i, "mgc_gain",config->channel[ch].rf_para.mgc_gain_value);
        json_write_array_double_param("spectrum_parm", "rf_parm", i, "middle_freq",config->channel[ch].rf_para.mid_freq);
        json_write_array_int_param("spectrum_parm", "rf_parm", i, "mid_bw",config->channel[ch].rf_para.mid_bw);
    }
#endif
#if 0
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
#endif

    return 0;
}


int8_t split_string_2_number(char *str, int32_t *n1, int32_t *n2, char split_c)
{
    int rc;
    if(str == NULL || strlen(str) == 0)
        return -1;
    
    rc = sscanf(str, "%d~%d", n1, n2);
    if(rc <= 0)
        return -1;
    
    return 0;
}


static int json_parse_config_param(const cJSON* root, s_config *sconfig)
{
    cJSON *value = NULL;
    cJSON *node = NULL;
    struct poal_config *config = &sconfig->oal_config;
/* network */
    //struct sockaddr_in saddr;
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
    }
    value = cJSON_GetObjectItem(status_parm, "device_sn");
    if(value!= NULL && cJSON_IsString(value)){
        config->status_para.device_sn = value->valuestring;
        printf_debug("config->status_para.device_sn=>value is:%s\n",config->status_para.device_sn);
    }else{
        config->status_para.device_sn = NULL;
    }

    cJSON *version = cJSON_GetObjectItem(root, "version");
    if(version!= NULL && cJSON_IsString(version)){
        cJSON *port;
        sconfig->version = strdup(version->valuestring);
        printf_note("json config version: %s\n", sconfig->version);
        network = cJSON_GetObjectItem(root, "network");
        if(!strcmp("1.0", version->valuestring)){
            int index = 0;
            if(network != NULL){
                port = cJSON_GetObjectItem(network, "port");
                index ++;
                if(port!= NULL && cJSON_IsNumber(port)){
                    config->network[index].port = port->valueint;
                    config->network[index].data_port[0] = port->valueint+1;
                    printf_debug("port=>value is:%d\n",config->network[index].port);
                }
            }
            network = cJSON_GetObjectItem(root, "network_10g");
            index ++;
            if(network != NULL){
                port = cJSON_GetObjectItem(network, "port");
                if(port!= NULL && cJSON_IsNumber(port)){
                    config->network[index].port = port->valueint;
                    config->network[index].data_port[0] = port->valueint+1;
                    printf_debug("10g port=>value is:%d\n",config->network[index].port);
                }
            }
        }else{
            if(network != NULL){
                for(int i = 0; i < cJSON_GetArraySize(network); i++){
                    node = cJSON_GetArrayItem(network, i);
                    value = cJSON_GetObjectItem(node, "ifname");
                    if(i >= ARRAY_SIZE(config->network)){
                        break;
                    }
                    if(cJSON_IsString(value)){
                        config->network[i].ifname = strdup(value->valuestring);
                    }
                    value = cJSON_GetObjectItem(node, "port");
                    if(value != NULL){
                        port = cJSON_GetObjectItem(value, "command");
                        if(cJSON_IsNumber(port)){
                            config->network[i].port = port->valueint;
                            printf_debug("%s cmd port:%d\n",config->network[i].ifname, config->network[i].port);
                        }
                        port = cJSON_GetObjectItem(value, "data");
                        if(port != NULL){
                            printf_debug("%s data port:\n", config->network[i].ifname);
                            cJSON *port_details = NULL;
                           for(int j = 0; j < cJSON_GetArraySize(port); j++)
                           {
                               port_details = cJSON_GetArrayItem(port, j);
                               if(cJSON_IsNumber(port_details)){
                                  config->network[i].data_port[j] = port_details->valueint;
                                  printf_debug("config->network[%d].data_port[%d]=%d\n",i,j,config->network[i].data_port[j]);
                               }
                           }
                        }
                    }
                }
            }
        }
    }
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
    value = cJSON_GetObjectItem(control_parm, "disk_full_alert_threshold_MB");
    if(value!= NULL && cJSON_IsNumber(value)){
        config->status_para.diskInfo.alert.alert_threshold_byte = (uint64_t)value->valueint * 1024 * 1024;
        printf_debug("alert_threshold_byte: %dMbyte, %" PRIu64"\n",value->valueint, config->status_para.diskInfo.alert.alert_threshold_byte);
    }
    value = cJSON_GetObjectItem(control_parm, "disk_split_file_threshold_MB");
    if(value!= NULL && cJSON_IsNumber(value)){
        config->status_para.diskInfo.alert.split_file_threshold_byte = (uint64_t)value->valueint * 1024 * 1024;
        printf_debug("split_file_threshold_mb:%dMbyte, %" PRIu64"\n",value->valueint, config->status_para.diskInfo.alert.split_file_threshold_byte);
    }
    value = cJSON_GetObjectItem(control_parm, "disk_file_notifier_timeout_ms");
    if(value!= NULL && cJSON_IsNumber(value)){
        config->ctrl_para.disk_file_notifier_timeout_ms = value->valueint;
        printf_debug("disk_file_notifier_timeout_ms:%dms, %ums, \n",value->valueint, config->ctrl_para.disk_file_notifier_timeout_ms);
    }
    /* link switch */
    cJSON *linkSwitch = NULL;
    linkSwitch = cJSON_GetObjectItem(control_parm, "linkSwitch");
    if(linkSwitch != NULL){
        printf_note("linkSwitch:\n");
        for(int i = 0; i < cJSON_GetArraySize(linkSwitch); i++){
            printfn("index:%d ", i);
            node = cJSON_GetArrayItem(linkSwitch, i);
            value = cJSON_GetObjectItem(node, "slotId");
            if(value->valueint >= MAX_FPGA_CARD_SLOT_NUM)
                continue;
            printfn("node: %d, ", value->valueint);
            if(cJSON_IsNumber(value))
            {
                config->ctrl_para.link_switch[value->valueint].slod_id = value->valueint;
                printfn("slotId:%d, ", config->ctrl_para.link_switch[value->valueint].slod_id);
            }
            
            value = cJSON_GetObjectItem(node, "on");
            if(cJSON_IsNumber(value)){
                config->ctrl_para.link_switch[value->valueint].onoff = value->valueint;
                printfn("onoff:%d, ", config->ctrl_para.link_switch[value->valueint].onoff);
            }
             printfn("\n");
        }
    }

    
    
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
    value = cJSON_GetObjectItem(calibration, "gain_calibration");
    if(value!= NULL && cJSON_IsString(value)){
        if(!strcmp(value->valuestring, "true"))
            config->cal_level.specturm.gain_calibration_onoff = true;
        else
            config->cal_level.specturm.gain_calibration_onoff = false;
        printf_debug("gain_calibration_onoff=>value is:%d\n",config->cal_level.specturm.gain_calibration_onoff);
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

    /* dc offset mshift  */
    value = cJSON_GetObjectItem(calibration, "dc_offset_mshift_onoff");
    if(value!= NULL && cJSON_IsNumber(value)){
        config->cal_level.dc_offset.is_open = (value->valueint == 0 ? false : true);
        printf_debug("calibration dc offset is:%s\n", config->cal_level.dc_offset.is_open == true ? "open" : "close");
    } 

    if(config->cal_level.dc_offset.is_open){
        value = cJSON_GetObjectItem(calibration, "dc_offset_mshift_global");
        if(value!= NULL && cJSON_IsNumber(value)){
            config->cal_level.dc_offset.global_mshift = value->valueint;
            printf_debug("calibration dc offset global mshift is:%d\n", config->cal_level.dc_offset.global_mshift);
        } 
        
        cJSON *dc_offset_mshift=NULL;
        dc_offset_mshift = cJSON_GetObjectItem(calibration, "dc_offset_mshift");
        if(dc_offset_mshift!=NULL){
            printf_debug("dc offset:\n");
            for(int i = 0; i < cJSON_GetArraySize(dc_offset_mshift); i++){
                printfd("index:%d ", i);
                node = cJSON_GetArrayItem(dc_offset_mshift, i);            
                value = cJSON_GetObjectItem(node, "start_freq");
                if(cJSON_IsNumber(value)){
                    config->cal_level.dc_offset.start_freq_khz[i]=value->valueint;
                     printfd("start_freq:%ukhz, ", config->cal_level.dc_offset.start_freq_khz[i]);
                }
                value = cJSON_GetObjectItem(node, "end_freq");
                if(cJSON_IsNumber(value)){
                    config->cal_level.dc_offset.end_freq_khz[i]=value->valueint;
                     printfd("end_freq:%ukhz, ", config->cal_level.dc_offset.end_freq_khz[i]);
                } 
                value = cJSON_GetObjectItem(node, "value");
                 if(cJSON_IsNumber(value)){
                    config->cal_level.dc_offset.mshift[i]=value->valueint;
                    printfd("value:%d", config->cal_level.dc_offset.mshift[i]);
                } 
                printfd("\n");
            }
        }
    }

    value = cJSON_GetObjectItem(calibration, "low_distortion_power_agc");
    if(value!= NULL && cJSON_IsNumber(value)){
        config->cal_level.specturm.low_distortion_power_level = value->valueint;
        printf_debug("low_distortion_power_agc=>value is:%d\n",config->cal_level.specturm.low_distortion_power_level);
    }
    value = cJSON_GetObjectItem(calibration, "low_noise_power_agc");
    if(value!= NULL && cJSON_IsNumber(value)){
        config->cal_level.specturm.low_noise_power_level = value->valueint;
        printf_debug("low_noise_power_agc=>value is:%d  %d\n",config->cal_level.specturm.low_noise_power_level,value->valueint);
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
            value = cJSON_GetObjectItem(node, "fixed");
            if(cJSON_IsNumber(value)){
                config->ctrl_para.scan_bw.fixed_bindwidth_flag[i]=(value->valueint == 0 ? false : true);
                printfd("[%d]fixed:%d, ", i, config->ctrl_para.scan_bw.fixed_bindwidth_flag[i]);
            }
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
                config->channel[i].multi_freq_point_param.points[0].index=value->valueint;
                 printfd("channel:%d, ",config->channel[i].multi_freq_point_param.points[0].index);
            }
            value = cJSON_GetObjectItem(node, "middle_freq");
            if(cJSON_IsNumber(value)){
               // printfd("middle_freq:%d, ", value->valueint);
                config->channel[i].multi_freq_point_param.points[0].center_freq=value->valuedouble;
                printfd("middle_freq:%"PRIu64 ", ",config->channel[i].multi_freq_point_param.points[0].center_freq);
            } 
            value = cJSON_GetObjectItem(node, "bandwith");
            if(cJSON_IsNumber(value)){
               // printfd("bandwidth:%d, ", value->valueint);
                config->channel[i].multi_freq_point_param.points[0].d_bandwith=value->valueint;
                 printfd("bandwidth:%d, ", config->channel[i].multi_freq_point_param.points[0].d_bandwith);
            } 
            value = cJSON_GetObjectItem(node, "mute_switch");
            if(cJSON_IsNumber(value)){
                config->channel[i].multi_freq_point_param.points[0].noise_en=value->valueint;
                printfd("noise_en[mute_switch]:%d, ", config->channel[i].multi_freq_point_param.points[0].noise_en);
            } 
            value = cJSON_GetObjectItem(node, "dec_method");
            if(cJSON_IsNumber(value)){
                config->channel[i].multi_freq_point_param.points[0].d_method=value->valueint;
                printfd("dec_method:%d, ", config->channel[i].multi_freq_point_param.points[0].d_method);
            }
            value = cJSON_GetObjectItem(node, "dec_bandwith");
            if(cJSON_IsNumber(value)){
                config->channel[i].multi_freq_point_param.points[0].d_bandwith=value->valueint;
                printfd("dec_bandwith:%u, ", config->channel[i].multi_freq_point_param.points[0].d_bandwith);
            } 
            value = cJSON_GetObjectItem(node, "audio_volume");
            if(cJSON_IsNumber(value)){
                for (int j = 0; j < MAX_SIG_CHANNLE; j++) {
                    config->channel[i].multi_freq_point_param.points[j].audio_volume=value->valueint;
                }
                printfd("audio_volume:%d, ", config->channel[i].multi_freq_point_param.points[0].audio_volume);
            } 

            printfd("\n");
        }
    }
    cJSON *rf_parm = NULL;
    int ch;
    rf_parm = cJSON_GetObjectItem(spectrum_parm, "rf_parm");
    if(rf_parm != NULL){
        printf_debug("rf_parm:\n");
        for(int i = 0; i < cJSON_GetArraySize(rf_parm); i++){
            printfd("\n\n  index:%d ", i);
            node = cJSON_GetArrayItem(rf_parm, i);
            value = cJSON_GetObjectItem(node, "channel");
            if(cJSON_IsNumber(value)){
                 ch = value->valueint;
                 if(ch >= MAX_RF_NUM)
                    break;
                 config->channel[ch].rf_para.cid=value->valueint;
                 printfd("ch:%d ", ch);
                 printfd(" channel:%d, ", config->channel[ch].rf_para.cid);
            }
            value = cJSON_GetObjectItem(node, "attenuation");
            if(cJSON_IsNumber(value)){
                // printfd(" attenuation:%d,", value->valueint);
                 config->channel[ch].rf_para.attenuation=value->valueint;
                 printfd(" attenuation:%d,", config->channel[ch].rf_para.attenuation);                   
            }
            value = cJSON_GetObjectItem(node, "mode_code");
            if(cJSON_IsNumber(value)){
                 //printfd(" mode_code:%d,", value->valueint);
                 config->channel[ch].rf_para.rf_mode_code=value->valueint;
                 printfd(" mode_code:%d,", config->channel[ch].rf_para.rf_mode_code);                   
            }
            value = cJSON_GetObjectItem(node, "gain_mode");
            if(cJSON_IsNumber(value)){
                // printfd(" gain_mode:%s,", value->valuestring);
                 config->channel[ch].rf_para.gain_ctrl_method=value->valueint;
                 printfd(" gain_mode:%d,", config->channel[ch].rf_para.gain_ctrl_method);                   
            }
            value = cJSON_GetObjectItem(node, "agc_ctrl_time");
            if(cJSON_IsNumber(value)){
                // printfd(" agc_ctrl_time:%d,", value->valueint);
                 config->channel[ch].rf_para.agc_ctrl_time=value->valueint;
                 printfd(" agc_ctrl_time:%d,", config->channel[ch].rf_para.agc_ctrl_time);                   
            }
            value = cJSON_GetObjectItem(node, "agc_ref_val_0dbm");
            if(cJSON_IsNumber(value)){
                config->channel[ch].rf_para.agc_ref_val_0dbm = value->valueint;
                printf_debug("agc_ref_val_0dbm:%d, \n",config->channel[ch].rf_para.agc_ref_val_0dbm);
            } 
            value = cJSON_GetObjectItem(node, "subch_ref_val_0dbm");
            if(cJSON_IsNumber(value)){
                config->channel[ch].rf_para.subch_ref_val_0dbm = value->valueint;
                printf_debug("subch_ref_val_0dbm:%d, \n", config->channel[ch].rf_para.subch_ref_val_0dbm);
            } 
            value = cJSON_GetObjectItem(node, "agc_output_amp_dbm");
            if(cJSON_IsNumber(value)){
                config->channel[ch].rf_para.agc_mid_freq_out_level=value->valueint;
                printfd(" agc_output_amp_dbm:%d", config->channel[ch].rf_para.agc_mid_freq_out_level);
            }
            value = cJSON_GetObjectItem(node, "middle_freq");
            if(cJSON_IsNumber(value)){
                config->channel[ch].rf_para.mid_freq=value->valuedouble;
               printfd(" middle_freq:%" PRIu64" ", config->channel[ch].rf_para.mid_freq);
            }
            value = cJSON_GetObjectItem(node, "mid_bw");
            if(cJSON_IsNumber(value)){
                config->channel[ch].rf_para.mid_bw=value->valueint;
                printfd(" mid_bw:%u", config->channel[ch].rf_para.mid_bw);
            }
            value = cJSON_GetObjectItem(node, "mgc_gain");
            if(cJSON_IsNumber(value)){
                config->channel[ch].rf_para.mgc_gain_value=value->valueint;
                printfd(" mgc_gain:%d", config->channel[ch].rf_para.mgc_gain_value);
            }
            printfd("\n");
 
            cJSON *rf_mode_parm = NULL;
            cJSON *mode_node = NULL;
            cJSON *mode_value = NULL;
            int32_t start, end;
            rf_mode_parm = cJSON_GetObjectItem(node, "rf_mode_param");
            if(rf_mode_parm != NULL){
                for(int j = 0; j < cJSON_GetArraySize(rf_mode_parm); j++){
                    if(j >= RF_MODE_NUMBER){
                        break;
                    }
                    mode_node = cJSON_GetArrayItem(rf_mode_parm, j);
                    mode_value = cJSON_GetObjectItem(mode_node, "mode");
                    printfd("\n ch = %d, j=%d, mode=%s\n", ch, j, mode_value->valuestring);
                    if(mode_value!= NULL && cJSON_IsString(mode_value)){
                         if(!strcmp(mode_value->valuestring, "low_distortion")){
                            config->channel[ch].rf_para.rf_mode.mode[j] = POAL_LOW_DISTORTION;
                            printfd("[%d] low_distortion: ", j);
                         }else if(!strcmp(mode_value->valuestring, "normal")){
                            config->channel[ch].rf_para.rf_mode.mode[j] = POAL_NORMAL;
                            printfd("[%d] normal: ", j);
                         }else if(!strcmp(mode_value->valuestring, "low_noise")){
                            config->channel[ch].rf_para.rf_mode.mode[j] = POAL_LOW_NOISE;
                            printfd("[%d] low_noise: ", j);
                         }
                    }
                    
                    value = cJSON_GetObjectItem(mode_node, "magification");
                    if(value != NULL &&cJSON_IsNumber(value)){
                        config->channel[ch].rf_para.rf_mode.mag[j] = value->valueint;
                        printfd("magification: %d,", config->channel[ch].rf_para.rf_mode.mag[j]);
                    }
                    value = cJSON_GetObjectItem(mode_node, "rf_attenuation_range");
                    if(value != NULL &&cJSON_IsString(value)){
                        if(split_string_2_number(value->valuestring, &start, &end, '~') == 0){
                            config->channel[ch].rf_para.rf_mode.rf_attenuation[j].start = start;
                            config->channel[ch].rf_para.rf_mode.rf_attenuation[j].end = end;
                            printfd("rf_attenuation_range: %d~%d,", start, end);
                        }
                    }
                    value = cJSON_GetObjectItem(mode_node, "mgc_attenuation_range");
                    if(value != NULL &&cJSON_IsString(value)){
                        if(split_string_2_number(value->valuestring, &start, &end, '~') == 0){
                            config->channel[ch].rf_para.rf_mode.mgc_attenuation[j].start = start;
                            config->channel[ch].rf_para.rf_mode.mgc_attenuation[j].end = end;
                            printfd("mgc_attenuation: %d~%d,", start, end);
                        }
                    }
                    value = cJSON_GetObjectItem(mode_node, "detection_range");
                    if(value != NULL &&cJSON_IsString(value)){
                        if(split_string_2_number(value->valuestring, &start, &end, '~') == 0){
                            config->channel[ch].rf_para.rf_mode.detection[j].start = start;
                            config->channel[ch].rf_para.rf_mode.detection[j].end = end;
                            printfd("detection: %d~%d,", start, end);
                        }
                    }
                }
            }
        }
        printfd("\n");
    }
    return 0;
}

int json_read_config_file(const void *config)
{
    char *file;
    s_config *conf = (s_config*)config;
    
    file = conf->configfile;
    if(file == NULL || config == NULL)
        exit(1);
    
    root_json = json_read_file(file, root_json);
    if(root_json == NULL){
        printf_err("json read error:%s\n", file);
        exit(1);
    }
    //json_print(root_json, 1);
    if(json_parse_config_param(root_json, conf) == -1){
        exit(1);
    }
    return 0;
}
int json_write_config_file(void *config)
{
    int ret = -1;
    char *file;
    s_config *conf = (s_config *)config;
    file = conf->configfile;
    if(file == NULL || config == NULL || root_json == NULL)
        return -1;
     json_write_config_param(root_json, conf);
    //json_print(root_json, 1);
    ret = json_write_file(file, root_json);

    return ret;
}



