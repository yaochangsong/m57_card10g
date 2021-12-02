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

static const char *const vendor_code_str[] = {
    [0] = "57#",
    [1] = "chongqing huiling",
    [2]  = "Electrinic University",
    [3] = "chengdu MircoBand",
    [4] = "chengdu Flex"    , 
    [5] = "chengdu xinren", 
    [6] = "chengdu zhengxin", 
    [7] = "chengdu zhongya", 
    [8] = "Telecom 5#", 
    [9] = "chengdu xinghang", 
    [10] = "Tongfang Electronic",
};

static void _print_array(uint8_t *ptr, ssize_t len)
{
    if(ptr == NULL || len <= 0)
        return;
    
    for(int i = 0; i< len; i++){
        printfi("%02x ", *ptr++);
    }
    printfi("\n");
}


static uint8_t spi_checksum(uint8_t *buffer,uint8_t len){
    uint32_t check_sum = 0;
    uint8_t i;
    for(i=0;i <len ;i++){
        check_sum += buffer[i];
    }
    return check_sum&0xff;
}

static uint8_t _spi_checksum(uint8_t *buffer,uint8_t len){
    uint8_t check_sum = 0;
    uint8_t i, *ptr = buffer;
    
    check_sum = *ptr++;
    for(i = 1; i < 4+len; i++){
        check_sum ^= *ptr++;
    }
    return check_sum;
}

static int  spi_dev_init(char *dev_name)
{
    uint8_t mode = 0;  //0
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

    if(spi_fd <= 0)
        return -1;
    memset(xfer, 0, sizeof(xfer));
    if((recv_len > 0) && (recv_buffer != NULL)){
        memset(recv_buffer, 0, recv_len);
    }
#if 0
    printf_note("send frame buffer[%ld]:", send_len);
    for(int i = 0; i< send_len; i++)
        printf("0x%x ", send_buffer[i]);
    printf("\n");
#endif
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

                                    
static int _spi_assemble_send_data(uint8_t *buffer, uint8_t mcode, uint8_t addr, uint8_t ch, uint8_t icode, 
                                            uint8_t ex_icode, uint8_t *pdata, size_t data_len)
{
    uint8_t *ptr = buffer;
    uint8_t *pcheck = NULL;
    if(ptr == NULL)
        return -1;
    
    *ptr++ = mcode;
    pcheck = ptr;
    *ptr++ = ((addr << 4) & 0xf0) | ch;
    *ptr++ = icode;
    *ptr++ = ex_icode;
    *ptr++ = data_len;
    if(pdata){
        memcpy(ptr, pdata, data_len);
    }
    ptr += data_len;
    *ptr++ = _spi_checksum(pcheck, data_len);

    return (ptr - buffer);/* RETURN: total frame byte count */
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


static ssize_t prase_recv_data(uint8_t cmd, uint8_t *recv_buffer, size_t data_len, void *data)
{
    uint8_t *ptr_data, status;
    int status_len = 0;
    ptr_data = recv_buffer+3;
    if(cmd == RF_QUERY_CMD_CODE_FREQUENCY || cmd == RF_CMD_CODE_FREQUENCY){
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
    status = *ptr_data;  //特殊（射频频率设置查询回复，状态码两个字节，Byte1：0-锁定，1-失锁 Byte2：0-成功 1-失败）
    printf_debug("status=%d\n", status);
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
    frame_size = spi_assemble_send_data(frame_buffer, RF_CMD_CODE_TEMPERATURE, NULL, 0);
    
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
    recv_size = data_len + 5; //5 is frame header & tail length
    frame_size = spi_assemble_send_data(frame_buffer, RF_QUERY_CMD_CODE_MID_FREQ, NULL, 0);
    
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
    recv_size = data_len + 5; //5 is frame header & tail length
    frame_size = spi_assemble_send_data(frame_buffer, RF_QUERY_CMD_CODE_BW, NULL, 0);
    
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
    recv_size = data_len + 5; //5 is frame header & tail length
    frame_size = spi_assemble_send_data(frame_buffer, RF_QUERY_CMD_CODE_MODE, NULL, 0);
    
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
    recv_size = data_len + 5; //5 is frame header & tail length
    frame_size = spi_assemble_send_data(frame_buffer, RF_QUERY_CMD_CODE_MF_GAIN, NULL, 0);
    
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
    recv_size = data_len + 5; //5 is frame header & tail length
    frame_size = spi_assemble_send_data(frame_buffer, RF_QUERY_CMD_CODE_RF_GAIN, NULL, 0);
    
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
    recv_size = data_len + 5; //5 is frame header & tail length
    frame_size = spi_assemble_send_data(frame_buffer, RF_QUERY_CMD_CODE_FREQUENCY, NULL, 0);
    
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
    uint8_t frame_buffer[64];
    ssize_t frame_size = -1;

    uint8_t data = bw;
    size_t data_len = 1;
    frame_size = spi_assemble_send_data(frame_buffer, RF_CMD_CODE_BW, &data, data_len);
    
    ret = spi_send_data(_fd[channel], frame_buffer, frame_size, NULL, 0);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

static int _rf_set_bw(uint8_t channel, uint32_t bw_hz)
{
    uint8_t mbw = 0;
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
            
    return rf_set_bw(channel, mbw);
}


static int rf_set_mode(uint8_t channel, uint8_t mode)
{
    if (channel >= MAX_SPI_CH_NUM) {
        return -1;
    }
    int ret = -1;
    uint8_t frame_buffer[64];
    ssize_t frame_size = -1;

    uint8_t data = mode;
    size_t data_len = 1;
    frame_size = spi_assemble_send_data(frame_buffer, RF_CMD_CODE_MODE, &data, data_len);
    
    ret = spi_send_data(_fd[channel], frame_buffer, frame_size, NULL, 0);
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
    uint8_t frame_buffer[64];
    ssize_t frame_size = -1;

    uint8_t data = freq;
    size_t data_len = 1;
    frame_size = spi_assemble_send_data(frame_buffer, RF_CMD_CODE_MID_FREQ, &data, data_len);
    
    ret = spi_send_data(_fd[channel], frame_buffer, frame_size, NULL, 0);
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
    uint8_t frame_buffer[64];
    ssize_t frame_size = -1;

    uint8_t data = gain;
    size_t data_len = 1;
    frame_size = spi_assemble_send_data(frame_buffer, RF_CMD_CODE_MF_GAIN, &data, data_len);
    
    ret = spi_send_data(_fd[channel], frame_buffer, frame_size, NULL, 0);
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
    uint8_t frame_buffer[64];
    ssize_t frame_size = -1;

    uint8_t data = gain;
    size_t data_len = 1;
    frame_size = spi_assemble_send_data(frame_buffer, RF_CMD_CODE_RF_GAIN, &data, data_len);
    
    ret = spi_send_data(_fd[channel], frame_buffer, frame_size, NULL, 0);
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
    recv_size = 7 + 5;
    frame_size = spi_assemble_send_data(frame_buffer, RF_CMD_CODE_FREQUENCY, &data, data_len);
    
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

    for (int i = 0; i < 3; i++){
        ret = rf_set_frequency(channel, host_freq);
        if (ret < 0)
{
            printf_warn("-----------------set rf freq fail\n");
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
    return ret;
}

static int _refill_recv_data_test(uint8_t *buffer)
{
    uint8_t *ptr = buffer;
    //00 73 a1 a1 05 00 d5 02 b5 a9 00 00
    *ptr++ = 0x00;
    *ptr++ = 0x00;
    *ptr++ = 0xa9;
    *ptr++ = 0xb5;
    *ptr++ = 0x02;
    *ptr++ = 0xd5;
    *ptr++ = 0x00;
    *ptr++ = 0x05;
    *ptr++ = 0xa1;
    *ptr++ = 0xa1;
    *ptr++ = 0x73;
    *ptr++ = 0x00;
    return (ptr - buffer);
}

static int _get_rf_identify_info(uint8_t ch, uint8_t addr, void *info)
{
    uint8_t frame_buffer[128], recv_buffer[128];
    int ret, frame_size,recv_size =12;
    struct rf_identity_info_t *_info;
    
    frame_size = _spi_assemble_send_data(frame_buffer, MODE_CODE_RF, addr, ch, INS_CODE_IDENTITY, 
                            EX_ICMD_NO_RESP, NULL, 0);
    printf_info("send: ");
    _print_array(frame_buffer, frame_size);
    ret = spi_send_data(_fd[0], frame_buffer, frame_size, recv_buffer, recv_size);
    if (ret < 0) {
        return -1;
    }
    //recv_size = _refill_recv_data_test(recv_buffer);
    printf_info("recv: ");
    _print_array(recv_buffer, recv_size);
    _info = (struct rf_identity_info_t *)recv_buffer;

    char buffer[128] = {0};
    printfi("=========Rf Info=============================\n");
    printfi("addr_ch: 0x%x\n", _info->addr_ch);
    printfi("serial_num: 0x%x\n", _info->serial_num);
    printfi("type_code: 0x%x\n", _info->type_code);
    printfi("vender: 0x%x(%s)\n", _info->vender,vendor_code_str[_info->vender]);
    
    RF_MAXCLK_TO_STR(buffer, _info->max_clk_mhz);
    printfi("max_clk_mhz: 0x%x(%s)\n", _info->max_clk_mhz,buffer);
    
    RF_FW_VERSION_TO_STR(buffer, _info->fw_version);
    printfi("fw_version: 0x%x(%s)\n", _info->fw_version,buffer);
    
    RF_FATHER_VERSION_TO_STR(buffer, _info->father_version);
    printfi("father_version: 0x%x(%s)\n", _info->father_version, buffer);
    
    RF_PROTOCOL_VERSION_TO_STR(buffer, _info->protocol_version);
    printfi("protocol_version: 0x%x(%s)\n", _info->protocol_version, buffer);
    printfi("==============================================\n");
    if(info){
        memcpy(info, recv_buffer, recv_size);
    }
    
    return 0;
}

static int check_rf_handshake(uint8_t ch)
{
    return 0;
}

static bool check_rf_status(uint8_t ch)
{
    int ret = 0;
    uint8_t  info[128];
    ret = _get_rf_identify_info(ch, 0, info);
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
    .get_rf_identify_info = _get_rf_identify_info,
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

