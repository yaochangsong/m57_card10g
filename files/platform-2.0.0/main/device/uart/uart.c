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
*  Rev 1.0   06 July 2019   yaochangsong
*  Initial revision.
*  Rev 2.0   2020-05-15   wangzhiqiang
*  修改：
*	串口信息结构体增加波特率, 定时器，定时回调函数字段
*	串口初始化函数，增加波特率参数
*   串口设置参数VTIME修改为0
*	GPS读采用定时器读方式，获取有效时间后，设置fpga时间、系统时间，定时器关闭
*  Rev 3.0   2021-06-28   yaochangsong
*   整理相关函数
******************************************************************************/
#include "config.h"


unsigned long speed_arr[] = {B230400, B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1200, B300,};
unsigned long name_arr[] = {230400, 115200, 57600, 38400, 19200, 9600, 4800, 2400, 1200, 300,};

ssize_t uart_send_data(int funcode, uint8_t *buf, uint32_t len)
{
    int c;
    struct uart_info_t* dev = dev_get_uart(&c);
    for(int i = 0; i< c; i++){
        if(dev[i].funcode == funcode && dev[i].sfd > 0)
            return writen(dev[i].sfd, buf, len);
    }
    return -1;
}

static void uart_read_cb(struct uloop_fd *fd, unsigned int events)
{
    uint8_t buf[SERIAL_BUF_LEN]; 
    int32_t  nread, i; 
    struct uart_info_t *dev = container_of(fd, struct uart_info_t, fd);
    
    memset(buf,0,SERIAL_BUF_LEN);
    nread = read(fd->fd, buf, SERIAL_BUF_LEN);
    for(i = 0; i< nread; i++){
        printfd(" 0x%x",buf[i]);
    }
    printfd("\n");
    if (nread > 0){
       /* deal read work */
#ifdef CONFIG_DEVICE_LCD
    if(dev->funcode == UART_LCD_CODE)
        lcd_scanf(buf, nread);
#endif
    }
}


ssize_t uart_read_timeout(int funcode, uint8_t *buf, int time_sec_ms)
{
    int fd = -1;
    int ret = -1;
    int ms_count = 0, i = 0;
    int c, found = 0;
    struct uart_info_t* dev = dev_get_uart(&c);
    for(int i = 0; i< c; i++){
        if(dev[i].funcode == funcode && dev[i].sfd > 0){
            fd = dev[i].sfd;
            found ++;
            break;
        }
    }
    if(found == 0){
        printf_warn("uart funcode: %d not find\n", funcode);
        return -1;
    }
    
    if(time_sec_ms < 0) {
        time_sec_ms = 1;
    }
    do {
        ret = read(fd, buf, SERIAL_BUF_LEN);
        if (ret == 0) {
            usleep(1000);
            if(ms_count++ >= time_sec_ms){
                break;
            }
        } else if (ret < 0) {
            printf_err("read error:%d\n", ret);
            break;
        } else {
            printf_debug("uart recv[%d]:\n", ret);
            for (i = 0; i < ret; i++)
                printf_debug("%02x ", buf[i]);
            printf_debug("\n");
            break;
        }
    } while(1);

    return ret;
}

static void uart_set_speed(int fd, unsigned long speed)
{
    int   i;
    int   status;
    struct termios   Opt;
    tcgetattr(fd, &Opt); 
    for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++) {
        if  (speed == name_arr[i]) {
            tcflush(fd, TCIOFLUSH);
            cfsetispeed(&Opt, speed_arr[i]);  /*对于波特率的设置函数*/
            cfsetospeed(&Opt, speed_arr[i]);
            status = tcsetattr(fd, TCSANOW, &Opt);
            if  (status != 0) {
                perror("tcsetattr fd");
                return;
            }
            tcflush(fd,TCIOFLUSH);
        }
    }
}

/**
*@brief   设置串口数据位，停止位和效验位
*@param  fd     类型  int  打开的串口文件句柄
*@param  databits 类型  int 数据位   取值 为 7 或者8
*@param  stopbits 类型  int 停止位   取值为 1 或者2
*@param  parity  类型  int  效验类型 取值为N,E,O,,S
*/
static int uart_set_parity(int fd,int databits,int stopbits,int parity)
{
    struct termios options;
    if  ( tcgetattr( fd,&options)  !=  0)
    { 
    perror("SetupSerial 1");
        return(false);
    }

   options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP| INLCR | IGNCR | ICRNL | IXON);
   options.c_oflag = 0;// &= ~OPOST;
   options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

switch (databits) /*设置数据位数*/
    {
    case 7:		
    options.c_cflag |= CS7;
    break;
    case 8:
    options.c_cflag |= CS8;
    break;
    default:
    printf_warn("Unsupported data size\n"); return (false);
    }
switch (parity) 
    {
    case 'n':
    case 'N':    
    options.c_cflag &= ~PARENB;   /* Clear parity enable */
    options.c_iflag &= ~INPCK;     /* Enable parity checking */
         //  options.c_iflag &= ~(ICRNL | IGNCR);
         //   options.c_lflag &= ~(ICANON);
    break;  
    case 'o':   
    case 'O':     
    options.c_cflag |= (PARODD | PARENB); /* 设置为奇效验*/
    options.c_iflag |= INPCK;             /* Disnable parity checking */
    break;  
    case 'e':  
    case 'E':   
    options.c_cflag |= PARENB;     /* Enable parity */
    options.c_cflag &= ~PARODD;   /* 转换为偶效验*/
    options.c_iflag |= INPCK;       /* Disnable parity checking */
    break;
    case 'S':
    case 's':  /*as no parity*/
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;break;
    default:
    printf_warn("Unsupported parity\n");
    return (false);
    }
/* 设置停止位*/
switch (stopbits)
    {
    case 1:
    options.c_cflag &= ~CSTOPB;
    break;
    case 2:
    options.c_cflag |= CSTOPB;
    break;
    default:
    printf_warn("Unsupported stop bits\n");
    return (false);
    }
    /* Set input parity option */
    //if (parity != 'n')
    //    options.c_iflag |= INPCK;
    //options.c_cc[VTIME] = 100; /* 设置超时15 seconds*/
    options.c_cc[VTIME] = 0;
    //options.c_cc[VMIN] = 1;
    options.c_cc[VMIN] = 0; /* Update the options and do it NOW */
    tcflush(fd,TCIFLUSH);
    if (tcsetattr(fd,TCSANOW,&options) != 0)
    {
        perror("SetupSerial");
        return (false);
    }
    return (true);
}


static int uart_openserial(char *name)
{
    int fd =-1;
    fd = open(name,O_RDWR| O_NOCTTY|O_NONBLOCK);
    if(-1 == fd){
        perror("open serial 1 failed\n");
    }else{
        fcntl(fd, F_SETFL, 0);
    }

    return fd;
}

int uart_init_dev(char *name, uint32_t baudrate)
{
    int fd;
    fd = uart_openserial(name);
    if ( fd < 0) {
        return -1;
    }
    uart_set_speed(fd, baudrate);
    if (uart_set_parity(fd, 8, 1, 'N') == false) {
        printf_note("Set Parity Error\n");
        close(fd);
        return -1;
    }
    usleep(1000);
    return fd;
}

struct uart_info_t *uart_get_dev_by_code(int funcode)
{
    int c;
    struct uart_info_t* dev = dev_get_uart(&c);
    
    for(int i = 0; i< c; i++){
        if(dev[i].funcode == funcode && dev[i].sfd > 0)
            return &dev[i];
    }
    return NULL;
}

void uart_init(void)
{
    printf_note("Uart init\n");
    usleep(1000);
#if defined(CONFIG_DEVICE_UART)
    int i, fd;
    int c;
    struct uart_info_t* dev = dev_get_uart(&c);

    for(i = 0; i < c; i++){
        fd = uart_init_dev(dev[i].devname, dev[i].baudrate);
        if(fd <= 0){
            printf_err("%s[%s]init failed\n", dev[i].devname, dev[i].info);
            continue;
        }
        dev[i].sfd = fd;
        printf_note("%s[%s]init ok,fd:%d\n", dev[i].devname, dev[i].info, fd);
        if(dev[i].is_cb){
            printf_note("%s add callback\n", dev[i].devname);
            memset(&dev[i].fd, 0, sizeof(dev[i].fd));
            dev[i].fd.fd = fd;
            dev[i].fd.cb = uart_read_cb;
            uloop_fd_add(&dev[i].fd, ULOOP_READ);
        }
    }
#if defined(CONFIG_DEVICE_RS485)
    rs485_com_init();
#endif
#if defined(CONFIG_DEVICE_GPS)
    gps_init();
#endif

#endif   
}
