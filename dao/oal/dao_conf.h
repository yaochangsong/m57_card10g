#ifndef _DAO_CONF_H_
#define _DAO_CONF_H_

#include "config.h"

#define XMLFILENAME DEFAULT_CONFIGFILE

#define MAX_SIG_CHANNLE 8
void dao_conf_save_batch(exec_cmd cmd, uint8_t type, s_config *config);

/*1      中频参数*/

/*静噪开关设置*/
struct mute_switch{
    uint8_t channel;
    int16_t subChannel;  //?
    bool muteSwitch;
}__attribute__ ((packed));

/*静噪门限设置*/
struct noise_threshold{
    uint8_t channel;
    int16_t   subChannel; 
    int8_t muteThreshold;
}__attribute__ ((packed));

/*解调方式参数*/
struct demodulation_way{
    uint8_t channel;
    uint8_t decMethodId; 
}__attribute__ ((packed));

/*音频采样率设置参数*/
struct audio_sampling_rate{
    uint8_t channel;
    int16_t subChannel; 
    float audioSampleRate;
}__attribute__ ((packed));

/*2        射频参数设置*/
/*射频参数设置*/
struct RF_parameters{
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

struct RF_antenna_selection{
    uint8_t channel;
    int8_t antennaSelect;
}__attribute__ ((packed));

/*射频输出衰减*/
struct Rf_output_attenuation{
    uint8_t channel;
    int8_t rfOutputAttenuation;
}__attribute__ ((packed));


/*射频输入衰减*/
struct Rf_Input_attenuation{
    uint8_t channel;
    int8_t rfInputAttenuation;
}__attribute__ ((packed));


/*射频带宽设置*/
struct Rf_bandwidth_setting{
    uint8_t channel;
    uint32_t rfInputAttenuation;
}__attribute__ ((packed));




/*3     数据输出使能控制*/

struct data_output_enable_control{
    uint8_t channel;
    int16_t subChannel;
    bool    psdEnable;
    bool    audioEnable;
    bool    IQEnable;
    bool    spectrumAnalysisEn;
    bool    directionEn;
}__attribute__ ((packed));

/*校准控制*/
struct calibration_control{
    uint8_t channel;
    uint8_t ctrlMode;
    bool    enable;
    char* midFreq;  //String 
    uint32_t refPowerLevel;//?
}__attribute__ ((packed));

/*本远控制*/
struct Local_remote_control{
    uint8_t ctrlMode;
}__attribute__ ((packed));

/*通道电源控制*/


struct power_control{
    uint8_t channel;
    bool    enable;
}__attribute__ ((packed));

struct Channel_power_control{
    struct power_control channelNode[MAX_SIG_CHANNLE];//?
}__attribute__ ((packed));



/*时间控制*/

struct time_control{
    char* timeStamp;
    char* timeZone;
}__attribute__ ((packed));

/*状态参数*/

/*获取设备基本信息*/


struct soft_version{
    char* app;
    char* kernel;
    char* ubootp;
    char* fpga;
}__attribute__ ((packed));           //

struct disk_Node{
    char* totalSpace;
    char* freeSpace;
}__attribute__ ((packed));

struct disk_Info{
    uint16_t diskNum; //?
    struct disk_Node diskNode;
}__attribute__ ((packed));     

struct clk_Info{
    uint8_t inout;
    uint8_t  status;
    char* frequency;
}__attribute__ ((packed)); 

struct ad_Info{
    uint8_t status;
}__attribute__ ((packed)); 


struct rf_node{
    uint8_t status;
    char*   temprature;      
}__attribute__ ((packed));    

struct rf_Info{
    uint8_t rfnum;
    //struct rf_node  rfnode[MAX_SIG_CHANNLE];
}__attribute__ ((packed));  


struct fpga_Info{
    char* temprature;
}__attribute__ ((packed));



struct Equipment_basic_infor{
    struct soft_version softVersion;
    struct disk_Info diskInfo;
    struct clk_Info  clkInfo;
    struct ad_Info   adInfo;  
    struct rf_Info   rfInfo;
    struct fpga_Info  fpgaInfo;
    
}__attribute__ ((packed));


/*通道状态查询*/
struct channel_Node{
    uint8_t channel;
    uint8_t   channelStatus; 
    uint32_t  channelSignal;
}__attribute__ ((packed));  

struct Channel_status_check{
    uint8_t channelNum;
    //struct channel_Node  channelNode[MAX_SIG_CHANNLE];
}__attribute__ ((packed)); 

struct Check_power_status{
    uint8_t powerStatus;
}__attribute__ ((packed));


struct Query_storage_state{
    char* diskNum;
    struct disk_Node diskNode[MAX_SIG_CHANNLE];
}__attribute__ ((packed));  


struct software_version_infor{
    char* app;
    char* kernel;
    char* uboot;
    char* fpga;

}__attribute__ ((packed));  







#endif


