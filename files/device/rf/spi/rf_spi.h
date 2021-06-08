#ifndef RF_SPI_H
#define RF_SPI_H

#define SPI_PATH_NAME_MAX_LEN 64
#define SPI_MAX_NUM 8


typedef enum _rf_spi_get_cmd{
#if defined(SUPPORT_RF_SPI_TF713)
    SPI_RF_WORK_PARA_GET    = 0x07,         //工作参数查询
    SPI_RF_TEMPRATURE_GET   = 0x31,         //温度查询
    SPI_RF_BIT_QUERY        = 0x33,         //BIT查询
    SPI_RF_HANDSHAKE        = 0x3d,         //握手
#else
    SPI_RF_FREQ_GET = 0x10,
    SPI_RF_GAIN_GET = 0x11,
    SPI_RF_MIDFREQ_BW_FILTER_GET = 0x12,
    SPI_RF_NOISE_MODE_GET=0x13,
    SPI_RF_GAIN_CALIBRATE_GET = 0x14,
    SPI_RF_MIDFREQ_GAIN_GET = 0x15,
    SPI_RF_TEMPRATURE_GET = 0x41,
#endif
}rf_spi_get_cmd_code;

typedef enum _rf_spi_set_cmd{
#if defined(SUPPORT_RF_SPI_TF713)
    SPI_RF_FREQ_SET                 = 0x0,    //调节频率
    SPI_RF_GAIN_SET                 = 0x1,    //射频衰减
    SPI_RF_MIDFREQ_GAIN_SET         = 0x2,    //中频衰减
    SPI_RF_NOISE_MODE_SET           = 0x3,    //工作模式
    SPI_RF_MIDFREQ_BW_FILTER_SET    = 0x4,    //中频带宽
    SPI_RF_CALIBRATE_SOURCE_SET     = 0x6,    //校正-馈电开关
#else
    SPI_RF_FREQ_SET = 0x01,
    SPI_RF_GAIN_SET = 0x02,
    SPI_RF_MIDFREQ_BW_FILTER_SET = 0x03,
    SPI_RF_NOISE_MODE_SET = 0x04,
    SPI_RF_GAIN_CALIBRATE_SET = 0x05,
    SPI_RF_MIDFREQ_GAIN_SET = 0x06,
    SPI_RF_CALIBRATE_SOURCE_SET = 0x40,
#endif
}rf_spi_set_cmd_code;


typedef enum _spi_func_code{
    SPI_FUNC_RF = 0x01,
    SPI_FUNC_CLOCK,
    SPI_FUNC_AD,
    SPI_FUNC_NULL,
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
    ssize_t send_len;/* SPI发送数据(包括状态等字节)字节或bit长度 */
    ssize_t recv_len;/* SPI接收数据(包括状态等字节)字节或bit长度 */
};


extern    int spi_rf_init(void); 
extern    ssize_t spi_rf_set_command(int ch, rf_spi_set_cmd_code cmd, void *data);
extern    ssize_t spi_rf_get_command(int ch, rf_spi_get_cmd_code cmd, void *data, int len);
#endif

