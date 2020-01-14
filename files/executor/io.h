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
*  Rev 1.0   09 July 2019   yaochangsong
*  Initial revision.
******************************************************************************/
#ifndef __IO_H__
#define __IO_H__

#include "config.h"

#define DMA_DEVICE "/dev/dma_rf"

#define MAX_COMMON_PARAM_LEN 512

#ifdef SUPPORT_PLATFORM_ARCH_ARM
#define NETWORK_EHTHERNET_POINT       "eth0"
#else
#define NETWORK_EHTHERNET_POINT       "eno1"
#endif

typedef enum _IOCTL_CMD {
    ENABLE_DISABLE = 0x1,
    TRANSLEN = 0x2,
    EXTRACT_CH0 = 0x3,
    EXTRACT_CH1 = 0x4,
    EXTRACT_CH2 = 0x5,
    EXTRACT_CH3 = 0x6,
    SMOOTH_CH0 = 0x7,
    SMOOTH_CH1 = 0x8,
    SMOOTH_CH2 = 0x9,
    SMOOTH_CH3 = 0x10,
    DATA_PATTERN = 0x11,
    BUFFER_LEN = 0x12,
    CALIBRATE_CH0 = 0x13,
    CALIBRATE_CH1 = 0x14,
    CALIBRATE_CH2 = 0x15,
    CALIBRATE_CH3 = 0x16,
    FPGA_VERSION = 0x17,
    AGC_CH0 = 0x18,
    AGC_CH1 = 0x19,
    AGC_CH2 = 0x1a,
    AGC_CH3 = 0x1b,
    FFT_SIZE_CH0 = 0x1c,
    FFT_SIZE_CH1 = 0x1d,
    FFT_SIZE_CH2 = 0x1e,
    FFT_SIZE_CH3 = 0x1f,
    FFT_HDR_PARAM = 0x20,
    RUN_RF_PARAM = 0x21,
    RUN_CONFIG_PARAM = 0x22,
    RUN_DEC_PARAM = 0x23,
    RUN_NET_PARAM = 0x24,
    KEEPALIVE_PARAM = 0x25,
    STA_INFO_PARAM = 0x26,
    QUIET_NOISE_CH0 = 0x27,
    QUIET_NOISE_CH1 = 0x28,
    QUIET_NOISE_CH2 = 0x29,
    QUIET_NOISE_CH3 = 0x2a,
    FREQUENCY_BAND_CONFIG0 = 0x2b,
    FREQUENCY_BAND_CONFIG1 = 0x2c,
    DATA_PATTERN_FREQ = 0x2d,
    GET_DDC_REGISTER_VALUE = 0x2e,
    SET_DDC_REGISTER_VALUE = 0x2f,
    SIGNAL_THRESHOLD_CH0 = 0x30,
    DECODE_MID_FREQ = 0x31,
    COMMON_PARAM_CONFIG=0x32,
    GET_CH_SIGNAL_AMP=0x33,
    SUB_CH_ONOFF=0x34,
    SUB_CH_BANDWIDTH=0x35,
    SUB_CH_MIDDLE_FREQ=0x36,
    NET_10G_LOCAL_SET=0x37,
    NET_10G_REMOTE_SET=0x38,
    NET_10G_ONOFF_SET=0x39,
    NET_1G_ONOFF_SET=0x40,
    UDP_CLIENT_INFO_NOTIFY=0x41,

    /* disk */
    DISK_REFRESH_FILE_BUFFER=0x70,
    DISK_READ_FILE_INFO=71,
    DISK_FIND_FILE=0x72,
    DISK_START_SAVE_FILE=0x73,
    DISK_STOP_SAVE_FILE = 0x74,
    DISK_DELETE_FILE=0x75,
    DISK_START_BACKTRACE_FILE=0x76,
    DISK_STOP_BACKTRACE_FILE=0x77,
    DISK_GET_INFO=0x78,
    DISK_FORMAT=0x79,
}IOCTL_CMD;


#define DMA_RF_IOC_MAGIC  'k'

#define IOCTL_ENABLE_DISABLE    _IOW(DMA_RF_IOC_MAGIC, ENABLE_DISABLE, uint32_t)
#define IOCTL_TRANSLEN      _IOW(DMA_RF_IOC_MAGIC, TRANSLEN, uint32_t)
#define IOCTL_EXTRACT_CH0   _IOW(DMA_RF_IOC_MAGIC, EXTRACT_CH0, uint32_t)
#define IOCTL_EXTRACT_CH1   _IOW(DMA_RF_IOC_MAGIC, EXTRACT_CH1, uint32_t)
#define IOCTL_EXTRACT_CH2   _IOW(DMA_RF_IOC_MAGIC, EXTRACT_CH2, uint32_t)
#define IOCTL_EXTRACT_CH3   _IOW(DMA_RF_IOC_MAGIC, EXTRACT_CH3, uint32_t)
#define IOCTL_SMOOTH_CH0    _IOW(DMA_RF_IOC_MAGIC, SMOOTH_CH0, uint32_t)
#define IOCTL_SMOOTH_CH1    _IOW(DMA_RF_IOC_MAGIC, SMOOTH_CH1, uint32_t)
#define IOCTL_SMOOTH_CH2    _IOW(DMA_RF_IOC_MAGIC, SMOOTH_CH2, uint32_t)
#define IOCTL_SMOOTH_CH3    _IOW(DMA_RF_IOC_MAGIC, SMOOTH_CH3, uint32_t)
#define IOCTL_DATA_PATTERN  _IOW(DMA_RF_IOC_MAGIC, DATA_PATTERN, uint32_t)
#define IOCTL_DATA_PATTERN_FREQ  _IOW(DMA_RF_IOC_MAGIC, DATA_PATTERN_FREQ, uint32_t)
#define IOCTL_BUFFER_LEN    _IOW(DMA_RF_IOC_MAGIC, BUFFER_LEN, uint32_t)

#define IOCTL_CALIBRATE_CH0    _IOW(DMA_RF_IOC_MAGIC, CALIBRATE_CH0, uint32_t)
#define IOCTL_CALIBRATE_CH1    _IOW(DMA_RF_IOC_MAGIC, CALIBRATE_CH1, uint32_t)
#define IOCTL_CALIBRATE_CH2    _IOW(DMA_RF_IOC_MAGIC, CALIBRATE_CH2, uint32_t)
#define IOCTL_CALIBRATE_CH3    _IOW(DMA_RF_IOC_MAGIC, CALIBRATE_CH3, uint32_t)
#define IOCTL_FPGA_VERSION  _IOW(DMA_RF_IOC_MAGIC, IOCTL_FPGA_VERSION, uint32_t)
#define IOCTL_AGC_CH0    _IOW(DMA_RF_IOC_MAGIC, AGC_CH0, uint32_t)
#define IOCTL_AGC_CH1    _IOW(DMA_RF_IOC_MAGIC, AGC_CH1, uint32_t)
#define IOCTL_AGC_CH2    _IOW(DMA_RF_IOC_MAGIC, AGC_CH2, uint32_t)
#define IOCTL_AGC_CH3    _IOW(DMA_RF_IOC_MAGIC, AGC_CH3, uint32_t)
#define IOCTL_FFT_SIZE_CH0    _IOW(DMA_RF_IOC_MAGIC, FFT_SIZE_CH0, uint32_t)
#define IOCTL_FFT_SIZE_CH1    _IOW(DMA_RF_IOC_MAGIC, FFT_SIZE_CH1, uint32_t)
#define IOCTL_FFT_SIZE_CH2    _IOW(DMA_RF_IOC_MAGIC, FFT_SIZE_CH2, uint32_t)
#define IOCTL_FFT_SIZE_CH3    _IOW(DMA_RF_IOC_MAGIC, FFT_SIZE_CH3, uint32_t)
#define IOCTL_FFT_HDR_PARAM  _IOW(DMA_RF_IOC_MAGIC, FFT_HDR_PARAM, uint32_t)
#define IOCTL_RF_PARAM  _IOW(DMA_RF_IOC_MAGIC, RUN_RF_PARAM, uint32_t)
#define IOCTL_RUN_CONFIG_PARAM  _IOW(DMA_RF_IOC_MAGIC, RUN_CONFIG_PARAM, uint32_t)
#define IOCTL_RUN_DEC_PARAM  _IOW(DMA_RF_IOC_MAGIC, RUN_DEC_PARAM, uint32_t)
#define IOCTL_RUN_NET_PARAM  _IOW(DMA_RF_IOC_MAGIC, RUN_NET_PARAM, uint32_t)
#define IOCTL_KEEPALIVE_PARAM _IOW(DMA_RF_IOC_MAGIC,KEEPALIVE_PARAM, uint32_t)
#define IOCTL_STA_INFO_PARAM _IOW(DMA_RF_IOC_MAGIC, STA_INFO_PARAM, uint32_t)
#define IOCTL_QUIET_NOISE_CH0  _IOW(DMA_RF_IOC_MAGIC, QUIET_NOISE_CH0, uint32_t)
#define IOCTL_QUIET_NOISE_CH1  _IOW(DMA_RF_IOC_MAGIC, QUIET_NOISE_CH1, uint32_t)
#define IOCTL_QUIET_NOISE_CH2  _IOW(DMA_RF_IOC_MAGIC, QUIET_NOISE_CH2, uint32_t)
#define IOCTL_QUIET_NOISE_CH3  _IOW(DMA_RF_IOC_MAGIC, QUIET_NOISE_CH3, uint32_t)
#define IOCTL_FREQUENCY_BAND_CONFIG0  _IOW(DMA_RF_IOC_MAGIC, FREQUENCY_BAND_CONFIG0, uint32_t)
#define IOCTL_FREQUENCY_BAND_CONFIG1  _IOW(DMA_RF_IOC_MAGIC, FREQUENCY_BAND_CONFIG1, uint32_t)
#define IOCTL_GET_DDC_REGISTER_VALUE _IOW(DMA_RF_IOC_MAGIC, GET_DDC_REGISTER_VALUE, uint32_t)
#define IOCTL_SET_DDC_REGISTER_VALUE _IOW(DMA_RF_IOC_MAGIC, SET_DDC_REGISTER_VALUE, uint32_t)
#define IOCTL_SIGNAL_THRESHOLD_CH0    _IOW(DMA_RF_IOC_MAGIC, SIGNAL_THRESHOLD_CH0, uint32_t)
#define IOCTL_DECODE_MID_FREQ    _IOW(DMA_RF_IOC_MAGIC, DECODE_MID_FREQ, uint32_t)
#define IOCTL_COMMON_PARAM_CMD    _IOW(DMA_RF_IOC_MAGIC, COMMON_PARAM_CONFIG, uint32_t)
#define IOCTL_GET_CH_SIGNAL_AMP    _IOW(DMA_RF_IOC_MAGIC, GET_CH_SIGNAL_AMP, uint32_t)
#define IOCTL_SUB_CH_ONOFF    _IOW(DMA_RF_IOC_MAGIC, SUB_CH_ONOFF, uint32_t)
#define IOCTL_SUB_CH_BANDWIDTH    _IOW(DMA_RF_IOC_MAGIC, SUB_CH_BANDWIDTH, uint32_t)
#define IOCTL_SUB_CH_MIDDLE_FREQ    _IOW(DMA_RF_IOC_MAGIC, SUB_CH_MIDDLE_FREQ, uint32_t)
#define IOCTL_NET_10G_LOCAL_SET    _IOW(DMA_RF_IOC_MAGIC, NET_10G_LOCAL_SET, uint32_t)
#define IOCTL_NET_10G_REMOTE_SET    _IOW(DMA_RF_IOC_MAGIC, NET_10G_REMOTE_SET, uint32_t)
#define IOCTL_NET_10G_ONOFF_SET    _IOW(DMA_RF_IOC_MAGIC, NET_10G_ONOFF_SET, uint32_t)
#define IOCTL_NET_1G_IQ_ONOFF_SET    _IOW(DMA_RF_IOC_MAGIC, NET_1G_ONOFF_SET, uint32_t)
#define IOCTL_UDP_CLIENT_INFO_NOTIFY    _IOW(DMA_RF_IOC_MAGIC, UDP_CLIENT_INFO_NOTIFY, uint32_t)



/* disk ioctl */
#define IOCTL_DISK_REFRESH_FILE_BUFFER    _IOW(DMA_RF_IOC_MAGIC, DISK_REFRESH_FILE_BUFFER, uint32_t)
#define IOCTL_DISK_READ_FILE_INFO    _IOW(DMA_RF_IOC_MAGIC, DISK_READ_FILE_INFO, uint32_t)
#define IOCTL_DISK_FIND_FILE_INFO    _IOW(DMA_RF_IOC_MAGIC, DISK_FIND_FILE, uint32_t)
#define IOCTL_DISK_START_SAVE_FILE_INFO    _IOW(DMA_RF_IOC_MAGIC, DISK_START_SAVE_FILE, uint32_t)
#define IOCTL_DISK_STOP_SAVE_FILE_INFO    _IOW(DMA_RF_IOC_MAGIC, DISK_STOP_SAVE_FILE, uint32_t)
#define IOCTL_DISK_DELETE_FILE_INFO    _IOW(DMA_RF_IOC_MAGIC, DISK_DELETE_FILE, uint32_t)
#define IOCTL_DISK_START_BACKTRACE_FILE_INFO    _IOW(DMA_RF_IOC_MAGIC, DISK_START_BACKTRACE_FILE, uint32_t)
#define IOCTL_DISK_STOP_BACKTRACE_FILE_INFO    _IOW(DMA_RF_IOC_MAGIC, DISK_STOP_BACKTRACE_FILE, uint32_t)
#define IOCTL_DISK_GET_INFO    _IOW(DMA_RF_IOC_MAGIC, DISK_GET_INFO, uint32_t)
#define IOCTL_DISK_FORMAT    _IOW(DMA_RF_IOC_MAGIC, DISK_FORMAT, uint32_t)

enum _data_type{
    IO_IQ_TYPE = 0,
    IO_SPECTRUM_TYPE,
    IO_DQ_TYPE,
};


/* kernel client info */
typedef struct  _STATION_INFO{
    uint32_t index;
    int32_t connfd;
    struct sockaddr_in client;
    struct timespec keepalive_time;
    SNIFFER_DATA_REPORT_ST target_addr[MAX_CHANNEL_NUM];
}__attribute__ ((packed)) STATION_INFO;


struct udp_client_info{
    uint8_t cid;
    uint32_t ipaddr;
    uint16_t port;
#ifdef SUPPORT_NET_WZ
    uint32_t wz_ipaddr; /* 添加上位机万兆IP和端口*/
    uint16_t wz_port;
#endif
}__attribute__ ((packed));


typedef struct  _COMMON_PARAM_ST{
    uint8_t type;
    uint8_t buf[MAX_COMMON_PARAM_LEN];
}__attribute__ ((packed)) COMMON_PARAM_ST; 

struct  ioctl_data_t{
    uint8_t ch;
    uint8_t data[MAX_COMMON_PARAM_LEN];
}__attribute__ ((packed)); 


struct  band_table_t{
    uint32_t extract_factor;
    uint32_t filter_factor;
    uint32_t band;
}__attribute__ ((packed)); 

struct io_decode_param_st{
    uint8_t  cid;
    uint8_t  sub_ch;
    uint8_t  d_method;
    uint32_t d_bandwidth;  
    uint64_t center_freq;
};

typedef enum _io_dq_method_code{
    IO_DQ_MODE_AM = 0x00,
    IO_DQ_MODE_FM = 0x01,
    IO_DQ_MODE_WFM = 0x01,
    IO_DQ_MODE_LSB = 0x02,
    IO_DQ_MODE_USB = 0x02,
    IO_DQ_MODE_CW = 0x03,
    IO_DQ_MODE_IQ = 0x07,
}io_dq_method_code;

#ifdef SUPPORT_AMBIENT_TEMP_HUMIDITY
    #ifdef SUPPORT_TEMP_HUMIDITY_SI7021
    #define io_get_ambient_temperature() si7021_read_temperature()
    #define io_get_ambient_humidity()    si7021_read_humidity()
    #else
    #define io_get_ambient_temperature() 0
    #define io_get_ambient_humidity()    0
    #endif
#else
    #define io_get_ambient_temperature() 0
    #define io_get_ambient_humidity()    0
#endif

extern void io_init(void);
extern int8_t io_set_enable_command(uint8_t type, uint8_t ch, uint32_t fftsize);
extern int8_t io_set_work_mode_command(void *data);
extern int8_t io_set_para_command(uint8_t type, uint8_t ch, void *data);
extern int16_t io_get_adc_temperature(void);
extern void io_set_dq_param(void *pdata);
extern void io_set_smooth_time(uint16_t factor);
extern void io_set_fft_size(uint32_t ch, uint32_t fft_size);
extern uint8_t  io_set_network_to_interfaces(void *netinfo);
extern int32_t io_set_sta_info_param(STATION_INFO *data);
extern int32_t io_set_refresh_keepalive_time(uint32_t index);
extern int32_t io_set_extract_ch0(uint32_t ch, uint32_t bandwith);
extern int io_read_more_info_by_name(const char *name, void *info, int32_t (*iofunc)(void *));
extern int32_t io_find_file_info(void *arg);
#endif
