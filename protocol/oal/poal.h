#ifndef _PROTOCOL_OAL_H_
#define _PROTOCOL_OAL_H_

#include "config.h"

#if PROTOCAL_XNRP != 0
    #define oal_handle_request  xnrp_handle_request
    #define assamble_response_data  xnrp_assamble_response_data
#elif PROTOCAL_ATE != 0
    #define oal_handle_request  akt_handle_request
    #define assamble_response_data  akt_assamble_response_data
#else
    #error "NOT DEFINE PROTOCAL"
#endif


/* 工作模式参数参数 */
typedef enum _work_mode_type {
    OAL_FIXED_FREQ_ANYS_MODE = 0x00,
    OAL_FAST_SCAN_MODE = 0x01,
    OAL_MULTI_ZONE_SCAN_MODE = 0x02,
    OAL_MULTI_POINT_SCAN_MODE = 0x03
}work_mode_type;


/* 输出使能 */
struct output_en_st{
    uint8_t cid;
    int8_t sub_id;
    uint8_t  psd_en;
    uint8_t  audio_en;
    uint8_t  iq_en;
    uint8_t  spec_analy_en;
    uint8_t  direction_en;
}__attribute__ ((packed));

/* 频点参数 */
struct freq_points_st{
    int16_t index;
    uint64_t center_freq; /* rf */
    uint64_t bandwidth;   /* rf */
    float freq_resolution;
    uint32_t fft_size;
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


struct poal_config{
    work_mode_type work_mode;
    struct output_en_st enable;
    struct multi_freq_point_para_st  multi_freq_point_param;
    struct sub_channel_freq_para_st sub_channel_para;
    struct multi_freq_fregment_para_st  multi_freq_fregment_para;
}__attribute__ ((packed));

struct protocal_oal_handle {
    uint8_t  class_code;
    uint8_t  bussiness_code;
    
    //bool (*poal_parse_header)(const uint8_t *data, int len, uint8_t **payload);
   // bool (*poal_execute_method)(void);
    bool (*poal_execute_get_command)(void);
    bool (*poal_execute_set_command)(void);
    bool (*dao_save_config)(void);
    bool (*executor_set_command)(void);
};

/* Third party protocol comparison table define */
struct third_party_comparison_table {
    uint8_t  xnrp_class_code;
    uint8_t  xnrp_bussiness_code;
    uint8_t  akt_class_code;
    uint8_t  akt_bussiness_code;
};

int poal_handle_request(struct net_tcp_client *cl, char *data, int len);
#endif