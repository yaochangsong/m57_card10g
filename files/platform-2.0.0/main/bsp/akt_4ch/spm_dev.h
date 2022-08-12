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
#include "../../executor/spm/spm.h"
#include "../../executor/spm/spm_fpga/spm_fpga.h"
#include "../../device/gpio/gpio_raw.h"


enum gpio_func_code{
    GPIO_POWER_CH0          = 0x00,
    GPIO_POWER_CH1          = 0x01,
    GPIO_POWER_CH2          = 0x02,
    GPIO_POWER_CH3          = 0x03,
};


//UART
enum uart_func_code{
  UART_LCD_CODE = 0x00,
};


extern struct _spm_stream* spm_dev_get_stream(int *count);
extern struct gpio_node_info* dev_get_gpio(int *count);
extern struct uart_info_t* dev_get_uart(int *count);

#endif
