#ifndef _PROTOCOL_OAL_H_
#define _PROTOCOL_OAL_H_

//#include "config.h"
#define FILE_PATH_MAX_LEN 256

/* 工作模式参数参数 */
typedef enum _work_mode_type {
    OAL_NULL_MODE               = 0xff,
    OAL_FIXED_FREQ_ANYS_MODE    = 0x00,
    OAL_FAST_SCAN_MODE          = 0x01,
    OAL_MULTI_ZONE_SCAN_MODE    = 0x02,
    OAL_MULTI_POINT_SCAN_MODE   = 0x03,
}work_mode_type;

/* bit_en：内部使能位定义 */
enum {
    PSD_EN_BIT_OFFSET           = 0x00,
    AUDIO_EN_BIT_OFFSET         = 0x01,
    IQ_EN_BIT_OFFSET            = 0x02,
    SPEC_ANALY_EN_BIT_OFFSET    = 0x03,
    DIRECTION_EN_BIT_OFFSET     = 0x04,
};

typedef enum _ctrl_mode_param {	
    CTRL_MODE_LOCAL   = 0,
    CTRL_MODE_REMOTE  = 1,
}ctrl_mode_param;

    
#define INTERNEL_ENABLE_BIT_SET(en, s) \
    (en=(s.psd_en<<PSD_EN_BIT_OFFSET|s.audio_en<<AUDIO_EN_BIT_OFFSET|s.iq_en<<IQ_EN_BIT_OFFSET|s.spec_analy_en<<SPEC_ANALY_EN_BIT_OFFSET|s.direction_en<<DIRECTION_EN_BIT_OFFSET))
    
/* 输出使能 */
struct output_en_st{
    volatile uint8_t cid;
    volatile int8_t  sub_id;
    volatile uint8_t  psd_en;
    volatile uint8_t  audio_en;
    volatile uint8_t  iq_en;
    volatile uint8_t  spec_analy_en;
    volatile uint8_t  direction_en;
    volatile uint8_t  cali_source_en;
    /*        bit_en: 8bit
    bit[7]---bit[6]---bit[5]------bit[4]--------bit[3]--------bit[2]-----bit[1]------bit[0]
     **-----**-------**-- [direction_en]--[spec_analy_en]--[iq_en]--[audio_en]--[psd_en]
    */
    volatile uint8_t  bit_en;
    volatile bool  bit_reset;
};//__attribute__ ((packed));

/* 频点参数 */
struct freq_points_st{
    int16_t index;
    uint8_t  noise_en;  /* mute_switch 静噪开关 */
    int8_t noise_thrh;
    uint8_t raw_d_method; /* Original demodulation */
    uint8_t d_method;
    float freq_resolution;
    volatile uint32_t fft_size;
    uint32_t d_bandwith;
    uint64_t center_freq; /* middle_freq 中心频率 */
    uint64_t bandwidth;   /* rf */
    int16_t audio_volume;   /* 音频音量 */
};//__attribute__ ((packed));


/* 多频点扫描参数 */
struct multi_freq_point_para_st{
    uint8_t cid;
    uint8_t window_type;
    uint8_t frame_drop_cnt;
    uint16_t smooth_time;
    uint32_t residence_time;
    int32_t residence_policy;
    float audio_sample_rate;
    uint32_t freq_point_cnt;
    struct freq_points_st  points[MAX_SIG_CHANNLE];
};//__attribute__ ((packed));


/* 子通道解调参数 */
struct sub_channel_freq_para_st{
    uint8_t cid;
    uint8_t frame_drop_cnt;
    uint16_t sub_channel_num;
    float audio_sample_rate;
    struct output_en_st sub_ch_enable[MAX_SIGNAL_CHANNEL_NUM];
    struct freq_points_st  sub_ch[MAX_SIGNAL_CHANNEL_NUM];
};//__attribute__ ((packed));


/* 频段参数 */
struct freq_fregment_para_st{
    int16_t  index;
    uint32_t  step;
    float freq_resolution;
    uint32_t fft_size;
    uint64_t start_freq;
    uint64_t end_freq;
};//__attribute__ ((packed));

/* 多频段扫描参数 */
struct multi_freq_fregment_para_st{
    uint8_t cid;
    uint8_t window_type;
    uint8_t frame_drop_cnt;
    uint16_t smooth_time;
    uint32_t freq_segment_cnt;
    struct freq_fregment_para_st  fregment[MAX_SIG_CHANNLE];
};//__attribute__ ((packed));


struct db_rang_st{
    int32_t start;
    int32_t end;
};

#define RF_MODE_NUMBER 3
struct  rf_mode_param_st{
    int32_t mode[RF_MODE_NUMBER];
    int32_t mag[RF_MODE_NUMBER];
    struct db_rang_st rf_attenuation[RF_MODE_NUMBER];
    struct db_rang_st mgc_attenuation[RF_MODE_NUMBER];
    struct db_rang_st detection[RF_MODE_NUMBER];
};

/* 射频参数 */
struct rf_para_st{
    uint8_t cid;
    volatile uint8_t rf_mode_code;           /* 射频模式码; 0：低失真 1：常规 2：低噪声 */
    volatile uint8_t gain_ctrl_method;       /* 增益方法; 0：手动控制（MGC） 1：自动控制（AGC）*/
    volatile int8_t mgc_gain_value;          /* MGC 增益值; 单位 dB，精度 1dB*/
    volatile int8_t agc_mid_freq_out_level;  /* AGC 中频 输出幅度;单位，dBm 默认值：-10dBm*/
    uint8_t antennas_elect;         /* 天线选择 */
    int8_t  attenuation;            /* 射频衰减 ;  -100 至 120 单位 dB，精度 1dB*/
    int16_t temperature;            /* 射频温度 */
    volatile uint32_t agc_ctrl_time;         /* AGC 控制时间; 单位：10 微秒 快速：100 微秒  中速：1000 微秒 慢速：10000 微秒*/
    uint32_t mid_bw;                /* 射频中频带宽; 0~2^32 */
    uint64_t mid_freq;              /* 中心频率 */
    int32_t agc_ref_val_0dbm;       /* AGC: 0DB 主通道读取值 */
    int32_t subch_ref_val_0dbm;       /* 多频点模式下: 0DB 主通道读取值  */
    struct  rf_mode_param_st rf_mode;  /* 射频模式参数 */
};//__attribute__ ((packed));

typedef enum _rf_ctrl_method_val{
    POAL_LOW_DISTORTION = 0,   //低失真
    POAL_NORMAL,               //常规
    POAL_LOW_NOISE,            //低噪声
}rf_ctrl_method_val;

typedef enum _rf_gain_mode_val{
    POAL_MGC_MODE = 0,          //手动增益
    POAL_AGC_MODE,              //自动增益
}rf_gain_mode_val;


/* 控制参数 */
struct ctrl_st{
    
}__attribute__ ((packed));


/* 网络参数 */
struct network_addr_st{
    uint8_t mac[6];
    uint32_t ipaddress;
    uint32_t netmask;
    uint32_t gateway;
}__attribute__ ((packed));

struct network_st{
    uint16_t port;
    uint16_t data_port;
    struct network_addr_st addr;
}__attribute__ ((packed));

/* 频谱分析控制参数 */
struct specturm_analysis_control_st{
    uint32_t bandwidth_hz;        /* 频谱分析带宽*/
    uint64_t frequency_hz;        /* 频谱分析频率点 */
};//__attribute__ ((packed));


struct residency_policy{
    int32_t policy[MAX_RADIO_CHANNEL_NUM];
};

struct scan_bindwidth_info{
    bool     fixed_bindwidth_flag[8];                            /* 固定扫描带宽标志： ture:使用某固定带宽扫描； false: 根据带宽扫描 */
    uint32_t bindwidth_hz[8];                                /* 扫描带宽 */
    float    sideband_rate[8];                                   /* 扫描带宽边带率 */
    bool     work_fixed_bindwidth_flag;
    uint32_t work_bindwidth_hz;
    float    work_sideband_rate;
};//__attribute__ ((packed));

struct calibration_singal_threshold_st{
    int32_t   threshold;    /* 有无信号门限值 */
};

/* 控制/配置参数 */
struct control_st{
    uint8_t remote_local;                                         /* 本控 or 远控 */
    uint8_t fft_noise_threshold;                                  /* FFT 计算噪音门限 */
    uint8_t internal_clock;                                       /* 内外时钟>=1:内部 0外部 */
    uint32_t spectrum_time_interval;                              /* 发送频谱时间间隔 */
    struct specturm_analysis_control_st specturm_analysis_param;  /* 频谱分析控制参数 */
    struct scan_bindwidth_info scan_bw;                           /* 扫描带宽参数 */
#ifdef SUPPORT_NET_WZ
    uint32_t wz_threshold_bandwidth;                              /* 万兆信道带宽阀值 ，带宽<该值，使用千兆口；否则使用万兆口传输IQ */
#endif
    struct residency_policy residency;                            /* 驻留时间策略 */
    struct calibration_singal_threshold_st signal;
	uint32_t iq_data_length;                                      /* iq数据包发送长度*/
    int32_t agc_ref_val_0dbm;                                     /*  AGC 模式下，0DB 对应校准值*/
    int32_t subch_ref_val_0dbm;                                   /*  多频点模式下 0DBm 对应校准值*/
    uint32_t disk_file_notifier_timeout_ms;
};//__attribute__ ((packed));

/*状态参数*/

/*4 获取设备基本信息*/


struct poal_soft_version{
   char* app;
   char* kernel;
   char* uboot;
   char* fpga;
}__attribute__ ((packed));           

struct poal_disk_Node{
    uint32_t totalSpace;
    uint32_t freeSpace;
    uint8_t status;
}__attribute__ ((packed));

struct poal_disk_alert{
    uint64_t alert_threshold_byte;
    uint64_t split_file_threshold_byte;
};

struct poal_disk_Info{
    uint16_t diskNum; //?
    struct poal_disk_Node diskNode;
    struct poal_disk_alert alert;
}__attribute__ ((packed));     

struct poal_clk_Info{
    uint8_t inout;
    uint8_t  status;
    uint32_t frequency;
}__attribute__ ((packed)); 

struct poal_ad_Info{
    uint8_t status;
}__attribute__ ((packed)); 


struct poal_rf_node{
    uint8_t status;
    uint16_t   temprature;      
}__attribute__ ((packed));    

struct poal_rf_Info{
    uint8_t rfnum;
    struct poal_rf_node  rfnode;//struct rf_node  rfnode[MAX_SIG_CHANNLE];
}__attribute__ ((packed));  


struct poal_fpga_Info{
    uint16_t temprature;
}__attribute__ ((packed));

struct poal_compile_Info{
    char *build_name;
    char *build_time;
    char *build_version;
    char *build_jenkins_id;
    char *code_url;
    char *code_branch;
    char *code_hash;
    char *release_debug;
}__attribute__ ((packed));


struct poal_status_info{
    struct poal_soft_version softVersion;
    struct poal_disk_Info diskInfo;
    struct poal_clk_Info  clkInfo;
    struct poal_ad_Info   adInfo;  
    struct poal_rf_Info   rfInfo;
    struct poal_fpga_Info  fpgaInfo;
    struct poal_compile_Info compileInfo;
    char *device_sn;
}__attribute__ ((packed));


struct calibration_specturm_info_st{
    uint32_t start_freq_khz[40];
    uint32_t end_freq_khz[40];
    int power_level[40];
    int global_roughly_power_lever;
    int low_distortion_power_level;
    int low_noise_power_level;
    bool gain_calibration_onoff;
};//__attribute__ ((packed));

struct calibration_specturm_node_st{
    uint32_t start_freq_khz;
    uint32_t end_freq_khz;
    int power_level;
    uint32_t fft;
}__attribute__ ((packed));

struct calibration_attenuation_node_st{
    int32_t  rf_mode;
    int32_t start_range;
    int32_t end_range;
}__attribute__ ((packed));


struct calibration_mgc_attenuation_node_st{
    int32_t start_range;
    int32_t end_range;
}__attribute__ ((packed));


struct calibration_specturm_v2_st{
    struct calibration_specturm_node_st cal_node[128];
    struct calibration_attenuation_node_st att_node[3];
    struct calibration_mgc_attenuation_node_st mgc_attr_node;
    int global_roughly_power_lever;
}__attribute__ ((packed));

/* For ADRVxxxx CHIP */
struct calibration_dc_offset_info_st{
    uint32_t start_freq_khz[40];
    uint32_t end_freq_khz[40];
    int mshift[40];
    int global_mshift;  /* mShift (valid value range is 8 to 20). */
    bool is_open;
};

struct calibration_analysis_info_st{
    uint32_t start_freq_khz[40];
    uint32_t end_freq_khz[40];
    int power_level[40];
    int global_roughly_power_lever;
};//__attribute__ ((packed));

struct calibration_lo_leakage_info_st{
    uint32_t fft_size[16];
    int16_t threshold[16];
    int16_t renew_data_len[16];
    int16_t global_threshold;
    int16_t global_renew_data_len;
};//__attribute__ ((packed));

struct calibration_mgc_info_st{
    uint32_t start_freq_khz[32];
    uint32_t end_freq_khz[32];
    int32_t  gain_val[32];
    int32_t  global_gain_val;
};//__attribute__ ((packed));

struct calibration_low_noise_info_st{
    int32_t  global_power_val;
};//__attribute__ ((packed));


struct calibration_fft_st{
    uint32_t  fftsize[16];
    int32_t   cali_value[16];
};

struct calibration_info_st{
    struct calibration_specturm_info_st specturm;
    struct calibration_specturm_v2_st   spm_level;
    struct calibration_analysis_info_st analysis;
    struct calibration_low_noise_info_st low_noise;
    struct calibration_lo_leakage_info_st lo_leakage;
    struct calibration_mgc_info_st mgc;
    struct calibration_fft_st cali_fft;
    struct calibration_dc_offset_info_st dc_offset;
};//__attribute__ ((packed));


struct channel_para{
    volatile work_mode_type work_mode;                              /* 通道工作模式 */
    struct output_en_st enable;                                     /* 通道使能 */
    struct multi_freq_point_para_st  multi_freq_point_param;        /* 通道多频点 */
    struct multi_freq_fregment_para_st  multi_freq_fregment_para;   /* 通道多频段 */
    struct sub_channel_freq_para_st sub_channel_para;               /* 通道子通道参数 */
    struct rf_para_st rf_para;                                      /* 通道射频参数 */
};


struct poal_config{
    uint8_t cid;                                                    /* 设置通道*/
    struct channel_para channel[MAX_RF_NUM];                     /* 通道参数 */
    struct network_st network;                                      /* 网络参数 */
    #ifdef SUPPORT_NET_WZ
    struct network_st network_10g;                                  /* 万兆网络参数 */
    #endif
    struct control_st ctrl_para;                                    /* 控制配置参数 */
    struct poal_status_info status_para;                            /* 状态信息参数 */
    struct calibration_info_st cal_level;                           /* 校准参数 */
};

#endif
