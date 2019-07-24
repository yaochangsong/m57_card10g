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
#include "dao_conf.h"




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
        printf_debug("Only supports fixed frequency mode, not multi-frequency point!");

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
                printf_debug("Unsupported parameter modification");
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
                printf_debug("Unsupported parameter modification");

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

                printf_debug("Unsupported parameter modification");

            }
            break;    

        

        case EX_WORK_MODE_CMD:
            if(type==EX_FIXED_FREQ_ANYS_MODE)/*定频模式*/
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

                printf_debug("Unsupported parameter modification");


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



