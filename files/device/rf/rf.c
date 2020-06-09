#include "config.h"


static int spidevfd[SPI_CONTROL_NUM] =  {-1};
static int gpiofd[MAX_GPIO_NUM] = {-1};

static int ledfd[MAX_LED_NUM] = {-1};
static RF_FREQ_STATS rf_freq_stats[MAX_CHANNEL_NUM] = {0};
static pthread_mutex_t mut;


uint8_t clock_7044_config[49][3]={
{0x00,0x01, 0x60},
{0x00,0x03, 0x24},
{0x00,0x04, 0x78},
{0x00,0x05, 0x6f},
{0x00,0x0b, 0x07},
{0x00,0x5a, 0x01},
{0x00,0x5c, 0x80},
{0x00,0x5d, 0x00},
{0x00,0x64, 0x01},
{0x01,0x04, 0xd1},
{0x01,0x05, 0x04},
{0x01,0x06, 0x00},
{0x01,0x0b, 0x00},
{0x01,0x0c, 0x08},
{0x01,0x18, 0xd1},
{0x01,0x19, 0x01},
{0x01,0x1a, 0x00},
{0x01,0x1f, 0x00},
{0x01,0x20, 0x08},
{0x01,0x22, 0x5d},
{0x01,0x23, 0x40},
{0x01,0x24, 0x00},
{0x01,0x29, 0x00},
{0x01,0x2a, 0x91},
{0x01,0x2c, 0xd1},
{0x01,0x2d, 0x04},
{0x01,0x2e, 0x00},
{0x01,0x33, 0x00},
{0x01,0x34, 0x08},
{0x01,0x36, 0x5d},
{0x01,0x37, 0x40},
{0x01,0x38, 0x00},
{0x01,0x3d, 0x00},
{0x01,0x3e, 0x91},
{0x01,0x40, 0xd1},
{0x01,0x41, 0x04},
{0x01,0x42, 0x00},
{0x01,0x47, 0x00},
{0x01,0x48, 0x08},
{0x01,0x4a, 0x5d},
{0x01,0x4b, 0x40},
{0x01,0x4c, 0x00},
{0x01,0x51, 0x00},
{0x01,0x52, 0x91},
{0x00,0x9f, 0x4d},
{0x00,0xa0, 0xdf},
{0x00,0xa5, 0x06},
{0x00,0xa8, 0x06},
{0x00,0xb0, 0x04},
};

#if defined(INTERNAL_CLK)
/* internal clock , for test , add by ycs */
uint8_t clock_7044_config_internal[78][3]={
{0x00,0x01, 0x60},
{0x00,0x03, 0x37},
{0x00,0x04, 0x78},
{0x00,0x05, 0x4f},
{0x00,0x09, 0x00},
{0x00,0x0a, 0x07},
{0x00,0x0b, 0x07},
{0x00,0x0c, 0x07},
{0x00,0x0d, 0x07},
{0x00,0x0e, 0x07},
{0x00,0x14, 0xff},
{0x00,0x15, 0x03},
{0x00,0x1a, 0x00},
{0x00,0x1b, 0x00},
{0x00,0x1c, 0x04},
{0x00,0x1d, 0x04},
{0x00,0x1e, 0x04},
{0x00,0x1f, 0x04},
{0x00,0x20, 0x04},
{0x00,0x21, 0x04},
{0x00,0x22, 0x00},
{0x00,0x26, 0x10},
{0x00,0x27, 0x00},
{0x00,0x28, 0x0f},
{0x00,0x29, 0x1c},
{0x00,0x31, 0x01},
{0x00,0x32, 0x01},
{0x00,0x33, 0x01},
{0x00,0x34, 0x00},
{0x00,0x35, 0x18},
{0x00,0x36, 0x00},
{0x00,0x37, 0x0f},
{0x00,0x38, 0x1e},
{0x00,0x48, 0x00},
{0x00,0x49, 0x00},
{0x00,0x5a, 0x01},
{0x00,0x5c, 0x00},
{0x00,0x5d, 0x03},
{0x00,0x64, 0x01},
{0x01,0x04, 0xd1},
{0x01,0x05, 0x18},
{0x01,0x06, 0x01},
{0x01,0x0b, 0x00},
{0x01,0x0c, 0x08},
{0x01,0x18, 0xd1},
{0x01,0x19, 0x06},
{0x01,0x1a, 0x00},
{0x01,0x1F, 0x00},
{0x01,0x20, 0x08},
{0x01,0x22, 0x5d},
{0x01,0x23, 0x80},
{0x01,0x24, 0x01},
{0x01,0x29, 0x00},
{0x01,0x2a, 0x91},
{0x01,0x2c, 0xd1},
{0x01,0x2d, 0x18},
{0x01,0x2e, 0x00},
{0x01,0x33, 0x00},
{0x01,0x34, 0x08},
{0x01,0x36, 0x5d},
{0x01,0x37, 0x80},
{0x01,0x38, 0x01},
{0x01,0x3d, 0x00},
{0x01,0x3e, 0x91},
{0x01,0x40, 0xd1},
{0x01,0x41, 0x18},
{0x01,0x42, 0x00},
{0x01,0x47, 0x00},
{0x01,0x48, 0x08},
{0x01,0x4a, 0x5d},
{0x01,0x4b, 0x80},
{0x01,0x4c, 0x01},
{0x01,0x51, 0x00},
{0x01,0x52, 0x91},
{0x00,0x9f, 0x4d},
{0x00,0xa0, 0xdf},
{0x00,0xa5, 0x06},
{0x00,0xa8, 0x06},
};
#endif


uint8_t ad_9690_config[26][3]={
{0x00,0x3f,0x80},
{0x00,0x40,0xbf},
{0x05,0x71,0x15},
{0x00,0x16,0x6c},
{0x02,0x00,0x02},
{0x02,0x01,0x01},
{0x03,0x10,0x43},
{0x03,0x11,0x00},
{0x03,0x14,0x00},
{0x03,0x15,0x0c},
{0x03,0x20,0x00},
{0x03,0x21,0x00},
{0x03,0x30,0x43},
{0x03,0x31,0x00},
{0x03,0x34,0x02},
{0x03,0x35,0x00},
{0x03,0x40,0x00},
{0x03,0x41,0x00},
{0x05,0x70,0x91},
{0x05,0x6e,0x10},
{0x05,0x8b,0x03},
{0x01,0x20,0x02},
{0x01,0x21,0x00},
{0x03,0x00,0x01},
{0x00,0x01,0x02},
{0x05,0x71,0x14},
};

uint32_t get_internal_clock_cfg_array_size(void)
{
    return (sizeof(clock_7044_config_internal)/3);
}

uint32_t get_external_clock_cfg_array_size(void)
{
    return (sizeof(clock_7044_config)/3);
}

uint32_t get_adc_cfg_array_size(void)
{
    return (sizeof(ad_9690_config)/3);
}



static void spi_fd_init(void)
{
    int i ;
    for(i=0;i<SPI_CONTROL_NUM;i++){
        spidevfd[i] = -1;
    }

    for(i = 0 ;i <MAX_GPIO_NUM;i++){
        gpiofd[i] = -1;
    }
    for(i = 0 ;i <MAX_LED_NUM;i++){
        ledfd[i] = -1;
    }
}

static int init_gpio(uint8_t index){

    char spi_offset_num[5];
    char spi_gpio_direction[128];
    char  spi_gpio_value[128];
    int valuefd, exportfd, directionfd;

    sprintf(spi_offset_num,"%d",GPIO_BASE_OFFSET+index);
    sprintf(spi_gpio_direction,"/sys/class/gpio/gpio%s/direction",spi_offset_num);
    sprintf(spi_gpio_value,"/sys/class/gpio/gpio%s/value",spi_offset_num);
    exportfd = open("/sys/class/gpio/export", O_WRONLY);
    if (exportfd < 0)
    {
        printf_err("Cannot open GPIO to export it\n");
        return -1;
    }
    write(exportfd, spi_offset_num, strlen(spi_offset_num)+1);
    close(exportfd);
    // Update the direction of the GPIO to be an output
    directionfd = open(spi_gpio_direction, O_WRONLY);
    if (directionfd < 0)
    {
        printf_err("Cannot open GPIO direction it\n");
        return -1;
    }

    if(index == B2B_STATUS_INDICATE_GPIO){
        write(directionfd, "in", 3);
    }else{
        write(directionfd, "high", 5);
    }
    close(directionfd);

    // Get the GPIO value ready to be toggled

    if(index == B2B_STATUS_INDICATE_GPIO){
        valuefd = open(spi_gpio_value, O_RDONLY);
    }else{
        valuefd = open(spi_gpio_value, O_WRONLY);
    }

    if (valuefd < 0)
    {
        printf_err("Cannot open GPIO value\n");
        return -1;
    }
    return valuefd;
}




static int init_led(uint8_t index){
    char led_path[128];
    int valuefd;
    if(index==0){
        sprintf(led_path,"/sys/class/leds/led-green/brightness");
    }else{
        sprintf(led_path,"/sys/class/leds/led-red/brightness");
    }
    valuefd = open(led_path, O_WRONLY);
    if (valuefd < 0)
    {
        printf_err("Cannot open led path\n");
        return -1;
    }
    return valuefd;
}

static int set_gpio_high(int fd){
    if(fd <=0){
        return -1;
    }
    return write(fd,"1", 2);
}
static int set_gpio_low(int fd){
    if(fd <=0){
        return -1;
    }
    return write(fd,"0", 2);
}

static void set_green_led_on(void){
    if(ledfd[0] >0 && ledfd[1] >0){
        set_gpio_low(ledfd[1]);
        set_gpio_high(ledfd[0]);
    }
}

static void set_green_red_led_off(void){
    if(ledfd[0] >0 && ledfd[1] >0){
        set_gpio_low(ledfd[0]);
        set_gpio_low(ledfd[1]);
    }
}

static void set_red_led_on(void){
    if(ledfd[0] >0 && ledfd[1] >0){
        set_gpio_low(ledfd[0]);
        set_gpio_high(ledfd[1]);
    }
}

static void set_spi_cs(uint8_t ch){
    //emio[10:8] for different control path
    if((ch&0x1)>0){
        set_gpio_high(gpiofd[8]);
    }else{
        set_gpio_low(gpiofd[8]);
    }

    if((ch&0x2)>0){
        set_gpio_high(gpiofd[9]);
    }else{
        set_gpio_low(gpiofd[9]);
    }
    if((ch&0x4)>0){
        set_gpio_high(gpiofd[10]);
    }else{
        set_gpio_low(gpiofd[10]);
    }	
}

#if 0
/* is_backtrace =1:backtrace mode; =0 ADC normal mode */
void switch_adc_mode(uint8_t is_backtrace){
    
    lseek(gpiofd[0], 0, SEEK_SET);
    if(is_backtrace >0){
        printf_note("set adc pin high...\n");
        set_gpio_high(gpiofd[0]);   
    }else{
        printf_note("set adc pin low...\n");
        set_gpio_low(gpiofd[0]);   
    }
}
#endif

int8_t  spi_dev_init(void)
{
    uint8_t mode = 0;
        uint32_t speed = 4000000;
        uint32_t i,cnum;
        char dev_name[32];
        static uint8_t adc_mode_init = 0;
    
        if(adc_mode_init != 0){
              printf("spi device has been inited.\n");
             return -1;
        }
        
        for(cnum=0 ;cnum <SPI_CONTROL_NUM ;cnum++){
            if(cnum == 0){
                mode = 0;
                sprintf(dev_name,"/dev/spidev32766.0");
            }else if(cnum == 1){
                mode = 0;
                sprintf(dev_name,"/dev/spidev32766.1");
            }else if(cnum == 2){
                mode = SPI_CPOL|SPI_CPHA ;
                sprintf(dev_name,"/dev/spidev32765.0");
            }else if(cnum == 3){
                mode = 0;
                sprintf(dev_name,"/dev/spidev32765.1");
            }else{
                //sprintf(dev_name,"/dev/spidev%d.0",dev_num);
                break;
            }
    
            if (spidevfd[cnum] <=0) {
                spidevfd[cnum] =  open(dev_name,O_RDWR);
                if(spidevfd[cnum] < 0){
                    return  -1;
                }
                if(ioctl(spidevfd[cnum], SPI_IOC_WR_MODE,&mode) <0){
                    return -1;   
                }
                if(ioctl(spidevfd[cnum],SPI_IOC_WR_MAX_SPEED_HZ, &speed)<0){
                    return -1;
                }
            }
            
        }
        for(i = 0 ;i <MAX_LED_NUM;i++){
            if(ledfd[i] <=0){
                ledfd[i] = init_led(i);
                if(ledfd[i] <0){
                    //return -1;
                    printf("led init failed\n");
                }
            }
        }
    
        set_green_led_on();
    
        //adc data source switch
        //gpiofd[0] = init_gpio(63);
        /* set normal mode */
        //switch_adc_mode(0);  
        adc_mode_init = 0xaa;
        return 0;

}



static uint8_t get_response_len_bytype(uint8_t type){
    uint8_t recv_len = 0;
    recv_len = PERMANENT_HEADER_LEN;
    switch(type){
        case RF_FREQ_SET :{
            recv_len =0;
            break;
        }
        case RF_GAIN_SET :{
            recv_len += sizeof(RF_GAIN_RESPONSE);
            break;
        }
        case RF_MIDDLE_FREQ_BANDWIDTH_FILTER_SET :{
            recv_len += sizeof(RF_MIDDLE_FREQ_BANDWIDTH_RESPONSE);
            break;
        }
        case RF_NOISE_MODE_SET :{
            recv_len += sizeof(RF_NOISE_MODE_RESPONSE);
            break;
        }
        case RF_GAIN_CALIBRATE_SET :{
            recv_len += sizeof(RF_GAIN_CALIBRATE_RESPONSE);
            break;
        }
        case RF_FREQ_QUERY :{
            recv_len += sizeof(RF_FREQ_RESPONSE);
            break;
        }
        case RF_GAIN_QUERY :{
            recv_len += sizeof(RF_GAIN_RESPONSE);
            break;
        }
        case MID_FREQ_GAIN_SET :{
            recv_len += sizeof(RF_GAIN_RESPONSE);
            break;
        }
        case RF_MIDDLE_FREQ_BANDWIDTH_FILTER_QUERY :{
            recv_len += sizeof(RF_MIDDLE_FREQ_BANDWIDTH_RESPONSE);
            break;
        }
        case RF_NOISE_MODE_QUERY :{
            recv_len += sizeof(RF_NOISE_MODE_RESPONSE);
            break;
        }
        case RF_GAIN_CALIBRATE_QUERY :{
            recv_len += sizeof(RF_GAIN_CALIBRATE_RESPONSE);
            break;
        }
        case RF_TEMPRATURE_QUERY :{
            recv_len += sizeof(RF_TEMPRATURE_RESPONSE);
            break;
        }
        default:{
            return 0;           
        }
    }
    return recv_len;
}


static uint8_t checksum(uint8_t *send_buf,uint8_t len){
    uint32_t check_sum = 0;
    uint8_t i;
    for(i=0;i <len ;i++){
        check_sum += send_buf[i];
    }
    return check_sum&0xff;
}


static uint8_t* create_buf_bytype(uint8_t cmd ,uint8_t *ret_len){
    uint8_t send_len;
    uint8_t *buf = NULL;
    send_len = PERMANENT_HEADER_LEN;
    switch(cmd){
        case RF_FREQ_SET :{
            send_len += sizeof(RF_FREQ_SET_REQUEST);
            break;
        }

        case RF_GAIN_SET :{
            send_len += sizeof(RF_GAIN_SET_REQUEST);
            break;
        }

        case MID_FREQ_GAIN_SET :{
            send_len += sizeof(RF_GAIN_SET_REQUEST);
            break;
        }
        case RF_MIDDLE_FREQ_BANDWIDTH_FILTER_SET :{
            send_len += sizeof(RF_MIDDLE_FREQ_BANDWIDTH_SET_REQUEST);
            break;
        }
        case RF_NOISE_MODE_SET :{
            send_len += sizeof(RF_NOISE_MODE_SET_REQUEST);
            break;
        }
    }

    //buf = new uint8_t[send_len];
    buf = (uint8_t *)malloc(send_len);
    if(buf){
        bzero(buf,send_len);
        buf[0] = 0xaa;
        buf[1] = (send_len - PERMANENT_HEADER_LEN)&0xff;
        buf[2] = cmd;
        buf[send_len-2] = checksum(buf+CHECKSUM_OFFSET,send_len-CHECKSUM_IGNORE_LEN);
        buf[send_len-1] = 0x55;
    }
    *ret_len = send_len;
    return buf;
}


static uint8_t* send_packet(int fd, uint8_t *buf,uint32_t buf_len,uint8_t recv_len,uint32_t speed){
    int stau,i;
    struct spi_ioc_transfer xfer[2];
    uint8_t *recv_buf = NULL;

    if(recv_len>0){
        recv_buf = (uint8_t *)malloc(recv_len);
    }

    printf_debug("spi send data:");
    for(i=0;i<buf_len ;i++){
        printfd("%02x ",buf[i]);
    }
    printfd("\n");
    memset(xfer, 0, sizeof(xfer));
    if(recv_len >0){
        memset(recv_buf, 0, recv_len);
    }

    xfer[0].tx_buf = (unsigned long) buf;
    xfer[0].len = buf_len;

    if(recv_len >0){
        xfer[0].delay_usecs = 2;
        xfer[1].rx_buf = (unsigned long) recv_buf;
        xfer[1].len = recv_len;
        xfer[1].delay_usecs = 2;
    }

    if(speed >0){
        xfer[0].speed_hz = speed;
        xfer[1].speed_hz = speed;
    }

    if(recv_len >0){
        stau = ioctl(fd, SPI_IOC_MESSAGE(2), xfer);
    }else{
        stau = ioctl(fd, SPI_IOC_MESSAGE(1), xfer);
    }
    if (stau < 0) {
        printf_err("spi data happen err:%d\n",errno);
        perror("spi send data");
        return NULL;
    }

    if(recv_len >0){
        printf_debug("spi recv data:");
        for(i=0;i<recv_len ;i++){
            printfd("%02x ",recv_buf[i]);
        }
        printfd("\n");
    }

    return recv_buf;
}


static uint8_t* translate_data(uint8_t spi_num,uint8_t *buf,uint32_t len,uint8_t recv_len){
    uint8_t *data = NULL;

    if(spidevfd[spi_num] <=0){
        spi_dev_init();
    }

    //std::unique_lock<std::mutex>lck(CSpi::spi_lock);
    pthread_mutex_lock(&mut);
    set_spi_cs(spi_num);
    data = send_packet(spidevfd[spi_num],buf,len,recv_len,0);
    pthread_mutex_unlock(&mut);
    return data;
}


static long time_usec_diff(struct timespec now,struct timespec old){
    long diff;
    diff = (now.tv_sec - old.tv_sec)*1000000 + (now.tv_nsec - old.tv_nsec)/1000;
    return diff;
}

void clock_7044_init(void){
    uint8_t send_buf[10],bits_len,recv_len,*precv;
    uint32_t i;
    
    if(spidevfd[0] <=0){
        spi_dev_init();
    }
    
    recv_len =0;
    bits_len = 24;

    send_buf[0] = 0x0;
    send_buf[1] = 0x0;
    send_buf[2] = 0x01;
    translate_data(0,send_buf,bits_len/8,recv_len);
    usleep(50000);

    send_buf[0] = 0x0;
    send_buf[1] = 0x0;
    send_buf[2] = 0x00;
    translate_data(0,send_buf,bits_len/8,recv_len);
    usleep(50000);

    for(i=0;i<(sizeof(clock_7044_config)/3);i++){
        uint8_t tmp_buf[4];
        memcpy(tmp_buf,clock_7044_config[i],3);
        translate_data(0,tmp_buf,bits_len/8,recv_len);
    }

    
    bits_len = 24;
    recv_len =0;

    send_buf[0] = 0x0;
    send_buf[1] = 0x1;
    send_buf[2] = 0x62;
    translate_data(0,send_buf,bits_len/8,recv_len);
    usleep(100);

    send_buf[0] = 0x0;
    send_buf[1] = 0x01;
    send_buf[2] = 0x60;
    translate_data(0,send_buf,bits_len/8,recv_len);

    send_buf[0] = 0x0;
    send_buf[1] = 0x01;
    send_buf[2] = 0xe0;
    translate_data(0,send_buf,bits_len/8,recv_len);
    usleep(100);
    
    send_buf[0] = 0x0;
    send_buf[1] = 0x01;
    send_buf[2] = 0x60;
    translate_data(0,send_buf,bits_len/8,recv_len);
    sleep(1);
    printf_note("clock 7044 init finished \n");
}

#if defined(INTERNAL_CLK)
/* internal clock , for test */
void clock_7044_init_internal(void){
     uint8_t send_buf[10],bits_len,recv_len,*precv;
    uint32_t i;
    
    if(spidevfd[0] <=0){
        spi_dev_init();
    }
    
    recv_len =0;
    bits_len = 24;

    send_buf[0] = 0x0;
    send_buf[1] = 0x0;
    send_buf[2] = 0x01;
    translate_data(0,send_buf,bits_len/8,recv_len);
    usleep(50000);

    send_buf[0] = 0x0;
    send_buf[1] = 0x0;
    send_buf[2] = 0x00;
    translate_data(0,send_buf,bits_len/8,recv_len);
    usleep(50000);

    for(i=0;i<(sizeof(clock_7044_config_internal)/3);i++){
        uint8_t tmp_buf[4];
        memcpy(tmp_buf,clock_7044_config_internal[i],3);
        translate_data(0,tmp_buf,bits_len/8,recv_len);
    }

    
    bits_len = 24;
    recv_len =0;

    send_buf[0] = 0x0;
    send_buf[1] = 0xb0;
    send_buf[2] = 0x04;
    translate_data(0,send_buf,bits_len/8,recv_len);
    usleep(10000);

    send_buf[0] = 0x0;
    send_buf[1] = 0x01;
    send_buf[2] = 0x62;
    translate_data(0,send_buf,bits_len/8,recv_len);

    send_buf[0] = 0x0;
    send_buf[1] = 0x01;
    send_buf[2] = 0x60;
    translate_data(0,send_buf,bits_len/8,recv_len);
    usleep(5000);
    
    send_buf[0] = 0x0;
    send_buf[1] = 0x01;
    send_buf[2] = 0xe0;
    translate_data(0,send_buf,bits_len/8,recv_len);
    usleep(100);
    send_buf[0] = 0x0;
    send_buf[1] = 0x01;
    send_buf[2] = 0x60;
    translate_data(0,send_buf,bits_len/8,recv_len);
    usleep(100);
    printf_warn("clock 7044 init finished \n");
}
#endif

void ad9690_init(void){
    uint8_t send_buf[10],bits_len,recv_len,*precv;
    uint32_t i;

    if(spidevfd[1] <=0){
        spi_dev_init();
    }
    
    recv_len =0;
    bits_len = 24;
    
    send_buf[0] = 0x0;
    send_buf[1] = 0x0;
    send_buf[2] = 0x81;
    translate_data(1,send_buf,bits_len/8,recv_len);
    usleep(10000);

    for(i=0;i<(sizeof(ad_9690_config)/3);i++){
        uint8_t tmp_buf[4];
        memcpy(tmp_buf,ad_9690_config[i],3);
        //PRINTF("reg value:%02x %02x %02x",tmp_buf[0],tmp_buf[1],tmp_buf[2]);
        translate_data(1,tmp_buf,bits_len/8,recv_len);
    }
    printf_note("ad chip 9690 init finished\n");
}


static uint8_t send_gain_set_cmd(uint8_t ch,uint8_t gain){
    uint8_t *send_buf,send_len;
    uint8_t recv_len;
    uint8_t *precv;
    uint8_t ret = 1;
    RF_TRANSLATE_CMD *pcmd,*pres_cmd;
    send_buf = create_buf_bytype(RF_GAIN_SET,&send_len);
    pcmd = (RF_TRANSLATE_CMD *)(send_buf);
    pcmd->body.gain.gain_val = gain;
    recv_len = get_response_len_bytype(RF_GAIN_SET);
    send_buf[send_len-2] = checksum(send_buf+CHECKSUM_OFFSET,send_len-CHECKSUM_IGNORE_LEN);
    precv = translate_data(ch,send_buf,send_len,recv_len);

    if(precv){
        pres_cmd = (RF_TRANSLATE_CMD *)(precv);
        if(pres_cmd->body.gain_response.gain_val == pcmd->body.gain.gain_val){
            ret = 0;
        }else{
            ret = pres_cmd->body.gain_response.status;
        }
    }else{
        ret = 1;
    }
    if(precv){
        free(precv);
    }

    if(send_buf){
        free(send_buf);
    }
    return ret;
}





static uint8_t send_noise_mode_set_cmd(uint8_t ch,uint8_t noise_mode_flag){
    uint8_t *send_buf,send_len;
    uint8_t recv_len;
    uint8_t *precv;
    uint8_t ret = 1;
    RF_TRANSLATE_CMD *pcmd,*pres_cmd;
    send_buf = create_buf_bytype(RF_NOISE_MODE_SET,&send_len);
    pcmd = (RF_TRANSLATE_CMD *)(send_buf);
    pcmd->body.noise_mode.noise_mode_flag = noise_mode_flag;
    recv_len = get_response_len_bytype(RF_NOISE_MODE_SET);
    send_buf[send_len-2] = checksum(send_buf+CHECKSUM_OFFSET,send_len-CHECKSUM_IGNORE_LEN);
    precv = translate_data(ch,send_buf,send_len,recv_len);
    if(precv){
        pres_cmd = (RF_TRANSLATE_CMD *)(precv);
        if(pres_cmd->body.noise_mode_response.noise_mode_flag == pcmd->body.noise_mode.noise_mode_flag){
            ret = 0;
        }else{
            ret = pres_cmd->body.noise_mode_response.status;
        }
    }else{
        ret = 1;
    }

    if(precv){
        free(precv);
    }

    if(send_buf){
        free(send_buf);
    }
    return  ret;
}


static uint8_t send_rf_freq_bandwidth_set_cmd(uint8_t ch,uint32_t bandwidth_flag){
    uint8_t *send_buf,send_len;
    uint8_t recv_len;
    uint8_t *precv;
    uint8_t ret = 1;
    RF_TRANSLATE_CMD *pcmd,*pres_cmd;
    send_buf = create_buf_bytype(RF_MIDDLE_FREQ_BANDWIDTH_FILTER_SET,&send_len);
    pcmd = (RF_TRANSLATE_CMD *)(send_buf);
    pcmd->body.middle_freq_bandwidth.bandwidth_flag = bandwidth_flag;
    send_buf[send_len-2] = checksum(send_buf+CHECKSUM_OFFSET,send_len-CHECKSUM_IGNORE_LEN);
    recv_len = get_response_len_bytype(RF_MIDDLE_FREQ_BANDWIDTH_FILTER_SET);
    precv = translate_data(ch,send_buf,send_len,recv_len);
    if(precv){
        pres_cmd = (RF_TRANSLATE_CMD *)(precv);
        if(pres_cmd->body.middle_freq_bandwidth_response.bandwidth_flag == pcmd->body.middle_freq_bandwidth.bandwidth_flag){
            ret = 0;
        }else{
            ret = pres_cmd->body.middle_freq_bandwidth_response.status;
        }
    }else{
        ret = 1;
    }

    if(precv){
        free(precv);
    }
    if(send_buf){
        free(send_buf);
    }
    return ret;
}


static uint8_t* send_query_cmd(uint8_t ch,uint8_t cmd){
    uint8_t *send_buf,send_len;
    uint8_t i,bytes_num,recv_len;
    uint8_t *precv;

    RF_TRANSLATE_CMD *pcmd;
    send_buf = create_buf_bytype(cmd,&send_len);
    recv_len = get_response_len_bytype(cmd);
    precv = translate_data(ch,send_buf,send_len,recv_len);
    if(send_buf){
       free(send_buf);
    }
    return precv;
}

static uint8_t send_freq_set_cmd(uint8_t ch,uint64_t freq){
    uint8_t *send_buf,send_len;
    uint8_t i,bytes_num,recv_len;
    uint8_t *precv;
    uint8_t ret = 1,retry=0,set_count=0;
    struct timespec st_time,end_time;
    int j;
    //freq /=1000;

    RF_TRANSLATE_CMD *pcmd,*pres_cmd;
    send_buf = create_buf_bytype(RF_FREQ_SET,&send_len);
    //pcmd = reinterpret_cast<RF_TRANSLATE_CMD *>(send_buf);
    pcmd = (RF_TRANSLATE_CMD *)(send_buf);
    bytes_num = sizeof(pcmd->body.freq);
    for(i=0;i<bytes_num;i++){
        pcmd->body.freq.freq_val[i] = (freq >>(8*(bytes_num-i-1))) &0xff;   
    }
    
    clock_gettime(CLOCK_REALTIME, &st_time);
    send_buf[send_len-2] = checksum(send_buf+CHECKSUM_OFFSET,send_len-CHECKSUM_IGNORE_LEN);
    recv_len = get_response_len_bytype(RF_FREQ_SET);

    printf_debug("freq send buf[%d]:", send_len);
    for( j =0 ;j< send_len; j++){
        printfd("%02x ", send_buf[j]);
    }
    printfd("\n");

    while((set_count++) <FREQ_SET_MAX_RETRY){
        precv = translate_data(ch,send_buf,send_len,recv_len);
        //printf("freq_set: ch=%d,retry time=%d,%x,%lld\n",ch,set_count,freq,freq);
        if(precv){
            free(precv);   
            precv = NULL;
        }
        //stats
        rf_freq_stats[ch].ch=ch;
        rf_freq_stats[ch].total_cmd ++;

        retry = 0;
        while((retry++) <FREQ_QUERY_MAX_RETRY){
            usleep(FREQ_QUERY_WAIT_INTERVAL);
            precv = send_query_cmd(ch,RF_FREQ_QUERY);
            pres_cmd = NULL;
            if(precv){
                pres_cmd = (RF_TRANSLATE_CMD *)(precv);
                printf_debug("lock query num:%d,return status=%d\n",retry,pres_cmd->body.freq_response.status);
                if(strncmp((char *)pres_cmd->body.freq_response.freq_val,(char *)pcmd->body.freq.freq_val,5) ==0 &&  pres_cmd->body.freq_response.status == 0 ){
                    printf_debug("lock ok query num:%d,return status=%d\n",retry,pres_cmd->body.freq_response.status);
                    ret = 0;
                    if(pres_cmd->body.freq_response.oscillator_lock_num ==0){
                        rf_freq_stats[ch].retry_one++;
                    }else if( pres_cmd->body.freq_response.oscillator_lock_num ==1){
                        rf_freq_stats[ch].retry_two++;
                    }else if(pres_cmd->body.freq_response.oscillator_lock_num ==2){
                        rf_freq_stats[ch].retry_three++;
                    }
                    break;
                }else{
                    ret = pres_cmd->body.freq_response.status;
                    printf_debug("lock failed query num:%d ch=%d,retry num=%d,status=%d\n",retry,ch,retry,pres_cmd->body.freq_response.status);
                }

                free(precv); 
                precv = NULL;
            }else{
                ret = 1;
            }
        }

        //
        if(pres_cmd){
            if(pres_cmd->body.freq_response.status == 1){
                rf_freq_stats[ch].fail_num++;
            }
            if(pres_cmd->body.freq_response.status == 2){
                rf_freq_stats[ch].cmd_parse_fail_num++;
            }
        }
        if(retry >FREQ_QUERY_MAX_RETRY){
            rf_freq_stats[ch].timeout_num++;
            if(precv){
                free(precv); 
                precv = NULL;
            }
            //printf_debug("freq_set:retry:%d,ret=%d,package:%d\n",retry,ret,pres_cmd->body.freq_response.status);
        }else{
            break;
        }
    }

    clock_gettime(CLOCK_REALTIME, &end_time);
    if(set_count >FREQ_SET_MAX_RETRY ){
        ret = 2;
        // printf_debug("freq_set:retry:%d,ret=%d,package:%d\n",retry,ret,pres_cmd->body.freq_response.status);
    }
    rf_freq_stats[ch].comm_time += time_usec_diff(end_time,st_time)/1000;

    //printf_debug("ch=%d,max retry num=%d,%ul,%ld,%x\n",ch,retry,freq,freq,freq);
    if(send_buf){
        free(send_buf);
    }
    if(precv){
        free(precv);
    }

    return ret;
}

static uint8_t send_mid_freq_gain_set_cmd(uint8_t ch,uint8_t gain){
    uint8_t *send_buf,send_len;
    uint8_t recv_len;
    uint8_t *precv;
    uint8_t ret = 1;
    RF_TRANSLATE_CMD *pcmd,*pres_cmd;
    send_buf = create_buf_bytype(MID_FREQ_GAIN_SET,&send_len);
    pcmd = (RF_TRANSLATE_CMD *)(send_buf);

    pcmd->body.gain.gain_val = gain;
    recv_len = get_response_len_bytype(MID_FREQ_GAIN_SET);
    send_buf[send_len-2] = checksum(send_buf+CHECKSUM_OFFSET,send_len-CHECKSUM_IGNORE_LEN);
    precv = translate_data(ch,send_buf,send_len,recv_len);

    if(precv){
        pres_cmd = (RF_TRANSLATE_CMD *)(precv);
        if(pres_cmd->body.gain_response.gain_val == pcmd->body.gain.gain_val){
            ret = 0;
        }else{
            ret = pres_cmd->body.gain_response.status;
        }
    }else{
        ret = 1;
    }
    if(precv){
        free(precv);
    }

    if(send_buf){
        free(send_buf);
    }
    return ret;
}

static uint8_t send_rf_attenuation_set_cmd(uint8_t ch,uint8_t attenuation){
    uint8_t ret = 0,gain_val = 0;
    gain_val = attenuation ;
#if 0
    if(attenuation >=0 && attenuation <=60){
    gain_val = 60- attenuation;

    }
#endif
    ret = send_gain_set_cmd(ch,gain_val);
    if(ret >0){
        printf_err("set rf attenuation  failed,errocode=%d\n",ret);
        return ret;   
    }
    printf_debug("set rf gain  val:%d\n",gain_val);
    return ret;
}

static uint8_t send_mid_freq_attenuation_set_cmd(uint8_t ch,uint8_t attenuation){
    uint8_t ret = 0,gain_val = 0;

    gain_val = attenuation;
#if 0
    if(attenuation >=0 && attenuation <=60){
    gain_val = 60- attenuation;

    }
#endif

    ret = send_mid_freq_gain_set_cmd(ch,gain_val);
    if(ret >0){
        printf_err("set mid freq attenuation  failed,errocode=%d\n",ret);
        return ret;
    }
    printf_debug("set mid freq  attenuation  val: %d\n",gain_val);
    return ret;

}

#if 0
uint8_t send_middle_freq_bandwidth_set_cmd(uint8_t ch,uint32_t bandwidth_flag){
    uint8_t *send_buf,send_len;
    uint8_t recv_len;
    uint8_t *precv;
    uint8_t ret = 1;
    RF_TRANSLATE_CMD *pcmd,*pres_cmd;
    send_buf = create_buf_bytype(RF_MIDDLE_FREQ_BANDWIDTH_FILTER_SET,&send_len);
    pcmd = (RF_TRANSLATE_CMD *) (send_buf); 
    pcmd->body.middle_freq_bandwidth.bandwidth_flag = bandwidth_flag;
    send_buf[send_len-2] = checksum(send_buf+CHECKSUM_OFFSET,send_len-CHECKSUM_IGNORE_LEN);
    recv_len = get_response_len_bytype(RF_MIDDLE_FREQ_BANDWIDTH_FILTER_SET);
    precv = translate_data(2,send_buf,send_len,recv_len);
    if(precv){
        pres_cmd = (RF_TRANSLATE_CMD *) (precv);
        if(pres_cmd->body.middle_freq_bandwidth_response.bandwidth_flag == pcmd->body.middle_freq_bandwidth.bandwidth_flag){
            ret = 0;
        }else{
            ret = pres_cmd->body.middle_freq_bandwidth_response.status;
        }
    }else{
        ret = 1;
    }

    if(precv){
        free(precv);
    }
    if(send_buf){
        free(send_buf);
    }
    return ret;
}

#endif


static uint8_t query_rf_temperature(uint8_t ch,int16_t *temperature){
    uint8_t ret = 0;
    uint8_t *precv;
    RF_TRANSLATE_CMD *pres_cmd;

    precv = send_query_cmd(ch,RF_TEMPRATURE_QUERY);
    pres_cmd = (RF_TRANSLATE_CMD *)(precv);
    printf_debug(" ch:%d,return status=%d temperature=%d\n",ch,pres_cmd->body.temprature_response.status,pres_cmd->body.temprature_response.temperature);

    *temperature = pres_cmd->body.temprature_response.temperature;
    ret = pres_cmd->body.temprature_response.status;
    return ret;
}


uint8_t rf_set_interface(uint8_t cmd,uint8_t ch,void *data){
    uint8_t ret = -1;

    switch(cmd){
        case EX_RF_MID_FREQ :{
            /* only set once when value changed */
            static uint64_t old_freq = 0;/* Hz */
            if(old_freq != *(uint64_t*)data){
                old_freq = *(uint64_t*)data;
            }else{
                break;
            }
            printf_note("[**RF**]ch=%d, middle_freq=%llu\n",ch, *(uint64_t*)data);
#ifdef SUPPORT_RF_ADRV9009
            gpio_select_rf_channel(*(uint64_t*)data);
            adrv9009_iio_set_freq(*(uint64_t*)data);
#elif defined(SUPPORT_RF_SPI)
            uint64_t freq_khz = old_freq/1000;/* NOTE: Hz => KHz */
            uint64_t host_freq=htobe64(freq_khz) >> 24; //小端转大端（文档中心频率为大端字节序列，5个字节,单位为Hz,实际为Khz）
            uint64_t recv_freq = 0, recv_freq_htob;

            for(int i = 0; i<3; i++){
                ret = spi_rf_set_command(SPI_RF_FREQ_SET, &host_freq);
                usleep(300);
                recv_freq = 0;
                ret = spi_rf_get_command(SPI_RF_FREQ_GET, &recv_freq);
                recv_freq_htob =  (htobe64(recv_freq) >> 24) ;  /* khz */
                printf_debug("host_freq=%llu, 0x%llx, recv_freq = %llu, 0x%llx, htobe64=0x%llx\n",freq_khz, freq_khz,recv_freq ,recv_freq,recv_freq_htob);
                if(recv_freq_htob == freq_khz){
                        break;
                }
            }
#else
            ret = send_freq_set_cmd(ch,*(uint64_t*)data);//设置射频频率
#endif
            break; 
        }
         case EX_RF_MID_BW :   {
            printf_note("[**RF**]ch=%d, middle bw=%u\n", ch, *(uint32_t *) data);
            #ifdef  SUPPORT_RF_SPI
            //uint32_t filter=htobe32(*(uint32_t *)data) >> 24;
            uint32_t filter= *(uint32_t *)data;
            ret = spi_rf_set_command(SPI_RF_MIDFREQ_BW_FILTER_SET, &filter);
            usleep(300);
            ret = spi_rf_get_command(SPI_RF_MIDFREQ_BW_FILTER_GET, &filter);
            #else
            send_rf_freq_bandwidth_set_cmd(ch, *(uint32_t *) data);
            #endif
            break; 
        }

        case EX_RF_MODE_CODE :{
            uint8_t noise_mode;
            noise_mode = *((uint8_t *)data);
            printf_info("[**RF**]ch=%d, noise_mode=%d\n", ch, noise_mode);
        #if defined(SUPPORT_PLATFORM_ARCH_ARM)
            #ifdef SUPPORT_RF_ADRV9009
            #elif defined(SUPPORT_RF_SPI)
            ret = spi_rf_set_command(SPI_RF_NOISE_MODE_SET, &noise_mode);
            usleep(300);
            ret = spi_rf_get_command(SPI_RF_NOISE_MODE_GET, &noise_mode);
            #else
            ret = send_noise_mode_set_cmd(ch,noise_mode);//设置射频接收模式
            #endif
        #endif
            break; 
        }
        case EX_RF_GAIN_MODE :{
            break; 

        }
        case EX_RF_MGC_GAIN : {
            int8_t mgc_gain_value;
            mgc_gain_value = *((int8_t *)data);
            printf_note("[**RF**]ch=%d, mgc_gain_value=%d\n",ch, mgc_gain_value);
        #if defined(SUPPORT_PLATFORM_ARCH_ARM)
            #ifdef SUPPORT_RF_ADRV9009
            #elif defined(SUPPORT_RF_SPI)
            ret = spi_rf_set_command(SPI_RF_MIDFREQ_GAIN_SET, &mgc_gain_value);
            usleep(300);
            ret = spi_rf_get_command(SPI_RF_MIDFREQ_GAIN_GET, &mgc_gain_value);
            #else
            ret = send_mid_freq_attenuation_set_cmd(ch,mgc_gain_value);//设置中频增益
            #endif
        #endif
            break; 
        }
        case EX_RF_AGC_CTRL_TIME :{
            break; 
        }
        case EX_RF_AGC_OUTPUT_AMP :{
            break; 
        }
        case EX_RF_ANTENNA_SELECT :{
            break; 
        }
        case EX_RF_ATTENUATION :{
            int8_t rf_gain_value = 0;
            rf_gain_value = *((int8_t *)data);
            printf_note("[**RF**]ch=%d, rf_gain_value=%d\n",ch, rf_gain_value);
        #if defined(SUPPORT_PLATFORM_ARCH_ARM)
            #ifdef SUPPORT_RF_ADRV9009
            gpio_select_rf_attenuation(rf_gain_value);
            #elif defined(SUPPORT_RF_SPI)
            ret = spi_rf_set_command(SPI_RF_GAIN_SET, &rf_gain_value);
            usleep(300);
            ret = spi_rf_get_command(SPI_RF_GAIN_GET, &rf_gain_value);
            #else
            ret = send_rf_attenuation_set_cmd(ch,rf_gain_value);//设置射频增益
            #endif
        #endif
            break; 
        }
        case EX_RF_CALIBRATE:
        {
            CALIBRATION_SOURCE_ST *akt_cs;
            struct calibration_source_t cs;
            akt_cs = (CALIBRATION_SOURCE_ST *)data;
            cs.source = akt_cs->enable;
            cs.middle_freq_mhz = akt_cs->middle_freq_hz/1000000;
            cs.power = (float)akt_cs->power;
            printf_note("source=%d, middle_freq_mhz=%uMhz, power=%f\n", cs.source, cs.middle_freq_mhz, cs.power);
#if defined(SUPPORT_RF_SPI)
            ret = spi_rf_set_command(SPI_RF_CALIBRATE_SOURCE_SET, &cs);
#endif
            break;
        }

        default:{
            break;
        }
    }
    return ret;
}

uint8_t rf_read_interface(uint8_t cmd,uint8_t ch,void *data){
    uint8_t ret = -1;

    switch(cmd){
        case EX_RF_MID_FREQ : {
            printf_debug("rf_read_interface %d\n",EX_RF_MID_FREQ);
            break;
        }
        case EX_RF_MID_BW :   {
            break; 
        }
        case EX_RF_MODE_CODE :{
            break; 
        }
        case EX_RF_GAIN_MODE :{
            break; 
        }
        case EX_RF_MGC_GAIN : {
            break; 
        }
        case EX_RF_AGC_CTRL_TIME : {
            break; 
        }
        case EX_RF_AGC_OUTPUT_AMP :{
            break; 
        }
        case EX_RF_ANTENNA_SELECT :{
            break; 
        }
        case EX_RF_ATTENUATION :{
            break; 
        }
        case EX_RF_STATUS_TEMPERAT :{
            int16_t  rf_temperature = 0;
            #if defined(SUPPORT_PLATFORM_ARCH_ARM)
            #elif defined(SUPPORT_RF_SPI)
            ret = spi_rf_get_command(SPI_RF_TEMPRATURE_GET, &rf_temperature);
            #else
            ret = query_rf_temperature(ch,&rf_temperature);//设置射频增益
            #endif
            *(int16_t *)data = rf_temperature;
            printf_info("rf temprature:%d\n", *(int16_t *)data);
            break;
        }
        default:{
            break;
        }

    }
    return ret;

}


void spi_close(void)
{
    int i;
    for(i=0;i<SPI_CONTROL_NUM;i++){
        if (spidevfd[i] > 0) {
            close(spidevfd[i]);
            spidevfd[i] = -1;
        }
    }
    for(i = 0 ;i <MAX_GPIO_NUM ;i++){
        if (gpiofd[i] > 0) {
            close(gpiofd[i]);
            gpiofd[i] = -1;
        }
    }
}



int8_t rf_init(void)
{
    int ret = -1;
    uint8_t status = 0;
    printf_note("RF init...!\n");
#ifdef SUPPORT_RF_ADRV9009
    adrv9009_iio_init();
#endif
#if defined(SUPPORT_RF_SPI)
    ret = spi_init();
#endif
    return ret;
}



