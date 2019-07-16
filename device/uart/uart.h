#ifndef __SERIALCOM_H
#define __SERIALCOM_H
#include     <stdio.h>      /*标准输入输出定义*/
#include     <stdlib.h>     /*标准函数库定义*/
#include     <unistd.h>     /*Unix 标准函数定义*/
#include     <sys/types.h>  
#include     <sys/stat.h>   
#include     <fcntl.h>      /*文件控制定义*/
#include     <termios.h>    /*PPSIX 终端控制定义*/
#include     <errno.h>      /*错误号定义*/
#include "CLogger.h"

int uart_init_serialcom(char *name);
char *uart_serial_write_read(int fd ,char *cmd,unsigned int cmd_len ,int *recv_len);
#endif

