#ifndef _SPM_DEV_H
#define _SPM_DEV_H

#include "config.h"


//UART
enum uart_func_code{
  UART_GPS_CODE = 0x00,
  UART_LCD_CODE = 0x01,
};

#define RF_CH_POWER_ONOFF(ch, val) gpio_raw_write_value(ch, val)

extern struct _spm_stream* spm_dev_get_stream(int *count);
extern struct gpio_node_info* dev_get_gpio(int *count);
extern struct uart_info_t* dev_get_uart(int *count);
struct band_table_t* broadband_factor_table(int *count);
struct band_table_t* narrowband_demo_factor_table(int *count);
struct band_table_t* narrowband_iq_factor_table(int *count);

#endif
