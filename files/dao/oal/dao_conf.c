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
//#include "dao_conf.h"



void dao_conf_save_batch(exec_cmd cmd, uint8_t type, s_config *config)
{


    char temp[20];
    char signalnum[10];
    memset(temp,0,sizeof(char)*20);

    /*通道号*/
    sprintf(temp,"%d",config->oal_config.cid);
    memset(signalnum,0,sizeof(uint8_t)*10);
    sprintf(signalnum,"%d",config->oal_config.multi_freq_point_param[config->oal_config.cid].points[0].index);
    if(config->oal_config.multi_freq_point_param[config->oal_config.cid].points[0].index >1)
    {
        printf_warn("Only supports fixed frequency mode, not multi-frequency point!");

        return 0;

    }
    switch(cmd)
    {
 
        case EX_MID_FREQ_CMD:/*中频参数命令*/
            if(type==EX_MID_FREQ) /*中心频率*/ 
            {

                write_config_file_array(XMLFILENAME,"channel","index",temp,"mediumfrequency","centerFreq","freqPoint","index",signalnum,config->oal_config.multi_freq_point_param[config->oal_config.cid].points[0].center_freq,ARRAY_ARRAY);
                //write_config_file_array(XMLFILENAME,"channel","index",temp,"mediumfrequency","centerFreq","freqPoint","index",signalnum,300000.0624,ARRAY_ARRAY);
            }
            else if(type==EX_BANDWITH)/*中心带宽*/
            { 
                write_config_file_array(XMLFILENAME,"channel","index",temp,"mediumfrequency","bandwith","freqPoint","index",signalnum,config->oal_config.multi_freq_point_param[config->oal_config.cid].points[0].bandwidth,ARRAY_ARRAY);
            }
            else if(type==EX_FFT_SIZE)/*FFT点数*/
            {
                write_config_file_array(XMLFILENAME,"channel","index",temp,"mediumfrequency","fftSize","freqPoint","index",signalnum,config->oal_config.multi_freq_point_param[config->oal_config.cid].points[0].fft_size,ARRAY_ARRAY);
            }
            else if(type==EX_DEC_BW)/*解调带宽*/
            {

                write_config_file_array(XMLFILENAME,"channel","index",temp,"mediumfrequency","decBandwidth","freqPoint","index",signalnum,config->oal_config.multi_freq_point_param[config->oal_config.cid].points[0].d_bandwith,ARRAY_ARRAY);
            }
            else if(type==EX_MUTE_SW)/*静噪开关  */
            {
                write_config_file_array(XMLFILENAME,"channel","index",temp,"mediumfrequency","muteSwitch","freqPoint","index",signalnum,config->oal_config.multi_freq_point_param[config->oal_config.cid].points[0].noise_en,ARRAY_ARRAY);
            }
            else if(type==EX_MUTE_THRE) /*静噪门限*/ 
            {
                write_config_file_array(XMLFILENAME,"channel","index",temp,"mediumfrequency","muteThreshold","freqPoint","index",signalnum,config->oal_config.multi_freq_point_param[config->oal_config.cid].points[0].noise_thrh,ARRAY_ARRAY);

               
            }
            else if(type==EX_DEC_METHOD)/*解调方式*/ 
            {
                 write_config_file_array(XMLFILENAME,"channel","index",temp,"mediumfrequency","decMethodId","freqPoint","index",signalnum,config->oal_config.multi_freq_point_param[config->oal_config.cid].points[0].d_method,ARRAY_ARRAY);

            }else 
            {
                printf_warn("Unsupported parameter modification");
            }
              
            /*差频率分辨率*/
            break;
        case EX_RF_FREQ_CMD:/*射频参数命令*/
            printf_debug("EX_RF_FREQ_CMD\n");
            if(type==EX_RF_MID_FREQ)  /* 射频中心频率 */
            {
               printf_debug("Unsupported parameter modification\n");
               // write_config_file_array(XMLFILENAME,"channel","index",temp,"radiofrequency","decMethodId","freqPoint","index",signalnum,config->oal_config->multi_freq_point_param->points->d_method,ARRAY_ARRAY);
               // write_config_file_array(XMLFILENAME,"channel","index",temp,"radiofrequency","midBw",NULL,NULL,NULL,config->oal_config->rf_para->mid_bw,ARRAY_PARENT);
             
            }
            else if(type==EX_RF_MID_BW) /*中频带宽*/
            {
                //printf_debug("config->oal_config.rf_para.mid_bw=%d",config->oal_config.rf_para[config->oal_config.cid].mid_bw);
                write_config_file_array(XMLFILENAME,"channel","index",temp,"radiofrequency","midBw",NULL,NULL,NULL,config->oal_config.rf_para[config->oal_config.cid].mid_bw,ARRAY_PARENT);
             
            }
            else if(type==EX_RF_MODE_CODE)/*模式码 0x00：低失真 0x01：常规 0x02：低噪声*/
            {
                write_config_file_array(XMLFILENAME,"channel","index",temp,"radiofrequency","modeCode",NULL,NULL,NULL,config->oal_config.rf_para[config->oal_config.cid].rf_mode_code,ARRAY_PARENT);
            }
            else if(type==EX_RF_GAIN_MODE)/*增益模式*/
            {
                write_config_file_array(XMLFILENAME,"channel","index",temp,"radiofrequency","gainMode",NULL,NULL,NULL,config->oal_config.rf_para[config->oal_config.cid].gain_ctrl_method,ARRAY_PARENT);

            }
            else if(type==EX_RF_MGC_GAIN)/*MGC 增益值*/
            {
                //write_config_file_array(XMLFILENAME,"channel","index",temp,"radiofrequency","midBw",NULL,NULL,NULL,config->oal_config->rf_para->mgc_gain_value,ARRAY_PARENT);

            }
            else if(type==EX_RF_AGC_CTRL_TIME)/*AGC 控制时间*/
            {
                write_config_file_array(XMLFILENAME,"channel","index",temp,"radiofrequency","agcCtrlTime",NULL,NULL,NULL,config->oal_config.rf_para[config->oal_config.cid].agc_ctrl_time,ARRAY_PARENT);
            }
            else if(type==EX_RF_AGC_OUTPUT_AMP)  /*AGC 中频输出幅度*/
            {
                write_config_file_array(XMLFILENAME,"channel","index",temp,"radiofrequency","agcOutPutAmp",NULL,NULL,NULL,config->oal_config.rf_para[config->oal_config.cid].agc_mid_freq_out_level,ARRAY_PARENT); 

            }  
            else if(type==EX_RF_ATTENUATION) /*射频衰减*/
            {
                write_config_file_array(XMLFILENAME,"channel","index",temp,"radiofrequency","rfAttenuation",NULL,NULL,NULL,config->oal_config.rf_para[config->oal_config.cid].attenuation,ARRAY_PARENT);

            }else{
                printf_warn("Unsupported parameter modification");

            }
       
             break;  
        case EX_ENABLE_CMD:
            if(type==PSD_MODE_ENABLE||type==PSD_MODE_DISABLE)
            {
                 write_config_file_single(XMLFILENAME,"dataOutPutEn","psdEnable",NULL,config->oal_config.enable.psd_en);
            }
            else if(type==AUDIO_MODE_ENABLE||type==AUDIO_MODE_DISABLE)
            {
                write_config_file_single(XMLFILENAME,"dataOutPutEn","audioEnable",NULL,config->oal_config.enable.audio_en);
            }
            else if(type==IQ_MODE_ENABLE||type==IQ_MODE_DISABLE)
            {
                write_config_file_single(XMLFILENAME,"dataOutPutEn","IQEnable",NULL,config->oal_config.enable.iq_en);

            }
            else if(type==SPCTRUM_MODE_ANALYSIS_ENABLE||type==SPCTRUM_MODE_ANALYSIS_DISABLE)
            {
                 write_config_file_single(XMLFILENAME,"dataOutPutEn","spectrumAnalysisEn",NULL,config->oal_config.enable.spec_analy_en);


            }
            else if(type==DIRECTION_MODE_ENABLE||type==DIRECTION_MODE_ENABLE_DISABLE)
            {
                write_config_file_single(XMLFILENAME,"dataOutPutEn","directionEn",NULL,config->oal_config.enable.direction_en);
            }else{

                printf_warn("Unsupported parameter modification");

            }
            break;    

        

        case EX_WORK_MODE_CMD:
            if(type==OAL_FIXED_FREQ_ANYS_MODE)/*定频模式*/
            {
                write_config_file_array(XMLFILENAME,"channel","index",temp,"cid",NULL,NULL,NULL,NULL,config->oal_config.multi_freq_point_param[config->oal_config.cid].points[0].center_freq,ARRAY);
                write_config_file_array(XMLFILENAME,"channel","index",temp,"mediumfrequency","centerFreq","freqPoint","index",signalnum,config->oal_config.multi_freq_point_param[config->oal_config.cid].points[0].center_freq,ARRAY_ARRAY);
                write_config_file_array(XMLFILENAME,"channel","index",temp,"mediumfrequency","bandwith","freqPoint","index",signalnum,config->oal_config.multi_freq_point_param[config->oal_config.cid].points[0].bandwidth,ARRAY_ARRAY);
                write_config_file_array(XMLFILENAME,"channel","index",temp,"mediumfrequency","freqResolution","freqPoint","index",signalnum,config->oal_config.multi_freq_point_param[config->oal_config.cid].points[0].freq_resolution,ARRAY_ARRAY);
                write_config_file_array(XMLFILENAME,"channel","index",temp,"mediumfrequency","fftSize","freqPoint","index",signalnum,config->oal_config.multi_freq_point_param[config->oal_config.cid].points[0].fft_size,ARRAY_ARRAY);
                write_config_file_array(XMLFILENAME,"channel","index",temp,"mediumfrequency","decMethodId","freqPoint","index",signalnum,config->oal_config.multi_freq_point_param[config->oal_config.cid].points[0].d_method,ARRAY_ARRAY);
                write_config_file_array(XMLFILENAME,"channel","index",temp,"mediumfrequency","decBandwidth","freqPoint","index",signalnum,config->oal_config.multi_freq_point_param[config->oal_config.cid].points[0].d_bandwith,ARRAY_ARRAY);
                write_config_file_array(XMLFILENAME,"channel","index",temp,"mediumfrequency","muteSwitch","freqPoint","index",signalnum,config->oal_config.multi_freq_point_param[config->oal_config.cid].points[0].noise_en,ARRAY_ARRAY);
                write_config_file_array(XMLFILENAME,"channel","index",temp,"mediumfrequency","muteThreshold","freqPoint","index",signalnum,config->oal_config.multi_freq_point_param[config->oal_config.cid].points[0].noise_thrh,ARRAY_ARRAY);

            }else{

                printf_warn("Unsupported parameter modification");


            }
            
            break; 
        case EX_STATUS_CMD:
            //write_config_file_single(XMLFILENAME,"softVersion","app",config->oal_config->,0);

                printf_debug("Unsupported parameter modification");
                break;
        case EX_NETWORK_CMD:
            {
                char temp[30];

                sprintf(temp,"%02x:%02x:%02x:%02x:%02x:%02x\n", config->oal_config.network.mac[0],config->oal_config.network.mac[1],
                config->oal_config.network.mac[2],config->oal_config.network.mac[3],config->oal_config.network.mac[4],
                config->oal_config.network.mac[5]);


                printf_debug(" config->oal_config.network.mac -----------%s\n",temp);
                write_config_file_single(XMLFILENAME,"network","mac",temp,0);
                
                struct in_addr netpara;
                const char *ipstr=NULL;
                netpara.s_addr=config->oal_config.network.gateway;
                
                ipstr= inet_ntoa(netpara);
                printf_debug("----------gateway=%s\n", ipstr);
                write_config_file_single(XMLFILENAME,"network","gateway",ipstr,0);

                
                ipstr=NULL;
                netpara.s_addr=config->oal_config.network.netmask;
                ipstr= inet_ntoa(netpara);
                printf_debug("----------netmask=%s", ipstr);
                write_config_file_single(XMLFILENAME,"network","netmask",ipstr,0);

                ipstr=NULL;
                netpara.s_addr=config->oal_config.network.ipaddress;
                ipstr= inet_ntoa(netpara);
                printf_debug("----------ipaddress=%s", ipstr);
                write_config_file_single(XMLFILENAME,"network","ipaddress",ipstr,0);
                write_config_file_single(XMLFILENAME,"network","port",NULL,(int)config->oal_config.network.port);
                break;
            }
     default:

            break;
    }  
    printf_debug("save parse data over\n");
}
int safe_atoi(const char *str)
{
    if(str==NULL)
    {

        printf_warn("Some of the parameter node values you input are null\n");
        return NULL;

    }else{
        // printf_debug("str=%s",str);
         
        // printf_debug("atoi(str)=%d",atoi(str));

        return atoi(str);
    }


}

float safe_atof(const char *str)
{
    if(str==NULL)
    {

        printf_warn("Some of the parameter node values you input are null\n");
        return 0;

    }else{
        // printf_debug("str=%s",str);
         
        // printf_debug("atoi(str)=%d",atoi(str));

        return atof(str);
    }


}

void* dao_conf_parse_batch(uint8_t classcode, uint8_t methodcode, char *data)
{
    printf_debug("====start dao_conf_parse_batch\n");

    mxml_node_t *tree;
    mxml_node_t *node;
    mxml_node_t *group;
    mxml_node_t *child;
    int16_t i=0;
    int16_t j=0;
    tree=mxmlLoadString(NULL,data,MXML_TEXT_CALLBACK);
    /*中频参数*/
    static struct dao_mute_switch muteswitch;/*静噪开关设置*/
    static struct dao_noise_threshold noisethreshold;/*静噪门限设置*/
    static struct  dao_demodulation_way demodulationway;/*解调方式参数*/
    static struct dao_audio_sampling_rate audiosamprate;/*音频采样率设置参数*/
    /*射频参数*/
    static struct dao_RF_parameters RFpara;
    static struct  dao_RF_antenna_selection RFantennaselect;
    static struct  dao_Rf_output_attenuation Rfoutputattenuation;
    static struct dao_Rf_Input_attenuation RfInputattenuation;
    static struct  dao_Rf_bandwidth_setting Rfbdsetting;
    /*工作模式*/
    static struct  multi_freq_point_para_st multifrepoint;
    static struct dao_sub_channel_freq_para_st  subchanelpara;
    static struct multi_freq_fregment_para_st  freqfergmentpara;
    /*网络参数*/
    //struct network_st netpara;
    static struct network_st netpara;
    /*控制参数*/
    static struct dao_data_output_enable_control outputenablecontr;
    static struct dao_calibration_control  calibratcontrol;
    static struct dao_Local_remote_control Localremotecontr;
    static struct dao_channel_power_control  channelpowercontrol[MAX_SIG_CHANNLE];
   static struct  dao_time_control  timecontrol;
    /*设备基本信息*/

    static  struct  dao_Equipment_basic_infor   Epbasicinfor;
    static struct  dao_Channel_status_check    channelstatus;
    static struct  dao_Check_power_status      power_status;
    static struct dao_Query_storage_state     storagestate;
    static struct dao_software_version_infor  softwareversion;
    
   
    
   switch(classcode)
    {
 
        case CLASS_CODE_MID_FRQ:/*中频参数命令*/
           {
                if(methodcode==B_CODE_MID_FRQ_DEC_METHOD){/*解调方式参数*/
                    node=mxmlFindElement(tree,tree,"channel", NULL, NULL, MXML_DESCEND); 
                    demodulationway.channel=(uint8_t)safe_atoi(mxmlGetText(node, NULL));
                    node=mxmlFindElement(tree,tree,"decMethodId", NULL, NULL, MXML_DESCEND);
                    demodulationway.decMethodId=(uint8_t)safe_atoi(mxmlGetText(node, NULL)); 
                    return (void*)&demodulationway;
                }else if(methodcode==B_CODE_MID_FRQ_MUTE_SW){/*静噪开关设置*/

                    node=mxmlFindElement(tree,tree,"channel", NULL, NULL, MXML_DESCEND);
                    muteswitch.channel=(uint8_t)safe_atoi(mxmlGetText(node, NULL));
                    node=mxmlFindElement(tree,tree,"subChannel", NULL, NULL, MXML_DESCEND);
                    muteswitch.subChannel=(int16_t)safe_atoi(mxmlGetText(node, NULL));
                    node=mxmlFindElement(tree,tree,"muteSwitch", NULL, NULL, MXML_DESCEND);
                    muteswitch.muteSwitch=(bool)safe_atoi(mxmlGetText(node, NULL));
                    return (void*)&muteswitch;
                }else if(methodcode==B_CODE_MID_FRQ_MUTE_THRE){/*静噪门限设置*/
                    node=mxmlFindElement(tree,tree,"channel", NULL, NULL, MXML_DESCEND);
                    noisethreshold.channel=(uint8_t)safe_atoi(mxmlGetText(node, NULL));
                    node=mxmlFindElement(tree,tree,"subChannel", NULL, NULL, MXML_DESCEND);
                    noisethreshold.subChannel=(int16_t)safe_atoi(mxmlGetText(node, NULL));
                    node=mxmlFindElement(tree,tree,"muteThreshold", NULL, NULL, MXML_DESCEND);
                    noisethreshold.muteThreshold=(int8_t)safe_atoi(mxmlGetText(node, NULL));
                    return (void*)&noisethreshold;
                }else if(methodcode==B_CODE_MID_FRQ_AU_SAMPLE_RATE){/*音频采样率设置参数*/
                    node=mxmlFindElement(tree,tree,"channel", NULL, NULL, MXML_DESCEND);
                    audiosamprate.channel=(uint8_t)safe_atoi(mxmlGetText(node, NULL));
                    node=mxmlFindElement(tree,tree,"subChannel", NULL, NULL, MXML_DESCEND);
                    audiosamprate.subChannel=(int16_t)safe_atoi(mxmlGetText(node, NULL));
                    node=mxmlFindElement(tree,tree,"audioSampleRate", NULL, NULL, MXML_DESCEND);
                    audiosamprate.audioSampleRate=(float)safe_atoi(mxmlGetText(node, NULL));
                    return (void*)&audiosamprate;
                }else {
                    printf_warn("In middle frequency parameter, there is no such parameter");
                    

                 }
                break;
            }

        case CLASS_CODE_RF:/*射频参数命令*/
            {
                if(methodcode==B_CODE_RF_FRQ_PARA){

                    
                    node=mxmlFindElement(tree,tree,"channel", NULL, NULL, MXML_DESCEND);
                    RFpara.channel=(uint8_t)safe_atoi(mxmlGetText(node, NULL));
                    node=mxmlFindElement(tree,tree,"rfModeCode", NULL, NULL, MXML_DESCEND);
                    RFpara.rfModeCode=(uint8_t)safe_atoi(mxmlGetText(node, NULL));
                    node=mxmlFindElement(tree,tree,"gainControlMethod", NULL, NULL, MXML_DESCEND);
                    RFpara.gainControlMethod=(uint8_t)safe_atoi(mxmlGetText(node, NULL));
                    node=mxmlFindElement(tree,tree,"mgcGainValue", NULL, NULL, MXML_DESCEND);
                    RFpara.mgcGainValue=(int8_t)safe_atoi(mxmlGetText(node, NULL));
                    node=mxmlFindElement(tree,tree,"agcControlTime", NULL, NULL, MXML_DESCEND);
                    RFpara.agcControlTime=(uint32_t)safe_atoi(mxmlGetText(node, NULL));
                    node=mxmlFindElement(tree,tree,"agcMidFreqOutLevel", NULL, NULL, MXML_DESCEND);
                    RFpara.agcMidFreqOutLevel=(int8_t)safe_atoi(mxmlGetText(node, NULL));
                    node=mxmlFindElement(tree,tree,"rfBandwidth", NULL, NULL, MXML_DESCEND);
                    RFpara.rfBandwidth=(uint32_t)safe_atoi(mxmlGetText(node, NULL));
                    node=mxmlFindElement(tree,tree,"antennaSelect", NULL, NULL, MXML_DESCEND);
                    RFpara.antennaSelect=(uint8_t)safe_atoi(mxmlGetText(node, NULL));
                    return (void*)&RFpara;
                }else if(methodcode==B_CODE_RF_FRQ_ANTENASELEC){
                        
                    node=mxmlFindElement(tree,tree,"channel", NULL, NULL, MXML_DESCEND);
                    RFantennaselect.channel=(uint8_t)safe_atoi(mxmlGetText(node, NULL));
                    node=mxmlFindElement(tree,tree,"antennaSelect", NULL, NULL, MXML_DESCEND);
                    RFantennaselect.antennaSelect=(int8_t)safe_atoi(mxmlGetText(node, NULL));
                    return (void*)&RFantennaselect;

                }else if(methodcode==B_CODE_RF_FRQ_OUTPUT_ATTENUATION){
                    node=mxmlFindElement(tree,tree,"channel", NULL, NULL, MXML_DESCEND);
                    Rfoutputattenuation.channel=(uint8_t)safe_atoi(mxmlGetText(node, NULL));;
                    node=mxmlFindElement(tree,tree,"rfOutputAttenuation", NULL, NULL, MXML_DESCEND);
                    Rfoutputattenuation.rfOutputAttenuation=(int8_t)safe_atoi(mxmlGetText(node, NULL));
                    return (void*)&Rfoutputattenuation;
                }else if(methodcode==B_CODE_RF_FRQ_INTPUT_ATTENUATION){
                    node=mxmlFindElement(tree,tree,"channel", NULL, NULL, MXML_DESCEND);
                    RfInputattenuation.channel=(uint8_t)safe_atoi(mxmlGetText(node, NULL));
                    node=mxmlFindElement(tree,tree,"rfInputAttenuation", NULL, NULL, MXML_DESCEND);
                    RfInputattenuation.rfInputAttenuation=(int8_t)safe_atoi(mxmlGetText(node, NULL));
                    return (void*)&RfInputattenuation;
                }else if(methodcode==B_CODE_RF_FRQ_INTPUT_BANDWIDTH){
                    node=mxmlFindElement(tree,tree,"channel", NULL, NULL, MXML_DESCEND);
                    Rfbdsetting.channel=(uint8_t)safe_atoi(mxmlGetText(node, NULL));
                    node=mxmlFindElement(tree,tree,"rfBandwidth", NULL, NULL, MXML_DESCEND);
                    Rfbdsetting.rfBandwidth=(uint32_t)safe_atoi(mxmlGetText(node, NULL));
                    return (void*)&Rfbdsetting;
                }else{
                    printf_warn("In rf parameter, there is no such parameter\n");
                   
                }
                 break;
                

            }

        case CLASS_CODE_NET:
            { 
                struct sockaddr_in saddr;
                char temp[30];
                char mactemp[30]={0};
                
                char *p=mactemp;

                char buff[20]={0};
                int j=0,k=0;
                char * str;      
    
    
                if(methodcode==B_CODE_ALL_NET)
                {
                     printf_debug("调试1\n");
                
                    node=mxmlFindElement(tree,tree,"mac", NULL, NULL, MXML_DESCEND);
                    printf_debug("调试2\n");
                    if( mxmlGetText(node, NULL)==NULL)
                    {
                        printf_warn("The XML you want to parse is empty, please check your Settings or XML file\n");
                        break;
                    }
                    sprintf(temp,"%s",mxmlGetText(node, NULL));
                    printf_debug("调试4\n");
                    for(i=0;i<30;i++)
                    {
                        if(temp[i]!=':')
                        {
                           *p=temp[i];
                           p++;
                        }
                    }
                     printf_debug("mactemp=%s\n",mactemp);
                    for(i=0;i<6;i++)
                    {
                       buff[0] = mactemp[k++];
                       buff[1] = mactemp[k++];
                       //printf_debug("buff :%s\n",buff);

                       netpara.mac[i] = strtol(buff, NULL, 16);
        
                       //printf_debug("netpara.mac[%d] :%x\n",i,netpara.mac[i]);
                    }

                    printf_debug("mactemp=%s\n",mactemp);
                    printf_debug("解析mac=%02x%02x%02x%02x%02x%02x\n",netpara.mac[0],netpara.mac[1],netpara.mac[2],netpara.mac[3],netpara.mac[4],netpara.mac[5]);




                    memset(temp,0,30);
                    memset(mactemp,0,30);
                    memset(buff,0,8);
                    j=0;k=0;
                    
                    node=mxmlFindElement(tree,tree,"gateway", NULL, NULL, MXML_DESCEND);
                    printf_debug("解析gateway = %s\n",mxmlGetText(node, NULL));

                    
                        
                    saddr.sin_addr.s_addr=inet_addr(mxmlGetText(node, NULL));
                    printf_debug("解析gateway mactemp=%x\n",saddr.sin_addr.s_addr);
                    netpara.gateway = saddr.sin_addr.s_addr;
                    printf_debug("解析gateway = %x\n",netpara.gateway);
                    
                    node=mxmlFindElement(tree,tree,"netmask", NULL, NULL, MXML_DESCEND);
                    saddr.sin_addr.s_addr=inet_addr(mxmlGetText(node, NULL));
                    netpara.netmask= saddr.sin_addr.s_addr;
                    printf_debug("解析netmask = %x\n",netpara.netmask);



                    
                    node=mxmlFindElement(tree,tree,"ipaddress", NULL, NULL, MXML_DESCEND);
                    saddr.sin_addr.s_addr=inet_addr(mxmlGetText(node, NULL));
                    netpara.ipaddress = saddr.sin_addr.s_addr;
                    printf_debug("解析ipaddress = %x\n",netpara.ipaddress);


                    node=mxmlFindElement(tree,tree,"port", NULL, NULL, MXML_DESCEND);
                    netpara.port=(uint16_t)atoi(mxmlGetText(node, NULL));
                    printf_debug("解析网络参数over\n");
                    return(void *) &netpara;

                }else{
                    printf_warn("There is no such parameter in the network parameter\n");
                   

                }
               break;
            }

        case CLASS_CODE_WORK_MODE:
            {
               if(methodcode==B_CODE_WK_MODE_MULTI_FRQ_POINT)
               {
                   j=0;
                   node = mxmlFindElement(tree, tree,"channel",NULL, NULL,MXML_DESCEND );
                   multifrepoint.cid=(uint8_t)safe_atoi(mxmlGetText(node, NULL));
                   node=mxmlFindElement(tree,tree,"windowType", NULL, NULL, MXML_DESCEND);
                   multifrepoint.window_type=(uint8_t)safe_atoi(mxmlGetText(node, NULL));
                   node=mxmlFindElement(tree,tree,"frameDropCnt", NULL, NULL, MXML_DESCEND);
                   multifrepoint.frame_drop_cnt=(uint8_t)safe_atoi(mxmlGetText(node, NULL));
                   node=mxmlFindElement(tree,tree,"smoothTimes", NULL, NULL, MXML_DESCEND);
                   multifrepoint.smooth_time=(uint8_t)safe_atoi(mxmlGetText(node, NULL));
                   node=mxmlFindElement(tree,tree,"residenceTime", NULL, NULL, MXML_DESCEND);
                   multifrepoint.residence_time=(uint32_t)safe_atoi(mxmlGetText(node, NULL));
                   node=mxmlFindElement(tree,tree,"residencePolicy", NULL, NULL, MXML_DESCEND);
                   multifrepoint.residence_policy=(int32_t)safe_atoi(mxmlGetText(node, NULL));
                   node=mxmlFindElement(tree,tree,"audioSampleRate", NULL, NULL, MXML_DESCEND);
                   multifrepoint.audio_sample_rate=safe_atof(mxmlGetText(node, NULL));//转化为浮点数
                   node=mxmlFindElement(tree,tree,"freqPointCnt", NULL, NULL, MXML_DESCEND);
                   multifrepoint.freq_point_cnt=(uint32_t)safe_atoi(mxmlGetText(node, NULL));
                    for (node = mxmlFindElement(tree, tree,"array",NULL, NULL,MXML_DESCEND);
                         node != NULL;
                         node =mxmlFindElement(node, tree,"array",NULL, NULL,MXML_NO_DESCEND))
                       {
                           for(i=0;i<MAX_SIGNAL_CHANNEL_NUM;i++)
                           {
                               if(safe_atoi(mxmlElementGetAttr(node,"index"))==i)
                                { 
                                    multifrepoint.points[j].index=(uint8_t)i;
                                    group=mxmlFindElement(node,tree,"centerFreq",NULL, NULL, MXML_DESCEND);
                                    multifrepoint.points[j].center_freq=(uint64_t)safe_atoi(mxmlGetText(group, NULL));
                                    group=mxmlFindElement(node,tree,"bandwith",NULL, NULL, MXML_DESCEND);
                                    multifrepoint.points[j].bandwidth=(uint64_t)safe_atoi(mxmlGetText(group, NULL));
                               
                                    group=mxmlFindElement(node,tree,"freqResolution",NULL, NULL, MXML_DESCEND);
                                    multifrepoint.points[j].freq_resolution=(uint64_t)safe_atof(mxmlGetText(group, NULL));
                                    group=mxmlFindElement(node,tree,"fftSize",NULL, NULL, MXML_DESCEND);
                                    multifrepoint.points[j].fft_size=(uint32_t)safe_atoi(mxmlGetText(group, NULL));
                                    group=mxmlFindElement(node,tree,"decMethodId",NULL, NULL, MXML_DESCEND);
                                    multifrepoint.points[j].d_method=(uint8_t)safe_atoi(mxmlGetText(group, NULL));
                                    group=mxmlFindElement(node,tree,"decBandwidth",NULL, NULL, MXML_DESCEND);
                                    multifrepoint.points[j].d_bandwith=(uint32_t)safe_atoi(mxmlGetText(group, NULL));
                   
                                    group=mxmlFindElement(node,tree,"muteSwitch",NULL, NULL, MXML_DESCEND);
                                    multifrepoint.points[j].noise_en=(uint8_t)safe_atoi(mxmlGetText(group, NULL));
                                    group=mxmlFindElement(node,tree,"muteThreshold",NULL, NULL, MXML_DESCEND);
                                    multifrepoint.points[j].noise_thrh=(int8_t)safe_atoi(mxmlGetText(group, NULL));
                                    j++;
                   
                              }
                          }
                      }
                printf_debug("channel=%d\n",multifrepoint.cid);
                printf_debug("windowType=%d\n",multifrepoint.window_type);
                printf_debug("frameDropCnt=%d\n",multifrepoint.frame_drop_cnt);
                printf_debug("smoothTimes=%d\n",multifrepoint.smooth_time);
                printf_debug("residenceTime=%d\n",multifrepoint.residence_time);
                printf_debug("residencePolicy=%d\n",multifrepoint.residence_policy);
                printf_debug("audioSampleRate=%f\n",multifrepoint.audio_sample_rate);
                printf_debug("freqPointCnt=%d\n",multifrepoint.freq_point_cnt);
                printf_debug("bandwidth=%d\n",multifrepoint.points[0].bandwidth);
                printf_debug("center_freq=%d\n",multifrepoint.points[0].center_freq);
                printf_debug("d_bandwith=%d\n",multifrepoint.points[0].d_bandwith);
                printf_debug("d_method=%d\n",multifrepoint.points[0].d_method);       
                printf_debug("fft_size=%d\n",multifrepoint.points[0].fft_size);
                printf_debug("freq_resolution=%f\n",multifrepoint.points[0].freq_resolution);
                printf_debug("index=%d\n",multifrepoint.points[0].index);
                printf_debug("noise_en=%d\n",multifrepoint.points[0].noise_en);
                printf_debug("noise_thrh=%d\n",multifrepoint.points[0].noise_thrh);
                return (void *)&multifrepoint;
            }else if(methodcode==B_CODE_WK_MODE_SUB_CH_DEC){    
               i=0;
               node = mxmlFindElement(tree, tree,"channel",NULL, NULL,MXML_DESCEND );
               subchanelpara.cid=(uint8_t)safe_atoi(mxmlGetText(node, NULL));
               node =mxmlFindElement(tree, tree,"audioSampleRate",NULL, NULL,MXML_DESCEND);
               subchanelpara.audio_sample_rate=safe_atof(mxmlGetText(node, NULL));
               node =mxmlFindElement(tree, tree,"subChannelNum",NULL, NULL,MXML_DESCEND);
               subchanelpara.sub_channel_num=(uint16_t)safe_atoi(mxmlGetText(node, NULL));

               for (node = mxmlFindElement(tree, tree,"array",NULL, NULL,MXML_DESCEND );
                node != NULL;
                node =mxmlFindElement(node, tree,"array",NULL, NULL,MXML_NO_DESCEND ))
                {
                    for(j=0;j<MAX_SIGNAL_CHANNEL_NUM;j++)
                    {
                        if(safe_atoi(mxmlElementGetAttr(node,"index"))==j)
                        {
                            subchanelpara.sub_ch[i].index=(uint8_t)j;
                            group=mxmlFindElement(node,tree,"centerFreq",NULL, NULL, MXML_DESCEND);
                            subchanelpara.sub_ch[i].center_freq=(uint64_t)safe_atoi(mxmlGetText(group, NULL));
                            group=mxmlFindElement(node,tree,"decBandwidth",NULL, NULL, MXML_DESCEND);
                            subchanelpara.sub_ch[i].d_bandwith=(uint64_t)safe_atoi(mxmlGetText(group, NULL));
                            group=mxmlFindElement(node,tree,"fftSize",NULL, NULL, MXML_DESCEND);
                            subchanelpara.sub_ch[i].fft_size=(uint64_t)safe_atoi(mxmlGetText(group, NULL));
                            group=mxmlFindElement(node,tree,"decMethodId",NULL, NULL, MXML_DESCEND);
                            subchanelpara.sub_ch[i].d_method=(uint64_t)safe_atoi(mxmlGetText(group, NULL));
                            group=mxmlFindElement(node,tree,"muteSwitch",NULL, NULL, MXML_DESCEND);
                            subchanelpara.sub_ch[i].noise_en=(uint64_t)safe_atoi(mxmlGetText(group, NULL));
                            group=mxmlFindElement(node,tree,"muteThreshold",NULL, NULL, MXML_DESCEND);
                            subchanelpara.sub_ch[i].noise_thrh=(uint64_t)safe_atoi(mxmlGetText(group, NULL));
                            i++;
                        }
                    }
                }

                
                printf_debug("channel=%d\n",subchanelpara.cid);
                printf_debug("audioSampleRate=%f\n",subchanelpara.audio_sample_rate);
                printf_debug("sub_channel_num=%d\n",subchanelpara.sub_channel_num);
                printf_debug("center_freq=%d\n",subchanelpara.sub_ch[0].center_freq);
                printf_debug("d_bandwith=%d\n",subchanelpara.sub_ch[0].d_bandwith);
                printf_debug("d_method=%d\n",subchanelpara.sub_ch[0].d_method);       
                printf_debug("fft_size=%d\n",subchanelpara.sub_ch[0].fft_size);
                printf_debug("index=%d\n",subchanelpara.sub_ch[0].index);
                printf_debug("noise_en=%d\n",subchanelpara.sub_ch[0].noise_en);
                printf_debug("noise_thrh=%d\n",subchanelpara.sub_ch[0].noise_thrh);
                return (void *)&subchanelpara;
            }else if(methodcode==B_CODE_WK_MODE_MULTI_FRQ_FREGMENT){

                i=0;
                node = mxmlFindElement(tree, tree,"channel",NULL, NULL,MXML_DESCEND );
                freqfergmentpara.cid=(uint8_t)safe_atoi(mxmlGetText(node, NULL));
                node =mxmlFindElement(tree, tree,"windowType",NULL, NULL,MXML_DESCEND);
                freqfergmentpara.window_type=(uint8_t)safe_atoi(mxmlGetText(node, NULL));
                node =mxmlFindElement(tree, tree,"frameDropCnt",NULL, NULL,MXML_DESCEND);
                freqfergmentpara.frame_drop_cnt=(uint8_t)safe_atoi(mxmlGetText(node, NULL));
                node =mxmlFindElement(tree, tree,"smoothTimes",NULL, NULL,MXML_DESCEND);
                freqfergmentpara.smooth_time=(uint8_t)safe_atoi(mxmlGetText(node, NULL));
                node =mxmlFindElement(tree, tree,"freqSegmentCnt",NULL, NULL,MXML_DESCEND);
                freqfergmentpara.freq_segment_cnt=(uint8_t)safe_atoi(mxmlGetText(node, NULL));

                for (node = mxmlFindElement(tree, tree,"array",NULL, NULL,MXML_DESCEND );
                 node != NULL;
                 node =mxmlFindElement(node, tree,"array",NULL, NULL,MXML_NO_DESCEND ))
                 {
                     for(j=0;j<MAX_SIGNAL_CHANNEL_NUM;j++)
                     {
                         if(safe_atoi(mxmlElementGetAttr(node,"index"))==j)
                         {
                             freqfergmentpara.fregment[i].index=(uint8_t)j;
                             group=mxmlFindElement(node,tree,"startFrequency",NULL, NULL, MXML_DESCEND);
                             freqfergmentpara.fregment[i].start_freq=(uint64_t)safe_atoi(mxmlGetText(group, NULL));
                             group=mxmlFindElement(node,tree,"endFrequency",NULL, NULL, MXML_DESCEND);
                             freqfergmentpara.fregment[i].end_freq=(uint64_t)safe_atoi(mxmlGetText(group, NULL));
                             group=mxmlFindElement(node,tree,"step",NULL, NULL, MXML_DESCEND);
                             freqfergmentpara.fregment[i].step=(uint32_t)safe_atoi(mxmlGetText(group, NULL));
                             group=mxmlFindElement(node,tree,"freqResolution",NULL, NULL, MXML_DESCEND);
                             freqfergmentpara.fregment[i].freq_resolution=safe_atof(mxmlGetText(group, NULL));
                             group=mxmlFindElement(node,tree,"fftSize",NULL, NULL, MXML_DESCEND);
                             freqfergmentpara.fregment[i].fft_size=(uint32_t)safe_atoi(mxmlGetText(group, NULL));
                             i++;

                         }
                     }
                 }
                 return &freqfergmentpara;
            }else{
                printf_warn("There is no such parameter in the center frequency parameter\n");
                
            }
            break;    
         }
           
        case CLASS_CODE_CONTROL:
            {
                if(methodcode == B_CODE_CONTROL_CONTR_DATA_OUTPUT_ENABLE)
                {
                    node = mxmlFindElement(tree, tree,"channel",NULL, NULL,MXML_DESCEND);
                    outputenablecontr.channel=(uint8_t)safe_atoi(mxmlGetText(node , NULL));
                    node =mxmlFindElement(tree, tree,"subChannel",NULL, NULL,MXML_DESCEND);
                    outputenablecontr.subChannel=(int16_t)safe_atoi(mxmlGetText(node, NULL));
                    node =mxmlFindElement(tree, tree,"psdEnable",NULL, NULL,MXML_DESCEND);
                    outputenablecontr.psdEnable=(bool)safe_atoi(mxmlGetText(node, NULL));
                    node =mxmlFindElement(tree, tree,"audioEnable",NULL, NULL,MXML_DESCEND);
                    outputenablecontr.audioEnable=(bool)safe_atoi(mxmlGetText(node, NULL));
                    node =mxmlFindElement(tree, tree,"IQEnable",NULL, NULL,MXML_DESCEND);
                    outputenablecontr.IQEnable=(bool)safe_atoi(mxmlGetText(node, NULL));
                    node =mxmlFindElement(tree, tree,"spectrumAnalysisEn",NULL, NULL,MXML_DESCEND);
                    outputenablecontr.spectrumAnalysisEn=(bool)safe_atoi(mxmlGetText(node, NULL));
                    node =mxmlFindElement(tree, tree,"directionEn",NULL, NULL,MXML_DESCEND);
                    outputenablecontr.directionEn=(bool)safe_atoi(mxmlGetText(node, NULL));
                    return (void*)&outputenablecontr;

                }else if(methodcode==B_CODE_CONTROL_CONTR_CALIBRATION_CONTR){
                    node = mxmlFindElement(tree, tree,"channel",NULL, NULL,MXML_DESCEND);
                    calibratcontrol.channel=(uint8_t)safe_atoi(mxmlGetText(node , NULL));
                    node = mxmlFindElement(tree, tree,"ctrlMode",NULL, NULL,MXML_DESCEND);
                    calibratcontrol.ctrlMode=(uint8_t)safe_atoi(mxmlGetText(node , NULL));
                    node = mxmlFindElement(tree, tree,"enable",NULL, NULL,MXML_DESCEND);
                    calibratcontrol.enable=(bool)safe_atoi(mxmlGetText(node , NULL));
                    node = mxmlFindElement(tree, tree,"midFreq",NULL, NULL,MXML_DESCEND);
                    calibratcontrol.midFreq=(uint64_t)safe_atoi(mxmlGetText(node , NULL));
                    node = mxmlFindElement(tree, tree,"refPowerLevel",NULL, NULL,MXML_DESCEND);
                    calibratcontrol.refPowerLevel=(uint32_t)safe_atoi(mxmlGetText(node , NULL));
                    return (void*)&calibratcontrol;
                }else if(methodcode==B_CODE_CONTROL_CONTR_LOCAL_REMOTE){
                    node = mxmlFindElement(tree, tree,"ctrlMode",NULL, NULL,MXML_DESCEND);
                    Localremotecontr.ctrlMode=(uint8_t)safe_atoi(mxmlGetText(node , NULL));
                    return (void*)&Localremotecontr;
                }else if(methodcode==B_CODE_CONTROL_CONTR_CHANNEL_POWER){
                    j=0;
                    for (node = mxmlFindElement(tree, tree,"channelNode",NULL, NULL,MXML_DESCEND );
                         node != NULL;
                         node =mxmlFindElement(node, tree,"channelNode",NULL, NULL,MXML_NO_DESCEND ))
                        {
                            for(i=0;i<MAX_SIGNAL_CHANNEL_NUM;i++)
                            {
                                if(safe_atoi(mxmlElementGetAttr(node,"index"))==i)
                                {
                                    channelpowercontrol[j].channelNode.channel=(uint8_t)i;
                                    group = mxmlFindElement(node, tree,"enable",NULL, NULL,MXML_DESCEND);
                                    channelpowercontrol[j].channelNode.enable=(bool)safe_atoi(mxmlGetText(group, NULL));
                                    j++;
                                }
                            }


                        }
                     for(i=0;i<2;i++)
                     {
                        printf_debug("channelpowercontrol[%d].channelNode.channel=%d\n",i,channelpowercontrol[i].channelNode.channel);
                        printf_debug("channelpowercontrol[%d].channelNode.enable=%d\n",i,channelpowercontrol[i].channelNode.enable);


                     }
                    return (void*)&channelpowercontrol;
                }else if(methodcode==B_CODE_CONTROL_CONTR_TIME_CONTR){
                    node = mxmlFindElement(tree, tree,"timeStamp",NULL, NULL,MXML_DESCEND );
                    timecontrol.timeStamp=mxmlGetText(node , NULL);
                    node = mxmlFindElement(tree, tree,"timeZone",NULL, NULL,MXML_DESCEND );
                    timecontrol.timeZone=mxmlGetText(node , NULL);
                    return (void*)&timecontrol;

                }else{
                    printf_warn("There are no such parameters in the control parameters.\n"); 
                  
                }
                 break;
            }
        case CLASS_CODE_STATUS:
            {

                if(methodcode==B_CODE_STATE_GET_EQUIP_BASIC_INFOR){
                    node = mxmlFindElement(tree, tree,"softVersion",NULL, NULL,MXML_DESCEND );
                    group= mxmlFindElement(node, tree,"app",NULL, NULL,MXML_DESCEND );
                    Epbasicinfor.softVersion.app=mxmlGetText(group, NULL);
                    group= mxmlFindElement(node, tree,"kernel",NULL, NULL,MXML_DESCEND );
                    Epbasicinfor.softVersion.kernel=mxmlGetText(group, NULL);
                    group= mxmlFindElement(node, tree,"uboot",NULL, NULL,MXML_DESCEND );
                    Epbasicinfor.softVersion.uboot=mxmlGetText(group, NULL);
                    group= mxmlFindElement(node, tree,"fpga",NULL, NULL,MXML_DESCEND );
                    Epbasicinfor.softVersion.fpga=mxmlGetText(group, NULL);

                    node = mxmlFindElement(tree, tree,"diskInfo",NULL, NULL,MXML_DESCEND );
                    group = mxmlFindElement(node, tree,"diskNum",NULL, NULL,MXML_DESCEND );
                    Epbasicinfor.diskInfo.diskNum=(uint16_t)safe_atoi (mxmlGetText(group, NULL));
                    group = mxmlFindElement(node, tree,"diskNode",NULL, NULL,MXML_DESCEND );
                    child= mxmlFindElement(group, tree,"totalSpace",NULL, NULL,MXML_DESCEND );
                    Epbasicinfor.diskInfo.diskNode.totalSpace=(uint32_t)safe_atoi (mxmlGetText(child, NULL));
                    child= mxmlFindElement(group, tree,"freeSpace",NULL, NULL,MXML_DESCEND );
                    Epbasicinfor.diskInfo.diskNode.freeSpace=(uint32_t)safe_atoi (mxmlGetText(child, NULL));
                    
                    node = mxmlFindElement(tree, tree,"clkInfo",NULL, NULL,MXML_DESCEND );
                    group = mxmlFindElement(node, tree,"inout",NULL, NULL,MXML_DESCEND );
                    Epbasicinfor.clkInfo.inout=(uint16_t)safe_atoi(mxmlGetText(group, NULL)); 
                    group = mxmlFindElement(node, tree,"status",NULL, NULL,MXML_DESCEND );
                    Epbasicinfor.clkInfo.status=(uint16_t)safe_atoi(mxmlGetText(group, NULL)); 
                    group = mxmlFindElement(node, tree,"frequency",NULL, NULL,MXML_DESCEND );
                    Epbasicinfor.clkInfo.frequency=(uint32_t)safe_atoi(mxmlGetText(group, NULL));


                    node = mxmlFindElement(tree, tree,"adInfo",NULL, NULL,MXML_DESCEND );
                    group = mxmlFindElement(node, tree,"status",NULL, NULL,MXML_DESCEND );
                    Epbasicinfor.adInfo.status=(uint8_t)safe_atoi(mxmlGetText(group, NULL)); 

                    node = mxmlFindElement(tree, tree,"rfInfo",NULL, NULL,MXML_DESCEND );
                    group = mxmlFindElement(node, tree,"rfnum",NULL, NULL,MXML_DESCEND );
                    Epbasicinfor.rfInfo.rfnum=(uint8_t)safe_atoi(mxmlGetText(group, NULL)); 
                    group = mxmlFindElement(node, tree,"rfnode",NULL, NULL,MXML_DESCEND );
                    child = mxmlFindElement(group, tree,"status",NULL, NULL,MXML_DESCEND );
                    Epbasicinfor.rfInfo.rfnode.status=(uint8_t)safe_atoi(mxmlGetText(child, NULL));
                    child = mxmlFindElement(group, tree,"temprature",NULL, NULL,MXML_DESCEND );
                    Epbasicinfor.rfInfo.rfnode.temprature=(uint16_t)safe_atoi(mxmlGetText(child, NULL));


                    node = mxmlFindElement(tree, tree,"fpgaInfo",NULL, NULL,MXML_DESCEND );

                    group = mxmlFindElement(node, tree,"temprature",NULL, NULL,MXML_DESCEND );

                    Epbasicinfor.fpgaInfo.temprature=(uint16_t)safe_atoi(mxmlGetText(group, NULL)); 

                    return (void *)&Epbasicinfor;
                }else if(methodcode==B_CODE_STATE_CHANNEL_STATUS_QUERY){
                    i=0;j=0;

                    node = mxmlFindElement(tree, tree,"channelNum",NULL, NULL,MXML_DESCEND );
                    channelstatus.channelNum=(bool)safe_atoi(mxmlGetText(node , NULL));
                    for (node = mxmlFindElement(tree, tree,"channelNode",NULL, NULL,MXML_DESCEND );
                         node != NULL;
                         node =mxmlFindElement(node, tree,"channelNode",NULL, NULL,MXML_NO_DESCEND))
                        {
                            for(i=0;i<MAX_SIG_CHANNLE;i++)
                            {
                                if(safe_atoi(mxmlElementGetAttr(node,"index"))==i){

                                    channelstatus.channelNode[j].channel=(uint8_t)i;
                                    group = mxmlFindElement(node, tree,"channelStatus",NULL, NULL,MXML_DESCEND);
                                    channelstatus.channelNode[j].channelStatus=(uint8_t)safe_atoi(mxmlGetText(group , NULL));
                                    group = mxmlFindElement(node, tree,"channelSignal",NULL, NULL,MXML_DESCEND);
                                    channelstatus.channelNode[j].channelSignal=(uint32_t)safe_atoi(mxmlGetText(group, NULL));
                                    
                                    j++;
                                }
                            }
                        }

                         
                   return (void *)&channelstatus;
                }else if(methodcode==B_CODE_STATE_POWER_STATUS_QUERY){
                    node = mxmlFindElement(tree, tree,"powerStatus",NULL, NULL,MXML_DESCEND);
                    power_status.powerStatus=(uint8_t)safe_atoi(mxmlGetText(node, NULL));
                    return (void *)&power_status;
                }else if(methodcode==B_CODE_STATE_STORAGE_STATUS_QUERY){
                    node = mxmlFindElement(tree, tree,"diskNum",NULL, NULL,MXML_DESCEND);
                    storagestate.diskNum=(uint8_t)safe_atoi(mxmlGetText(node, NULL));
                    node = mxmlFindElement(tree, tree,"diskNode",NULL, NULL,MXML_DESCEND);
                    group = mxmlFindElement(node, tree,"totalSpace",NULL, NULL,MXML_DESCEND);
                    storagestate.diskNode.totalSpace=(uint32_t)safe_atoi(mxmlGetText(group, NULL));
                    group = mxmlFindElement(node, tree,"freeSpace",NULL, NULL,MXML_DESCEND);
                    storagestate.diskNode.freeSpace=(uint32_t)safe_atoi(mxmlGetText(group, NULL));
                    return (void *)&storagestate;
                }else if(methodcode==B_CODE_STATE_GET_SOFTWARE_VERSION){
                    node = mxmlFindElement(tree, tree,"softVersion",NULL, NULL,MXML_DESCEND );
                    group= mxmlFindElement(node, tree,"app",NULL, NULL,MXML_DESCEND );
                    softwareversion.app=mxmlGetText(group, NULL);
                    group= mxmlFindElement(node, tree,"kernel",NULL, NULL,MXML_DESCEND );
                    softwareversion.kernel=mxmlGetText(group, NULL);
                    group= mxmlFindElement(node, tree,"uboot",NULL, NULL,MXML_DESCEND );
                    softwareversion.uboot=mxmlGetText(group, NULL);
                    group= mxmlFindElement(node, tree,"fpga",NULL, NULL,MXML_DESCEND );
                    softwareversion.fpga=mxmlGetText(group, NULL);
                    return (void *)&softwareversion;
               }else{

                    printf_warn("This parameter does not exist in the device information\n");
                    

               }
               break;
            }

        default:
        

            break;
    }
    printf_debug("save parse data over\n");
    return NULL;
}




