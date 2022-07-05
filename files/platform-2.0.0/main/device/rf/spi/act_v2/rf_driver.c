

/*
* Copyright (C) 2013 - 2016  Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person
* obtaining a copy of this software and associated documentation
* files (the "Software"), to deal in the Software without restriction,
* including without limitation the rights to use, copy, modify, merge,
* publish, distribute, sublicense, and/or sell copies of the Software,
* and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or (b) that interact
* with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in this
* Software without prior written authorization from Xilinx.
*
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <math.h>
#include <linux/spi/spidev.h>
#include <stdbool.h>
#include "rf_driver.h"
#include "../../../../log/log.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#if defined(SUPPORT_PROJECT_AKT_4CH)
#define MAX_SPI_CH_NUM  4
#elif defined(SUPPORT_PROJECT_YF21025)
#define MAX_SPI_CH_NUM  1
#else
#define MAX_SPI_CH_NUM  0
#endif

int _fd[MAX_SPI_CH_NUM];

static const char *spi_dev_name[] = {
#if defined(SUPPORT_PROJECT_AKT_4CH)	
    "/dev/spidev1.0",
    "/dev/spidev1.1",
    "/dev/spidev1.2",
    "/dev/spidev1.3",
#elif defined(SUPPORT_PROJECT_YF21025)
    "/dev/spidev1.1",  //spi rf
    "/dev/spidev1.0",
#else
    NULL,
#endif
};

static uint8_t spi_checksum(uint8_t *buffer,uint8_t len){
    uint32_t check_sum = 0;
    uint8_t i;
    for(i=0;i <len ;i++){
        check_sum += buffer[i];
    }
    return check_sum&0xff;
}

static int  spi_dev_init(char *dev_name)
{
    uint8_t mode = SPI_CPOL|SPI_CPHA;  //3
    uint32_t speed = 1000000;
	int spidevfd = -1;
	
	if (dev_name != NULL) 
	{
	    spidevfd =  open(dev_name,O_RDWR);
	    if(spidevfd < 0){
	        return  -1;
	    }
	    if(ioctl(spidevfd, SPI_IOC_WR_MODE,&mode) <0){
	        return -1;   
	    }
	    if(ioctl(spidevfd,SPI_IOC_WR_MAX_SPEED_HZ, &speed)<0){
	        return -1;
	    }
	}
        
    return spidevfd;
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

static int spi_assemble_send_data(uint8_t ch, uint8_t *buffer, uint8_t cmd, void *data, size_t len)
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
    if (cmd != RF_CMD_CODE_TEMPERATURE) {
        *ptr++ = ch;
    }
    if(len != 0){
        memcpy(ptr, data, len);
        ptr += len;
    }
    if (cmd == RF_CMD_CODE_TEMPERATURE) {
        *ptr++ = spi_checksum(pchecksum, len+2);
    } else {
        *ptr++ = spi_checksum(pchecksum, len+3);
    }
    *ptr++ = SPI_DATA_END;
    return (ptr - buffer);/* RETURN: total frame byte count */
}


static ssize_t prase_recv_data(uint8_t cmd, uint8_t *recv_buffer, size_t data_len, void *data)
{
    uint8_t *ptr_data, *res_data, status;
    int status_len = 0;
    
    if (cmd == RF_CMD_CODE_TEMPERATURE) {
        ptr_data = res_data = recv_buffer+3;
    } else {
        ptr_data = res_data = recv_buffer+4;
    }
    if(cmd == RF_QUERY_CMD_CODE_FREQUENCY || cmd == RF_CMD_CODE_FREQUENCY){
        status_len = 2;
    }else{
        status_len = 1;
    }
    if(data_len < status_len){
        return -1;
    }

    ptr_data += data_len - status_len;
    status = *ptr_data;  //特殊（射频频率设置查询回复，状态码两个字节，Byte1：0-锁定，1-失锁 Byte2：0-成功 1-失败）

    if (status == 0 && data != NULL) {
        memcpy(data, res_data, data_len - status_len);
    }
    return status;
}

static int16_t show_temp(uint8_t ch, int16_t *buffer)
{
    int16_t  rf_temperature = 0;
    rf_temperature = htons(*buffer);
    printf("-->ch:%d, temperature=%d\n", ch, rf_temperature);
    return rf_temperature;
}

static int rf_get_temperature(uint8_t ch, int16_t *temperatue)
{
    if (ch >= MAX_SPI_CH_NUM) {
        return -1;
    }
    int ret = -1;
    uint8_t frame_buffer[64], recv_buffer[64];
    int frame_size = -1, recv_size = 0, data_len = 0;

    data_len = 3;
    recv_size = data_len + 5; //5 is frame header & tail length
    frame_size = spi_assemble_send_data(ch, frame_buffer, RF_CMD_CODE_TEMPERATURE, NULL, 0);
    
    ret = spi_send_data(_fd[ch], frame_buffer, frame_size, recv_buffer, recv_size);
    if (ret < 0) {
        return -1;
    }

    printf_debug("receive data buffer[%d]:", recv_size);
    for(int i = 0; i< recv_size; i++)
        printfd("0x%x ", recv_buffer[i]);
    printfd("\n");
    ret = prase_recv_data(RF_CMD_CODE_TEMPERATURE, recv_buffer, data_len, temperatue);
    if(ret != 0)
        return -1;
    return 0;
}

static int rf_get_mid_freq(uint8_t ch, uint8_t *freq)
{
    if (ch >= MAX_SPI_CH_NUM) {
        return -1;
    }
    int ret = -1;
    uint8_t frame_buffer[64], recv_buffer[64];
    int frame_size = -1, recv_size = 0, data_len = 0;

    data_len = 2;
    recv_size = data_len + 5 + 1; //5 is frame header & tail length
    frame_size = spi_assemble_send_data(ch, frame_buffer, RF_QUERY_CMD_CODE_MID_FREQ, NULL, 0);
    
    ret = spi_send_data(_fd[ch], frame_buffer, frame_size, recv_buffer, recv_size);
    if (ret < 0) {
        return -1;
    }

    printf_debug("receive data buffer[%d]:", recv_size);
    for(int i = 0; i< recv_size; i++)
        printfd("0x%x ", recv_buffer[i]);
    printfd("\n");
    ret = prase_recv_data(RF_QUERY_CMD_CODE_MID_FREQ, recv_buffer, data_len, freq);
    if(ret != 0)
        return -1;
    return 0;
}

static int _rf_get_mid_freq(uint8_t ch, uint64_t *freq)
{
    uint8_t freq_flag = 0;
    int ret = rf_get_mid_freq(ch, &freq_flag);
    if (ret == 0) {
        if (freq_flag == 1) {
            *freq = MHZ(70);
        } else if (freq_flag == 2) {
            *freq = MHZ(21.4);
        } else if (freq_flag == 3) {
            *freq = MHZ(140);
        } else {
            printf_warn("get unsupport rf mid freq data:%d\n", freq_flag);
            return -1;
        }
    }
    return ret;
}

static int rf_get_bw(uint8_t ch, uint8_t *bw)
{
    if (ch >= MAX_SPI_CH_NUM) {
        return -1;
    }
    int ret = -1;
    uint8_t frame_buffer[64], recv_buffer[64];
    int frame_size = -1, recv_size = 0, data_len = 0;

    data_len = 2;
    recv_size = data_len + 5 + 1; //5 is frame header & tail length
    frame_size = spi_assemble_send_data(ch, frame_buffer, RF_QUERY_CMD_CODE_BW, NULL, 0);
    
    ret = spi_send_data(_fd[ch], frame_buffer, frame_size, recv_buffer, recv_size);
    if (ret < 0) {
        return -1;
    }

    printf_debug("receive data buffer[%d]:", recv_size);
    for(int i = 0; i< recv_size; i++)
        printfd("0x%x ", recv_buffer[i]);
    printfd("\n");
    ret = prase_recv_data(RF_QUERY_CMD_CODE_BW, recv_buffer, data_len, bw);
    if(ret != 0)
        return -1;
    return 0;
}

static int _rf_get_bw(uint8_t ch, uint32_t *bw)
{
	uint8_t bw_tmp;
	int ret = rf_get_bw(ch, &bw_tmp);
	if (ret == 0) {
		if(bw_tmp == 5)
	        *bw = MHZ(20);
	    else if(bw_tmp == 4)
	        *bw = MHZ(10);
	    else if(bw_tmp == 3)
	        *bw = MHZ(5);
	    else if(bw_tmp == 2)
	        *bw = MHZ(2);
	    else if(bw_tmp == 1)
	        *bw = MHZ(1);
        else if(bw_tmp == 6)
	        *bw = MHZ(0.5);
        else if(bw_tmp == 7)
	        *bw = MHZ(40);
        else if(bw_tmp == 8)
	        *bw = MHZ(80);
	    else
	        return -1;
	}
	return ret;
}

static int rf_get_work_mode(uint8_t ch, uint8_t *mode)
{
    if (ch >= MAX_SPI_CH_NUM) {
        return -1;
    }
    int ret = -1;
    uint8_t frame_buffer[64], recv_buffer[64];
    int frame_size = -1, recv_size = 0, data_len = 0;

    data_len = 2;
    recv_size = data_len + 5 + 1; //5 is frame header & tail length
    frame_size = spi_assemble_send_data(ch, frame_buffer, RF_QUERY_CMD_CODE_MODE, NULL, 0);
    
    ret = spi_send_data(_fd[ch], frame_buffer, frame_size, recv_buffer, recv_size);
    if (ret < 0) {
        return -1;
    }

    printf_debug("receive data buffer[%d]:", recv_size);
    for(int i = 0; i< recv_size; i++)
        printfd("0x%x ", recv_buffer[i]);
    printfd("\n");
    ret = prase_recv_data(RF_QUERY_CMD_CODE_MODE, recv_buffer, data_len, mode);
    if(ret != 0)
        return -1;
    return 0;
}

static int _rf_get_work_mode(uint8_t ch, uint8_t *mode)
{
	uint8_t mode_tmp;
	int ret = rf_get_work_mode(ch, &mode_tmp);
	if (ret == 0) {
		*mode = mode_tmp - 1;
	}
	return ret;
}

static int rf_get_mgc_gain(uint8_t ch, uint8_t *gain)
{
    if (ch >= MAX_SPI_CH_NUM) {
        return -1;
    }
    int ret = -1;
    uint8_t frame_buffer[64], recv_buffer[64];
    int frame_size = -1, recv_size = 0, data_len = 0;

    data_len = 2;
    recv_size = data_len + 5 + 1; //5 is frame header & tail length
    frame_size = spi_assemble_send_data(ch, frame_buffer, RF_QUERY_CMD_CODE_MF_GAIN, NULL, 0);
    
    ret = spi_send_data(_fd[ch], frame_buffer, frame_size, recv_buffer, recv_size);
    if (ret < 0) {
        return -1;
    }

    printf_debug("receive data buffer[%d]:", recv_size);
    for(int i = 0; i< recv_size; i++)
        printfd("0x%x ", recv_buffer[i]);
    printfd("\n");
    ret = prase_recv_data(RF_QUERY_CMD_CODE_MF_GAIN, recv_buffer, data_len, gain);
    if(ret != 0)
        return -1;
    return 0;
}

static int rf_get_rf_gain(uint8_t ch, uint8_t *gain)
{
    if (ch >= MAX_SPI_CH_NUM) {
        return -1;
    }
    int ret = -1;
    uint8_t frame_buffer[64], recv_buffer[64];
    int frame_size = -1, recv_size = 0, data_len = 0;

    data_len = 2;
    recv_size = data_len + 5 + 1; //5 is frame header & tail length
    frame_size = spi_assemble_send_data(ch, frame_buffer, RF_QUERY_CMD_CODE_RF_GAIN, NULL, 0);
    
    ret = spi_send_data(_fd[ch], frame_buffer, frame_size, recv_buffer, recv_size);
    if (ret < 0) {
        return -1;
    }

    printf_debug("receive data buffer[%d]:", recv_size);
    for(int i = 0; i< recv_size; i++)
        printfd("0x%x ", recv_buffer[i]);
    printfd("\n");
    ret = prase_recv_data(RF_QUERY_CMD_CODE_RF_GAIN, recv_buffer, data_len, gain);
    if(ret != 0)
        return -1;
    return 0;
}

static int rf_get_frequency(uint8_t ch, uint64_t *freq)
{
    if (ch >= MAX_SPI_CH_NUM) {
        return -1;
    }
    int ret = -1;
    uint8_t frame_buffer[64], recv_buffer[64];
    int frame_size = -1, recv_size = 0, data_len = 0;

    data_len = 7;
    recv_size = data_len + 5 + 1; //5 is frame header & tail length, 1 is ch number
    frame_size = spi_assemble_send_data(ch, frame_buffer, RF_QUERY_CMD_CODE_FREQUENCY, NULL, 0);
    
    ret = spi_send_data(_fd[ch], frame_buffer, frame_size, recv_buffer, recv_size);
    if (ret < 0) {
        return -1;
    }

    printf_debug("receive data buffer[%d]:", recv_size);
    for(int i = 0; i< recv_size; i++)
        printfd("0x%x ", recv_buffer[i]);
    printfd("\n");
    ret = prase_recv_data(RF_QUERY_CMD_CODE_FREQUENCY, recv_buffer, data_len, freq);
    if(ret != 0)
        return -1;
    return 0;
}

static int _rf_get_frequency(uint8_t ch, uint64_t *freq)
{
	uint64_t freq_tmp;
	int ret = rf_get_frequency(ch, &freq_tmp);
	if (ret == 0) {
		*freq = (htobe64(freq_tmp) >> 24);
	}
	return ret;
}

static int rf_set_bw(uint8_t channel, uint8_t bw)
{
    if (channel >= MAX_SPI_CH_NUM) {
        return -1;
    }
    int ret = -1;
    uint8_t frame_buffer[64], recv_buffer[64];
    ssize_t frame_size = -1;

    uint8_t data = bw;
    size_t data_len = 1;
    frame_size = spi_assemble_send_data(channel, frame_buffer, RF_CMD_CODE_BW, &data, data_len);
    int recv_size = 2+5+1;
    ret = spi_send_data(_fd[channel], frame_buffer, frame_size, recv_buffer, recv_size);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

static int _rf_set_bw(uint8_t channel, uint32_t bw_hz)
{
    uint8_t mbw = 0;
#if 0
    if(bw_hz == MHZ(20))
        mbw = 5;
    else if(bw_hz == MHZ(10))
        mbw = 4;
    else if(bw_hz == MHZ(5))
        mbw = 3;
    else if(bw_hz == MHZ(2))
        mbw = 2;
    else if(bw_hz == MHZ(1))
        mbw = 1;
    else if(bw_hz == MHZ(0.5))
        mbw = 6;
    else if(bw_hz == MHZ(40))
        mbw = 7;
    else if(bw_hz == MHZ(80))
        mbw = 8;
    else
        mbw = 7;
#else
    if(bw_hz == MHZ(1))
        mbw = 0;
    else if(bw_hz == MHZ(2))
        mbw = 1;
    else if(bw_hz == MHZ(5))
        mbw = 2;
    else if(bw_hz == MHZ(10))
        mbw = 3;
    else if(bw_hz == MHZ(20))
        mbw = 4;
    else if(bw_hz == MHZ(0.5))
        mbw = 5;
    else if(bw_hz == MHZ(40))
        mbw = 6;
    else if(bw_hz == MHZ(80))
        mbw = 7;
    else if(bw_hz == MHZ(0.2))
        mbw = 8;
    else if(bw_hz == MHZ(70))
        mbw = 9;
    else if(bw_hz == MHZ(60))
        mbw = 10;
    else if(bw_hz == MHZ(200))
        mbw = 11;
    else {
        printf_warn("not supported rf bw %u hz\n", bw_hz);
        return -1;
    }
#endif
            
    return rf_set_bw(channel, mbw);
}


static int rf_set_mode(uint8_t channel, uint8_t mode)
{
    if (channel >= MAX_SPI_CH_NUM) {
        return -1;
    }
    int ret = -1;
    uint8_t frame_buffer[64],recv_buffer[64];
    ssize_t frame_size = -1;

    uint8_t data = mode;
    size_t data_len = 1;
    frame_size = spi_assemble_send_data(channel, frame_buffer, RF_CMD_CODE_MODE, &data, data_len);
    int recv_size = 2+5+1;
    ret = spi_send_data(_fd[channel], frame_buffer, frame_size, recv_buffer, recv_size);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

//底层: 低失真模式(0x01)     常规模式(0x02) 低噪声模式(0x03)
static int _rf_set_mode(uint8_t channel, uint8_t mode)
{
    uint8_t mode_bsp = mode + 1;
    return rf_set_mode(channel, mode_bsp);
}

static int rf_set_mid_freq(uint8_t channel, uint8_t freq)
{
    if (channel >= MAX_SPI_CH_NUM) {
        return -1;
    }
    int ret = -1;
    uint8_t frame_buffer[64],recv_buffer[64];
    ssize_t frame_size = -1;

    uint8_t data = freq;
    size_t data_len = 1;
    frame_size = spi_assemble_send_data(channel, frame_buffer, RF_CMD_CODE_MID_FREQ, &data, data_len);
    int recv_size = 2+5+1;
    ret = spi_send_data(_fd[channel], frame_buffer, frame_size, recv_buffer, recv_size);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

//射频中频频率设置
static int _rf_set_mid_freq(uint8_t channel, uint64_t freq)
{
    uint8_t freq_flag = 0;
    if (freq == MHZ(70)) {
        freq_flag = 1;
    } else if (freq == MHZ(21.4)) {
        freq_flag = 2;
    } else if (freq == MHZ(140)) {
        freq_flag = 3;
    }else {
        printf_warn("unsupport rf mid freq %lu hz\n", freq);
        return -1;
    }
    return rf_set_mid_freq(channel, freq_flag);
}

static int rf_set_mf_gain(uint8_t channel, uint8_t gain)
{
    if (channel >= MAX_SPI_CH_NUM) {
        return -1;
    }
    int ret = -1;
    uint8_t frame_buffer[64],recv_buffer[64];
    ssize_t frame_size = -1;

    uint8_t data = gain;
    size_t data_len = 1;
    frame_size = spi_assemble_send_data(channel, frame_buffer, RF_CMD_CODE_MF_GAIN, &data, data_len);
    int recv_size = 2+5+1;
    ret = spi_send_data(_fd[channel], frame_buffer, frame_size, recv_buffer, recv_size);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

static int rf_set_rf_gain(uint8_t channel, uint8_t gain)
{
    if (channel >= MAX_SPI_CH_NUM) {
        return -1;
    }
    int ret = -1;
    uint8_t frame_buffer[64],recv_buffer[64];
    ssize_t frame_size = -1;

    uint8_t data = gain;
    size_t data_len = 1;
    frame_size = spi_assemble_send_data(channel, frame_buffer, RF_CMD_CODE_RF_GAIN, &data, data_len);
    int recv_size = 2+5+1;
    ret = spi_send_data(_fd[channel], frame_buffer, frame_size, recv_buffer, recv_size);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

static int rf_set_frequency(uint8_t channel, uint64_t freq)
{
    if (channel >= MAX_SPI_CH_NUM) {
        return -1;
    }
    int ret = -1;
    uint8_t frame_buffer[64], recv_buffer[64];
    ssize_t frame_size = -1;
    int recv_size;
    
    uint64_t data = freq;
    size_t data_len = 5;
    recv_size = 7 + 5 + 1;
    frame_size = spi_assemble_send_data(channel, frame_buffer, RF_CMD_CODE_FREQUENCY, &data, data_len);
    
    ret = spi_send_data(_fd[channel], frame_buffer, frame_size, recv_buffer, recv_size);
    if (ret < 0) {
        return -1;
    }
    return 0;
}


static int _rf_set_frequency(uint8_t channel, uint64_t freq_hz)
{
    int ret = -1;
    uint64_t host_freq = htobe64(freq_hz) >> 24;  //act 4ch:单位hz
    uint64_t recv_freq = 0;
    int i = 0;
    for (i = 0; i < 3; i++){
        ret = rf_set_frequency(channel, host_freq);
        if (ret < 0){
            printf_warn("set rf freq fail\n");
            usleep(100);
            continue;
        }
        usleep(300);
        recv_freq = 0;
        rf_get_frequency(channel, &recv_freq);
        recv_freq = (htobe64(recv_freq) >> 24);
        printf_debug("set freq:%lu, recv freq:%lu\n", freq_hz, recv_freq);
        if (recv_freq == freq_hz){
            ret = 0;
            break;
        }
        printf_note("set rf freq, try again index:%d\n", i);
    }
    if (i>=3)
        ret = -1;
    return ret;
}


static int check_rf_handshake(uint8_t ch)
{
    return 0;
}

static bool check_rf_status(uint8_t ch)
{
    int ret = 0;
    int16_t temperature;
    ret = rf_get_temperature(ch, &temperature);
    return (ret == 0 ? true : false);
}

static int _rf_init(void)
{
    for (int i = 0; i < ARRAY_SIZE(spi_dev_name); i++)
    {
        if (i >= MAX_SPI_CH_NUM) {
            break;
        }
        _fd[i] = spi_dev_init(spi_dev_name[i]);
        if (_fd[i] <= 0) {
            printf_err("init spi dev %s fail!\n", spi_dev_name[i]);
            continue;
        }
        printf_note("init spi dev %s ok!, fd:%d\n", spi_dev_name[i], _fd[i]);
    }
    
    return 0;
}

static const struct rf_ops _rf_ops = {
    .init = _rf_init,
    .set_mid_freq   = _rf_set_frequency,
    .set_rf_mid_freq = _rf_set_mid_freq,
    .set_bindwidth  = _rf_set_bw,
    .set_work_mode  = _rf_set_mode,
    .set_mgc_gain   = rf_set_mf_gain,
    .set_rf_gain    = rf_set_rf_gain,

	.get_mid_freq = _rf_get_frequency,
    .get_status     = check_rf_status,
    .get_temperature = rf_get_temperature,
    .get_work_mode  = _rf_get_work_mode,
    .get_mgc_gain   = rf_get_mgc_gain,
    .get_rf_gain    = rf_get_rf_gain,
    .get_rf_mid_freq = _rf_get_mid_freq,
    .get_bindwidth = _rf_get_bw,
};


struct rf_ctx * rf_create_context(void)
{
    int ret = -1;
    struct rf_ctx *ctx = calloc(1, sizeof(*ctx));
     if (!ctx)
        return NULL;
     
    ctx->ops = &_rf_ops;
    ret = _rf_init();
    
    if(ret != 0)
        return NULL;
    return ctx;
}
