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
*  Rev 1.0   19 Nov 2019   yaochangsong
*  Initial revision.
******************************************************************************/
#include "config.h"
#include "spi.h"

pthread_mutex_t spi_mutex = PTHREAD_MUTEX_INITIALIZER;


struct rf_spi_cmd_info spi_cmd[] ={
    /* setcmd                 get cmd              send data byte len    receive data(&status) byte len */
    {SPI_RF_FREQ_SET,              SPI_RF_FREQ_GET,                 5,                    7 },
    {SPI_RF_GAIN_SET,              SPI_RF_GAIN_GET,                 1,                    2 },
    {SPI_RF_MIDFREQ_BW_FILTER_SET, SPI_RF_MIDFREQ_BW_FILTER_GET,    1,                    2 },
    {SPI_RF_NOISE_MODE_SET,        SPI_RF_NOISE_MODE_GET,           1,                    2 },
    {SPI_RF_GAIN_CALIBRATE_SET,    SPI_RF_GAIN_CALIBRATE_GET,       0,                    1 },
    {SPI_RF_MIDFREQ_GAIN_SET,      SPI_RF_MIDFREQ_GAIN_GET,         1,                    2 },
    {-1,                           SPI_RF_TEMPRATURE_GET,           0,                    3 },
    {SPI_RF_CALIBRATE_SOURCE_SET,  -1,                              9,                   10 }
    
};


struct rf_spi_node_info spi_node[] ={
    /* name path               function code    pin  spifd      pinfd  info */
    {"/dev/spidev32765.0",     SPI_FUNC_RF,     8,   -1,      -1,  "spi rf"},
    {"/dev/spidev32766.0",     SPI_FUNC_CLOCK,  8,   -1,      -1,  "spi clock 7044 chip"},
    {"/dev/spidev32766.1",     SPI_FUNC_AD,     8,   -1,      -1,  "spi ad 9690 chip"},
    {NULL,                     -1              -1,   -1,      -1,  NULL},
};

static uint8_t spi_checksum(uint8_t *buffer,uint8_t len){
    uint32_t check_sum = 0;
    uint8_t i;
    for(i=0;i <len ;i++){
        check_sum += buffer[i];
    }
    return check_sum&0xff;
}


static int spi_assemble_send_data(uint8_t *buffer, uint8_t cmd, void *data, size_t len)
{
    #define SPI_DATA_HADER 0xaa
    #define SPI_DATA_END   0x55
    uint8_t *ptr = buffer, *pchecksum;
    if(ptr == NULL)
        return -1;
    *ptr++ = SPI_DATA_HADER;
    pchecksum = ptr;
    *ptr++ = len;
    *ptr++ = cmd;
    if(len != 0){
        memcpy(ptr, data, len);
        ptr += len;
    }
    *ptr++ = spi_checksum(pchecksum, len+2);
    *ptr++ = SPI_DATA_END;
    return (ptr - buffer);/* RETURN: total frame byte count */
}

int spi_send_data(int spi_fd, uint8_t *send_buffer, size_t send_len,
                                    uint8_t *recv_buffer,  size_t recv_len)
{
    struct spi_ioc_transfer xfer[2];
    int ret = -1;
    
    memset(xfer, 0, sizeof(xfer));
    if((recv_len > 0) && (recv_buffer != NULL)){
        memset(recv_buffer, 0, recv_len);
    }

    xfer[0].tx_buf = (unsigned long) send_buffer;
    xfer[0].len = send_len;
    xfer[0].delay_usecs = 2;
    if((recv_len > 0) && (recv_buffer != NULL)){
        xfer[1].rx_buf = (unsigned long) recv_buffer;
        xfer[1].len = recv_len;
        xfer[1].delay_usecs = 2;
    }
    if((recv_len > 0) && (recv_buffer != NULL)){
        ret = ioctl(spi_fd, SPI_IOC_MESSAGE(2), xfer);
    }else{
        ret = ioctl(spi_fd, SPI_IOC_MESSAGE(1), xfer);
    }
    if(ret < 0){
        printf_err("spi send error:%d, %s\n", ret, strerror(errno));
    }
    return ret;
}

int spi_init(void)
{
    struct rf_spi_node_info *ptr = &spi_node;
    uint8_t mode = 0;
    uint32_t speed = 4000000;
    
    for(int i = 0; i< ARRAY_SIZE(spi_node); i++){
        if(ptr[i].name != NULL){
            ptr[i].fd =  open(ptr[i].name,O_RDWR);
            if(ioctl(ptr[i].fd, SPI_IOC_WR_MODE,&mode) <0){
                printf_err("can't set spi mode\n");
                continue;
            }
            printf_note("spi[%s] mode: %d\n", ptr[i].name, mode);
            if(ioctl(ptr[i].fd,SPI_IOC_WR_MAX_SPEED_HZ, &speed)<0){
                printf_err("can't set max speed hz\n");
                continue;
            }
            printf_note("spi[%s, %s][fd=%d]max speed: %d Hz (%d KHz)\n", ptr[i].name, ptr[i].info,ptr[i].fd, speed, speed/1000);
        }
    }
    return 0;
}


static ssize_t prase_recv_data(uint8_t cmd, uint8_t *recv_buffer, size_t data_len, void *data)
{
    uint8_t *ptr_data, status;
    int status_len = 0;
    ptr_data = recv_buffer+3;
    if(cmd == SPI_RF_FREQ_GET || cmd == SPI_RF_FREQ_SET){
        status_len = 2;
    }else{
        status_len = 1;
    }
    if(data_len < status_len){
        return -1;
    }
    if(data != NULL){
        memcpy(data, ptr_data, data_len -status_len);
    }
    ptr_data += data_len -status_len;
    status = *ptr_data;
    printf_info("status=%d\n", status);
    return status;
}


ssize_t spi_rf_set_command(rf_spi_set_cmd_code cmd, void *data)
{
    uint8_t frame_buffer[64], recv_buffer[64];
    ssize_t frame_size = -1, recv_size = 0,data_len =0;
    int found = 0, ret = 0;
    int spi_fd;
    for(int i = 0; i< ARRAY_SIZE(spi_cmd); i++){
        if(cmd == spi_cmd[i].scode){
             frame_size = spi_assemble_send_data(frame_buffer, cmd, data, spi_cmd[i].send_byte_len);
             data_len = spi_cmd[i].recv_byte_len;
             recv_size = data_len + 5; /* 5 is frame header & tail length */
             found = 1;
             break;
        }
    }
    if(found == 0 || frame_size == -1){
        return -1;
    }
    found = 0;
    for(int i = 0; i< ARRAY_SIZE(spi_node); i++){
        if(spi_node[i].func_code == SPI_FUNC_RF){
            spi_fd = spi_node[i].fd;
            found = 1;
        }
    }
    if(found == 0 || spi_fd == -1){
        return -1;
    }
    printf_info("send frame buffer[%d]:", frame_size);
    for(int i = 0; i< frame_size; i++)
        printfi("%02x ", frame_buffer[i]);
    printfi("\n");
    ret = spi_send_data(spi_fd, frame_buffer, frame_size, recv_buffer, recv_size);
    if(ret < 0){
        return NULL;
    }
    printf_info("receive data buffer[%d]:", recv_size);
    for(int i = 0; i< recv_size; i++)
        printfi("0x%x ", recv_buffer[i]);
    printfi("\n");
    ret = prase_recv_data(cmd, recv_buffer, data_len, NULL);
    if(ret != 0)
        return -1;
    return ret;
}

ssize_t spi_rf_get_command(rf_spi_get_cmd_code cmd, void *data)
{
    uint8_t frame_buffer[64], recv_buffer[64];
    ssize_t frame_size = -1, recv_size = 0, data_len =0;
    int found = 0, ret = 0;
    int spi_fd;
    for(int i = 0; i< ARRAY_SIZE(spi_cmd); i++){
        if(cmd == spi_cmd[i].gcode){
             frame_size = spi_assemble_send_data(frame_buffer, cmd, NULL, 0);
             data_len = spi_cmd[i].recv_byte_len;
             recv_size = data_len + 5; /* 5 is frame header & tail length */
             found = 1;
             break;
        }
    }
    if(found == 0 || frame_size == -1){
        return -1;
    }
    found = 0;
    for(int i = 0; i< ARRAY_SIZE(spi_node); i++){
        if(spi_node[i].func_code == SPI_FUNC_RF){
            spi_fd = spi_node[i].fd;
            found = 1;
        }
    }
    if(found == 0 || spi_fd == -1){
        return -1;
    }
    printf_info("send frame buffer[%d]:", frame_size);
    for(int i = 0; i< frame_size; i++)
        printfi("0x%x ", frame_buffer[i]);
    printfi("\n");
    ret = spi_send_data(spi_fd, frame_buffer, frame_size, recv_buffer, recv_size);
    if(ret < 0){
        return -1;
    }
    printf_info("receive data buffer[%d]:", recv_size);
    for(int i = 0; i< recv_size; i++)
        printfi("0x%x ", recv_buffer[i]);
    printfi("\n");
    ret = prase_recv_data(cmd, recv_buffer, data_len,data);
    if(ret != 0)
        return -1;
    return 0;
}

int spi_clock_init_before(void)
{
    uint8_t send_buf[8];
    int spi_fd, ret =0;
    int found = 0, index = 0;
    uint32_t array_len = 3;
    bool internal_clock = config_get_is_internal_clock();
    
    for(int i = 0; i< ARRAY_SIZE(spi_node); i++){
        if(spi_node[i].func_code == SPI_FUNC_CLOCK){
            spi_fd = spi_node[i].fd;
            index = i;
            found = 1;
        }
    }
    if(found == 0 || spi_fd == -1){
        printf_err("SPI CLOCK init Faild!\n");
        return -1;
    }

    send_buf[0] = 0x0;
    send_buf[1] = 0x0;
    send_buf[2] = 0x01;
    ret = spi_send_data(spi_fd, send_buf, array_len, NULL, 0);
    usleep(50000);

    send_buf[0] = 0x0;
    send_buf[1] = 0x0;
    send_buf[2] = 0x00;
    ret = spi_send_data(spi_fd, send_buf, array_len, NULL, 0);
    usleep(50000);

    if(internal_clock){
        for(int i=0; i<get_internal_clock_cfg_array_size(); i++){
             memcpy(send_buf,clock_7044_config_internal[i],array_len);
             ret = spi_send_data(spi_fd, send_buf, array_len, NULL, 0);
         }
        send_buf[0] = 0x0;
        send_buf[1] = 0xb0;
        send_buf[2] = 0x04;
        ret = spi_send_data(spi_fd, send_buf, array_len, NULL, 0);
        usleep(10000);
    }else{
        for(int i=0; i<get_external_clock_cfg_array_size(); i++){
            memcpy(send_buf,clock_7044_config[i],array_len);
            ret = spi_send_data(spi_fd, send_buf, array_len, NULL, 0);
        }
    }
    send_buf[0] = 0x0;
    send_buf[1] = 0x01;
    send_buf[2] = 0x62;
    ret = spi_send_data(spi_fd, send_buf, array_len, NULL, 0);

    send_buf[0] = 0x0;
    send_buf[1] = 0x01;
    send_buf[2] = 0x60;
    ret = spi_send_data(spi_fd, send_buf, array_len, NULL, 0);
    usleep(5000);

    send_buf[0] = 0x0;
    send_buf[1] = 0x01;
    send_buf[2] = 0xe0;
    ret = spi_send_data(spi_fd, send_buf, array_len, NULL, 0);
    usleep(100);
    
    send_buf[0] = 0x0;
    send_buf[1] = 0x01;
    send_buf[2] = 0x60;
    ret = spi_send_data(spi_fd, send_buf, array_len, NULL, 0);
    usleep(100);
    
    printf_note("[%s]SPI CLOCK init OK![%s] Clock\n",  spi_node[index].info, internal_clock== true ? "Internal" : "External");
    return ret;
}

int spi_clock_init_after(void)
{
    uint8_t send_buf[8];
    int spi_fd, ret =0;
    int found = 0, index = 0;
    uint32_t array_len = 3;
    
    for(int i = 0; i< ARRAY_SIZE(spi_node); i++){
        if(spi_node[i].func_code == SPI_FUNC_CLOCK){
            spi_fd = spi_node[i].fd;
            index = i;
            found = 1;
        }
    }
    if(found == 0 || spi_fd == -1){
        printf_err("SPI CLOCK init Faild!\n");
        return -1;
    }

    send_buf[0] = 0x0;
    send_buf[1] = 0x1;
    send_buf[2] = 0x44;
    ret = spi_send_data(spi_fd, send_buf, array_len, NULL, 0);

    send_buf[0] = 0x0;
    send_buf[1] = 0x1;
    send_buf[2] = 0x40;
    ret = spi_send_data(spi_fd, send_buf, array_len, NULL, 0);

    printf_note("SPI CLOCK init OK!\n");
    return ret;
}

int spi_clock_check(void)
{
    int value;
    if(gpio_raw_read_value(GPIO_FUNC_ADC_STATUS, &value)!= 0){
        printf_err("SPI CLOCK Check Faild!\n");
        return -1;
    }
    if(value != 1){
        printf_err("SPI CLOCK Check Faild!\n");
        return -1;
    }
    printf_note("SPI CLOCK Check OK!\n");
    return 0;
}


int spi_adc_init(void)
{
    
    uint8_t send_buf[8];
    int spi_fd, ret =0;
    int found = 0, index = 0;
    uint32_t array_len = 3;
    
    for(int i = 0; i< ARRAY_SIZE(spi_node); i++){
        if(spi_node[i].func_code == SPI_FUNC_AD){
            spi_fd = spi_node[i].fd;
            index = i;
            found = 1;
        }
    }
    if(found == 0 || spi_fd == -1){
        printf_err("SPI ADC init Faild!\n");
        return -1;
    }
    printf_note("SPI ADC spi_fd=%d\n", spi_fd);

    send_buf[0] = 0x0;
    send_buf[1] = 0x0;
    send_buf[2] = 0x81;
    ret = spi_send_data(spi_fd, send_buf, array_len, NULL, 0);
    usleep(10000);

    for(int i=0; i<get_adc_cfg_array_size(); i++){
        memcpy(send_buf,ad_9690_config[i],array_len);
        printfd("0x%02x 0x%02x 0x%02x\n",  send_buf[0], send_buf[1], send_buf[2]);
        ret = spi_send_data(spi_fd, send_buf, array_len, NULL, 0);
    }
    printf_note("[%s]SPI ADC init OK!\n",  spi_node[index].info);
    return ret;
}



