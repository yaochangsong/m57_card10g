#ifndef _SPM_DEV_H
#define _SPM_DEV_H

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <stdint.h>
#include "config.h"


enum gpio_func_code{
    GPIO_RF_POWER_ONOFF     = 0x00,
    GPIO_GPS_LOCK           = 0x01,
};

#define RF_POWER_ON()           gpio_raw_write_value(GPIO_RF_POWER_ONOFF, 0)
#define RF_POWER_OFF()          gpio_raw_write_value(GPIO_RF_POWER_ONOFF, 1)
#define GPS_LOCKED()            gpio_raw_write_value(GPIO_GPS_LOCK, 1)
#define GPS_UNLOCKED()          gpio_raw_write_value(GPIO_GPS_LOCK, 0)


//UART
enum uart_func_code{
  UART_GPS_CODE = 0x00,
};

extern struct _spm_stream* spm_dev_get_stream(int *count);
extern struct gpio_node_info* dev_get_gpio(int *count);
extern struct uart_info_t* dev_get_uart(int *count);

extern struct band_table_t* broadband_factor_table(int *count);
extern struct band_table_t* narrowband_demo_factor_table(int *count);
extern struct band_table_t* narrowband_iq_factor_table(int *count);

#endif
