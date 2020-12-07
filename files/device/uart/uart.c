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
******************************************************************************/


#include<config.h>

#define SERIAL_BUF_LEN 1024
#define UART_HANDER_TIMROUT 1000   //ms
#define FPGA_TIME_INTERVAL (1000 * 360)

unsigned long speed_arr[] = {B230400, B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1200, B300,};
unsigned long name_arr[] = {230400, 115200, 57600, 38400, 19200, 9600, 4800, 2400, 1200, 300,};

static void uart0_read_cb(struct uloop_fd *fd, unsigned int events);

struct uloop_timeout *gps_timer = NULL;
struct uloop_timeout *fpga_timer = NULL;

#define  GPS_INDEX      0     //gps节点在uartinfo中的索引
#define  COMPASS1_INDEX 3     //电子罗盘1 30M-1350M
#define  COMPASS2_INDEX 2     //电子罗盘2 1G-6G


struct uart_info uartinfo[] = {
    /* NOTE:  坑：ttyUL0为PL侧控制，波特率由PL侧设置（默认为9600，不可更改），需要更改连续FPGA同事 */
   {"/dev/ttyUL1", 9600,   "gps",               -1,   NULL, NULL},
   {"/dev/ttyUL2", 9600,   "low noiser",        -1,   NULL, NULL},             //接低噪放
   {"/dev/ttyUL3", 9600,   "compass 1-6G",      -1,   NULL, NULL},             //接电子罗盘2 1G-6G
   {"/dev/ttyUL4", 9600,   "compass 30-1350M",  -1,   NULL, NULL},             //接电子罗盘1 30M-1350M
};



long uart0_send_data(uint8_t *buf, uint32_t len)
{
    return write(uartinfo[1].sfd,buf,len);
}

long uart0_send_string(uint8_t *buf)
{
    return write(uartinfo[1].sfd,buf,strlen(buf));
}



static void set_speed(int fd, unsigned long speed)
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
static int set_Parity(int fd,int databits,int stopbits,int parity)
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


static int openserial(char *name)
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
    fd = openserial(name);
    if ( fd < 0) {
        return -1;
    }
    set_speed(fd, baudrate);
    if (set_Parity(fd, 8, 1, 'N') == false) {
        printf_note("Set Parity Error\n");
        close(fd);
        return -1;
    }
    usleep(1000);
    return fd;
}

char *uart_write_read(int fd ,char *cmd,unsigned int cmd_len ,int *recv_len){
    int nbytes=0,i;
    char recv_buf[512],*pbuf=NULL;
    if(fd <=0){
        return NULL;
    } 
    for(i=0;i<cmd_len;i++){
        nbytes = write(fd,cmd+i,1);    
        printf_info("write serial fd=%d, data:%s,len=%d/%d\n",fd,cmd,i,cmd_len);
    }
    nbytes = read(fd,recv_buf,512);
    printf_info("read serial len=%d\n",nbytes);
    *recv_len = nbytes;
    if(nbytes >0){
        pbuf= (char *) malloc(nbytes+1);
        if(pbuf){
            memset(pbuf,0,nbytes+1);
            memcpy(pbuf,recv_buf,nbytes);
            printf_info("read serial data=%s\n",recv_buf);
        }
    }
    return pbuf;
}


static void uart0_read_cb(struct uloop_fd *fd, unsigned int events)
{
    uint8_t buf[SERIAL_BUF_LEN]; 
    int32_t  nread, i; 
    memset(buf,0,SERIAL_BUF_LEN);
    nread = read(fd->fd, buf, SERIAL_BUF_LEN);
    for(i = 0; i< nread; i++){
        printfd(" 0x%x",buf[i]);
    }
    printfd("\n");
    if (nread > 0){
       /* deal read work */
#ifdef SUPPORT_LCD
       lcd_scanf(buf, nread);
#endif
    }
}

long rs485_compass1_send_data(uint8_t *buf, uint32_t len)
{
    return write(uartinfo[COMPASS1_INDEX].sfd,buf,len);
}

int rs485_compass1_read_block_timeout(uint8_t *buf, int time_sec_ms)
{
    int fd = uartinfo[COMPASS1_INDEX].sfd;
    int ret = -1;
    int ms_count = 0, i = 0;
    
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
            printf_err("read error\n", ret);
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

long rs485_compass2_send_data(uint8_t *buf, uint32_t len)
{
    return write(uartinfo[COMPASS2_INDEX].sfd,buf,len);
}

int rs485_compass2_read_block_timeout(uint8_t *buf, int time_sec_ms)
{
    int fd = uartinfo[COMPASS2_INDEX].sfd;
    int ret = -1;
    int ms_count = 0, i = 0;
   
    if(time_sec_ms < 0) {
        time_sec_ms = 1;
    }
    do {
        ret = read(fd, buf, SERIAL_BUF_LEN);
        if (ret == 0) {
            usleep(1000);
            if (ms_count++ >= time_sec_ms) {
                break;
            }
        } else if(ret < 0) {
            printf_err("read error\n", ret);
            break;
        } else {
            printf_debug("uart recv[%d]:\n", ret);
            for(i = 0; i < ret; i++)
                printf_debug("%02x ", buf[i]);
            printf_debug("\n");
            break;
        }
    } while(1);

    return ret;
}


int uart0_read_block_timeout(uint8_t *buf, int time_sec_ms)
{

    int fd = uartinfo[1].sfd;
    int ret = -1;
    int ms_count = 0, i = 0;
   
    if(time_sec_ms < 0)
        time_sec_ms = 1;
    do{
        ret = read(fd, buf, SERIAL_BUF_LEN);
        if(ret == 0){
            usleep(1000);
            if(ms_count++ >= time_sec_ms){
                printf_warn("read timeout[%d ms]\n", time_sec_ms);
                break;
            }
        }else if(ret < 0){
            printf_err("read error\n", ret);
            break;
        }else{  /* read ok */
            printfd("uart recv[%d]:\n", ret);
            for(i = 0; i < ret; i++)
                printfd("%02x ", buf[i]);
            printfd("\n");
            break;
        }
    }while(1);

    return ret;
}



void gps_timer_task_cb(struct uloop_timeout *t)
{
    #define TIME_INTERVAL_SEC   (300)
    char buf[SERIAL_BUF_LEN];
    char cmdbuf[128];
    int32_t  nread; 
    static uint8_t timed_flag = 0;   //授时完成标志
    static int tick = 0;
    memset(buf,0,SERIAL_BUF_LEN);
    
    tick++;
    int fd = uartinfo[GPS_INDEX].sfd;
    nread = read(fd, buf, SERIAL_BUF_LEN);
    if(nread > 0) {
        if (0 == gps_parse_recv_msg_rmc(buf, nread)) {
            if(timed_flag == 0) {
                io_set_fpga_sys_time(gps_get_format_date());  //fpga
                memset(cmdbuf, 0, sizeof(cmdbuf));
                gps_get_date_cmdstring(cmdbuf);
                printf_note("set sys time:%s\n", cmdbuf);
                safe_system(cmdbuf);
                safe_system("hwclock -w");
                timed_flag = 1;
            } else if (tick >= TIME_INTERVAL_SEC) {
                memset(cmdbuf, 0, sizeof(cmdbuf));
                gps_get_date_cmdstring(cmdbuf);
                printf_note("set sys time:%s\n", cmdbuf);
                safe_system(cmdbuf);
                safe_system("hwclock -w");
                tick = 0;
            }
        }
        gps_parse_recv_msg_gga(buf, nread);
    } else {
        if((nread < 0) && (EAGAIN != errno) && (EINTR != errno)) {
            printf_err("gps fd err!\r\n");
            close(fd);
            usleep(100);
            uartinfo[GPS_INDEX].sfd = uart_init_dev(uartinfo[GPS_INDEX].devname, uartinfo[GPS_INDEX].baudrate);
        }
    }
   
    tcflush(uartinfo[GPS_INDEX].sfd, TCIOFLUSH);
    uloop_timeout_set(t, UART_HANDER_TIMROUT);
}


int gps_task_init(void)
{
    if (!gps_timer) {
        gps_timer = (struct uloop_timeout *)malloc(sizeof(struct uloop_timeout));
        if (!gps_timer) {
            printf_err("create gps task failed!\n");
            return -1;
        }
    }

    gps_timer->cb = gps_timer_task_cb;
    gps_timer->pending = false;
    uloop_timeout_set(gps_timer, UART_HANDER_TIMROUT);
    printf_note("create gps task ok!\n");
    return 0;
}

//将系统时间设置到fpga
void fpga_timer_task_cb(struct uloop_timeout *t)
{
    uint32_t date = syscall_get_format_date();
    if (date > 0) {
        io_set_fpga_sys_time(date);
    }
    uloop_timeout_set(t, FPGA_TIME_INTERVAL);
}

int time_fpga_task_init(void)
{
    if (!fpga_timer) {
        fpga_timer = (struct uloop_timeout *)malloc(sizeof(struct uloop_timeout));
        if (!fpga_timer) {
            printf_err("create fpga timer task failed!\n");
            return -1;
        }
    }

    fpga_timer->cb = fpga_timer_task_cb;
    fpga_timer->pending = false;
    uloop_timeout_set(fpga_timer, FPGA_TIME_INTERVAL);
    printf_note("create fpga task ok!\n");
    return 0;
}

void uart_init(void)
{
    printf_note("Uart init\n");
    usleep(1000);
#if defined(SUPPORT_UART)
    int i, fd;
    for(i = 0; i<ARRAY_SIZE(uartinfo); i++){
        fd = uart_init_dev(uartinfo[i].devname, uartinfo[i].baudrate);
        if(fd <= 0){
            printf_err("%s[%s]init failed\n", uartinfo[i].devname, uartinfo[i].info);
            continue;
        }
        uartinfo[i].sfd = fd;
        printf_note("%s[%s]init ok,fd:%d\n", uartinfo[i].devname, uartinfo[i].info, fd);
        
        if(uartinfo[i].cb)
        {
            uartinfo[i].fd = safe_malloc(sizeof(struct uloop_fd));
            memset(uartinfo[i].fd, 0, sizeof(struct uloop_fd));
            uartinfo[i].fd->fd = fd;
            uartinfo[i].fd->cb = uartinfo[i].cb;
            uloop_fd_add(uartinfo[i].fd, ULOOP_READ);
        }
    }
#endif
#if defined(SUPPORT_RS485)
    rs485_com_init();
#endif
#if defined(SUPPORT_GPS)
    gps_task_init();
    gps_init();
#endif
    time_fpga_task_init();
}
