#ifndef RF_H
#define RF_H

#define GPIO_BASE_OFFSET              960
struct calibration_source_t{
    uint8_t source;
    uint32_t middle_freq_khz;
    int8_t power;
}__attribute__ ((packed));

struct  calibration_source_args_t{
    struct calibration_source_t rf_args;
    uint64_t s_freq;
    uint64_t e_freq;
    uint64_t step;
    uint32_t r_time_ms;
}__attribute__ ((packed)); 
extern    int8_t   rf_init(void);                                                      //射频初始化
extern    uint8_t  rf_set_interface(uint8_t cmd,uint8_t ch,void *data);                 //射频设置接口
extern    int8_t  rf_read_interface(uint8_t cmd,uint8_t ch,void *data, va_list ap);                //射频查询接口
extern    int rf_calibration_source_start(void *args);
extern    int rf_calibration_source_stop(void *args);
#endif

