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
unsigned long  speed_arr[] = {B115200,B57600,B38400, B19200, B9600, B4800, B2400, B1200, B300,
          B38400, B19200, B9600, B4800, B2400, B1200, B300, };
unsigned long name_arr[] = {115200,57600,38400,  19200,  9600,  4800,  2400,  1200,  300, 38400,
          19200,  9600, 4800, 2400, 1200,  300, };

static void uart1_read_cb(struct uloop_fd *fd, unsigned int events);
static void uart0_read_cb(struct uloop_fd *fd, unsigned int events);
void gps_timer_task_cb(struct uloop_timeout *t);


struct uart_t uart[2];
struct uart_info uartinfo[] = {
    /* NOTE:  坑：ttyUL0为PL侧控制，波特率由PL侧设置（默认为115200，不可更改），需要更改连续FPGA同事 */
   {0, "/dev/ttyUL4", 9600, "ttyUL4 rs485", NULL, NULL, NULL,              NULL},
   {1, "/dev/ttyUL1", 9600,   "ttyUL1 gps",    NULL, NULL,          gps_timer_task_cb, NULL},
   //{1, "/dev/ttyPS0", 9600,   "ttyps0 gps",    NULL, NULL,          gps_timer_task_cb, NULL},
};

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
    #if 0
    options.c_lflag &= ~(ICANON|ECHO|ECHOE|ECHOK|ECHONL|NOFLSH|ISIG);
    //options.c_lflag |= (ICANON | ECHO | ECHOE);
   // options.c_lflag |= (ICANON | ECHO | ECHOE |ISIG);
    options.c_oflag = 0;
    options.c_cflag &= ~CSIZE;
    options.c_cflag &= ~CRTSCTS;
    
    options.c_iflag &= ~(BRKINT | ICRNL | ISTRIP | IXON);
   // options.c_iflag &= ~(IGNCR);

    //options.c_cflag |= CLOCAL | CREAD;  
   // options.c_cflag &= ~CSIZE;
   // options.c_lflag &=~ICANON;
   #else
   options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP| INLCR | IGNCR | ICRNL | IXON);
   options.c_oflag = 0;// &= ~OPOST;
   options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
 //  options.c_cflag &= ~(CSIZE | PARENB);
 //  options.c_cflag |= CS8;
 //  options.c_cflag &= ~CRTSCTS;
  // options.c_cflag |= CLOCAL | CREAD;  
   #endif

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
    fd = open(name,O_RDWR| O_NOCTTY|O_NDELAY);
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
    if(fd<0)return -1;

    set_speed(fd, baudrate);
    if (set_Parity(fd,8,1,'N') == false)  {
        printf_note("Set Parity Error\n");
        exit (0);
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

int uart0_read_block_timeout(uint8_t *buf, int time_sec_ms)
{

    int fd = uartinfo[0].fd->fd;
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


long uart0_send_data(uint8_t *buf, uint32_t len)
{
    return write(uartinfo[0].fd->fd,buf,len);
}

long uart0_send_string(uint8_t *buf)
{
    return write(uartinfo[0].fd->fd,buf,strlen(buf));
}


long uart1_send_data(uint8_t *buf, uint32_t len)
{
#if defined(SUPPORT_UART)
    return write(uartinfo[1].fd->fd,buf,len);
#else
    return 0;
#endif
}

long uart1_send_string(uint8_t *buf)
{
    return write(uartinfo[1].fd->fd,buf,strlen(buf));
}


static void uart1_read_cb(struct uloop_fd *fd, unsigned int events)
{
	uint8_t buf[SERIAL_BUF_LEN]; 
	int32_t  nread; 

	//printf_note("uart1 read cb\n");
	memset(buf,0,SERIAL_BUF_LEN);
	nread = read(fd->fd, buf, SERIAL_BUF_LEN);
	if (nread > 0){
	/* deal uart1 read work */
		printf_note("recv %d Bytes:\n%s\r\n",nread, buf);
		if (gps_parse_recv_msg(buf, nread) == 0)
		{
			io_set_fpga_sys_time(gps_get_utc_hms());		
		}
	}
}

void gps_timer_task_cb(struct uloop_timeout *t)
{
	char buf[SERIAL_BUF_LEN];
	char cmdbuf[128];
	
	int32_t  nread; 
    static uint8_t timed_flag = 0;
	memset(buf,0,SERIAL_BUF_LEN);
	nread = read(uartinfo[1].fd->fd, buf, SERIAL_BUF_LEN);
	if(nread > 0)
	{
		//printf_note("\r\n*******************recv:%d Bytes*******************\r\n%s\n",nread,buf);

		if (gps_parse_recv_msg(buf, nread) == 0)
		{
		    if(timed_flag == 0)
		    {
    			io_set_fpga_sys_time(gps_get_format_date());
    			memset(cmdbuf, 0, sizeof(cmdbuf));
    			if(0 == gps_get_date_cmdstring(cmdbuf))
    			{
    				printf("set sys time:%s\n", cmdbuf);
    				system(cmdbuf);
    			}
    			timed_flag = 1;
			}
			#if 0
			uloop_timeout_cancel(t);
			free(t);
			t = NULL;
			close( uartinfo[1].fd->fd);
			uartinfo[1].fd->fd = -1;
			printf("gps exit!\n");
			return;
			#endif
		}
	}
	else
	{
		if((nread < 0) && (EAGAIN != errno) && (EINTR != errno))
		{
			printf("gps fd err!\r\n");
			close( uartinfo[1].fd->fd);
			uartinfo[1].fd->fd = uart_init_dev(uartinfo[1].devname, uartinfo[1].baudrate);
		}
	}

	tcflush(uartinfo[1].fd->fd, TCIOFLUSH);
    uloop_timeout_set(t, UART_HANDER_TIMROUT);
}

void uart_timer_task_init(int index)
{
	if(index < 0 || index > 1)
	{
		return;
	}
    if(NULL == uartinfo[index].timeout)
    {
		uartinfo[index].timeout = (struct uloop_timeout *)malloc(sizeof(struct uloop_timeout));
		if(NULL == uartinfo[index].timeout)
		{
			printf("malloc uart uloop_timeout struct fail!\n");
			return;
		}
    }
    
    uartinfo[index].timeout->cb = uartinfo[index].timeout_hander;
    uartinfo[index].timeout->pending = false;
    uloop_timeout_set(uartinfo[index].timeout, UART_HANDER_TIMROUT);
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
        printf_note("[%d]%s[%s]init ok,fd:%d\n", uartinfo[i].index, uartinfo[i].devname, uartinfo[i].info, fd);
        uartinfo[i].fd = safe_malloc(sizeof(struct uloop_fd));
        memset(uartinfo[i].fd, 0, sizeof(struct uloop_fd));
        uartinfo[i].fd->fd = fd;
	    if(uartinfo[i].cb)
	    {
	    	uartinfo[i].fd->cb = uartinfo[i].cb;
	        uloop_fd_add(uartinfo[i].fd, ULOOP_READ);
        }
        else
        {
        	if(uartinfo[i].timeout_hander)
        	{
        		uart_timer_task_init(i);
        	}
        }
    }
#endif
#if defined(SUPPORT_RS485)
    rs485_com_init();
#endif
}
