#ifndef _GPIO_RAW_H
#define _GPIO_RAW_H

#include "config.h"

struct gpio_node_info{
    int pin_num;
    char direction[8];
    int value;
    int func_code;
    char func_name[32];
    int fd;
};

/* 
  GPIO_CHIP_ARCH :
    /sys/class/gpio, the name of the chip appear to be the 1st GPIO of the controller 
  GPIO_START_OFFSET:
    ZynqMP: 78 GPIO signals for device pins
    zynq: 54  GPIO signals for device pins
*/
#if CONFIG_ARCH_ARM
#if (defined CONFIG_XILINX_ARCH_ZYNQ) 
#define GPIO_CHIP_ARCH 906
#define EMIO_START_OFFSET 54
#elif defined(CONFIG_XILINX_ARCH_ZYNQMP)
#define GPIO_CHIP_ARCH 338  
#define EMIO_START_OFFSET 78
#else
#error "[EMIO]Undefined Xilinx Arch: Zynq or ZynqMP!!!"
#endif
#define GPIO_RAW_BASE_OFFSET (GPIO_CHIP_ARCH + EMIO_START_OFFSET)
#endif
extern int gpio_raw_init(void);
extern int gpio_raw_write_value(int func_code, int value);
extern int gpio_raw_read_value(int func_code, int *value);


#endif
