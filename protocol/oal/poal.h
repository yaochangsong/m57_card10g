#ifndef _PROTOCOL_OAL_H_
#define _PROTOCOL_OAL_H_

#include "config.h"

#define MAX_RADIO_CHANNEL_NUM 8
#define MAX_SIG_CHANNLE 128

#if PROTOCAL_XNRP != 0
    #define poal_parse_header xnrp_parse_header
    #define poal_parse_data   xnrp_parse_data
    #define poal_execute_method xnrp_execute_method
    #define poal_assamble_response_data  xnrp_assamble_response_data
    #define poal_assamble_error_response_data xnrp_assamble_response_data
#elif PROTOCAL_ATE != 0
    #define poal_parse_header akt_parse_header
    #define poal_parse_data   akt_parse_data
    #define poal_execute_method akt_execute_method
    #define poal_assamble_response_data  akt_assamble_response_data
    #define poal_assamble_error_response_data akt_assamble_error_response_data
#else
    #error "NOT DEFINE PROTOCAL"
#endif


/* 工作模式参数参数 */
typedef enum _work_mode_type {
    OAL_NULL_MODE               = 0x00,
    OAL_FIXED_FREQ_ANYS_MODE    = 0x01,
    OAL_FAST_SCAN_MODE          = 0x02,
    OAL_MULTI_ZONE_SCAN_MODE    = 0x03,
    OAL_MULTI_POINT_SCAN_MODE   = 0x04,
    
}work_mode_type;

/* bit_en：内部使能位定义 */
enum {
    PSD_EN_BIT_OFFSET           = 0x00,
    AUDIO_EN_BIT_OFFSET         = 0x01,
    IQ_EN_BIT_OFFSET            = 0x02,
    SPEC_ANALY_EN_BIT_OFFSET    = 0x03,
    DIRECTION_EN_BIT_OFFSET     = 0x04,
};
    
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
    /*        bit_en: 8bit
    bit[7]---bit[6]---bit[5]------bit[4]--------bit[3]--------bit[2]-----bit[1]------bit[0]
     **-----**-------**-- [direction_en]--[spec_analy_en]--[iq_en]--[audio_en]--[psd_en]
    */
    volatile uint8_t  bit_en;
    volatile bool  bit_reset;
}__attribute__ ((packed));

/* 频点参数 */
struct freq_points_st{
    int16_t index;
    uint64_t center_freq; /* rf */
    uint64_t bandwidth;   /* rf */
    float freq_resolution;
    volatile uint32_t fft_size;
    uint8_t d_method;
    uint32_t d_bandwith;
    uint8_t  noise_en;
    int8_t noise_thrh;
}__attribute__ ((packed));


/* 多频点扫描参数 */
struct multi_freq_point_para_st{
    uint8_t cid;
    uint8_t window_type;
    uint8_t frame_drop_cnt;
    uint8_t smooth_time;
    uint32_t residence_time;
    int32_t residence_policy;
    float audio_sample_rate;
    uint32_t freq_point_cnt;
    struct freq_points_st  points[MAX_SIG_CHANNLE];
}__attribute__ ((packed));


/* 子通道解调参数 */
struct sub_channel_freq_para_st{
    uint8_t cid;
    float audio_sample_rate;
    uint8_t frame_drop_cnt;
    uint16_t sub_channel_num;
    struct freq_points_st  sub_ch[MAX_SIG_CHANNLE];
}__attribute__ ((packed));

/* 频段参数 */
struct freq_fregment_para_st{
    int16_t  index;
    uint64_t start_freq;
    uint64_t end_freq;
    uint32_t  step;
    float freq_resolution;
    uint32_t fft_size;
}__attribute__ ((packed));

/* 多频段扫描参数 */
struct multi_freq_fregment_para_st{
    uint8_t cid;
    uint8_t window_type;
    uint8_t frame_drop_cnt;
    uint16_t smooth_time;
    uint32_t freq_segment_cnt;
    struct freq_fregment_para_st  fregment[MAX_SIG_CHANNLE];
}__attribute__ ((packed));

/* 射频参数 */
struct rf_para_st{
    uint8_t cid;
    uint64_t mid_freq;              /* 中心频率 */
    uint8_t rf_mode_code;           /* 射频模式码; 0：低失真 1：常规 2：低噪声 */
    uint8_t gain_ctrl_method;       /* 增益方法; 0：手动控制（MGC） 1：自动控制（AGC）*/
    int8_t mgc_gain_value;          /* MGC 增益值; 单位 dB，精度 1dB*/
    uint32_t agc_ctrl_time;         /* AGC 控制时间; 单位：10 微秒 快速：100 微秒  中速：1000 微秒 慢速：10000 微秒*/
    int8_t agc_mid_freq_out_level;  /* AGC 中频 输出幅度;单位，dBm 默认值：-10dBm*/
    uint32_t mid_bw;                /* 射频中频带宽; 0~2^32 */
    uint8_t antennas_elect;         /* 天线选择 */
    int8_t  attenuation;            /* 射频衰减 ;  -100 至 120 单位 dB，精度 1dB*/
    int16_t temperature;            /* 射频温度 */
}__attribute__ ((packed));

/* 控制参数 */
struct ctrl_st{
    
}__attribute__ ((packed));


/* 网络参数 */
struct network_st{
    uint8_t mac[6];
    uint32_t ipaddress;
    uint32_t netmask;
    uint32_t gateway;
    uint16_t port;
}__attribute__ ((packed));


struct poal_config{
    uint8_t cid;
    volatile work_mode_type work_mode;
    struct output_en_st enable;
    struct multi_freq_point_para_st  multi_freq_point_param[MAX_RADIO_CHANNEL_NUM];
    struct sub_channel_freq_para_st sub_channel_para[MAX_RADIO_CHANNEL_NUM];
    struct multi_freq_fregment_para_st  multi_freq_fregment_para[MAX_RADIO_CHANNEL_NUM];
    struct rf_para_st rf_para[MAX_RADIO_CHANNEL_NUM];
    struct network_st network;
    bool (*assamble_kernel_response_data)(char *, uint8_t, void *);
}__attribute__ ((packed));


int poal_handle_request(struct net_tcp_client *cl, uint8_t *data, int len);
int poal_udp_handle_request(struct net_udp_client *cl, uint8_t *data, int len);

#endif
