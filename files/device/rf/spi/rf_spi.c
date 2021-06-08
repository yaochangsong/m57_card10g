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
#include "rf_spi.h"

static struct rf_spi_node_info spi_node[] ={
    /* name path               function code    pin  spifd      pinfd  info */
    /* petalinux2019.1 */
#if 1//#ifdef //PETALINUX_VER_2019_1
    {"/dev/spidev2.0",     SPI_FUNC_RF,     8,   -1,      -1,  "spi rf"},
    /* petalinux2016.4 */
#else
    {"/dev/spidev32765.0",     SPI_FUNC_RF,     8,   -1,      -1,  "spi rf"},
#endif
    {NULL,                     -1,              -1,   -1,      -1,  NULL},
};


static struct rf_spi_cmd_info spi_cmd[] ={
#if defined(SUPPORT_RF_SPI_TF713)
    {SPI_RF_FREQ_SET,              -1,                              38,                    0 },
    {SPI_RF_NOISE_MODE_SET,        -1,                              16,                    0 },
    {SPI_RF_GAIN_SET,              -1,                              19,                    0 },
    {SPI_RF_MIDFREQ_GAIN_SET,      -1,                              19,                    0 },
    {SPI_RF_MIDFREQ_BW_FILTER_SET, -1,                              16,                    0 },
    {-1,                           SPI_RF_WORK_PARA_GET,            13,                    6 },
    {-1,                           SPI_RF_TEMPRATURE_GET,           13,                    2 },
    {-1,                           SPI_RF_HANDSHAKE,                13,                    1 },
#else
    /* setcmd                 get cmd              send data byte len    receive data(&status) byte len */
    {SPI_RF_FREQ_SET,              SPI_RF_FREQ_GET,                 5,                    7 },
    {SPI_RF_GAIN_SET,              SPI_RF_GAIN_GET,                 1,                    2 },
    {SPI_RF_MIDFREQ_BW_FILTER_SET, SPI_RF_MIDFREQ_BW_FILTER_GET,    1,                    2 },
    {SPI_RF_NOISE_MODE_SET,        SPI_RF_NOISE_MODE_GET,           1,                    2 },
    {SPI_RF_GAIN_CALIBRATE_SET,    SPI_RF_GAIN_CALIBRATE_GET,       0,                    1 },
    {SPI_RF_MIDFREQ_GAIN_SET,      SPI_RF_MIDFREQ_GAIN_GET,         1,                    2 },
    {-1,                           SPI_RF_TEMPRATURE_GET,           0,                    3 },
    {SPI_RF_CALIBRATE_SOURCE_SET,  -1,                              9,                   10 }
#endif
};


static bool spi_has_inited = false;
int spi_rf_init(void)
{
    struct rf_spi_node_info *ptr = spi_node;
    uint8_t mode = 0;
    uint32_t speed = 4000000;
#if defined(SUPPORT_RF_SPI_TF713)
    speed = 1000000;
    mode = 0;
#else
    speed = 4000000;
    mode = SPI_CPOL|SPI_CPHA ;
#endif

    if(spi_has_inited == true)
        return 0;
    for(int i = 0; i< ARRAY_SIZE(spi_node); i++){
        if(ptr[i].name != NULL){
            ptr[i].fd =  open(ptr[i].name,O_RDWR);
            if(ptr[i].fd < 0){
                printf_err("[%d]can't open spi %s, fd:%d\n", i, ptr[i].name, ptr[i].fd);
                continue;
            }
            if(ptr[i].func_code == SPI_FUNC_RF){
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
    }
    spi_has_inited = true;
    return 0;
}

static uint8_t spi_checksum(uint8_t *buffer,uint8_t len){
    uint32_t check_sum = 0;
    uint8_t i;
    for(i=0;i <len ;i++){
        check_sum += buffer[i];
    }
    return check_sum&0xff;
}


#if defined(SUPPORT_RF_SPI_TF713)
static int spi_send_data_bits(int spi_fd, uint8_t *send_buffer, size_t send_len,
                            uint8_t *recv_buffer,  size_t recv_len)
{
    char resp_buf[100];
    struct spi_ioc_transfer xfer[3];
    int ret = -1, i;
    uint8_t mode = 0;

    printfd("spi send data:");
    for(i = 0; i < send_len / 8 + (send_len % 8 > 0 ? 1 : 0); i++){
        printfd("0x%02x ", send_buffer[i]);
    }
    printfd("\n");
    
    memset(xfer, 0, sizeof(xfer));
    if((recv_len > 0) && (recv_buffer != NULL)){
        memset(recv_buffer, 0, recv_len);
    }

    mode =0;
    ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);

    xfer[0].tx_buf = (unsigned long) send_buffer;
    xfer[0].rx_buf = (unsigned long) resp_buf;
    xfer[0].len = send_len / 8;
    xfer[0].bits_per_word = 8;
    
    if(send_len % 8 > 0) {
        xfer[1].tx_buf = (unsigned long) (send_buffer + send_len / 8);
        xfer[1].len = 1;
        xfer[1].bits_per_word = send_len % 8;
        ret = ioctl(spi_fd, SPI_IOC_MESSAGE(2), xfer);
    } else {
        ret = ioctl(spi_fd, SPI_IOC_MESSAGE(1), &xfer[0]);
    }
    
    if((recv_len > 0) && (recv_buffer != NULL)){
        xfer[2].rx_buf = (unsigned long) recv_buffer;
        xfer[2].len = recv_len;
    
        mode |= SPI_CS_HIGH;
        ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
        ret = ioctl(spi_fd, SPI_IOC_MESSAGE(1), &xfer[2]);
        
        printfd("recv data:");
        for(i = 0; i < recv_len; i++) {
            printfd("0x%02x ", recv_buffer[i]);
        }
        printfd("\n");
        mode = 0;
        ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);

    }else{
        printf_info("spi no response data\n");
        return 0;
    }
    
    if (ret < 0) {
        printf_err("spi data happen err\n");
        perror("spi error:");
        return -1;
    }

#if 0
    printfd("send response:");
    for(i = 0; i < send_len/8; i++)
    {
        printfd("0x%02x ",resp_buf[i]);
    }

    printfd("\n");
#endif
    return 0;
}

static int spi_assemble_get_data(int ch, uint8_t *buffer, uint8_t cmd)
{
    #define RF_CHANNEL_CS 0x1   //Í¨µÀÑ¡ÔñÂë
    uint8_t *ptr = buffer,  val = 0;
    val = ch << 3;  /* 5bit */
    val |= (RF_CHANNEL_CS << 1) & 0x07; /* 2bit */
    val |= (cmd >> 5) & 0x1;     /*cmd: 6bit */
    *ptr++ = val;
    *ptr++ = cmd;
    
    return (ptr - buffer);
}


static int spi_assemble_set_data(int ch, uint8_t *buffer, uint8_t cmd, void *vdata, int bits_len)
{
    #define RF_HEADER_BEGIN_LEN 7
    #define RF_HEADER_END_LEN 6
    #define RF_HEADER_LEN (RF_HEADER_BEGIN_LEN + RF_HEADER_END_LEN)
    #define RF_CHANNEL_CS 0x1   //Í¨µÀÑ¡ÔñÂë
    uint8_t *ptr = buffer,  val = 0;
    int tmp_bits = 0;
    uint32_t data = *(uint32_t *)vdata;
    *ptr = ch << 3;  /* 5bit */
    *ptr |= (RF_CHANNEL_CS << 1) & 0x07; /* 2bit */
    tmp_bits = bits_len - RF_HEADER_LEN - 1;
    if(tmp_bits < 0)
        return -1;
    *ptr |=  (data >> tmp_bits) & 0x1; /* 1bit */
    ptr++;
    
    while(tmp_bits > 0){
        if(tmp_bits < 8)
            *ptr |= data << (8 - tmp_bits);
        else
            *ptr |= data >> (tmp_bits - 8);
        if((tmp_bits + RF_HEADER_END_LEN) > 8)
            ptr++;
        tmp_bits = tmp_bits - 8;
    }
    tmp_bits = 8 -((bits_len -RF_HEADER_END_LEN) % 8);
    tmp_bits = tmp_bits % 8;
    if(tmp_bits > 0){
        *ptr |= (cmd >> tmp_bits);
    }
    *ptr |= cmd;
    ptr ++;

    return (ptr - buffer);
}



ssize_t spi_rf_get_command_bits(int ch, rf_spi_get_cmd_code cmd, void *data, int len)
{
    int rf_ch = ch +1;
    uint8_t frame_buffer[64], recv_buffer[64];
    ssize_t frame_size = -1, recv_size = 0;
    int found = 0, ret = 0;
    int spi_fd = -1;
    
    for(int i = 0; i< ARRAY_SIZE(spi_cmd); i++){
        if(cmd == spi_cmd[i].gcode){
            memset(frame_buffer, 0, sizeof(frame_buffer));
            spi_assemble_get_data(rf_ch, frame_buffer, cmd);
            frame_size = spi_cmd[i].send_len;
            recv_size = spi_cmd[i].recv_len;
            found = 1;
            break;
        }
    }
    if(found == 0 || recv_size <= 0){
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

    ret = spi_send_data_bits(spi_fd, frame_buffer, frame_size, recv_buffer, recv_size);
    if(ret == 0 && (len <= sizeof(recv_buffer))){
        memcpy(data, recv_buffer, len); 
    }
    return ret;
}


ssize_t spi_rf_set_command_bits(int ch, rf_spi_set_cmd_code cmd, void *data)
{
    int rf_ch = ch +1;
    uint8_t frame_buffer[64], recv_buffer[64];
    ssize_t frame_size = -1;
    int found = 0, ret = 0;
    int spi_fd = -1;
    
    for(int i = 0; i< ARRAY_SIZE(spi_cmd); i++){
        if(cmd == spi_cmd[i].scode){
            frame_size = spi_cmd[i].send_len;
            memset(frame_buffer, 0, sizeof(frame_buffer));
            spi_assemble_set_data(rf_ch, frame_buffer, cmd, data, frame_size);
            found = 1;
            break;
        }
    }
    if(found == 0 || frame_size <= 0){
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

    ret = spi_send_data_bits(spi_fd, frame_buffer, frame_size, NULL, 0);

    return ret;
}
#else

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

static int spi_send_data(int spi_fd, uint8_t *send_buffer, size_t send_len,
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
    printf_debug("status=%d\n", status);
    return status;
}


static ssize_t _spi_rf_set_command(rf_spi_set_cmd_code cmd, void *data)
{
    uint8_t frame_buffer[64], recv_buffer[64];
    ssize_t frame_size = -1, recv_size = 0,data_len =0;
    int found = 0, ret = 0;
    int spi_fd;
    for(int i = 0; i< ARRAY_SIZE(spi_cmd); i++){
        if(cmd == spi_cmd[i].scode){
             frame_size = spi_assemble_send_data(frame_buffer, cmd, data, spi_cmd[i].send_len);
             data_len = spi_cmd[i].recv_len;
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
    printf_debug("send frame buffer[%d]:", frame_size);
    for(int i = 0; i< frame_size; i++)
        printfd("%02x ", frame_buffer[i]);
    printfd("\n");
    ret = spi_send_data(spi_fd, frame_buffer, frame_size, recv_buffer, recv_size);
    if(ret < 0){
        return NULL;
    }
    printf_debug("receive data buffer[%d]:", recv_size);
    for(int i = 0; i< recv_size; i++)
        printfd("0x%x ", recv_buffer[i]);
    printfd("\n");
    ret = prase_recv_data(cmd, recv_buffer, data_len, NULL);
    if(ret != 0)
        return -1;
    return ret;
}

static ssize_t _spi_rf_get_command(rf_spi_get_cmd_code cmd, void *data)
{
    uint8_t frame_buffer[64], recv_buffer[64];
    ssize_t frame_size = -1, recv_size = 0, data_len =0;
    int found = 0, ret = 0;
    int spi_fd;
    for(int i = 0; i< ARRAY_SIZE(spi_cmd); i++){
        if(cmd == spi_cmd[i].gcode){
             frame_size = spi_assemble_send_data(frame_buffer, cmd, NULL, 0);
             data_len = spi_cmd[i].recv_len;
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
    printf_debug("send frame buffer[%d]:", frame_size);
    for(int i = 0; i< frame_size; i++)
        printfd("0x%x ", frame_buffer[i]);
    printfd("\n");
    ret = spi_send_data(spi_fd, frame_buffer, frame_size, recv_buffer, recv_size);
    if(ret < 0){
        return -1;
    }
    printf_debug("receive data buffer[%d]:", recv_size);
    for(int i = 0; i< recv_size; i++)
        printfd("0x%x ", recv_buffer[i]);
    printfd("\n");
    ret = prase_recv_data(cmd, recv_buffer, data_len,data);
    if(ret != 0)
        return -1;
    return 0;
}
#endif


ssize_t spi_rf_set_command(int ch, rf_spi_set_cmd_code cmd, void *data)
{
#if defined(SUPPORT_RF_SPI_TF713)
    return spi_rf_set_command_bits(ch, cmd, data);
#else
    return _spi_rf_set_command(cmd, data);
#endif
}

ssize_t spi_rf_get_command(int ch, rf_spi_get_cmd_code cmd, void *data, int len)
{
#if defined(SUPPORT_RF_SPI_TF713)
    return spi_rf_get_command_bits(ch, cmd, data, len);
#else
    return _spi_rf_get_command(cmd, data);
#endif
}
