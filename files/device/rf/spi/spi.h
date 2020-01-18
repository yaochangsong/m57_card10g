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
*  Rev 1.0   19 Nov 2019   yaochangsong
*  Initial revision.
******************************************************************************/
#ifndef _SPI_RF_H
#define _SPI_RF_H

#define SPI_PATH_NAME_MAX_LEN 64
#define SPI_MAX_NUM 8


typedef enum _rf_spi_get_cmd{
    SPI_RF_FREQ_GET = 0x10,
    SPI_RF_GAIN_GET = 0x11,
    SPI_RF_MIDFREQ_BW_FILTER_GET = 0x12,
    SPI_RF_NOISE_MODE_GET=0x13,
    SPI_RF_GAIN_CALIBRATE_GET = 0x14,
    SPI_RF_MIDFREQ_GAIN_GET = 0x15,
    SPI_RF_TEMPRATURE_GET = 0x41
}rf_spi_get_cmd_code;

typedef enum _rf_spi_set_cmd{
    SPI_RF_FREQ_SET = 0x01,
    SPI_RF_GAIN_SET = 0x02,
    SPI_RF_MIDFREQ_BW_FILTER_SET = 0x03,
    SPI_RF_NOISE_MODE_SET = 0x04,
    SPI_RF_GAIN_CALIBRATE_SET = 0x05,
    SPI_RF_MIDFREQ_GAIN_SET = 0x06,
    SPI_RF_CALIBRATE_SOURCE_SET = 0x40,
}rf_spi_set_cmd_code;


typedef enum _spi_func_code{
    SPI_FUNC_RF = 0x01,
    SPI_FUNC_CLOCK,
    SPI_FUNC_AD,
}spi_func_code;


struct rf_spi_node_info{
    char *name;
    spi_func_code func_code; 
    int pin_cs;
    int fd;
    int pin_fd;
    char *info;
};
struct rf_spi_cmd_info{
    rf_spi_set_cmd_code scode;
    rf_spi_get_cmd_code gcode;
    size_t send_byte_len;/* SPI发送数据(包括状态等字节)字节长度 */
    size_t recv_byte_len;/* SPI接收数据(包括状态等字节)字节长度 */
};

extern pthread_mutex_t spi_mutex;

#define LOCK_SPI() do { \
	pthread_mutex_lock(&spi_mutex); \
} while (0)

#define UNLOCK_SPI() do { \
	pthread_mutex_unlock(&spi_mutex); \
} while (0)


extern int spi_init(void);
extern int spi_adc_init(void);
extern int spi_clock_init(void);
extern ssize_t spi_rf_get_command(rf_spi_get_cmd_code cmd, void *data);
extern ssize_t spi_rf_set_command(rf_spi_set_cmd_code cmd, void *data);
extern int spi_clock_init_after(void);

#endif
