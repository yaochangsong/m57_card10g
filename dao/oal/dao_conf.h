#ifndef _DAO_CONF_H_
#define _DAO_CONF_H_

#include "config.h"

#define MAX_SIG_CHANNLE 128
#define MAX_SIGNAL_CHANNEL_NUM (16)


#define XMLFILENAME DEFAULT_CONFIGFILE




//void dao_conf_save_batch(exec_cmd cmd, uint8_t type, s_config *config);

void* dao_conf_parse_batch(uint8_t classcode, uint8_t methodcode, char *data);
/*1      中频参数*/

/*静噪开关设置*/
struct dao_mute_switch{
    uint8_t channel;
    int16_t subChannel;
    bool muteSwitch;
}__attribute__ ((packed));

/*静噪门限设置*/
struct dao_noise_threshold{
    uint8_t channel;
    int16_t   subChannel; 
    int8_t muteThreshold;
}__attribute__ ((packed));

/*解调方式参数*/
struct dao_demodulation_way{
    uint8_t channel;
    uint8_t decMethodId; 
}__attribute__ ((packed));

/*音频采样率设置参数*/
struct dao_audio_sampling_rate{
    uint8_t channel;
    int16_t subChannel; 
    float audioSampleRate;
}__attribute__ ((packed));

/*2        射频参数设置*/
/*射频参数设置*/
struct dao_RF_parameters{
    uint8_t channel;
    uint8_t rfModeCode;
    uint8_t gainControlMethod;
    int8_t mgcGainValue;
    uint32_t agcControlTime;
    int8_t agcMidFreqOutLevel;
    uint32_t rfBandwidth;
    uint8_t antennaSelect;
}__attribute__ ((packed));


/*射频天线选择*/

struct dao_RF_antenna_selection{
    uint8_t channel;
    int8_t antennaSelect;
}__attribute__ ((packed));

/*射频输出衰减*/
struct dao_Rf_output_attenuation{
    uint8_t channel;
    int8_t rfOutputAttenuation;
}__attribute__ ((packed));


/*射频输入衰减*/
struct dao_Rf_Input_attenuation{
    uint8_t channel;
    int8_t rfInputAttenuation;
}__attribute__ ((packed));


/*射频带宽设置*/
struct dao_Rf_bandwidth_setting{
    uint8_t channel;
    uint32_t rfBandwidth;
}__attribute__ ((packed));




/*3     控制参数*/
/*数据输出使能控制*/


struct dao_data_output_enable_control{
    uint8_t channel;
    int16_t subChannel;
    bool    psdEnable;
    bool    audioEnable;
    bool    IQEnable;
    bool    spectrumAnalysisEn;
    bool    directionEn;
}__attribute__ ((packed));

/*校准控制*/
struct dao_calibration_control{
    uint8_t channel;
    uint8_t ctrlMode;
    bool    enable;
    uint64_t midFreq;  //String 
    uint32_t refPowerLevel;//?
}__attribute__ ((packed));

/*本远控制*/
struct dao_Local_remote_control{
    uint8_t ctrlMode;
}__attribute__ ((packed));

/*通道电源控制*/


struct dao_power_control{
    uint8_t channel;
    bool    enable;
}__attribute__ ((packed));

struct dao_channel_power_control{
    struct dao_power_control channelNode;//?
}__attribute__ ((packed));



/*时间控制*/

struct dao_time_control{
    const char* timeStamp;
    const char* timeZone;
}__attribute__ ((packed));

/*状态参数*/

/*4 获取设备基本信息*/


struct dao_soft_version{
   const  char* app;
   const  char* kernel;
   const  char* uboot;
   const  char* fpga;
}__attribute__ ((packed));           //

struct dao_disk_Node{
    uint32_t totalSpace;
    uint32_t freeSpace;
}__attribute__ ((packed));

struct dao_disk_Info{
    uint16_t diskNum; //?
    struct dao_disk_Node diskNode;
}__attribute__ ((packed));     

struct dao_clk_Info{
    uint8_t inout;
    uint8_t  status;
    uint32_t frequency;
}__attribute__ ((packed)); 

struct dao_ad_Info{
    uint8_t status;
}__attribute__ ((packed)); 


struct dao_rf_node{
    uint8_t status;
    uint16_t   temprature;      
}__attribute__ ((packed));    

struct dao_rf_Info{
    uint8_t rfnum;
    struct dao_rf_node  rfnode;//struct rf_node  rfnode[MAX_SIG_CHANNLE];
}__attribute__ ((packed));  


struct dao_fpga_Info{
    uint16_t temprature;
}__attribute__ ((packed));



struct dao_Equipment_basic_infor{
    struct dao_soft_version softVersion;
    struct dao_disk_Info diskInfo;
    struct dao_clk_Info  clkInfo;
    struct dao_ad_Info   adInfo;  
    struct dao_rf_Info   rfInfo;
    struct dao_fpga_Info  fpgaInfo;
    
}__attribute__ ((packed));


/*通道状态查询*/
struct dao_channel_Node{
    uint8_t channel;
    uint8_t   channelStatus; 
    uint32_t  channelSignal;
}__attribute__ ((packed));  

struct dao_Channel_status_check{
    uint8_t channelNum;
    struct dao_channel_Node  channelNode[MAX_SIG_CHANNLE];
}__attribute__ ((packed)); 

struct dao_Check_power_status{
    uint8_t powerStatus;
}__attribute__ ((packed));


struct dao_Query_storage_state{
    uint16_t diskNum;
    struct dao_disk_Node diskNode;
}__attribute__ ((packed));  


struct dao_software_version_infor{
    const char* app;
    const char* kernel;
    const char* uboot;
    const  char* fpga;

}__attribute__ ((packed));  


/* 子通道解调参数 */

struct dao_freq_points_st{
    int16_t index;
    uint64_t center_freq; /* rf */
    uint32_t d_bandwith;
    volatile uint32_t fft_size;
    uint8_t d_method;
    uint8_t  noise_en;
    int8_t noise_thrh;
}__attribute__ ((packed));



struct dao_sub_channel_freq_para_st{
    uint8_t cid;
    float audio_sample_rate;
    uint16_t sub_channel_num;
    struct dao_freq_points_st  sub_ch[MAX_SIGNAL_CHANNEL_NUM];
}__attribute__ ((packed));




#endif


