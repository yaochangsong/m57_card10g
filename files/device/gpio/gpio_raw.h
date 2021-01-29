#ifndef _GPIO_RAW_H
#define _GPIO_RAW_H

#define GPIO_RAW_BASE_OFFSET (416)
#define SW_TO_AD_MODE()         gpio_raw_write_value(GPIO_FUNC_ADC, 0)
#define SW_TO_BACKTRACE_MODE()  gpio_raw_write_value(GPIO_FUNC_ADC, 1)

#define SW_TO_UART0_READ()      gpio_raw_write_value(GPIO_FUNC_COMPASS1, 0)
#define SW_TO_UART0_WRITE()     gpio_raw_write_value(GPIO_FUNC_COMPASS1, 1)
#define SW_TO_UART1_READ()      gpio_raw_write_value(GPIO_FUNC_COMPASS2, 0)
#define SW_TO_UART1_WRITE()     gpio_raw_write_value(GPIO_FUNC_COMPASS2, 1)
#define SW_TO_UART2_READ()      gpio_raw_write_value(GPIO_FUNC_LOW_NOISER, 0)
#define SW_TO_UART2_WRITE()     gpio_raw_write_value(GPIO_FUNC_LOW_NOISER, 1)

enum gpio_func_code{
    GPIO_FUNC_ADC           = 0x01,
    GPIO_FUNC_ADC_STATUS    = 0x02,
    GPIO_FUNC_COMPASS1      = 0x03,   //电子罗盘
    GPIO_FUNC_COMPASS2      = 0x04,
    GPIO_FUNC_LOW_NOISER    = 0x05,   //低噪放
    GPIO_RF_CH1             = 0x06,
    GPIO_RF_CH2             = 0x07,
    GPIO_RF_CH3             = 0x08,
    GPIO_RF_CH4             = 0x09,
    GPIO_RF_CH5             = 0x0a,
    GPIO_RF_CH6             = 0x0b,
};

#define SET_GPIO_ARRAY(v1, v2, v3) do{      \
        gpio_raw_write_value(GPIO_RF_CH1, v1);           \
        gpio_raw_write_value(GPIO_RF_CH2, v2);           \
        gpio_raw_write_value(GPIO_RF_CH3, v3);           \
    }while(0);


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
extern int gpio_raw_read_value(enum gpio_func_code func_code, int *value);


#endif
