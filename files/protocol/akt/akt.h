#ifndef _AKT_H_H
#define _AKT_H_H

#include "config.h"

#define AKT_START_FLAG 0x7E7E
#define AKT_END_FLAG  0xA5A5

#define SPI_START_FLAG 0xAA
#define SPI_END_FLAG  0x55

//#define MAX_RADIO_CHANNEL_NUM (8)

#define EXTRAC_FACTOR (4)
#define SMOOTH_FACTOR (128)
#define SAMPLE_RATE (102400000)

#define DEFAULT_FFT_SIZE (4096)
#define SZ_36M 0x02400000
#define SZ_32M 0x02000000
#define SZ_16M 0x01000000
#define SZ_8M  0x00800000
#define SZ_6M  0x00600000
#define SZ_5M  0x00500000
#define SZ_4M  0x00400000
#define SZ_3M  0x00300000
#define SZ_2M  0x00200000
#define SZ_1M  0x00100000
#define SZ_64K  0x00010000
#define DMA_CH_SIZE SZ_4M
#define DMA_ANGLE_MAP_OFFSET  (DMA_CH_SIZE*2)
#define MAX_QUEUE_LEN (128)
#define MIN_QUEUE_LEN (50)
#define DEFAULT_FFT_TIMEOUT (2)
#define KILL_THREAD_TIMEOUT (DEFAULT_FFT_TIMEOUT+2)
#define BAND_WITH_20M (20000000ULL)
#define BAND_WITH_10M (10000000ULL)
#define BAND_WITH_5M  (5000000ULL)
#define BAND_WITH_2M  (2000000ULL)
#define BAND_WITH_1M  (1000000ULL)


#define DEFAULT_IQ_SIZE (SZ_4M)
#define DEFAULT_DQ_SIZE (40960)
#define MAX_FFT_QUEUE_LEN 20
#define MAX_IQ_QUEUE_LEN 15
#define MAX_DQ_QUEUE_LEN 10
#define DQ_TIMEOUT 10
#define SAMPLE_RATE_HZ (102400000)
#define MIN_EXTRACT_FACTOR (4)
#define MAX_EXTRACT_FACTOR (0xffffff)
#define EXTRACT_STEP_SIZE 4
#define FFT_SIZE_256 256
#define FFT_SIZE_512 512
#define FFT_SIZE_1024 1024
#define FFT_SIZE_2048 2048
#define FFT_SIZE_4096 4096
#define FFT_SIZE_8192 8192
#define FFT_SIZE_16384 16384
#define FFT_SIZE_32768 32768

#define MAX_RF_OUTPUT_NUM 2
#define DATA_DELAY_COEFFICIENT 5
#define MAX_NETWORK_SPEED 100000000
#define MAX_SEND_DATA_THREAD_NUM 100
#define DECODE_BUFFER_SIZE (40960)

#define PORT 1234           //服务器端口
#define BACKLOG 8           //listen队列等待的连接数
#define MAXDATASIZE 1024    //缓冲区大小
#define TCP   0x01
#define UDP   0x10
#define KEEPALIVE_TIMEOUT_SECS 30

#define CENTER_POINT_NUM_ON_1M_BW      524288  // POINT_NUM_ON_1M_BW/2
#define POINT_NUM_ON_1M_BW             1048576

//#define MAX_SIGNAL_CHANNEL_NUM (16)
#define MAX_CALIBRATE_POINT_NUM (8)
#define CHAN_NUM_OFFSET  (0)
#define SPECTRUM_NUM_OFFSET (1)
#define ANGLE_NUM_OFFSET (3)
#define DQ_NUM_OFFSET  (2)
#define IQ_NUM_OFFSET  (0)
#define MAX_DIRECTION_FREQ_POINT_NUM (128)
#define BAND_WITH_300K  (300000)
#define BAND_WITH_15K  (15000)
#define BAND_FACTOR    1//(1.2288)//(1.25)

#define MAX_FREQ_NUMBER 3200
#define ANGLE_ASSERT_FLAG (0x55aa)
#define MAX_REFERENCE_ANTENNA_NUM 6
#define MAX_SERIAL_NUM 2
#define SERIAL_SCREEN_INDEX 1

#define MAX_RF_LOCK_STATUS_NUM 2
#define RF0_LOCK_INDICATE_GPIO 18
#define RF1_LOCK_INDICATE_GPIO 19

#define SAMPLE_ACCURACY_STEP 5
#define SAMPLE_ACCURACY_5_0_SIZE (360/SAMPLE_ACCURACY_STEP)
#define SAMPLE_ACCURACY_2_5_SIZE (SAMPLE_ACCURACY_5_0_SIZE*2)
#define SAMPLE_ACCURACY_1_2_5_SIZE (SAMPLE_ACCURACY_2_5_SIZE*2)
#define LINE_DATA_POINT_NUMBER (MAX_REFERENCE_ANTENNA_NUM*(360/SAMPLE_ACCURACY_STEP))
// 7: 1 symbol + 5 digital + 1 blank
// 100: reserve
#define MAX_LINE_CHAR_NUMBER (LINE_DATA_POINT_NUMBER*7+100)


typedef enum _OPERATION_CODE{
  SET_CMD_REQ = 0x00,
  QUERY_CMD_REQ = 0x01,
  SET_CMD_RSP = 0x80,
  QUERY_CMD_RSP = 0x81,
  NET_CTRL_CMD = 0xAA
}OPERATION_CODE;


typedef enum _BUSINESS_CODE{
    RCV_NET_PARAM = 0x01,
    RCV_RF_PARAM = 0x02,
    RCV_STA_PARAM = 0x03,
    FIXED_FEQ_BASIC_PARAM = 0x04,
    FIXED_FEQ_D_PARAM = 0x05,
    FAST_SCAN_PARAM = 0x06,
    MULTI_SCAN_PARAM = 0x07,
    MULTI_SERH_PARAM = 0x08,
    SNIFFER_DATA_REPORT_PARAM = 0x09,
    OUTPUT_ENABLE_PARAM = 0x0A,
    RF_OUTPUT_ENABLE_PARAM = 0x0B,
    HEART_BEAT_MSG_REQ = 0xA1,
    HEART_BEAT_MSG_RSP = 0xA2,
    SYSTEM_TIME_SET_REQ = 0xA3,
    LOG_STATS_REQ = 0xA4,
    POWER_STATUS_REQ = 0xA5,
    RF_FREQ_STATS_REQ = 0xFA,
    SOFTWARE_VERSION_CMD=0xfb,
    RF_ATTENUATION_CMD=0xfc,
    DISCOVER_LAN_DEV_PARAM = 0xFF,
    SELF_CHECK = 0xb1,
    DIRECTION_FREQ_POINT_REQ_CMD =0xb2,
    DEVICE_LOCATION_REQ_CMD = 0xb3,
    COMPASS_REQ_CMD=0xb4,
    DIRECTION_ENABLE_CMD=0xb5,
    DIRECTION_FREQ_POINT_RSP_CMD = 0xb6,
    RF_WORK_MODE_CMD=0xe0,
    RF_GAIN_MODE_CMD=0xe1,
    MID_FREQ_ATTENUATION_CMD=0xe2,
    RF_AGC_CMD=0xe3,
    MID_FREQ_BANDWIDTH_CMD=0xe4,
    DIRECTION_SMOOTH_CMD=0xe8,
    SAMPLE_CONTROL_FFT_CMD=0xe9,
    QUIET_NOISE_SWITCH_CMD=0xea,
    QUIET_NOISE_THRESHOLD_CMD=0xeb,
    DECODE_METHOD_CMD = 0xec,
    SINGLE_FREQ_DECODE_CMD= 0xed,
    MULTI_FREQ_DECODE_CMD = 0xee,
    SINGLE_FREQ_ZONE_SCAN_CMD=0xf0,
    DIRECTION_MULTI_FREQ_ZONE_CMD=0xf1,
    DIRECTION_SIGNAL_THRESHOLD_CMD=0xf2,
    DEVICE_MODEL_CMD=0xf3,
    SUB_SIGNAL_PARAM_CMD=0xf4,
    SUB_SIGNAL_OUTPUT_ENABLE_CMD=0xf5,
    DEVICE_CALIBRATE_CMD=0xf6,
    SET_CALIBRATE_VALUE_CMD=0xf7,
    DEVICE_SELF_CHECK_CMD=0xf8,
    STORAGE_STATUS_CMD=0xf9,
    STORAGE_IQ_CMD=0x10,
    BACKTRACE_IQ_CMD=0x11,
    SEARCH_FILE_STATUS_CMD=0x12,
    STORAGE_DELETE_POLICY_CMD=0x13,
    STORAGE_DELETE_FILE_CMD=0x14,
    DISK_FORMAT_CMD=0x27,
    NOTIFY_SIGNALl_STATUS=0x1b,
    AUDIO_SAMPLE_RATE=0x1d,
    CONTRL_CHANNEL_POWER=0x1e,
    SPCTRUM_PARAM_CMD=0x18,
    SPCTRUM_ANALYSIS_CTRL_EN_CMD=0x19,
    SPCTRUM_GET_RESULT_CMD=0x1a,
    FREQ_RESIDENT_MODE=0x1C,
#ifdef SUPPORT_NET_WZ
    SET_NET_WZ_THRESHOLD_CMD=0x16,
    SET_NET_WZ_NETWORK_CMD=0x26,
#endif
    LOW_NOISE_CMD=0x25,
    AUDIO_VOLUME_CMD=0x28,
    DEVICE_REBOOT_CMD=0x29,
    FILE_LIST_CMD=0x2a,
    FILE_STROE_SIZE_CMD=0x2b,
    FILE_STATUS_NOTIFY=0x2c,
    REPORT_DISK_ALARM=0x2d,
}BUSINESS_CODE;


typedef enum _CMD_RSP_CODE{
  SET_SUCCESS = 0x00,
  OUT_OF_FREQ = 0x01,
  LOCK_SUCCESS = 0x00,
  LOCK_FAILED = 0x01,
  SET_FAILED = 0x01,
}CMD_RSP_CODE;


typedef enum _DQ_MODE_CODE{
    DQ_MODE_IQ = 0x00,
    DQ_MODE_AM = 0x01,
    DQ_MODE_FM = 0x02,
    DQ_MODE_CW = 0x03,
    DQ_MODE_LSB = 0x04,
    DQ_MODE_USB = 0x05,
}DQ_MODE_CODE;


//table 16 page 28
typedef enum _WINDOW_TYPE {
    HANMIN_WIN = 0x00,
    KAISER_WIN = 0x01,
    BLACKMAN_WIN = 0x02,
    OTHER_WIN = 0x03
}WINDOW_TYPE;

//table 10 page 25
typedef enum _PATTEN_CODE{
    LOW_DISTORTION = 0x00,
    NORMAL = 0x01,
    LOW_NOISE = 0x2
}PATTEN_CODE;

//table 28 page 35
typedef enum _GAIN_MODE_CODE{
    MANUAL_GAIN = 0x00,
    AUTO_GAIN = 0x01,
}GAIN_MODE_CODE;

typedef enum _OUTPUT_EN_MASK {
    ALL_DIS_MASK = 0x00,
    SPECTRUM_MASK = 0x01,
    D_OUT_MASK = 0x02,
    IQ_OUT_MASK = 0x04,
    ANGLE_MASK = 0x08,
}OUTPUT_EN_MASK;

//table 23 page 33
typedef enum _DATUM_TYPE {
    SPECTRUM_DATUM_CHAR = 0x00,
    SPECTRUM_DATUM_FLOAT = 0x02,
    BASEBAND_DATUM_IQ = 0x04,
    DIGITAL_AUTDIO = 0x08
}DATUM_TYPE;
//table 24 page 33
typedef enum _EXT_HEADER_TYPE {
    SPECTRUM_DATUM = 0x00,
    DEMODULATE_DATUM = 0x01,
}EXT_HEADER_TYPE;

//table 27 page 35
typedef enum _WORK_MODE_TYPE {
    FIXED_FREQ_ANYS = 0x00,
    FAST_SCAN = 0x01,
    MULTI_ZONE_SCAN = 0x02,
    MULTI_POINT_SCAN = 0x03
}WORK_MODE_TYPE;


///SPI////

typedef enum _SPI_CTRL_CMD {
    SET_CH_FREQ = 0x01,
    SET_CH_GAIN = 0x02,
    SET_FILTER_SEL = 0x03,
    SET_NOISE_MODE = 0x04,
    GET_CH_FREQ = 0x11,
    GET_CH_GAIN = 0x12,
    GET_FILTER_SEL = 0x13,
    GET_NOISE_MODE = 0x14
}SPI_CTRL_CMD;

typedef struct  _PDU_REQ_HEADER{
    uint16_t start_flag;
    uint16_t len;
    uint8_t operation;
    uint8_t code;
    uint8_t usr_id[32];
    uint16_t receiver_id;
    uint16_t crc;
    uint8_t buf[MAX_RECEIVE_DATA_LEN];
}__attribute__ ((packed)) PDU_CFG_REQ_HEADER_ST;

typedef struct  _PDU_REQ_HEADER_EX{
    uint16_t start_flag;
    uint16_t len;
    uint8_t operation;
    uint8_t code;
    uint8_t usr_id[32];
    uint16_t receiver_id;
    uint16_t crc;
}__attribute__ ((packed)) PDU_CFG_REQ_HEADER_ST_EX;


//table 4 page 22
typedef struct  _PDU_RSP_HEADER{
    uint16_t start_flag;
    uint16_t len;
    uint8_t operation;
    uint8_t code;
    uint8_t usr_id[32];
    uint16_t receiver_id;
    uint16_t crc;
//    uint32_t *data;
//    uint16_t end_flag;
}__attribute__ ((packed)) PDU_CFG_RSP_HEADER_ST;





typedef struct  _TRANS_LEN_PARAMETERS{
    uint32_t ch;
    uint32_t len;
    uint32_t type;
}__attribute__ ((packed)) TRANS_LEN_PARAMETERS;




//table 9 page 25
typedef struct _DEVICE_NET_INFO{
    uint8_t mac[6];
    uint32_t gateway;
    uint32_t mask;
    uint32_t ipaddr;
    uint16_t port;
    uint8_t status;
}__attribute__ ((packed)) DEVICE_NET_INFO_ST;


//table 11 page 26
typedef struct _SNIFFER_DATA_REPORT{
    uint8_t cid;
    uint32_t ipaddr;
    uint8_t type;
    uint16_t port;
#ifdef SUPPORT_NET_WZ
    uint32_t wz_ipaddr; /* 添加上位机万兆IP和端口*/
    uint16_t wz_port;
#endif
    uint16_t iq_port;
    uint16_t audio_port; 
}__attribute__ ((packed)) SNIFFER_DATA_REPORT_ST;

//table 12 page 26
typedef struct _HEART_BEAT_PDU{
    uint32_t hop;
}__attribute__ ((packed)) HEART_BEAT_PDU_ST;

//table 13 page 26
typedef struct _DEV_DISCOVER_PARAM{
    uint32_t ipaddr;
    uint16_t port;
}__attribute__ ((packed)) DEV_DISCOVER_PARAM_ST;

//table 14 page 27
typedef struct _FIXED_FREQ_ANYS_B_PARAM{
    uint8_t cid;
    uint64_t center_freq;
    uint32_t bandwidth;
    float freq_resolution;
    uint32_t fft_size;
    uint8_t  win_type;
    uint8_t drop_frame_cnt;
    uint16_t smooth_cnt;
}__attribute__ ((packed)) FIXED_FREQ_ANYS_B_PARAM_ST;

typedef struct _CALIBRATION_PARAM{
    uint8_t cid;
    uint64_t center_freq;
    uint32_t bandwidth;
    float freq_resolution;
    uint32_t fft_size;
    uint8_t  win_type;
    uint8_t drop_frame_cnt;
    uint16_t smooth_cnt;
}__attribute__ ((packed)) CALIBRATION_PARAM_ST;


//table 15 page 27
typedef struct _FIXED_FREQ_ANYS_D_PARAM{
    uint8_t cid;
    uint64_t center_freq;
    uint32_t bandwidth;
    uint8_t  noise_en;
    int8_t noise_thrh;
    uint8_t d_method;
}__attribute__ ((packed)) FIXED_FREQ_ANYS_D_PARAM_ST;

//table 16 page 28
typedef struct _FAST_SCAN_PARAM{
    uint8_t cid;
    uint64_t start_freq;
    uint64_t cutoff_freq;
    uint32_t freq_step;
    float freq_resolution;
    uint32_t fft_size;
    uint8_t  win_type;
    uint8_t drop_frame_cnt;
    uint16_t smooth_cnt;
}__attribute__ ((packed)) FAST_SCAN_PARAM_ST;

typedef struct _MULTI_SCAN_CHANNLE_PARAM{
    uint64_t start_freq;
    uint64_t cutoff_freq;
    uint32_t freq_step;
    float freq_resolution;
    uint32_t fft_size;
}__attribute__ ((packed)) MULTI_SCAN_CHANNLE_PARAM;

//table 17 page 29
typedef struct _MULTI_SCAN_PARAM{
    uint8_t cid;
    uint8_t  win_type;
    uint8_t drop_frame_cnt;
    uint16_t smooth_cnt;
    uint8_t freq_band_cnt;
    MULTI_SCAN_CHANNLE_PARAM sig_ch[MAX_SIG_CHANNLE];
}__attribute__ ((packed)) MULTI_SCAN_PARAM_ST;



typedef struct _MULTI_SERH_CHANNLE_PARAM{
   uint64_t center_freq;
   uint64_t bandwidth;
   float freq_resolution;
   uint32_t fft_size;
   uint8_t d_method;
   uint32_t d_bandwith;
   uint8_t  noise_en;
   uint8_t noise_thrh;
}__attribute__ ((packed)) MULTI_SERH_CHANNLE_PARAM;

//table 18 page 30
typedef struct _MULTI_SERH_PARAM{
    uint8_t cid;
    uint8_t  win_type;
    uint8_t drop_frame_cnt;
    uint16_t smooth_cnt;
    uint16_t channle_cnt;
    uint8_t resident_time;
    MULTI_SERH_CHANNLE_PARAM sig_ch[MAX_SIG_CHANNLE];
}__attribute__ ((packed)) MULTI_SERH_PARAM_ST;

//table 19 page 31
typedef struct _OUTPUT_ENABLE_PARAM{
    uint8_t cid;
    uint8_t  output_en;
}__attribute__ ((packed)) OUTPUT_ENABLE_PARAM_ST;


//table 21 page 31
typedef struct _CHANNLE_STATUS_PARAM{
    uint8_t work_status;
    uint8_t  work_mode;
}__attribute__ ((packed)) CHANNLE_STATUS_PARAM;
//table 21 page 31
typedef struct _DEVICE_STATUS_PARAM{
    uint8_t num;
    CHANNLE_STATUS_PARAM sig_ch[MAX_RADIO_CHANNEL_NUM];
}__attribute__ ((packed)) DEVICE_STATUS_PARAM_ST;

//table 22 page 32
typedef struct  _DATUM_PDU_HEADER{
    uint16_t syn_flag;
    uint16_t type;
    uint64_t toa;
    uint16_t seqnum;
    uint16_t virtual_ch;
    uint8_t reserve[8];
    uint8_t ex_type;
    uint8_t ex_len;
    uint32_t data_len;
    uint16_t crc;
}__attribute__ ((packed)) DATUM_PDU_HEADER_ST;

//table 25 page 33
typedef struct  _DATUM_SPECTRUM_HEADER{
    uint16_t dev_id;
    uint8_t cid;
    uint8_t  work_mode;
    uint8_t  gain_mode;
    int8_t  gain;
    uint64_t duration;
    uint64_t start_freq;
    uint64_t cutoff_freq;
    uint64_t center_freq;
    uint32_t bandwidth;
    uint32_t sample_rate;
    float freq_resolution;
    uint16_t fft_len;
    uint8_t datum_total;
    uint8_t sn;
    uint8_t datum_type;
    uint8_t frag_total_num;
    uint8_t frag_cur_num;
    uint16_t frag_data_len;
    uint8_t pdata[0];
}__attribute__ ((packed)) DATUM_SPECTRUM_HEADER_ST;


//table 26 page 34
typedef struct  _DATUM_DEMODULATE_HEADER{
    uint16_t dev_id;
    uint8_t cid;
    uint8_t  work_mode;
    uint8_t  gain_mode;
    int8_t gain;
    uint64_t duration;
    uint64_t center_freq;
    uint32_t bandwidth;
    uint8_t demodulate_type;
    uint32_t sample_rate;
    uint8_t frag_total_num;
    uint8_t frag_cur_num;
    uint16_t frag_data_len;
    uint8_t pdata[0];
}__attribute__ ((packed)) DATUM_DEMODULATE_HEADER_ST;

typedef union  _RF_PARAMETERS{
    FIXED_FREQ_ANYS_B_PARAM_ST fixed_freq_ansy;
    FAST_SCAN_PARAM_ST fast_scan_anys;
    MULTI_SCAN_PARAM_ST multi_freq_zone_anys;
    MULTI_SERH_PARAM_ST multi_freq_point_anys;
}__attribute__ ((packed)) RF_PARAMETERS;


typedef struct _DIRECTION_MULTI_ZONE_CHANNEL_PARAM{
    uint64_t center_freq;
    uint64_t bandwidth;
    float freq_resolution;
    uint32_t fft_size;
    uint32_t freq_step;
}__attribute__ ((packed)) DIRECTION_MULTI_ZONE_CHANNEL_PARAM;

typedef struct _DIRECTION_MULTI_FREQ_ZONE_PARAM{
    uint8_t cid;
    uint32_t freq_band_cnt;
    uint8_t resident_time;
    DIRECTION_MULTI_ZONE_CHANNEL_PARAM  sig_ch[MAX_SIG_CHANNLE];
}__attribute__ ((packed)) DIRECTION_MULTI_FREQ_ZONE_PARAM;


typedef struct __MULTI_FREQ_DECODE_CHANNEL_PARAM{
    uint64_t center_freq;
    uint8_t decode_method_id;
    uint32_t bandwidth;
    uint8_t quiet_noise_switch;
    int8_t quiet_noise_threshold;
}__attribute__ ((packed)) MULTI_FREQ_DECODE_CHANNEL_PARAM;

typedef struct  _DIRECTION_FFT_PARAM{
    uint8_t channel;
    float freq_resolution;
    uint32_t fft_size;
}__attribute__ ((packed)) DIRECTION_FFT_PARAM;


typedef struct _MULTI_FREQ_DECODE_PARAM{
    uint8_t cid;
    uint32_t freq_band_cnt;
    uint8_t resident_time;
    MULTI_FREQ_DECODE_CHANNEL_PARAM  sig_ch[MAX_SIG_CHANNLE];
//    DIRECTION_FFT_PARAM fft[MAX_RADIO_CHANNEL_NUM];
}__attribute__ ((packed)) MULTI_FREQ_DECODE_PARAM;


typedef struct  _DEVICE_SELF_CHECK_TEMPERATUE_ST{
    signed short  rf_temperature;
    uint8_t ch_status;
}__attribute__ ((packed)) DEVICE_SELF_CHECK_TEMPERATUE_ST; 

typedef struct  _DEVICE_SELF_CHECK_STATUS_RSP_ST{
    uint8_t ext_clk;    
    uint8_t ad_status;
    signed short  pfga_temperature;
    int8_t system_power_on_time[20];
    uint8_t ch_num;
    DEVICE_SELF_CHECK_TEMPERATUE_ST t_s[MAX_RADIO_CHANNEL_NUM];
    uint8_t irig_b_status;
    uint8_t gps_status;
}__attribute__ ((packed)) DEVICE_SELF_CHECK_STATUS_RSP_ST; 


typedef struct _DIRECTION_SMOOTH_PARAM{
    uint8_t ch;
    uint16_t smooth;
}__attribute__ ((packed)) DIRECTION_SMOOTH_PARAM;

typedef struct _SUB_AUDIO_PARAM{
    uint8_t cid;
    uint8_t au_switch;
    uint8_t volume;
    uint8_t noise_en;
    uint32_t sample_rate;
}__attribute__ ((packed)) SUB_AUDIO_PARAM;

typedef struct _DIRECTION_MID_FREQ_BANDWIDTH_PARAM{
    uint8_t ch;
    uint32_t bandwidth;
}__attribute__ ((packed)) DIRECTION_MID_FREQ_BANDWIDTH_PARAM;

typedef struct _SUB_SIGNAL_PARAM{
    uint8_t cid;
    uint16_t signal_ch;
    uint64_t freq;
    uint8_t decode_method_id;
    uint32_t bandwidth;
}__attribute__ ((packed)) SUB_SIGNAL_PARAM;

typedef struct _SUB_SIGNAL_ENABLE_PARAM{
    uint8_t cid;
    uint16_t signal_ch;
    uint8_t en;
}__attribute__ ((packed)) SUB_SIGNAL_ENABLE_PARAM;

typedef struct _DEVICE_SIGNAL_PARAM_ST{
    uint8_t cid;
    uint64_t mid_freq;
    uint64_t bandwith;
    uint32_t level;
    uint8_t status;  /* 0: online 1: offline */
}__attribute__ ((packed)) DEVICE_SIGNAL_PARAM_ST;

typedef struct _FFT_SIGNAL_RESULT{
    uint64_t center_freq;
    uint64_t bandwidth;
    float power_level;
}__attribute__ ((packed)) FFT_SIGNAL_RESULT_ST;

typedef struct _FFT_SIGNAL_RESPINSE{
    uint16_t signal_num;
    float temperature;
    float humidity;
    FFT_SIGNAL_RESULT_ST signal_array[0];
}__attribute__ ((packed)) FFT_SIGNAL_RESPINSE_ST;

/************Disk-related parameters***********************/

typedef struct  _STORAGE_DISK_INFO_ST{
    uint8_t disk_state;
    uint64_t disk_capacity_byte;
    uint64_t disk_used_byte;
}__attribute__ ((packed)) STORAGE_DISK_INFO_ST; 

typedef struct  _STORAGE_STATUS_RSP_ST{
    uint32_t read_write_speed_kbytesps;
    uint8_t disk_num;
    STORAGE_DISK_INFO_ST disk_capacity[0];
}__attribute__ ((packed)) STORAGE_STATUS_RSP_ST; 

typedef struct  _STORAGE_IQ_ST{
    uint8_t cid;
    uint8_t cmd;
    uint16_t signal_ch;
    uint64_t freq;
    uint32_t bandwidth;
    uint16_t caputure_time_len;
    char filepath[FILE_PATH_MAX_LEN];
    uint8_t type;           /* 0x00：原始 AD 数据   0x01：DDC 处理后 IQ 数据 */
    uint64_t filesize;      /* 采集文件大小 */
}__attribute__ ((packed)) STORAGE_IQ_ST; 

typedef struct  _BACKTRACE_IQ_ST{
    uint8_t cid;
    uint8_t cmd;
    uint16_t signal_ch;
    uint64_t freq;
    uint32_t bandwidth;
    char filepath[FILE_PATH_MAX_LEN];
}__attribute__ ((packed)) BACKTRACE_IQ_ST; 

typedef struct  _SEARCH_FILE_STATUS_RSP_ST{
    char filepath[FILE_PATH_MAX_LEN];
    uint8_t status;                        /*0x00：不存在  0x01：正常  0x02:损坏*/
    uint64_t file_size;
}__attribute__ ((packed)) SEARCH_FILE_STATUS_RSP_ST; 


typedef struct  _CALIBRATION_SOURCE_ST{
    uint8_t cid;
    uint8_t enable;
    uint64_t middle_freq_hz;
    int8_t power;
}__attribute__ ((packed)) CALIBRATION_SOURCE_ST; 

typedef struct  _CALIBRATION_SOURCE_ST_V2{
    uint8_t cid;
    uint8_t enable;
    uint64_t middle_freq_hz;
    int8_t power;
    uint64_t s_freq;
    uint64_t e_freq;
    uint64_t step;
    uint32_t r_time_ms;
}__attribute__ ((packed)) CALIBRATION_SOURCE_ST_V2; 

struct udp_client_info{
    uint8_t cid;
    uint32_t ipaddr;
    uint16_t port;
#ifdef SUPPORT_NET_WZ
    uint32_t wz_ipaddr; /* 添加上位机万兆IP和端口*/
    uint16_t wz_port;
#endif
}__attribute__ ((packed));


typedef struct _SINGLE_FILE_INFO{
    uint8_t file_path[128];
    uint64_t file_size;
    uint64_t file_mtime;
}__attribute__ ((packed)) SINGLE_FILE_INFO;

typedef struct _FILE_LIST_INFO{
    int32_t file_num;
    SINGLE_FILE_INFO files[0];
}__attribute__ ((packed)) FILE_LIST_INFO;

struct fs_status {
    uint8_t ch;
    char path[256];
    uint64_t filesize;
    uint64_t duration_time;
    uint64_t middle_freq;
    uint64_t bindwidth;
    uint64_t sample_rate;
    uint64_t file_threshold;
    uint8_t state;
}__attribute__ ((packed));
struct disk_alarm_report {
    uint8_t ch;
    uint8_t code;
}__attribute__ ((packed));

/*************************************************************************/
#define check_radio_channel(ch)   (ch >= MAX_RADIO_CHANNEL_NUM ? 1 : 0) 
#define check_sub_channel(sub_ch) (sub_ch >= MAX_SIGNAL_CHANNEL_NUM ? 1 : 0) 

#define check_valid_channel(data)  do {                       \
            if(check_radio_channel(data)){                  \
                err_code = RET_CODE_PARAMTER_ERR;           \
                goto set_exit;                              \
            }                                               \
            pakt_config->cid = data;                        \
            ch = poal_config->cid = pakt_config->cid;       \
}while(0)

struct akt_protocal_param{
    uint8_t cid;
    uint8_t sub_cid;
    OUTPUT_ENABLE_PARAM_ST enable;
    DIRECTION_MULTI_FREQ_ZONE_PARAM multi_freq_zone[MAX_RADIO_CHANNEL_NUM];
    MULTI_FREQ_DECODE_PARAM decode_param[MAX_RADIO_CHANNEL_NUM];
    DIRECTION_FFT_PARAM fft[MAX_RADIO_CHANNEL_NUM];
    DIRECTION_SMOOTH_PARAM smooth[MAX_RADIO_CHANNEL_NUM];
    DIRECTION_MID_FREQ_BANDWIDTH_PARAM  mid_freq_bandwidth[MAX_RADIO_CHANNEL_NUM];
    SUB_SIGNAL_PARAM sub_channel[MAX_SIGNAL_CHANNEL_NUM];
    SUB_SIGNAL_ENABLE_PARAM sub_channel_enable[MAX_SIGNAL_CHANNEL_NUM];
}__attribute__ ((packed));

struct response_get_data{
    PDU_CFG_RSP_HEADER_ST header;
    uint8_t  payload_data[MAX_SEND_DATA_LEN];
    uint16_t end_flag;
};

struct response_set_data{
    PDU_CFG_RSP_HEADER_ST header;
    uint8_t cid;
    uint8_t  payload_data[MAX_SEND_DATA_LEN];
    uint16_t end_flag;
};

struct response_get_data_v2{
    PDU_CFG_RSP_HEADER_ST header;
//    uint8_t  payload_data[MAX_SEND_DATA_LEN];
    uint16_t end_flag;
};

struct response_set_data_v2{
    PDU_CFG_RSP_HEADER_ST header;
    uint8_t cid;
//    uint8_t  payload_data[MAX_SEND_DATA_LEN];
//    uint16_t end_flag;
};

typedef enum {
    RET_CODE_SUCCSESS         = 0,
    RET_CODE_FORMAT_ERR       = 1,
    RET_CODE_PARAMTER_ERR     = 2,
    RET_CODE_PARAMTER_INVALID = 3,
    RET_CODE_CHANNEL_ERR      = 4,
    RET_CODE_INVALID_MODULE   = 5,
    RET_CODE_INTERNAL_ERR     = 6,
    RET_CODE_PARAMTER_NOT_SET = 7,
    RET_CODE_PARAMTER_TOO_LONG= 8,
    RET_CODE_LOCAL_CTRL_MODE  = 9,
    RET_CODE_DEVICE_BUSY      = 10,
    RET_CODE_FILE_NOT_FOUND   = 11,
    RET_CODE_FILE_EXISTS      = 12,
    RET_CODE_HEADER_ERR       = 13,
} x_return_code;

/* method code */
typedef enum {
    METHOD_SET_COMMAND      = 0x01,
    METHOD_GET_COMMAND      = 0x02,
    METHOD_RESPONSE_COMMAND = 0x04,
    METHOD_REPORT_COMMAND   = 0x08,
}x_method_code;

/* class code */
typedef enum {
    CLASS_CODE_REGISTER  = 0x00,
    CLASS_CODE_NET       = 0x01,
    CLASS_CODE_WORK_MODE = 0x02,
    CLASS_CODE_MID_FRQ   = 0x04,
    CLASS_CODE_RF        = 0x08,
    CLASS_CODE_CONTROL   = 0x10,
    CLASS_CODE_STATUS    = 0x11,
    CLASS_CODE_JOURNAL   = 0x12,
    CLASS_CODE_FILE      = 0x14,
    CLASS_CODE_HEARTBEAT = 0xaa
}x_class_code;


extern bool akt_parse_header_v2(void *client, const char *buf, int len, int *head_len, int *code);
extern bool akt_execute_method(void *cl, int *code);
extern uint8_t *akt_assamble_data_extend_frame_header_data(uint32_t *len, void *config);
extern void *akt_assamble_data_frame_header_data(uint32_t *len, void *config);
extern void akt_send(const void *data, int len, int code);
extern void akt_send_resp(void *client, int code, void *args);
extern int  akt_parse_end(void *cl, char *buf, int len);
extern bool akt_parse_discovery(void *client, const char *buf, int len);
extern void akt_send_file_status(void *data, size_t data_len, void *args);
extern void akt_send_alert(int code);
extern int get_origin_start_end_freq(uint8_t ch, uint64_t start_in, uint64_t end_in, uint64_t *start_out, uint64_t *end_out);

#endif

