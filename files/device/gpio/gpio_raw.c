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
    {63,      "out",       0,               GPIO_FUNC_ADC,          "ADC&Backtrace gpio ctrl",    -1 },  /* low:  adc ; high : backtrace*/
    {62,      "in",        -1,              GPIO_FUNC_ADC_STATUS,   "ADC Status",                 -1 },
    {4,      "out",        0,               GPIO_FUNC_LOW_NOISER,     "RS485 0 ctrl",                 -1 },
    {5,      "out",        0,               GPIO_FUNC_COMPASS2,     "RS485 1 ctrl",                 -1 },
    {6,      "out",        0,               GPIO_FUNC_COMPASS1,   "RS485 2 ctrl",                 -1 },
#ifdef SUPPORT_PROJECT_WD_XCR_40G
    {8,      "out",        0,               GPIO_RF_CH1,            "RF C1",                        -1 },
    {9,      "out",        0,               GPIO_RF_CH2,            "RF C2",                        -1 },
    {10,     "out",        0,               GPIO_RF_CH3,            "RF C3",                        -1 },
#endif
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
    ret = write(fd, v_str, strlen(v_str)+1);
    close(fd);
    
    return ret;
}

static int gpio_read(int pin_number,int *value)
{
    int ret = 0;
    char w_str[128] = {0};
    char v_str[8] = {0}, *end;
    int fd;
    int val;
    sprintf(w_str, "/sys/class/gpio/gpio%d/value", pin_number+GPIO_RAW_BASE_OFFSET);
    fd = open(w_str, O_RDONLY);
    if(fd < 0){
        printf_err("open faild\n");
        return -1;
    }
    ret = read(fd, v_str, sizeof(v_str));
    val = (int) strtol(v_str, &end, 10);
    if (v_str == end){
        printf_note("read faild\n");
        close(fd);
        return -EINVAL;
    }
    *value = val;
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
    int ret = 0;
    struct gpio_node_info *ptr = &gpio_node;
    for(int i = 0; i< ARRAY_SIZE(gpio_node); i++){
        if(ptr[i].pin_num >= 0){
            gpio_export(ptr[i].pin_num);
            if(gpio_set_direction(ptr[i].pin_num, ptr[i].direction) == -1){
                printf_err("GPIO set direction: %s faild\n", ptr[i].func_name);
                ret = -1;
                continue;
            }
            if(ptr[i].value >= 0){
                if(gpio_write(ptr[i].pin_num, ptr[i].value) == -1){
                    printf_err("GPIO write: %s faild\n", ptr[i].func_name);
                    ret = -1;
                    continue;
                }
            }
            printf_note("GPIO init OK[%s]\n", ptr[i].func_name);
            if(ptr[i].func_code == GPIO_FUNC_ADC){
                    printf_note("Mode value:%d[%s]\n",ptr[i].value, ptr[i].value == 0?"ADC Mode":"Backtrace Mode");
            }

        }
    }
    return ret;
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
            if(ptr[i].value >= 0){
                if(gpio_write(ptr[i].pin_num, value) == -1){
                    printf_warn("write gpio %s: %d faild!\n", ptr[i].func_name, value);
                    return -1;
                }
            }else{
                return -1;
            }
            
            printf_info("write gpio %s[%d]: %d OK!\n", ptr[i].func_name, ptr[i].pin_num,value);
            found = 1;
            break;
        }
    }
    if(found == 0){
        return -1;
    }
    return 0;
}

int gpio_raw_read_value(enum gpio_func_code func_code, int *value)
{
    int found = 0;
    struct gpio_node_info *ptr = &gpio_node;
    for(int i = 0; i< ARRAY_SIZE(gpio_node); i++){
        if(ptr[i].func_code == func_code){
            if(gpio_set_direction(ptr[i].pin_num, "in") == -1){
                printf_err("GPIO set direction: %s faild\n", ptr[i].func_name);
            }
            if(gpio_read(ptr[i].pin_num, value) == -1){
                printf_warn("read gpio %s: %d faild!\n", ptr[i].func_name, *value);
                return -1;
            }
            printf_info("read gpio %s[%d]: %d OK!\n", ptr[i].func_name, ptr[i].pin_num,*value);
            found = 1;
            break;
        }
    }
    if(found == 0){
        return -1;
    }
    return 0;
}

