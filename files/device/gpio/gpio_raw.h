#ifndef _GPIO_RAW_H
#define _GPIO_RAW_H

#define GPIO_RAW_BASE_OFFSET (960)
#define SW_TO_AD_MODE()         gpio_raw_write_value(GPIO_FUNC_ADC, 0)
#define SW_TO_BACKTRACE_MODE()  gpio_raw_write_value(GPIO_FUNC_ADC, 1)
enum gpio_func_code{
    GPIO_FUNC_ADC = 0x01,
};
    

struct gpio_node_info{
    int pin_num;
    char direction[8];
    int value;
    enum gpio_func_code func_code;
    char func_name[32];
    int fd;
};

extern int gpio_raw_init(void);
extern int gpio_raw_write_value(enum gpio_func_code func_code, int value);

#endif