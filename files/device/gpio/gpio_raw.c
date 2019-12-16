/******************************************************************************
*  Copyright 2019, Showay Technology Dev Co.,Ltd.
*  ---------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Showay Technology Dev Co.,Ltd. (C) 2019
******************************************************************************/
/*****************************************************************************     
*  Rev 1.0   16 Dec 2019   yaochangsong
*  Initial revision.
******************************************************************************/
#include "config.h"
#include "gpio_raw.h"

struct gpio_node_info gpio_node[] ={
    /* pin   direction  default gpio value   func_code    func_name    fd */
    {63,      "out",       0,               GPIO_FUNC_ADC,  "ADC gpio",    -1 },  /* low:  adc ; high : backtrace*/
};

static int gpio_export(int pin_number)
{
    int ret = 0;
    char num_str[8] = {0};
    int fd;
    fd = open("/sys/class/gpio/export", O_WRONLY);
    if(fd < 0){
        printf_err("open faild\n");
        return -1;
    }
        
    sprintf(num_str, "%d", pin_number+GPIO_RAW_BASE_OFFSET);
    ret = write(fd, num_str, strlen(num_str)+1);
    close(fd);
    
    return ret;
}

static int gpio_write(int pin_number,int value)
{
    int ret = 0;
    char w_str[128] = {0};
    char v_str[8] = {0};
    int fd;
    sprintf(w_str, "/sys/class/gpio/gpio%d/value", pin_number+GPIO_RAW_BASE_OFFSET);
    fd = open(w_str, O_WRONLY);
    if(fd < 0){
        printf_err("open faild\n");
        return -1;
    }
    sprintf(v_str, "%d", value);
    lseek(fd, 0, SEEK_SET);
    printf_warn("w_str=%s, v_str=%s, %d\n", w_str, v_str, strlen(v_str));
    ret = write(fd, v_str, strlen(v_str)+1);
    close(fd);
    
    return ret;
}

/* @direction: "in", "out" */
static int gpio_set_direction(int pin_number,char *direction)
{
    int ret = 0;
    char w_str[128] = {0};
    char v_str[8] = {0};
    int fd;
    sprintf(w_str, "/sys/class/gpio/gpio%d/direction", pin_number+GPIO_RAW_BASE_OFFSET);
    fd = open(w_str, O_WRONLY);
    if(fd < 0){
        printf_err("open faild\n");
        return -1;
    }
    sprintf(v_str, "%s", direction);
    ret = write(fd, v_str, strlen(v_str)+1);
    close(fd);
    
    return ret;
}


int gpio_raw_init(void)
{
    int ret = -1;
    struct gpio_node_info *ptr = &gpio_node;

    for(int i = 0; i< ARRAY_SIZE(gpio_node); i++){
        if(ptr[i].pin_num >= 0){
            gpio_export(ptr[i].pin_num);
            if(gpio_set_direction(ptr[i].pin_num, ptr[i].direction) == -1){
                printf_err("GPIO set direction: %s faild\n", ptr[i].func_name);
                ret = -1;
                //break;
            }
            if(gpio_write(ptr[i].pin_num, ptr[i].value) == -1){
                printf_err("GPIO write: %s faild\n", ptr[i].func_name);
                ret = -1;
                //break;
            }
            printf_note("GPIO init: %s OK\n", ptr[i].func_name);
        }
    }
    return 0;
}


int gpio_raw_write_value(enum gpio_func_code func_code, int value)
{
    int found = 0;
    struct gpio_node_info *ptr = &gpio_node;
    for(int i = 0; i< ARRAY_SIZE(gpio_node); i++){
        if(ptr[i].func_code == func_code){
            if(gpio_set_direction(ptr[i].pin_num, "out") == -1){
                printf_err("GPIO set direction: %s faild\n", ptr[i].func_name);
            }
            if(gpio_write(ptr[i].pin_num, value) == -1){
                printf_warn("write gpio %s: %d faild!\n", ptr[i].func_name, value);
                return -1;
            }
            printf_note("write gpio %s[%d]: %d OK!\n", ptr[i].func_name, ptr[i].pin_num,value);
            found = 1;
            break;
        }
    }
    if(found == 0){
        return -1;
    }
    return 0;
}
