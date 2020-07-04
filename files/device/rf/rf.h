#ifndef RF_H
#define RF_H

#define GPIO_BASE_OFFSET              960
struct calibration_source_t{
    uint8_t source;
    uint32_t middle_freq_mhz;
    float power;
}__attribute__ ((packed));

extern    int8_t   rf_init(void);                                                      //射频初始化
extern    uint8_t  rf_set_interface(uint8_t cmd,uint8_t ch,void *data);                 //射频设置接口
extern    uint8_t  rf_read_interface(uint8_t cmd,uint8_t ch,void *data);                //射频查询接口
#endif

