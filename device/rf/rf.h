#ifndef RF_H
#define RF_H


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
//#include <list>
//#include <mutex>
#include <stdio.h>
#include <sys/ioctl.h> 
#include <linux/types.h> 
#include <linux/spi/spidev.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>


#define GPIO_BASE_OFFSET              960
#define SPI_CONTROL_NUM               8
#define MAX_GPIO_NUM                  64
#define MAX_LED_NUM                   2
#define B2B_STATUS_INDICATE_GPIO      63
#define PERMANENT_HEADER_LEN          5 
#define MAX_CHANNEL_NUM               8
#define CHECKSUM_OFFSET               1
#define CHECKSUM_IGNORE_LEN           3
#define FREQ_SET_MAX_RETRY            3
#define FREQ_QUERY_MAX_RETRY          2
#define FREQ_QUERY_WAIT_INTERVAL      300
//#define GPIO_USED_MASK                (1<<8 | 1<<9 | 1<<10 | 1<<63 | 1<<24 | 1<<25 | 1<<26 | 1<<27 | 1<<28 | 1<<29 | 1<<30 | 1<<31)
#define ULLI                          ((uint64_t)1)
#define GPIO_USED_MASK (ULLI<<8 | ULLI<<9 | ULLI<<10 | ULLI<<63 | ULLI<<24 | ULLI<<25 | ULLI<<26 | ULLI<<27 | ULLI<<28 | ULLI<<29 | ULLI<<30 | ULLI<<31)


#define DEBUG_FREQUENCY 
#ifdef DEBUG_FREQUENCY
    #define PRINTF(fmt, args...) printf("%s:" fmt,__FUNCTION__,##args)
#else
    #define PRINTF(fmt, args...) 
#endif


typedef struct _DEVICE_RF_INFO{
    uint8_t cid;
    uint8_t patten_id;
    uint8_t gain_patten_id;
    uint16_t gain_val;
    uint16_t auto_gain_time;
    int8_t mid_freq_amplitude;
    uint8_t bandwith_id;
    uint8_t antenna_sel;
}__attribute__ ((packed)) DEVICE_RF_INFO_ST;

typedef enum _GAIN_MODE_CODE{
    MANUAL_GAIN = 0x00,
    AUTO_GAIN = 0x01,
}GAIN_MODE_CODE;

typedef enum _RF_COMMAND_CODE{
    RF_FREQ_SET = 0x01,
    RF_GAIN_SET = 0x02,
    RF_MIDDLE_FREQ_BANDWIDTH_FILTER_SET = 0x03,
    RF_NOISE_MODE_SET = 0x04,
    RF_GAIN_CALIBRATE_SET = 0x05,
    MID_FREQ_GAIN_SET = 0x06,
    RF_FREQ_QUERY = 0x10,
    RF_GAIN_QUERY = 0x11,
    RF_MIDDLE_FREQ_BANDWIDTH_FILTER_QUERY = 0x12,
    RF_NOISE_MODE_QUERY = 0x13,
    RF_GAIN_CALIBRATE_QUERY = 0x14,
    MID_FREQ_GAIN_QUERY = 0x15,
    RF_TEMPRATURE_QUERY = 0x41,
}RF_COMMAND_CODE;

typedef struct _RF_GAIN_RESPONSE{
    uint8_t gain_val;
    uint8_t status;
}__attribute__ ((packed)) RF_GAIN_RESPONSE;

typedef struct _RF_MIDDLE_FREQ_BANDWIDTH_RESPONSE{
    uint8_t bandwidth_flag;
    uint8_t status;
}__attribute__ ((packed)) RF_MIDDLE_FREQ_BANDWIDTH_RESPONSE;

typedef struct _RF_NOISE_MODE_RESPONSE{
    uint8_t noise_mode_flag;
    uint8_t status;
}__attribute__ ((packed)) RF_NOISE_MODE_RESPONSE;

typedef struct _RF_GAIN_CALIBRATE_RESPONSE{
    uint8_t status;
}__attribute__ ((packed)) RF_GAIN_CALIBRATE_RESPONSE;

typedef struct _RF_FREQ_RESPONSE{
    uint8_t freq_val[5]; 
    uint8_t status;
    uint8_t oscillator_lock_num;
}__attribute__ ((packed)) RF_FREQ_RESPONSE;


typedef struct _RF_TEMPRATURE_RESPONSE{
	int16_t temperature;
    uint8_t status;
}__attribute__ ((packed)) RF_TEMPRATURE_RESPONSE;   //RF_TEMPRATURE_QUERY

typedef struct _RF_FREQ_SET_REQUEST{
    uint8_t freq_val[5];       
}__attribute__ ((packed)) RF_FREQ_SET_REQUEST;

typedef struct _RF_GAIN_SET_REQUEST{
    uint8_t gain_val;       
}__attribute__ ((packed)) RF_GAIN_SET_REQUEST;

typedef struct _RF_MIDDLE_FREQ_BANDWIDTH_SET_REQUEST{
    uint8_t bandwidth_flag;
}__attribute__ ((packed)) RF_MIDDLE_FREQ_BANDWIDTH_SET_REQUEST;

typedef struct _RF_NOISE_MODE_SET_REQUEST{
    uint8_t noise_mode_flag;       
}__attribute__ ((packed)) RF_NOISE_MODE_SET_REQUEST;

typedef union  _RF_MESSAGE_BODY{
    RF_FREQ_SET_REQUEST freq;
    RF_FREQ_RESPONSE freq_response;
    RF_GAIN_SET_REQUEST gain;
    RF_GAIN_RESPONSE gain_response;
    RF_MIDDLE_FREQ_BANDWIDTH_SET_REQUEST middle_freq_bandwidth;
    RF_MIDDLE_FREQ_BANDWIDTH_RESPONSE middle_freq_bandwidth_response;
    RF_NOISE_MODE_SET_REQUEST noise_mode;
    RF_NOISE_MODE_RESPONSE noise_mode_response;
    RF_GAIN_CALIBRATE_RESPONSE gain_calibrate_response;
	RF_TEMPRATURE_RESPONSE     temprature_response; 
}__attribute__ ((packed)) RF_MESSAGE_BODY;


typedef struct _RF_TRANSLATE_CMD{
    uint8_t start_flag;
    uint8_t data_len;
    uint8_t cmd;
    RF_MESSAGE_BODY body;
    uint8_t checksum;
    uint8_t end_flag;
}__attribute__ ((packed)) RF_TRANSLATE_CMD;

typedef struct _RF_FREQ_STATS{
    uint8_t ch;
    uint32_t total_cmd;
    uint32_t retry_one;
    uint32_t retry_two;
    uint32_t retry_three;
    uint32_t fail_num;
    uint32_t cmd_parse_fail_num;
    uint32_t timeout_num;
    uint32_t comm_time;
}__attribute__ ((packed)) RF_FREQ_STATS;

enum {
    EX_RF_MID_FREQ,               /* 射频中心频率 */
    EX_RF_MID_BW,                 /*中频带宽*/
    EX_RF_MODE_CODE,              /*模式码 0x00：低失真 0x01：常规 0x02：低噪声*/
    EX_RF_GAIN_MODE,              /*增益模式*/
    EX_RF_MGC_GAIN,               /*MGC 增益值*/
    EX_RF_AGC_CTRL_TIME,          /*AGC 控制时间*/
    EX_RF_AGC_OUTPUT_AMP,         /*AGC 中频输出幅度*/
    EX_RF_ANTENNA_SELECT,         /*天线选择*/
    EX_RF_ATTENUATION,            /*射频衰减*/
    EX_RF_STATUS_TEMPERAT,         /*射频温度*/
};

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



typedef struct _CSpi{
    int8_t   (*spi_dev_init)(void);                                                 //spi设备初始化

    #if !defined(INTERNAL_CLK)
    void     (*clock_7044_init)(void);                                              //7044时钟初始化
    #else
    void     (*clock_7044_init_internal)(void);                                     //7044内部时钟初始化
    #endif
    
    void     (*ad9690_init)(void);                                                  //ad9690初始
    void     (*spi_close)(void);                                                    //关闭描述符
    uint8_t  (*rf_set_interface)(uint8_t cmd,uint8_t ch,void *data);                //射频设置接口
    uint8_t  (*rf_read_interface)(uint8_t cmd,uint8_t ch,void *data);               //射频查询接口 
}CSpi;


extern    void spi_init_pointer(CSpi* spi);


#endif

