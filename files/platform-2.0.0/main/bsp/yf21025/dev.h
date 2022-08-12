#ifndef _SPM_DEV_H
#define _SPM_DEV_H

#include "config.h"

enum gpio_func_code{
    GPIO_POWER_CH0          = 0x00,
    GPIO_POWER_CH1          = 0x01,
    GPIO_POWER_CH2          = 0x02,
    GPIO_POWER_CH3          = 0x03,
    GPIO_LOCAL_CTR          = 0x04,
};

//UART
enum uart_func_code{
  UART_GPS_CODE = 0x00,
  UART_LCD_CODE = 0x01,
};

#define RF_CH_POWER_ONOFF(ch, val) gpio_raw_write_value(ch, val)
#define LED_LOCAL_ONOFF(val) gpio_raw_write_value(GPIO_LOCAL_CTR, val)

extern struct _spm_stream* spm_dev_get_stream(int *count);
extern struct gpio_node_info* dev_get_gpio(int *count);
extern struct uart_info_t* dev_get_uart(int *count);
struct band_table_t* broadband_factor_table(int *count);
struct band_table_t* narrowband_demo_factor_table(int *count);
struct band_table_t* narrowband_iq_factor_table(int *count);

#endif
