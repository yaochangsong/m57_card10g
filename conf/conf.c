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
*  Rev 1.0   06 July 2019   yaochangsong
*  Initial revision.
******************************************************************************/

#include "config.h"

static s_config config;

/**
 * Mutex for the configuration file, used by the related
 * functions. */
pthread_mutex_t config_mutex = PTHREAD_MUTEX_INITIALIZER;

/** Sets the default config parameters and initialises the configuration system */
void config_init(void)
{  
    printf_debug("config init\n");

    /* to test*/
    struct sockaddr_in saddr;
    uint8_t  mac[6];
    
    saddr.sin_addr.s_addr=inet_addr("192.168.0.105");
    config.oal_config.network.ipaddress = saddr.sin_addr.s_addr;
    saddr.sin_addr.s_addr=inet_addr("192.168.0.1");
    config.oal_config.network.gateway = saddr.sin_addr.s_addr;
    saddr.sin_addr.s_addr=inet_addr("255.255.255.0");
    config.oal_config.network.netmask = saddr.sin_addr.s_addr;
    config.oal_config.network.port = 1325;
    if(get_mac(mac, sizeof(mac)) != -1){
        memcpy(config.oal_config.network.mac, mac, sizeof(config.oal_config.network.mac));
    }
    printf_debug("mac:%x%x%x%x%x%x\n", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    printf_debug("config init\n");
    config.configfile = safe_strdup(DEFAULT_CONFIGFILE);
    config.daemon = -1;
    
    dao_read_create_config_file(config.configfile, &config);

}

/** Accessor for the current configuration
@return:  A pointer to the current config.  The pointer isn't opaque, but should be treated as READ-ONLY
 */
s_config *config_get_config(void)
{
    return &config;
}


/*int8_t config_parse_data(exec_cmd cmd, uint8_t type, void *data)
{
    printf_debug("start to config parse data\n");
    return 0;
}*/
int8_t config_parse_data(uint8_t classcode, uint8_t methodcode, char *data)
{
    printf_debug("start to config parse data\n");
    int16_t i=0;
    struct dao_mute_switch *muteswitch;/*静噪开关设置*/
    struct dao_noise_threshold *noisethreshold;/*静噪门限设置*/
    struct  dao_demodulation_way *demodulationway;/*解调方式参数*/
    struct dao_audio_sampling_rate *audio_samprate;/*音频采样率设置参数*/
    /*射频参数*/
    struct dao_RF_parameters *RFpara;
    struct  dao_RF_antenna_selection *RFantennaselect;
    struct  dao_Rf_output_attenuation *Rfoutputattenuation;
    struct dao_Rf_Input_attenuation *RfInputattenuation;
    struct  dao_Rf_bandwidth_setting *Rfbdsetting;
    /*工作模式*/
    struct  multi_freq_point_para_st *multifrepoint;
    struct dao_sub_channel_freq_para_st  *subchanelpara;
    struct multi_freq_fregment_para_st  *freqfergmentpara;
    /*网络参数*/
    struct network_st *netpara;
    /*控制参数*/
    struct dao_data_output_enable_control *outputenablecontr;
    struct dao_Local_remote_control *Localremotecontr;
    
    switch(classcode)
    {

         case CLASS_CODE_MID_FRQ:/*中频参数命令*/
            {
                 if(methodcode==B_CODE_MID_FRQ_DEC_METHOD){/*解调方式参数*/
                         demodulationway=(struct dao_demodulation_way *) dao_conf_parse_batch(classcode,methodcode,data);
                         struct multi_freq_point_para_st *point =  &config.oal_config.multi_freq_point_param[demodulationway->channel];
                 
                         config.oal_config.cid=demodulationway->channel;
                         point->cid=demodulationway->channel;
                         for(i=0;i<point->freq_point_cnt;i++)
                         {
                             point->points[i].d_method=demodulationway->decMethodId;
                             
                             printf_debug("config.oal_config.cid=%d\n",config.oal_config.cid);
                             printf_debug("config.oal_config.cid=%d\n",config.oal_config.multi_freq_fregment_para[demodulationway->channel].cid);
                             printf_debug("config.oal_config.d_method=%d\n",config.oal_config.multi_freq_point_param[demodulationway->channel].points[i].d_method);
                         }
                         break;

                         
                 }else if(methodcode==B_CODE_MID_FRQ_MUTE_SW){/*静噪开关设置*/
                         muteswitch=(struct dao_mute_switch *) dao_conf_parse_batch(classcode,methodcode,data);
                         struct multi_freq_point_para_st *multpoint =&config.oal_config.multi_freq_point_param[muteswitch->channel];
                         struct sub_channel_freq_para_st *subchannelpoint =&config.oal_config.sub_channel_para[muteswitch->channel];

                         if(muteswitch->subChannel==(-1))
                         {
                                 multpoint->cid=muteswitch->channel;
                                 for(i=0;i<multpoint->freq_point_cnt;i++)
                                 {
                                     
                                     multpoint->points[i].noise_en=muteswitch->muteSwitch;
                                     //multpoint->points[i].index=i;
                                     printf_debug("==config.oal_config.cid=%d\n",config.oal_config.cid);
                                     printf_debug("==config.oal_config.index=%d\n",config.oal_config.multi_freq_point_param[muteswitch->channel].points[i].index);
                                     printf_debug("==config.oal_config.noise_en=%d\n",config.oal_config.multi_freq_point_param[muteswitch->channel].points[i].noise_en);
                                 }
                                 
                         }else if(muteswitch->subChannel>=0){
                                subchannelpoint->cid=muteswitch->channel;
                                for(i=0;i<subchannelpoint->sub_channel_num;i++)
                                {
                                    //subchannelpoint->sub_ch[i].index=muteswitch->subChannel;
                                    subchannelpoint->sub_ch[i].noise_en=muteswitch->muteSwitch;
                                    printf_debug("config.oal_config.cid=%d\n",config.oal_config.cid);
                                    printf_debug("config.oal_config.index=%d\n",config.oal_config.sub_channel_para[muteswitch->channel].sub_ch[i].index);
                                    printf_debug("config.oal_config.noise_en=%d\n",config.oal_config.sub_channel_para[muteswitch->channel].sub_ch[i].noise_en);

                                }
                                 
                        }
                }else if(methodcode==B_CODE_MID_FRQ_MUTE_THRE){/*静噪门限设置*/
                         noisethreshold=(struct dao_noise_threshold *) dao_conf_parse_batch(classcode,methodcode,data);
                         struct multi_freq_point_para_st *multpoint =&config.oal_config.multi_freq_point_param[noisethreshold->channel];
                         struct sub_channel_freq_para_st *subchannelpoint =&config.oal_config.sub_channel_para[noisethreshold->channel];
                         config.oal_config.cid=noisethreshold->channel;
                         if(noisethreshold->subChannel==(-1)){
                             multpoint->cid=noisethreshold->channel;
                             for(i=0;i<multpoint->freq_point_cnt;i++)
                             {
                                 
                                 multpoint->points[i].noise_thrh=noisethreshold->muteThreshold;
                                 //multpoint->points->index=noisethreshold->subChannel;
                                 printf_debug("==config.oal_config.cid=%d\n",config.oal_config.cid);
                                 printf_debug("==config.oal_config.index=%d\n",config.oal_config.multi_freq_point_param[noisethreshold->channel].points[i].index);
                                 printf_debug("==config.oal_config.noise_thrh=%d\n",config.oal_config.multi_freq_point_param[noisethreshold->channel].points[i].noise_thrh);

                             }

                         }else if(noisethreshold->subChannel>=0){
                             subchannelpoint->cid=noisethreshold->channel;
                             for(i=0;i<subchannelpoint->sub_channel_num;i++)
                             {
                                 //subchannelpoint->sub_ch[noisethreshold->subChannel].index=noisethreshold->subChannel;
                                 subchannelpoint->sub_ch[i].noise_thrh=noisethreshold->muteThreshold;
                                 printf_debug("cid=%d\n",config.oal_config.cid);
                                 printf_debug("index=%d\n",config.oal_config.sub_channel_para[noisethreshold->channel].sub_ch[i].index);
                                 printf_debug("noise_thrh=%d\n",config.oal_config.sub_channel_para[noisethreshold->channel].sub_ch[i].noise_thrh);

                             }


                         }
               }else if(methodcode==B_CODE_MID_FRQ_AU_SAMPLE_RATE){/*音频采样率设置参数*/

                     audio_samprate=(struct dao_audio_sampling_rate *) dao_conf_parse_batch(classcode,methodcode,data);
                     struct multi_freq_point_para_st *multpoint =&config.oal_config.multi_freq_point_param[audio_samprate->channel];
                     struct sub_channel_freq_para_st *subchannelpoint =&config.oal_config.sub_channel_para[audio_samprate->channel];
                     config.oal_config.cid=audio_samprate->channel;
                     if(audio_samprate->subChannel==(-1))
                     {
                         multpoint->cid=audio_samprate->channel;
                         for(i=0;i<multpoint->freq_point_cnt;i++)
                         {
                             
                             //multpoint->points[audio_samprate->subChannel].index=audio_samprate->subChannel;
                             multpoint->audio_sample_rate=audio_samprate->audioSampleRate;
                             printf_debug("config.oal_config.cid=%d\n",config.oal_config.cid);
                             printf_debug("config.oal_config.index=%d\n",config.oal_config.multi_freq_point_param[audio_samprate->channel].points[audio_samprate->subChannel].index);
                             printf_debug("config.oal_config.audio_sample_rate=%f\n",config.oal_config.multi_freq_point_param[audio_samprate->channel].audio_sample_rate);

                         }

                     }else if(audio_samprate->subChannel>=0){
                         subchannelpoint->cid=audio_samprate->channel;
                         for(i=0;i<subchannelpoint->sub_channel_num;i++)
                         {
                             //subchannelpoint->sub_ch[audio_samprate->subChannel].index=audio_samprate->subChannel;
                             subchannelpoint->audio_sample_rate=audio_samprate->audioSampleRate;
                             printf_debug("cid=%d\n",config.oal_config.cid);
                             printf_debug("index=%d\n",config.oal_config.sub_channel_para[audio_samprate->channel].sub_ch[audio_samprate->subChannel].index);
                             printf_debug("audio_sample_rate=%f\n",config.oal_config.sub_channel_para[audio_samprate->channel].audio_sample_rate);

                         }
                     }
                 }else {
                     printf_warn("In middle frequency parameter, there is no such parameter\n");
                    

                  }
                  break;
             }

         case CLASS_CODE_RF:/*射频参数命令*/
             {
                 if(methodcode==B_CODE_RF_FRQ_PARA){

                     RFpara=(struct dao_RF_parameters *) dao_conf_parse_batch(classcode,methodcode,data);
                     config.oal_config.cid=RFpara->channel;
                     config.oal_config.rf_para[RFpara->channel].cid=RFpara->channel;
                     config.oal_config.rf_para[RFpara->channel].rf_mode_code=RFpara->rfModeCode;
                     config.oal_config.rf_para[RFpara->channel].gain_ctrl_method=RFpara->gainControlMethod;
                     config.oal_config.rf_para[RFpara->channel].agc_ctrl_time=RFpara->agcControlTime;
                     config.oal_config.rf_para[RFpara->channel].agc_mid_freq_out_level=RFpara->agcMidFreqOutLevel;
                     config.oal_config.rf_para[RFpara->channel].antennas_elect=RFpara->antennaSelect;
                     config.oal_config.rf_para[RFpara->channel].mgc_gain_value=RFpara->mgcGainValue;
                     config.oal_config.rf_para[RFpara->channel].mid_bw=RFpara->rfBandwidth;
                     printf_debug("===cid=%d",config.oal_config.cid);
                     printf_debug("====config.oal_config.rf_para[RFpara->channel].cid=%d\n",config.oal_config.rf_para[RFpara->channel].cid);
                     printf_debug("====config.oal_config.rf_para[RFpara->channel].rf_mode_code=%d\n",config.oal_config.rf_para[RFpara->channel].rf_mode_code);
                     printf_debug("========config.oal_config.rf_para[RFpara->channel].gain_ctrl_method=%d\n",config.oal_config.rf_para[RFpara->channel].gain_ctrl_method);
                     printf_debug("===config.oal_config.rf_para[RFpara->channel].agc_ctrl_time=%d\n",config.oal_config.rf_para[RFpara->channel].agc_ctrl_time);
                     printf_debug("===config.oal_config.rf_para[RFpara->channel].agc_mid_freq_out_level=%d\n",config.oal_config.rf_para[RFpara->channel].agc_mid_freq_out_level);
                     printf_debug("====config.oal_config.rf_para[RFpara->channel].antennas_elect=%d\n",config.oal_config.rf_para[RFpara->channel].antennas_elect);
                     printf_debug("===config.oal_config.rf_para[RFpara->channel].mgc_gain_value=%d\n",config.oal_config.rf_para[RFpara->channel].mgc_gain_value);
                     printf_debug("===config.oal_config.rf_para[RFpara->channel].mid_bwd=%d",config.oal_config.rf_para[RFpara->channel].mid_bw);
                 }else if(methodcode==B_CODE_RF_FRQ_ANTENASELEC){
                     RFantennaselect=(struct dao_RF_antenna_selection *) dao_conf_parse_batch(classcode,methodcode,data);
                     config.oal_config.cid=RFantennaselect->channel;
                     config.oal_config.rf_para[RFantennaselect->channel].cid=RFantennaselect->channel;
                     config.oal_config.rf_para[RFantennaselect->channel].antennas_elect=RFantennaselect->antennaSelect;
                     printf_debug("===cid=%d\n",config.oal_config.cid);
                     printf_debug("====config.oal_config.rf_para[RFantennaselect->channel].cid=%d\n",config.oal_config.rf_para[RFantennaselect->channel].cid);
                     printf_debug("===config.oal_config.rf_para[RFantennaselect->channel].antennas_elect=%d\n\n\n",config.oal_config.rf_para[RFantennaselect->channel].antennas_elect);

                     printf_debug("===cid=%d",config.oal_config.cid);
                     printf_debug("====config.oal_config.rf_para[RFpara->channel].cid=%d\n",config.oal_config.rf_para[RFantennaselect->channel].cid);
                     printf_debug("====config.oal_config.rf_para[RFpara->channel].rf_mode_code=%d\n",config.oal_config.rf_para[RFantennaselect->channel].rf_mode_code);
                     printf_debug("========config.oal_config.rf_para[RFpara->channel].gain_ctrl_method=%d\n",config.oal_config.rf_para[RFantennaselect->channel].gain_ctrl_method);
                     printf_debug("===config.oal_config.rf_para[RFpara->channel].agc_ctrl_time=%d\n",config.oal_config.rf_para[RFantennaselect->channel].agc_ctrl_time);
                     printf_debug("===config.oal_config.rf_para[RFpara->channel].agc_mid_freq_out_level=%d\n",config.oal_config.rf_para[RFantennaselect->channel].agc_mid_freq_out_level);
                     printf_debug("====config.oal_config.rf_para[RFpara->channel].antennas_elect=%d\n",config.oal_config.rf_para[RFantennaselect->channel].antennas_elect);
                     printf_debug("===config.oal_config.rf_para[RFpara->channel].mgc_gain_value=%d\n",config.oal_config.rf_para[RFantennaselect->channel].mgc_gain_value);
                     printf_debug("===config.oal_config.rf_para[RFpara->channel].mid_bwd=%d",config.oal_config.rf_para[RFantennaselect->channel].mid_bw);

                 }else if(methodcode==B_CODE_RF_FRQ_OUTPUT_ATTENUATION){
                     
                     Rfoutputattenuation=(struct dao_Rf_output_attenuation *) dao_conf_parse_batch(classcode,methodcode,data);
                     config.oal_config.cid=Rfoutputattenuation->channel;
                     config.oal_config.rf_para[Rfoutputattenuation->channel].cid=Rfoutputattenuation->channel;
                     config.oal_config.rf_para[Rfoutputattenuation->channel].attenuation=Rfoutputattenuation->rfOutputAttenuation;
                     printf_debug("====cid=%d\n",config.oal_config.cid);
                     printf_debug("==config.oal_config.rf_para[Rfoutputattenuation->channel].cid=%d\n",config.oal_config.rf_para[Rfoutputattenuation->channel].cid);
                     printf_debug("===config.oal_config.rf_para[Rfoutputattenuation->channel].attenuation=%d\n",config.oal_config.rf_para[Rfoutputattenuation->channel].attenuation);
                 }else if(methodcode==B_CODE_RF_FRQ_INTPUT_ATTENUATION){
                    printf_warn("In rf parameter, there is no such parameter\n");
                 }else if(methodcode==B_CODE_RF_FRQ_INTPUT_BANDWIDTH){
                     Rfbdsetting=(struct dao_Rf_bandwidth_setting *) dao_conf_parse_batch(classcode,methodcode,data);
                     config.oal_config.cid=Rfbdsetting->channel;
                     config.oal_config.rf_para[Rfbdsetting->channel].cid=Rfbdsetting->channel;
                     config.oal_config.rf_para[Rfbdsetting->channel].mid_bw=Rfbdsetting->rfBandwidth;
                     printf_debug("======cid=%d\n",config.oal_config.cid);
                     printf_debug("======config.oal_config.rf_para[Rfbdsetting->channel].cid=%d\n",config.oal_config.rf_para[Rfbdsetting->channel].cid);
                     printf_debug("======config.oal_config.rf_para[Rfbdsetting->channel].mid_bw=%d\n",config.oal_config.rf_para[Rfbdsetting->channel].mid_bw);
                 }else{
                     printf_warn("In rf parameter, there is no such parameter\n");
                 }
                  break;

             }

         case CLASS_CODE_NET:
            {
                if(methodcode==B_CODE_ALL_NET)
                {
                    
                     netpara =(struct network_st *) dao_conf_parse_batch(classcode,methodcode,data);
                     memcpy(config.oal_config.network.mac,netpara->mac,6);
                     config.oal_config.network.gateway=netpara->gateway;
                     config.oal_config.network.ipaddress=netpara->ipaddress;
                     config.oal_config.network.netmask=netpara->netmask;
                     config.oal_config.network.port=netpara->port;
                    
                     
                     printf_debug("====mac=%02x %02x %02x %02x %02x %02x\n",config.oal_config.network.mac[0],
                                                                       config.oal_config.network.mac[1],
                                                                        config.oal_config.network.mac[2],
                                                                       config.oal_config.network.mac[3],
                                                                       config.oal_config.network.mac[4],
                                                                       config.oal_config.network.mac[5]);
                    
                    printf_debug("======netpara->gateway=%x\n",netpara->gateway);
                    printf_debug("===netpara->ipaddress=%x\n",netpara->ipaddress);
                    printf_debug("=====netpara->netmask=%x\n",netpara->netmask);
                    printf_debug("=====netpara->port=%d",netpara->port);

                }else{


                 printf_warn("In net parameter, there is no such parameter");

                }
               break;
            }

         case CLASS_CODE_WORK_MODE:
             {
                struct  multi_freq_point_para_st *multifrepoint;
                if(methodcode==B_CODE_WK_MODE_MULTI_FRQ_POINT)
                {
                    printf_debug("B_CODE_WK_MODE_MULTI_FRQ_POINT\n");
                    
                    multifrepoint =(struct multi_freq_point_para_st *) dao_conf_parse_batch(classcode,methodcode,data);
                    printf_debug("===============config===============dao_conf_parse_batch over\n");
                    
                    struct multi_freq_point_para_st *point=&config.oal_config.multi_freq_point_param[multifrepoint->cid];
                    config.oal_config.cid=multifrepoint->cid;
                    point->cid=multifrepoint->cid;
                    point->audio_sample_rate=multifrepoint->audio_sample_rate;
                    point->frame_drop_cnt=multifrepoint->frame_drop_cnt;
                    point->freq_point_cnt=multifrepoint->freq_point_cnt;
                    point->residence_policy=multifrepoint->residence_policy;
                    point->residence_time=multifrepoint->residence_time;
                    point->smooth_time=multifrepoint->smooth_time;
                    point->window_type=multifrepoint->window_type;
                    for(i=0;i<multifrepoint->freq_point_cnt;i++)
                    {
                        point->points[i].index=multifrepoint->points[i].index;
                        point->points[i].bandwidth=multifrepoint->points[i].bandwidth;
                        point->points[i].center_freq=multifrepoint->points[i].center_freq;
                        point->points[i].d_bandwith=multifrepoint->points[i].d_bandwith;
                        point->points[i].d_method=multifrepoint->points[i].d_method;
                        point->points[i].fft_size=multifrepoint->points[i].fft_size;
                        point->points[i].freq_resolution=multifrepoint->points[i].freq_resolution;
                        point->points[i].noise_en=multifrepoint->points[i].noise_en;
                        point->points[i].noise_thrh=multifrepoint->points[i].noise_thrh;
                        printf_debug("===channel=%d\n",config.oal_config.multi_freq_point_param[multifrepoint->cid].cid);
                        printf_debug("===windowType=%d\n",config.oal_config.multi_freq_point_param[multifrepoint->cid].window_type);
                        printf_debug("===frameDropCnt=%d\n",config.oal_config.multi_freq_point_param[multifrepoint->cid].frame_drop_cnt);
                        printf_debug("===smoothTimes=%d\n",config.oal_config.multi_freq_point_param[multifrepoint->cid].smooth_time);
                        printf_debug("===residenceTime=%d\n",config.oal_config.multi_freq_point_param[multifrepoint->cid].residence_time);
                        printf_debug("===residencePolicy=%d\n",config.oal_config.multi_freq_point_param[multifrepoint->cid].residence_policy);
                        printf_debug("===audioSampleRate=%f\n",config.oal_config.multi_freq_point_param[multifrepoint->cid].audio_sample_rate);
                        printf_debug("===bandwidth=%d\n",config.oal_config.multi_freq_point_param[multifrepoint->cid].points[i].bandwidth);
                        printf_debug("===center_freq=%d\n",config.oal_config.multi_freq_point_param[multifrepoint->cid].points[i].center_freq);
                        printf_debug("===d_bandwith=%d\n",config.oal_config.multi_freq_point_param[multifrepoint->cid].points[i].d_bandwith);
                        printf_debug("===d_method=%d\n",config.oal_config.multi_freq_point_param[multifrepoint->cid].points[i].d_method);       
                        printf_debug("===fft_size=%d\n",config.oal_config.multi_freq_point_param[multifrepoint->cid].points[i].fft_size);
                        printf_debug("===freq_resolution=%f\n",multifrepoint->points[i].freq_resolution);
                        printf_debug("===index=%d\n",config.oal_config.multi_freq_point_param[multifrepoint->cid].points[i].index);
                        printf_debug("===noise_en=%d\n",config.oal_config.multi_freq_point_param[multifrepoint->cid].points[i].noise_en);
                        printf_debug("===noise_thrh=%d\n",config.oal_config.multi_freq_point_param[multifrepoint->cid].points[i].noise_thrh);
                    }


                }else if(methodcode==B_CODE_WK_MODE_SUB_CH_DEC){
                    subchanelpara =(struct dao_sub_channel_freq_para_st *) dao_conf_parse_batch(classcode,methodcode,data);
                    struct sub_channel_freq_para_st *subchannelpoint =&config.oal_config.sub_channel_para[subchanelpara->cid];
                    config.oal_config.cid=subchanelpara->cid;
                    subchannelpoint->cid=subchanelpara->cid;
                    subchannelpoint->audio_sample_rate=subchanelpara->audio_sample_rate;
                    subchannelpoint->sub_channel_num=subchanelpara->sub_channel_num;
                    subchannelpoint->audio_sample_rate=subchanelpara->audio_sample_rate;
                    i=0;
                    for(i=0;i<subchannelpoint->sub_channel_num;i++)
                    {
                        subchannelpoint->sub_ch[i].index=subchanelpara->sub_ch[i].index;
                        subchannelpoint->sub_ch[i].center_freq=subchanelpara->sub_ch[i].center_freq;
                        subchannelpoint->sub_ch[i].d_bandwith=subchanelpara->sub_ch[i].d_bandwith;
                        subchannelpoint->sub_ch[i].d_method=subchanelpara->sub_ch[i].d_method;
                        subchannelpoint->sub_ch[i].fft_size=subchanelpara->sub_ch[i].fft_size;
                        subchannelpoint->sub_ch[i].noise_thrh=subchanelpara->sub_ch[i].noise_thrh;
                        subchannelpoint->sub_ch[i].noise_en=subchanelpara->sub_ch[i].noise_en;


                        printf_debug("==channel=%d\n",config.oal_config.sub_channel_para[subchannelpoint->cid].cid);
                        printf_debug("==audioSampleRate=%f\n",config.oal_config.sub_channel_para[subchannelpoint->cid].audio_sample_rate);
                        printf_debug("==sub_channel_num=%d\n",config.oal_config.sub_channel_para[subchannelpoint->cid].sub_channel_num);
                        printf_debug("==bandwidth=%d\n",config.oal_config.sub_channel_para[subchannelpoint->cid].sub_ch[i].bandwidth);
                        printf_debug("==center_freq=%d\n",config.oal_config.sub_channel_para[subchannelpoint->cid].sub_ch[i].center_freq);
                        printf_debug("==d_bandwith=%d\n",config.oal_config.sub_channel_para[subchannelpoint->cid].sub_ch[i].d_bandwith);
                        printf_debug("==d_method=%d\n",config.oal_config.sub_channel_para[subchannelpoint->cid].sub_ch[i].d_method);       
                        printf_debug("==fft_size=%d\n",config.oal_config.sub_channel_para[subchannelpoint->cid].sub_ch[i].fft_size);
                        printf_debug("==freq_resolution=%f\n",config.oal_config.sub_channel_para[subchannelpoint->cid].sub_ch[i].freq_resolution);
                        printf_debug("==index=%d\n",config.oal_config.sub_channel_para[subchannelpoint->cid].sub_ch[i].index);
                        printf_debug("==noise_en=%d\n",config.oal_config.sub_channel_para[subchannelpoint->cid].sub_ch[i].noise_en);
                        printf_debug("==noise_thrh=%d\n",config.oal_config.sub_channel_para[subchannelpoint->cid].sub_ch[i].noise_thrh);
                    }




                 }else if(methodcode==B_CODE_WK_MODE_MULTI_FRQ_FREGMENT){

                    printf_debug("++++++++++++++++++++++B_CODE_WK_MODE_MULTI_FRQ_FREGMENT+++++\n");
                    freqfergmentpara =(struct multi_freq_fregment_para_st *) dao_conf_parse_batch(classcode,methodcode,data);
                    struct multi_freq_fregment_para_st *point=&config.oal_config.multi_freq_fregment_para[freqfergmentpara->cid];
                    config.oal_config.cid=freqfergmentpara->cid;
                    point->cid=freqfergmentpara->cid;
                    point->frame_drop_cnt=freqfergmentpara->frame_drop_cnt;
                    point->freq_segment_cnt=freqfergmentpara->freq_segment_cnt;
                    point->smooth_time=freqfergmentpara->smooth_time;
                    point->window_type=freqfergmentpara->window_type;
                    for(i=0;i<freqfergmentpara->freq_segment_cnt;i++)
                        {
                            point->fregment[i].index=freqfergmentpara->fregment[i].index;
                            point->fregment[i].end_freq=freqfergmentpara->fregment[i].end_freq;
                            point->fregment[i].start_freq=freqfergmentpara->fregment[i].start_freq;
                            point->fregment[i].fft_size=freqfergmentpara->fregment[i].fft_size;
                            point->fregment[i].freq_resolution=freqfergmentpara->fregment[i].freq_resolution;
                            point->fregment[i].step=freqfergmentpara->fregment[i].step;
                            printf_debug("==index=%d\n",config.oal_config.multi_freq_fregment_para[freqfergmentpara->cid].fregment[i].index);
                            printf_debug("==end_freq=%d\n",config.oal_config.multi_freq_fregment_para[freqfergmentpara->cid].fregment[i].end_freq);
                            printf_debug("==start_freq=%d\n",config.oal_config.multi_freq_fregment_para[freqfergmentpara->cid].fregment[i].start_freq);
                            printf_debug("==freq_resolution=%f\n",config.oal_config.multi_freq_fregment_para[freqfergmentpara->cid].fregment[i].freq_resolution);
                            printf_debug("==fft_size=%d\n",config.oal_config.multi_freq_fregment_para[freqfergmentpara->cid].fregment[i].fft_size);
                            printf_debug("==step=%d\n\n\n",config.oal_config.multi_freq_fregment_para[freqfergmentpara->cid].fregment[i].step);

                       }

                     printf_debug("++++++++++++++++++++++=======================+\n");
                    printf_debug("==freqfergmentpara=%d\n",freqfergmentpara->cid);
                    printf_debug("==channel=%d\n",config.oal_config.multi_freq_fregment_para[freqfergmentpara->cid].cid);
                    printf_debug("==frame_drop_cnt=%d\n",config.oal_config.multi_freq_fregment_para[freqfergmentpara->cid].frame_drop_cnt);
                    printf_debug("==freq_segment_cnt=%d\n",config.oal_config.multi_freq_fregment_para[freqfergmentpara->cid].freq_segment_cnt);
                    printf_debug("==smooth_time=%d\n",config.oal_config.multi_freq_fregment_para[freqfergmentpara->cid].smooth_time);
                    printf_debug("==window_type=%d\n",config.oal_config.multi_freq_fregment_para[freqfergmentpara->cid].window_type);

                    printf_debug("++++++++++++++++++++++B_CODE_WK_MODE_MULTI_FRQ_FREGMENT++over+\n");

                 }else{
                     printf_warn("There is no such parameter in the center frequency parameter\n");
                 }
                 break;
            }
             
            
         case CLASS_CODE_CONTROL:/*控制参数*/
             {
                 if(methodcode == B_CODE_CONTROL_CONTR_DATA_OUTPUT_ENABLE)
                 {
                     outputenablecontr =(struct dao_data_output_enable_control *) dao_conf_parse_batch(classcode,methodcode,data);
                     struct output_en_st *outputen=&config.oal_config.enable;
                     struct output_en_st *suboutputen=&config.oal_config.sub_ch_enable;
                     config.oal_config.cid=outputenablecontr->channel;
                     if(outputenablecontr->subChannel==(-1))
                     {
                         
                         outputen->audio_en=outputenablecontr->audioEnable;
                         outputen->cid=outputenablecontr->channel;
                         outputen->direction_en=outputenablecontr->directionEn;
                         outputen->iq_en=outputenablecontr->IQEnable;
                         outputen->psd_en=outputenablecontr->psdEnable;
                         outputen->spec_analy_en=outputenablecontr->spectrumAnalysisEn;
                         outputen->sub_id=outputenablecontr->subChannel;

                         printf_debug("===config.oal_config.enable.audio_en=%d\n",config.oal_config.enable.audio_en);
                         printf_debug("===config.oal_config.enable.cid=%d\n",config.oal_config.enable.cid);
                         printf_debug("===config.oal_config.enable.direction_en=%d\n",config.oal_config.enable.direction_en);
                         printf_debug("=======config.oal_config.enable.iq_en=%d\n",config.oal_config.enable.iq_en);
                         printf_debug("===config.oal_config.enable.psd_en=%d\n",config.oal_config.enable.psd_en);
                         printf_debug("===config.oal_config.enable.spec_analy_en=%d\n",config.oal_config.enable.spec_analy_en);
                         printf_debug("=====config.oal_config.enable.sub_id=%d\n",config.oal_config.enable.sub_id);
                         

                     }else if(outputenablecontr->subChannel>=0){
                         suboutputen->audio_en=outputenablecontr->audioEnable;
                         suboutputen->cid=outputenablecontr->channel;
                         suboutputen->direction_en=outputenablecontr->directionEn;
                         suboutputen->iq_en=outputenablecontr->IQEnable;
                         suboutputen->psd_en=outputenablecontr->psdEnable;
                         suboutputen->spec_analy_en=outputenablecontr->spectrumAnalysisEn;
                         suboutputen->sub_id=outputenablecontr->subChannel;


                         printf_debug("config.oal_config.enable.audio_en=%d\n",config.oal_config.sub_ch_enable.audio_en);
                         printf_debug("config.oal_config.enable.cid=%d\n",config.oal_config.sub_ch_enable.cid);
                         printf_debug("config.oal_config.enable.direction_en=%d\n",config.oal_config.sub_ch_enable.direction_en);
                         printf_debug("config.oal_config.enable.iq_en=%d\n",config.oal_config.sub_ch_enable.iq_en);
                         printf_debug("config.oal_config.enable.psd_en=%d\n",config.oal_config.sub_ch_enable.psd_en);
                         printf_debug("config.oal_config.enable.spec_analy_en=%d\n",config.oal_config.sub_ch_enable.spec_analy_en);
                         printf_debug("config.oal_config.enable.sub_id=%d\n",config.oal_config.sub_ch_enable.sub_id);
                     }
                 }else if(methodcode==B_CODE_CONTROL_CONTR_CALIBRATION_CONTR){
                 
                    printf_warn("There are no such parameters in the control parameters.\n"); 
                    

                 }else if(methodcode==B_CODE_CONTROL_CONTR_LOCAL_REMOTE){
                    
                    Localremotecontr =(struct dao_Local_remote_control *) dao_conf_parse_batch(classcode,methodcode,data);
                    config.oal_config.ctrl_para.remote_local=Localremotecontr->ctrlMode;
                    printf_debug("config.oal_config.ctrl_para.remote_local)=%d\n",config.oal_config.ctrl_para.remote_local);
                    
                 }else if(methodcode==B_CODE_CONTROL_CONTR_CHANNEL_POWER){

                    printf_warn("There are no such parameters in the control parameters.\n"); 
                    
                 }else if(methodcode==B_CODE_CONTROL_CONTR_TIME_CONTR){
                    printf_warn("There are no such parameters in the control parameters.\n"); 

                 }else{
                     printf_warn("There are no such parameters in the control parameters.\n"); 
                     
                 }
                 break;
             }
         case CLASS_CODE_STATUS:
             {
                printf_warn("There are no such parameters in the control parameters.\n"); 
             }

         default:
             break;
     }
     printf_debug("config_parse_data\n");
    return 0;
}


/*本控 or 远控 查看接口*/
ctrl_mode_param config_get_control_mode(void)
{
#if  (CONTROL_MODE_SUPPORT == 1)
    if(config.oal_config->ctrl_para.remote_local == CTRL_MODE_LOCAL){
        return CTRL_MODE_LOCAL;
    }else{
        return CTRL_MODE_REMOTE;
    }
#endif
    return CTRL_MODE_REMOTE;
}

/******************************************************************************
* FUNCTION:
*     config_save_batch
*
* DESCRIPTION:
*     根据命令批量（单个）保存对应参数
* PARAMETERS
*     cmd:  保存参数对应类型命令; 见executor.h定义
*    type:  子类型 见executor.h定义
*     data: 对应数据结构；默认专递全局配置config结构体指针
* RETURNS
*       -1:  失败
*        0：成功
******************************************************************************/

int8_t config_save_batch(exec_cmd cmd, uint8_t type,s_config *config)
{
    printf_info(" config_save_batch\n");

#if DAO_XML == 1
     dao_conf_save_batch(cmd,type,config);
        
#elif DAO_JSON == 1
    
#else
    #error "NOT SUPPORT DAO FORMAT"
#endif
    return 0;
}

/******************************************************************************
* FUNCTION:
*     config_refresh_from_data
*
* DESCRIPTION:
*     根据命令和参数保存数据到config结构体; 中频参数默认保存到频点为1的参数
* PARAMETERS
*     cmd:  保存参数对应类型命令; 见executor.h定义
*     type:  子类型 见executor.h定义
*     data:   数据
* RETURNS
*       -1:  失败
*        0：成功
******************************************************************************/

int8_t config_refresh_data(exec_cmd cmd, uint8_t type, uint8_t ch, void *data)
{
    printf_info(" config_load_from_data\n");

    struct poal_config *poal_config = &(config_get_config()->oal_config);

    if(data ==NULL)
        return -1;
    
     switch(cmd)
     {
        case EX_MID_FREQ_CMD:
        {
            switch(type)
            {
                case EX_MUTE_SW:
                    if(*(uint8_t *)data > 1){
                        return -1;
                    }
                    poal_config->multi_freq_point_param[ch].points[0].noise_en = *(uint8_t *)data;
                    break;
                case EX_MUTE_THRE:
                    poal_config->multi_freq_point_param[ch].points[0].noise_thrh = *(uint8_t *)data;
                    break;
                case EX_DEC_METHOD:
                    poal_config->multi_freq_point_param[ch].points[0].d_method = *(uint8_t *)data;
                    break;
                case EX_DEC_BW:
                    poal_config->multi_freq_point_param[ch].points[0].d_bandwith = *(uint32_t *)data;
                    break;
                case EX_AUDIO_SAMPLE_RATE:
                    poal_config->multi_freq_point_param[ch].audio_sample_rate = *(float *)data;
                    break;
                default:
                    printf_err("not surpport type\n");
                    return -1;
            }
            break;
        }
        case EX_RF_FREQ_CMD:
        {
            switch(type)
            {
                case EX_RF_MID_FREQ:
                    poal_config->rf_para[ch].mid_freq = *(uint64_t *)data;
                    break;
                case EX_RF_MID_BW:
                    poal_config->rf_para[ch].mid_bw = *(uint32_t *)data;
                    break;
                case EX_RF_MODE_CODE:
                    poal_config->rf_para[ch].rf_mode_code = *(uint8_t *)data;
                    break;
                case EX_RF_GAIN_MODE:
                    poal_config->rf_para[ch].gain_ctrl_method = *(uint8_t *)data;
                    break;
                case EX_RF_MGC_GAIN:
                    poal_config->rf_para[ch].mgc_gain_value = *(float *)data;
                    break;
                default:
                    printf_err("not surpport type\n");
                    return -1;
            }
            break;
        }
        case EX_ENABLE_CMD:
        {
            break;
        }
        case EX_NETWORK_CMD:
        {
            switch(type)
            {
                case EX_NETWORK_IP:
                    poal_config->network.ipaddress = *(uint32_t *)data;
                    break;
                case EX_NETWORK_MASK:
                    poal_config->network.netmask = *(uint32_t *)data;
                    break;
                case EX_NETWORK_GW:
                    poal_config->network.gateway = *(uint32_t *)data;
                    break;
                case EX_NETWORK_PORT:
                    poal_config->network.port = *(uint16_t *)data;
                    break;
                default:
                    printf_err("not surpport type\n");
                    return -1;
            }
            break;

        }
        case EX_CTRL_CMD:
        {
             switch(type)
             {
                case EX_CTRL_LOCAL_REMOTE:
                    break;
                default:
                    printf_err("not surpport type\n");
                    return -1;
             }
        }
            break;
        default:
            printf_err("invalid set data[%d]\n", cmd);
            return -1;
     }
     
    config_save_batch(cmd, type, config_get_config());
    return 0;
}


int8_t config_read_by_cmd(exec_cmd cmd, uint8_t type, uint8_t ch, void *data)
{
    printf_info("config_read_by_cmd\n");
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    
    if(data == NULL){
        return -1;
    }
     switch(cmd)
     {
        case EX_MID_FREQ_CMD:
        {
            switch(type)
            {
                case EX_MUTE_SW:
                    data = (void *)&poal_config->multi_freq_point_param[ch].points[0].noise_en;
                    break;
                case EX_MUTE_THRE:
                     data = (void *)&poal_config->multi_freq_point_param[ch].points[0].noise_thrh;
                    break;
                case EX_DEC_METHOD:
                     data = (void *)&poal_config->multi_freq_point_param[ch].points[0].d_method;
                    break;
                case EX_DEC_BW:
                     data = (void *)&poal_config->multi_freq_point_param[ch].points[0].d_bandwith;
                    break;
                case EX_AUDIO_SAMPLE_RATE:
                     data = (void *)&poal_config->multi_freq_point_param[ch].audio_sample_rate;
                    break;
                default:
                    printf_err("not surpport type\n");
                    return -1;
            }
            break;
        }
        case EX_RF_FREQ_CMD:
        {
            switch(type)
            {
                case EX_RF_MID_FREQ:
                    data = (void *)&poal_config->rf_para[ch].mid_freq;
                    break;
                case EX_RF_MID_BW:
                    data = (void *)&poal_config->rf_para[ch].mid_bw;
                    break;
                case EX_RF_MODE_CODE:
                    data = (void *)&poal_config->rf_para[ch].rf_mode_code;
                    break;
                case EX_RF_GAIN_MODE:
                    data = (void *)&poal_config->rf_para[ch].gain_ctrl_method;
                    break;
                case EX_RF_MGC_GAIN:
                    data = (void *)&poal_config->rf_para[ch].mgc_gain_value;
                    break;
                default:
                    printf_err("not surpport type\n");
                    return -1;
            }
            break;
        }
        case EX_ENABLE_CMD:
        {
            break;
        }
        case EX_NETWORK_CMD:
        {
            switch(type)
            {
                case EX_NETWORK_IP:
                    data = (void *)&poal_config->network.ipaddress;
                    break;
                case EX_NETWORK_MASK:
                    data = (void *)&poal_config->network.netmask;
                    break;
                case EX_NETWORK_GW:
                    data = (void *)&poal_config->network.gateway;
                    break;
                case EX_NETWORK_PORT:
                    data = (void *)&poal_config->network.port;
                    break;
                default:
                    printf_err("not surpport type\n");
                    return -1;
            }
            break;
        }
        case EX_CTRL_CMD:
        {
             switch(type)
             {
                case EX_CTRL_LOCAL_REMOTE:
                    break;
                default:
                    printf_err("not surpport type\n");
                    return -1;
             }
        }
        default:
            printf_err("invalid set data[%d]\n", cmd);
            return -1;
     }
     
    return 0;
}





