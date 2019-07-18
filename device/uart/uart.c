#include<config.h>
unsigned long  speed_arr[] = {B115200,B57600,B38400, B19200, B9600, B4800, B2400, B1200, B300,
          B38400, B19200, B9600, B4800, B2400, B1200, B300, };
unsigned long name_arr[] = {115200,57600,38400,  19200,  9600,  4800,  2400,  1200,  300, 38400,
          19200,  9600, 4800, 2400, 1200,  300, };

struct uart_t uart[2];

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
options.c_lflag &= ~(ICANON|ECHO|ECHOE|ECHOK|ECHONL|NOFLSH|ISIG);
options.c_oflag = 0;
options.c_cflag &= ~CSIZE;
options.c_iflag &= ~(BRKINT | ICRNL | ISTRIP | IXON);
options.c_iflag &= ~(IGNCR);
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
            options.c_iflag &= ~(ICRNL | IGNCR);
            options.c_lflag &= ~(ICANON);
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
if (parity != 'n')
options.c_iflag |= INPCK;
options.c_cc[VTIME] = 100; /* 设置超时15 seconds*/
options.c_cc[VMIN] = 0; /* Update the options and do it NOW */
tcflush(fd,TCIFLUSH);
if (tcsetattr(fd,TCSANOW,&options) != 0)
    {
    perror("SetupSerial 3");
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

int uart_init_dev(char *name)
{

    int fd;
    fd = openserial(name);
    if(fd<0)return -1;

    set_speed(fd, 115200);
    if (set_Parity(fd,8,1,'N') == false)  {
        printf_note("Set Parity Error\n");
        exit (0);
    }
    usleep(100);
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
    printf_debug("uart0 read cb\n");
}

long uart0_send_data(uint8_t *buf, uint32_t len)
{
    return write(uart[0].fd.fd,buf,len);
}

long uart1_send_data(uint8_t *buf, uint32_t len)
{
    return write(uart[1].fd.fd,buf,len);
}

static void uart1_read_cb(struct uloop_fd *fd, unsigned int events)
{
    uint8_t buf[SERIAL_BUF_LEN]; 
    int32_t  nread; 

    printf_debug("uart1 read cb\n");
    memset(buf,0,SERIAL_BUF_LEN);
    nread = read(uart[1].fd.fd, buf, SERIAL_BUF_LEN);
    if (nread > 0){
       /* deal uart1 read work */
    }
}

void uart_init(void)
{
    printf_info("uart init\n");
    
#if defined(PLAT_FORM_ARCH_ARM)
    uart[0].fd.fd = uart_init_dev("/dev/ttyPS0");
        if(uart[0].fd.fd <=0 ){
        printf_err("/dev/ttyPS0 serial init failed\n");
    }

    uart[1].fd.fd = uart_init_dev("/dev/ttyPS1");
        if(uart[1].fd.fd <=0 ){
        printf_err("/dev/ttyPS1 serial init failed\n");
    }
    uart[0].fd.cb = uart0_read_cb;
    uart[1].fd.cb = uart1_read_cb;
    
    uloop_fd_add(&uart[0].fd, ULOOP_READ);
    uloop_fd_add(&uart[1].fd, ULOOP_READ);
#endif
}
