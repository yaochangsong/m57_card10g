#ifndef _SPM_DEV_H
#define _SPM_DEV_H

#include "config.h"

//GPIO
enum gpio_func_code{
    GPIO_RF_POWER_ONOFF     = 0x00,
    GPIO_GPS_LOCK           = 0x01,
    GPIO_FUNC_ADC_STATUS    = 0x02,
    GPIO_FUNC_COMPASS1      = 0x03,   //电子罗盘
    GPIO_FUNC_COMPASS2      = 0x04,
    GPIO_FUNC_LOW_NOISER    = 0x05,   //低噪放

};

#define RF_POWER_ON()           gpio_raw_write_value(GPIO_RF_POWER_ONOFF, 0)
#define RF_POWER_OFF()          gpio_raw_write_value(GPIO_RF_POWER_ONOFF, 1)
#define GPS_LOCKED()            gpio_raw_write_value(GPIO_GPS_LOCK, 1)
#define GPS_UNLOCKED()          gpio_raw_write_value(GPIO_GPS_LOCK, 0)

#define SW_TO_UART0_READ()      gpio_raw_write_value(GPIO_FUNC_COMPASS1, 0)
#define SW_TO_UART0_WRITE()     gpio_raw_write_value(GPIO_FUNC_COMPASS1, 1)
#define SW_TO_UART1_READ()      gpio_raw_write_value(GPIO_FUNC_COMPASS2, 0)
#define SW_TO_UART1_WRITE()     gpio_raw_write_value(GPIO_FUNC_COMPASS2, 1)
#define SW_TO_UART2_READ()      gpio_raw_write_value(GPIO_FUNC_LOW_NOISER, 0)
#define SW_TO_UART2_WRITE()     gpio_raw_write_value(GPIO_FUNC_LOW_NOISER, 1)

#define ADC_READ_STATUS(value)  gpio_raw_read_value(GPIO_FUNC_ADC_STATUS, value)

//UART
enum uart_func_code{
  UART_GPS_CODE = 0x00,
  UART_LCD_CODE = 0x01,
  UART_RS485_CODE = 0x02,
  UART_NOISER_CODE = 0x03,
  UART_COMPASS_1_6G_CODE = 0x04,
  UART_COMPASS2_30_1350M_CODE = 0x05
};


extern struct _spm_stream* spm_dev_get_stream(int *count);
extern struct gpio_node_info* dev_get_gpio(int *count);
extern struct uart_info_t* dev_get_uart(int *count);
extern struct band_table_t* broadband_factor_table(int *count);
extern struct band_table_t* narrowband_demo_factor_table(int *count);
extern struct band_table_t* narrowband_iq_factor_table(int *count);


#endif
