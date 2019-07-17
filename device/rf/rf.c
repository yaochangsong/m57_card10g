#include "config.h"


static int spidevfd[SPI_CONTROL_NUM] =  {-1};
static int gpiofd[MAX_GPIO_NUM] = {-1};

static int ledfd[MAX_LED_NUM] = {-1};
static DEVICE_RF_INFO_ST rf_param[MAX_CHANNEL_NUM]={0};
static RF_FREQ_STATS rf_freq_stats[MAX_CHANNEL_NUM] = {0};
static pthread_mutex_t mut;


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


int8_t  spi_dev_init(void)
{
    uint8_t mode = 0;
    uint32_t speed = 4000000;
    uint32_t i,cnum;
    char dev_name[32];

    if(spidevfd[0] >0){
         //printf_err("spi device has been inited.\n");
         return -1;
    }
    for(cnum=0 ;cnum <SPI_CONTROL_NUM ;cnum++){

        if(cnum == 0){
            mode = 0;
            sprintf(dev_name,"/dev/spidev32765.0");
        }else{
            //sprintf(dev_name,"/dev/spidev%d.0",dev_num);
            //same one spi controller
            continue;
        }

        if (spidevfd[cnum] <=0){
            spidevfd[cnum] =  open(dev_name,O_RDWR);
            if(spidevfd[cnum] < 0){
                // return  -1;
                continue;
            }
            if(ioctl(spidevfd[cnum], SPI_IOC_WR_MODE,&mode) <0){
                return -1;   
            }
            if(ioctl(spidevfd[cnum],SPI_IOC_WR_MAX_SPEED_HZ, &speed)<0){
                return -1;
            }
        }

        //spi0 with gpio for multi bus
        if(cnum == 0){
            for(i = 0 ;i <MAX_GPIO_NUM;i++){
                //only init used gpio
                if((GPIO_USED_MASK&(ULLI<<i))>0){
                        if(gpiofd[i] <=0){
                                printf_debug("gpio init i = %d\n",i);
                                gpiofd[i] = init_gpio(i);
                            if(gpiofd[i] <0){
                                printf_err("gpio init failed\n");
                                continue;
                        }
                    }
                }
            }
        }

    }
    for(cnum=1 ;cnum <SPI_CONTROL_NUM ;cnum++){
        spidevfd[cnum] = spidevfd[cnum-1];
        printf_debug("spi num:%d,fd=%d\n",cnum,spidevfd[cnum]);
    }

#if 0
for(i = 0 ;i <MAX_LED_NUM;i++){
    if(ledfd[i] <=0){
        ledfd[i] = init_led(i);
        if(ledfd[i] <0){
            //return -1;
            printf_err("led[%d] init failed\n", i);
        }
    }
}


    set_green_led_on();
#endif
    return 0;
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

#ifdef DEBUG_FREQUENCY
    PRINTF("spi send data:");
    for(i=0;i<buf_len ;i++){
        printf_debug("%02x ",buf[i]);
    }
    printf_debug("\n");
#endif

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

#ifdef DEBUG_FREQUENCY
    if(recv_len >0){
        PRINTF("spi recv data:");
        for(i=0;i<recv_len ;i++){
            printf_debug("%02x ",recv_buf[i]);
        }
        printf_debug("\n");
    }
#endif

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

}

#if defined(INTERNAL_CLK)
/* internal clock , for test */
void clock_7044_init_internal(void){

}
#endif

void ad9690_init(void){

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


static uint8_t config_rf_param(uint8_t ch,DEVICE_RF_INFO_ST param){
    uint8_t ret ;
    ret= send_noise_mode_set_cmd(ch,param.patten_id);
    if(ret >0){
        printf_err("set noise mode failed,errcode=%d\n",ret);
        return ret;
    }
    if(param.gain_patten_id == MANUAL_GAIN){
        ret = send_gain_set_cmd(ch,param.gain_val);
        if(ret >0){
            printf_err("set gain set  failed,errocode=%d\n",ret);
            return ret;   
        }
    }

    ret = send_rf_freq_bandwidth_set_cmd(ch,param.bandwith_id);
    if(ret >0){
        printf_err("set middle freq band set  failed,errocde=%d\n",ret);
        return ret;   
    }

    return 0;
}


static void save_rf_parameter(uint8_t ch,  DEVICE_RF_INFO_ST para)
{
    rf_param[ch] = para;
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
#ifdef DEBUG_FREQUENCY
    printf("freq send buf[%d]:", send_len);
    for( j =0 ;j< send_len; j++){
        printf_debug("%02x ", send_buf[j]);
    }
    printf_debug("\n");
#endif	

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
    uint8_t mgc_gain_value,noise_mode,rf_gain_value,tmp;
    uint32_t rf_bw;
    uint64_t rf_freq;
    uint8_t *precv;
    RF_TRANSLATE_CMD *pres_cmd;

    switch(cmd){
        case EX_RF_MID_FREQ :{
            break; 
        }
        case EX_RF_MID_BW :{
            break; 
        }
        case EX_RF_MODE_CODE :{
            noise_mode = *((uint8_t *)data);
            printf_debug("rf_set_interface %d noise_mode=%d\n",EX_RF_MODE_CODE,noise_mode);
            #if defined(PLAT_FORM_ARCH_ARM)
            ret = send_noise_mode_set_cmd(ch,noise_mode);//设置射频接收模式
            #endif
            break; 
        }
        case EX_RF_GAIN_MODE :{
            break; 

        }
        case EX_RF_MGC_GAIN : {
            mgc_gain_value = *((uint8_t *)data);
            printf_debug("rf_set_interface %d mgc_gain_value=%d\n",EX_RF_MGC_GAIN,mgc_gain_value);
            #if defined(PLAT_FORM_ARCH_ARM)
            ret = send_mid_freq_attenuation_set_cmd(ch,mgc_gain_value);//设置中频增益
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
            rf_gain_value = *((uint8_t *)data);
            printf_debug("rf_set_interface %d rf_gain_value=%d\n",EX_RF_ATTENUATION,rf_gain_value);
            #if defined(PLAT_FORM_ARCH_ARM)
            ret = send_rf_attenuation_set_cmd(ch,rf_gain_value);//设置射频增益
            #endif
            break; 
        }
        case EX_RF_AGC_FREQUENCY :{
            rf_freq = *((uint64_t *)data);
            printf_debug("rf_set_interface %d rf_freq=%lld\n",EX_RF_AGC_FREQUENCY,rf_freq);
            #if defined(PLAT_FORM_ARCH_ARM)
            ret = send_freq_set_cmd(ch,rf_freq);//设置射频频率
            #endif
            break; 
        }
        case EX_RF_AGC_BW :{
            rf_bw = *((uint32_t *)data);
            printf_debug("rf_set_interface %d rf_freq=%d\n",EX_RF_AGC_BW,rf_bw);
            #if defined(PLAT_FORM_ARCH_ARM)
            ret = send_rf_freq_bandwidth_set_cmd(ch,rf_bw);//设置射频带宽
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
    uint8_t mgc_gain_value,noise_mode;
    uint8_t *precv;
    RF_TRANSLATE_CMD *pres_cmd;

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
            #if defined(PLAT_FORM_ARCH_ARM)
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


int8_t spi_init(void)
{
    int ret = -1;
    printf_debug("spi init!\n");
    #if defined(PLAT_FORM_ARCH_ARM)
    pthread_mutex_init(&mut,NULL);
    spi_fd_init();
    ret = spi_dev_init();
    #endif
    
    return ret;
}



