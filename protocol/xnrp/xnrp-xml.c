#include "config.h"

int8_t xnrp_xml_parse_data(uint8_t classcode, uint8_t methodcode, void *data, uint32_t len)
{
    struct poal_config *config = &(config_get_config()->oal_config);
    printf_debug("start to xml parse data\n");
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
                         struct multi_freq_point_para_st *point =  &config->multi_freq_point_param[demodulationway->channel];
                 
                         config->cid=demodulationway->channel;
                         point->cid=demodulationway->channel;
                         for(i=0;i<point->freq_point_cnt;i++)
                         {
                             point->points[i].d_method=demodulationway->decMethodId;
                             
                             printf_debug("config.oal_config.cid=%d\n",config->cid);
                             printf_debug("config.oal_config.cid=%d\n",config->multi_freq_fregment_para[demodulationway->channel].cid);
                             printf_debug("config.oal_config.d_method=%d\n",config->multi_freq_point_param[demodulationway->channel].points[i].d_method);
                         }
                         break;

                         
                 }else if(methodcode==B_CODE_MID_FRQ_MUTE_SW){/*静噪开关设置*/
                         muteswitch=(struct dao_mute_switch *) dao_conf_parse_batch(classcode,methodcode,data);
                         struct multi_freq_point_para_st *multpoint =&config->multi_freq_point_param[muteswitch->channel];
                         struct sub_channel_freq_para_st *subchannelpoint =&config->sub_channel_para[muteswitch->channel];

                         if(muteswitch->subChannel==(-1))
                         {
                                 multpoint->cid=muteswitch->channel;
                                 for(i=0;i<multpoint->freq_point_cnt;i++)
                                 {
                                     
                                     multpoint->points[i].noise_en=muteswitch->muteSwitch;
                                     //multpoint->points[i].index=i;
                                     printf_debug("==config.oal_config.cid=%d\n",config->cid);
                                     printf_debug("==config.oal_config.index=%d\n",config->multi_freq_point_param[muteswitch->channel].points[i].index);
                                     printf_debug("==config.oal_config.noise_en=%d\n",config->multi_freq_point_param[muteswitch->channel].points[i].noise_en);
                                 }
                                 
                         }else if(muteswitch->subChannel>=0){
                                subchannelpoint->cid=muteswitch->channel;
                                for(i=0;i<subchannelpoint->sub_channel_num;i++)
                                {
                                    //subchannelpoint->sub_ch[i].index=muteswitch->subChannel;
                                    subchannelpoint->sub_ch[i].noise_en=muteswitch->muteSwitch;
                                    printf_debug("config.oal_config.cid=%d\n",config->cid);
                                    printf_debug("config.oal_config.index=%d\n",config->sub_channel_para[muteswitch->channel].sub_ch[i].index);
                                    printf_debug("config.oal_config.noise_en=%d\n",config->sub_channel_para[muteswitch->channel].sub_ch[i].noise_en);

                                }
                                 
                        }
                }else if(methodcode==B_CODE_MID_FRQ_MUTE_THRE){/*静噪门限设置*/
                         noisethreshold=(struct dao_noise_threshold *) dao_conf_parse_batch(classcode,methodcode,data);
                         struct multi_freq_point_para_st *multpoint =&config->multi_freq_point_param[noisethreshold->channel];
                         struct sub_channel_freq_para_st *subchannelpoint =&config->sub_channel_para[noisethreshold->channel];
                         config->cid=noisethreshold->channel;
                         if(noisethreshold->subChannel==(-1)){
                             multpoint->cid=noisethreshold->channel;
                             for(i=0;i<multpoint->freq_point_cnt;i++)
                             {
                                 
                                 multpoint->points[i].noise_thrh=noisethreshold->muteThreshold;
                                 //multpoint->points->index=noisethreshold->subChannel;
                                 printf_debug("==config.oal_config.cid=%d\n",config->cid);
                                 printf_debug("==config.oal_config.index=%d\n",config->multi_freq_point_param[noisethreshold->channel].points[i].index);
                                 printf_debug("==config.oal_config.noise_thrh=%d\n",config->multi_freq_point_param[noisethreshold->channel].points[i].noise_thrh);

                             }

                         }else if(noisethreshold->subChannel>=0){
                             subchannelpoint->cid=noisethreshold->channel;
                             for(i=0;i<subchannelpoint->sub_channel_num;i++)
                             {
                                 //subchannelpoint->sub_ch[noisethreshold->subChannel].index=noisethreshold->subChannel;
                                 subchannelpoint->sub_ch[i].noise_thrh=noisethreshold->muteThreshold;
                                 printf_debug("cid=%d\n",config->cid);
                                 printf_debug("index=%d\n",config->sub_channel_para[noisethreshold->channel].sub_ch[i].index);
                                 printf_debug("noise_thrh=%d\n",config->sub_channel_para[noisethreshold->channel].sub_ch[i].noise_thrh);

                             }


                         }
               }else if(methodcode==B_CODE_MID_FRQ_AU_SAMPLE_RATE){/*音频采样率设置参数*/

                     audio_samprate=(struct dao_audio_sampling_rate *) dao_conf_parse_batch(classcode,methodcode,data);
                     struct multi_freq_point_para_st *multpoint =&config->multi_freq_point_param[audio_samprate->channel];
                     struct sub_channel_freq_para_st *subchannelpoint =&config->sub_channel_para[audio_samprate->channel];
                     config->cid=audio_samprate->channel;
                     if(audio_samprate->subChannel==(-1))
                     {
                         multpoint->cid=audio_samprate->channel;
                         for(i=0;i<multpoint->freq_point_cnt;i++)
                         {
                             
                             //multpoint->points[audio_samprate->subChannel].index=audio_samprate->subChannel;
                             multpoint->audio_sample_rate=audio_samprate->audioSampleRate;
                             printf_debug("config.oal_config.cid=%d\n",config->cid);
                             printf_debug("config.oal_config.index=%d\n",config->multi_freq_point_param[audio_samprate->channel].points[audio_samprate->subChannel].index);
                             printf_debug("config.oal_config.audio_sample_rate=%f\n",config->multi_freq_point_param[audio_samprate->channel].audio_sample_rate);

                         }

                     }else if(audio_samprate->subChannel>=0){
                         subchannelpoint->cid=audio_samprate->channel;
                         for(i=0;i<subchannelpoint->sub_channel_num;i++)
                         {
                             //subchannelpoint->sub_ch[audio_samprate->subChannel].index=audio_samprate->subChannel;
                             subchannelpoint->audio_sample_rate=audio_samprate->audioSampleRate;
                             printf_debug("cid=%d\n",config->cid);
                             printf_debug("index=%d\n",config->sub_channel_para[audio_samprate->channel].sub_ch[audio_samprate->subChannel].index);
                             printf_debug("audio_sample_rate=%f\n",config->sub_channel_para[audio_samprate->channel].audio_sample_rate);

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
                     config->cid=RFpara->channel;
                     config->rf_para[RFpara->channel].cid=RFpara->channel;
                     config->rf_para[RFpara->channel].rf_mode_code=RFpara->rfModeCode;
                     config->rf_para[RFpara->channel].gain_ctrl_method=RFpara->gainControlMethod;
                     config->rf_para[RFpara->channel].agc_ctrl_time=RFpara->agcControlTime;
                     config->rf_para[RFpara->channel].agc_mid_freq_out_level=RFpara->agcMidFreqOutLevel;
                     config->rf_para[RFpara->channel].antennas_elect=RFpara->antennaSelect;
                     config->rf_para[RFpara->channel].mgc_gain_value=RFpara->mgcGainValue;
                     config->rf_para[RFpara->channel].mid_bw=RFpara->rfBandwidth;
                     printf_debug("===cid=%d",config->cid);
                     printf_debug("====config.oal_config.rf_para[RFpara->channel].cid=%d\n",config->rf_para[RFpara->channel].cid);
                     printf_debug("====config.oal_config.rf_para[RFpara->channel].rf_mode_code=%d\n",config->rf_para[RFpara->channel].rf_mode_code);
                     printf_debug("========config.oal_config.rf_para[RFpara->channel].gain_ctrl_method=%d\n",config->rf_para[RFpara->channel].gain_ctrl_method);
                     printf_debug("===config.oal_config.rf_para[RFpara->channel].agc_ctrl_time=%d\n",config->rf_para[RFpara->channel].agc_ctrl_time);
                     printf_debug("===config.oal_config.rf_para[RFpara->channel].agc_mid_freq_out_level=%d\n",config->rf_para[RFpara->channel].agc_mid_freq_out_level);
                     printf_debug("====config.oal_config.rf_para[RFpara->channel].antennas_elect=%d\n",config->rf_para[RFpara->channel].antennas_elect);
                     printf_debug("===config.oal_config.rf_para[RFpara->channel].mgc_gain_value=%d\n",config->rf_para[RFpara->channel].mgc_gain_value);
                     printf_debug("===config.oal_config.rf_para[RFpara->channel].mid_bwd=%d",config->rf_para[RFpara->channel].mid_bw);
                 }else if(methodcode==B_CODE_RF_FRQ_ANTENASELEC){
                     RFantennaselect=(struct dao_RF_antenna_selection *) dao_conf_parse_batch(classcode,methodcode,data);
                     config->cid=RFantennaselect->channel;
                     config->rf_para[RFantennaselect->channel].cid=RFantennaselect->channel;
                     config->rf_para[RFantennaselect->channel].antennas_elect=RFantennaselect->antennaSelect;
                     printf_debug("===cid=%d\n",config->cid);
                     printf_debug("====config.oal_config.rf_para[RFantennaselect->channel].cid=%d\n",config->rf_para[RFantennaselect->channel].cid);
                     printf_debug("===config.oal_config.rf_para[RFantennaselect->channel].antennas_elect=%d\n\n\n",config->rf_para[RFantennaselect->channel].antennas_elect);

                     printf_debug("===cid=%d",config->cid);
                     printf_debug("====config.oal_config.rf_para[RFpara->channel].cid=%d\n",config->rf_para[RFantennaselect->channel].cid);
                     printf_debug("====config.oal_config.rf_para[RFpara->channel].rf_mode_code=%d\n",config->rf_para[RFantennaselect->channel].rf_mode_code);
                     printf_debug("========config.oal_config.rf_para[RFpara->channel].gain_ctrl_method=%d\n",config->rf_para[RFantennaselect->channel].gain_ctrl_method);
                     printf_debug("===config.oal_config.rf_para[RFpara->channel].agc_ctrl_time=%d\n",config->rf_para[RFantennaselect->channel].agc_ctrl_time);
                     printf_debug("===config.oal_config.rf_para[RFpara->channel].agc_mid_freq_out_level=%d\n",config->rf_para[RFantennaselect->channel].agc_mid_freq_out_level);
                     printf_debug("====config.oal_config.rf_para[RFpara->channel].antennas_elect=%d\n",config->rf_para[RFantennaselect->channel].antennas_elect);
                     printf_debug("===config.oal_config.rf_para[RFpara->channel].mgc_gain_value=%d\n",config->rf_para[RFantennaselect->channel].mgc_gain_value);
                     printf_debug("===config.oal_config.rf_para[RFpara->channel].mid_bwd=%d",config->rf_para[RFantennaselect->channel].mid_bw);

                 }else if(methodcode==B_CODE_RF_FRQ_OUTPUT_ATTENUATION){
                     
                     Rfoutputattenuation=(struct dao_Rf_output_attenuation *) dao_conf_parse_batch(classcode,methodcode,data);
                     config->cid=Rfoutputattenuation->channel;
                     config->rf_para[Rfoutputattenuation->channel].cid=Rfoutputattenuation->channel;
                     config->rf_para[Rfoutputattenuation->channel].attenuation=Rfoutputattenuation->rfOutputAttenuation;
                     printf_debug("====cid=%d\n",config->cid);
                     printf_debug("==config.oal_config.rf_para[Rfoutputattenuation->channel].cid=%d\n",config->rf_para[Rfoutputattenuation->channel].cid);
                     printf_debug("===config.oal_config.rf_para[Rfoutputattenuation->channel].attenuation=%d\n",config->rf_para[Rfoutputattenuation->channel].attenuation);
                 }else if(methodcode==B_CODE_RF_FRQ_INTPUT_ATTENUATION){
                    printf_warn("In rf parameter, there is no such parameter\n");
                 }else if(methodcode==B_CODE_RF_FRQ_INTPUT_BANDWIDTH){
                     Rfbdsetting=(struct dao_Rf_bandwidth_setting *) dao_conf_parse_batch(classcode,methodcode,data);
                     config->cid=Rfbdsetting->channel;
                     config->rf_para[Rfbdsetting->channel].cid=Rfbdsetting->channel;
                     config->rf_para[Rfbdsetting->channel].mid_bw=Rfbdsetting->rfBandwidth;
                     printf_debug("======cid=%d\n",config->cid);
                     printf_debug("======config.oal_config.rf_para[Rfbdsetting->channel].cid=%d\n",config->rf_para[Rfbdsetting->channel].cid);
                     printf_debug("======config.oal_config.rf_para[Rfbdsetting->channel].mid_bw=%d\n",config->rf_para[Rfbdsetting->channel].mid_bw);
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
                     memcpy(config->network.mac,netpara->mac,6);
                     config->network.gateway=netpara->gateway;
                     config->network.ipaddress=netpara->ipaddress;
                     config->network.netmask=netpara->netmask;
                     config->network.port=netpara->port;
                    
                     
                     printf_debug("====mac=%02x %02x %02x %02x %02x %02x\n",config->network.mac[0],
                                                                       config->network.mac[1],
                                                                        config->network.mac[2],
                                                                       config->network.mac[3],
                                                                       config->network.mac[4],
                                                                       config->network.mac[5]);
                    
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
                    
                    struct multi_freq_point_para_st *point=&config->multi_freq_point_param[multifrepoint->cid];
                    config->cid=multifrepoint->cid;
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
                        printf_debug("\n\n===channel=%d\n",config->multi_freq_point_param[multifrepoint->cid].cid);
                        printf_debug("===windowType=%d\n",config->multi_freq_point_param[multifrepoint->cid].window_type);
                        printf_debug("===frameDropCnt=%d\n",config->multi_freq_point_param[multifrepoint->cid].frame_drop_cnt);
                        printf_debug("===smoothTimes=%d\n",config->multi_freq_point_param[multifrepoint->cid].smooth_time);
                        printf_debug("===residenceTime=%d\n",config->multi_freq_point_param[multifrepoint->cid].residence_time);
                        printf_debug("===residencePolicy=%d\n",config->multi_freq_point_param[multifrepoint->cid].residence_policy);
                        printf_debug("===audioSampleRate=%f\n",config->multi_freq_point_param[multifrepoint->cid].audio_sample_rate);
                        printf_debug("===bandwidth=%d\n",config->multi_freq_point_param[multifrepoint->cid].points[i].bandwidth);
                        printf_debug("===center_freq=%d\n",config->multi_freq_point_param[multifrepoint->cid].points[i].center_freq);
                        printf_debug("===d_bandwith=%d\n",config->multi_freq_point_param[multifrepoint->cid].points[i].d_bandwith);
                        printf_debug("===d_method=%d\n",config->multi_freq_point_param[multifrepoint->cid].points[i].d_method);       
                        printf_debug("===fft_size=%d\n",config->multi_freq_point_param[multifrepoint->cid].points[i].fft_size);
                        printf_debug("===freq_resolution=%f\n",multifrepoint->points[i].freq_resolution);
                        printf_debug("===index=%d\n",config->multi_freq_point_param[multifrepoint->cid].points[i].index);
                        printf_debug("===noise_en=%d\n",config->multi_freq_point_param[multifrepoint->cid].points[i].noise_en);
                        printf_debug("===noise_thrh=%d\n\n\n",config->multi_freq_point_param[multifrepoint->cid].points[i].noise_thrh);
                    }


                }else if(methodcode==B_CODE_WK_MODE_SUB_CH_DEC){
                    subchanelpara =(struct dao_sub_channel_freq_para_st *) dao_conf_parse_batch(classcode,methodcode,data);
                    struct sub_channel_freq_para_st *subchannelpoint =&config->sub_channel_para[subchanelpara->cid];
                    config->cid=subchanelpara->cid;
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


                        printf_debug("==channel=%d\n",config->sub_channel_para[subchannelpoint->cid].cid);
                        printf_debug("==audioSampleRate=%f\n",config->sub_channel_para[subchannelpoint->cid].audio_sample_rate);
                        printf_debug("==sub_channel_num=%d\n",config->sub_channel_para[subchannelpoint->cid].sub_channel_num);
                        printf_debug("==bandwidth=%d\n",config->sub_channel_para[subchannelpoint->cid].sub_ch[i].bandwidth);
                        printf_debug("==center_freq=%d\n",config->sub_channel_para[subchannelpoint->cid].sub_ch[i].center_freq);
                        printf_debug("==d_bandwith=%d\n",config->sub_channel_para[subchannelpoint->cid].sub_ch[i].d_bandwith);
                        printf_debug("==d_method=%d\n",config->sub_channel_para[subchannelpoint->cid].sub_ch[i].d_method);       
                        printf_debug("==fft_size=%d\n",config->sub_channel_para[subchannelpoint->cid].sub_ch[i].fft_size);
                        printf_debug("==freq_resolution=%f\n",config->sub_channel_para[subchannelpoint->cid].sub_ch[i].freq_resolution);
                        printf_debug("==index=%d\n",config->sub_channel_para[subchannelpoint->cid].sub_ch[i].index);
                        printf_debug("==noise_en=%d\n",config->sub_channel_para[subchannelpoint->cid].sub_ch[i].noise_en);
                        printf_debug("==noise_thrh=%d\n",config->sub_channel_para[subchannelpoint->cid].sub_ch[i].noise_thrh);
                    }




                 }else if(methodcode==B_CODE_WK_MODE_MULTI_FRQ_FREGMENT){

                    printf_debug("++++++++++++++++++++++B_CODE_WK_MODE_MULTI_FRQ_FREGMENT+++++\n");
                    freqfergmentpara =(struct multi_freq_fregment_para_st *) dao_conf_parse_batch(classcode,methodcode,data);
                    struct multi_freq_fregment_para_st *point=&config->multi_freq_fregment_para[freqfergmentpara->cid];
                    config->cid=freqfergmentpara->cid;
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
                            printf_debug("==index=%d\n",config->multi_freq_fregment_para[freqfergmentpara->cid].fregment[i].index);
                            printf_debug("==end_freq=%d\n",config->multi_freq_fregment_para[freqfergmentpara->cid].fregment[i].end_freq);
                            printf_debug("==start_freq=%d\n",config->multi_freq_fregment_para[freqfergmentpara->cid].fregment[i].start_freq);
                            printf_debug("==freq_resolution=%f\n",config->multi_freq_fregment_para[freqfergmentpara->cid].fregment[i].freq_resolution);
                            printf_debug("==fft_size=%d\n",config->multi_freq_fregment_para[freqfergmentpara->cid].fregment[i].fft_size);
                            printf_debug("==step=%d\n\n\n",config->multi_freq_fregment_para[freqfergmentpara->cid].fregment[i].step);

                       }

                     printf_debug("++++++++++++++++++++++=======================+\n");
                    printf_debug("==freqfergmentpara=%d\n",freqfergmentpara->cid);
                    printf_debug("==channel=%d\n",config->multi_freq_fregment_para[freqfergmentpara->cid].cid);
                    printf_debug("==frame_drop_cnt=%d\n",config->multi_freq_fregment_para[freqfergmentpara->cid].frame_drop_cnt);
                    printf_debug("==freq_segment_cnt=%d\n",config->multi_freq_fregment_para[freqfergmentpara->cid].freq_segment_cnt);
                    printf_debug("==smooth_time=%d\n",config->multi_freq_fregment_para[freqfergmentpara->cid].smooth_time);
                    printf_debug("==window_type=%d\n",config->multi_freq_fregment_para[freqfergmentpara->cid].window_type);

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
                     struct output_en_st *outputen=&config->enable;
                     struct output_en_st *suboutputen=&config->sub_ch_enable;
                     config->cid=outputenablecontr->channel;
                     if(outputenablecontr->subChannel==(-1))
                     {
                         
                         outputen->audio_en=outputenablecontr->audioEnable;
                         outputen->cid=outputenablecontr->channel;
                         outputen->direction_en=outputenablecontr->directionEn;
                         outputen->iq_en=outputenablecontr->IQEnable;
                         outputen->psd_en=outputenablecontr->psdEnable;
                         outputen->spec_analy_en=outputenablecontr->spectrumAnalysisEn;
                         outputen->sub_id=outputenablecontr->subChannel;

                         printf_debug("===config.oal_config.enable.audio_en=%d\n",config->enable.audio_en);
                         printf_debug("===config.oal_config.enable.cid=%d\n",config->enable.cid);
                         printf_debug("===config.oal_config.enable.direction_en=%d\n",config->enable.direction_en);
                         printf_debug("=======config.oal_config.enable.iq_en=%d\n",config->enable.iq_en);
                         printf_debug("===config.oal_config.enable.psd_en=%d\n",config->enable.psd_en);
                         printf_debug("===config.oal_config.enable.spec_analy_en=%d\n",config->enable.spec_analy_en);
                         printf_debug("=====config.oal_config.enable.sub_id=%d\n",config->enable.sub_id);
                         

                     }else if(outputenablecontr->subChannel>=0){
                         suboutputen->audio_en=outputenablecontr->audioEnable;
                         suboutputen->cid=outputenablecontr->channel;
                         suboutputen->direction_en=outputenablecontr->directionEn;
                         suboutputen->iq_en=outputenablecontr->IQEnable;
                         suboutputen->psd_en=outputenablecontr->psdEnable;
                         suboutputen->spec_analy_en=outputenablecontr->spectrumAnalysisEn;
                         suboutputen->sub_id=outputenablecontr->subChannel;


                         printf_debug("config.oal_config.enable.audio_en=%d\n",config->sub_ch_enable.audio_en);
                         printf_debug("config.oal_config.enable.cid=%d\n",config->sub_ch_enable.cid);
                         printf_debug("config.oal_config.enable.direction_en=%d\n",config->sub_ch_enable.direction_en);
                         printf_debug("config.oal_config.enable.iq_en=%d\n",config->sub_ch_enable.iq_en);
                         printf_debug("config.oal_config.enable.psd_en=%d\n",config->sub_ch_enable.psd_en);
                         printf_debug("config.oal_config.enable.spec_analy_en=%d\n",config->sub_ch_enable.spec_analy_en);
                         printf_debug("config.oal_config.enable.sub_id=%d\n",config->sub_ch_enable.sub_id);
                     }
                 }else if(methodcode==B_CODE_CONTROL_CONTR_CALIBRATION_CONTR){
                 
                    printf_warn("There are no such parameters in the control parameters.\n"); 
                    

                 }else if(methodcode==B_CODE_CONTROL_CONTR_LOCAL_REMOTE){
                    
                    Localremotecontr =(struct dao_Local_remote_control *) dao_conf_parse_batch(classcode,methodcode,data);
                    config->ctrl_para.remote_local=Localremotecontr->ctrlMode;
                    printf_debug("config.oal_config.ctrl_para.remote_local)=%d\n",config->ctrl_para.remote_local);
                    
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


