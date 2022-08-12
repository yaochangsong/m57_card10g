

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
#include <stdint.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <math.h>
#include <linux/spi/spidev.h>
#include "rf713-test.h"

int  spi_dev_init(char *dev_name)
{
    uint8_t mode = 0;
    uint32_t speed = 1000000;
	int spidevfd;
	
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

int send_packet(int fd, uint8_t *buf,uint32_t buf_bits_len, uint8_t *recv_buf, uint8_t recv_len) 
{
    int ret,i;
    struct spi_ioc_transfer xfer[3];
    char resp_buf[100];
	uint8_t mode = 0;
	
    printf("spi send data:");
    for(i = 0; i < buf_bits_len / 8 + (buf_bits_len % 8 > 0 ? 1 : 0); i++)
    {
        printf("0x%02x ", buf[i]);
    }
    printf("\n");

    memset(xfer, 0, sizeof(xfer));

    mode =0;
    ioctl(fd, SPI_IOC_WR_MODE, &mode);    
    
    xfer[0].tx_buf = (unsigned long) buf;
    xfer[0].rx_buf = (unsigned long) resp_buf;
    xfer[0].len = buf_bits_len/8;
    xfer[0].bits_per_word = 8;

    if(buf_bits_len % 8 > 0)
    {
        xfer[1].tx_buf = (unsigned long) (buf + buf_bits_len / 8);
        xfer[1].len = 1;
        xfer[1].bits_per_word = buf_bits_len % 8;
        ret = ioctl(fd, SPI_IOC_MESSAGE(2), xfer);
    }
    else
    {
        ret = ioctl(fd, SPI_IOC_MESSAGE(1), &xfer[0]);
    }

    if(recv_len > 0)
    {
        xfer[2].rx_buf = (unsigned long) recv_buf;
        xfer[2].len = recv_len;

		mode |= SPI_CS_HIGH;
		ioctl(fd, SPI_IOC_WR_MODE, &mode);

        ret = ioctl(fd, SPI_IOC_MESSAGE(1), &xfer[2]);
        
        printf("recv data:");
        for(i=0;i<recv_len;i++)
        {
            printf("0x%02x ",recv_buf[i]);
        }
        printf("\n");
        mode = 0;
        ioctl(fd, SPI_IOC_WR_MODE, &mode);
    }
    else
    {
        printf("spi no response data\n");
        return 0;
    }
    
    if (ret < 0) 
    {
        printf("spi data happen err\n");
        perror("spi error:");
        return -1;
    }
    
    printf("send response:");
    for(i = 0; i < buf_bits_len/8; i++)
    {
        printf("0x%02x ",resp_buf[i]);
    }

	printf("\n");

    return 0;
}

void show_temp(uint8_t ch, int16_t *buffer)
{
	int16_t  rf_temperature = 0;
	int16_t sign_bit;
	
	rf_temperature = ntohs(*buffer);

	sign_bit=(rf_temperature&0x2000)>>13;    //sign_bit=0正数， sign_bit=1负数
	if(sign_bit == 0){
		rf_temperature = rf_temperature/32.0;
	}
	else if(sign_bit == 1){
		rf_temperature= (rf_temperature-16384)/32.0;
	}
	printf("-->ch:%d, temperature=%d\n", ch, rf_temperature);
}

void rf_get_temperature(int fd, uint8_t ch)
{
    uint8_t send_buf[10] = {0};
    uint8_t recv_buf[10] = {0}; 
    uint8_t bits_len,recv_len;
	//printf("func:%s\n", __FUNCTION__);

    recv_len = 2;
    bits_len = 13;

    send_buf[0] = ch << 3;
    send_buf[0] |= (RF_CHANNEL_CS << 1) & 0x07;
    send_buf[0] |= (RF_CMD_CODE_TEMPERATURE >> 5) & 0x1;
    send_buf[1] = RF_CMD_CODE_TEMPERATURE;
	
    if(send_packet(fd,send_buf,bits_len, recv_buf, recv_len))
    	printf("spi send packet failed.\n");
	
	show_temp(ch, (int16_t *)recv_buf);
}

void show_para(uint8_t ch, uint8_t *buffer)
{
	uint8_t *p_str = NULL;
	uint8_t mode, mfgain, rfgain;
	
	p_str = buffer;
	
	mode = *(p_str+4)&0x07;
	mfgain = ((*(p_str+4)&0xf8)>>3)|((*(p_str+3)&0x01)<<5);
	rfgain = ((*(p_str+3)&0x7e)>>1);
	printf("-->read reg: ch=%d, mode=%d, mfgain=%d, rfgain=%d\n", ch, mode, mfgain, rfgain);
}


void rf_get_para(int fd, uint8_t ch)
{
    uint8_t send_buf[10] = {0};
    uint8_t recv_buf[10] = {0}; 
    uint8_t bits_len,recv_len;
	//printf("func:%s\n", __FUNCTION__);

    recv_len = 6;
    bits_len = 13;

    send_buf[0] = ch << 3;
    send_buf[0] |= (RF_CHANNEL_CS << 1) & 0x07;
    send_buf[0] |= (RF_CMD_CODE_WORK_PARA >> 5) & 0x1;
    send_buf[1] = RF_CMD_CODE_WORK_PARA;
	
    if(send_packet(fd,send_buf,bits_len, recv_buf, recv_len))
    	printf("spi send packet failed.\n");
	show_para(ch, recv_buf);
}

void rf_set_bw(int fd, uint8_t channel, uint8_t bw)
{
    uint8_t send_buf[10] = {0};
    uint8_t bits_len;
	uint8_t flag = 0x15;
	uint8_t check = 0;
	
	printf("func:%s\n", __FUNCTION__);
    bits_len = 16;
    
    send_buf[0] = channel << 3;
    send_buf[0] |= (RF_CHANNEL_CS << 1) & 0x07;
    send_buf[0] |= (bw >> 2) & 0x1;
    send_buf[1] = (bw << 6) & 0xC0;
    send_buf[1] |= RF_CMD_CODE_BW & 0x03F;
	
    if(send_packet(fd,send_buf,bits_len, NULL, 0))
    	printf("spi send packet failed.\n");
    
}

void rf_set_mode(int fd, uint8_t channel, uint8_t mode)
{
    uint8_t send_buf[10] = {0};
    uint8_t bits_len;
	uint8_t flag = 0x15;
	uint8_t check = 0;
	
	printf("func:%s\n", __FUNCTION__);
    bits_len = 16;
    
    send_buf[0] = channel << 3;
    send_buf[0] |= (RF_CHANNEL_CS << 1) & 0x07;
    send_buf[0] |= (mode >> 2) & 0x1;
    send_buf[1] = (mode << 6) & 0xC0;
    send_buf[1] |= RF_CMD_CODE_MODE & 0x03F;
	
    if(send_packet(fd,send_buf,bits_len, NULL, 0))
    	printf("spi send packet failed.\n");
    
}

void rf_set_mf_gain(int fd, uint8_t channel, uint8_t gain)
{
    uint8_t send_buf[10] = {0};
    uint8_t bits_len;
	uint8_t flag = 0x15;
	uint8_t check = 0;
	
	printf("func:%s\n", __FUNCTION__);
    bits_len = 19;
    
    send_buf[0] = channel << 3;
    send_buf[0] |= (RF_CHANNEL_CS << 1) & 0x07;
    send_buf[0] |= (gain >> 5) & 0x1;
    send_buf[1] = (gain << 3) & 0xF8;
    send_buf[1] |= (RF_CMD_CODE_MF_GAIN >> 3) & 0x07;
    send_buf[2] = RF_CMD_CODE_MF_GAIN;
	
    if(send_packet(fd,send_buf,bits_len, NULL, 0))
    	printf("spi send packet failed.\n");
    
}

void rf_set_rf_gain(int fd, uint8_t channel, uint8_t gain)
{
    uint8_t send_buf[10] = {0};
    uint8_t bits_len;
	uint8_t flag = 0x15;
	uint8_t check = 0;
	
	printf("func:%s\n", __FUNCTION__);
    bits_len = 19;
    
    send_buf[0] = channel << 3;
    send_buf[0] |= (RF_CHANNEL_CS << 1) & 0x07;
    send_buf[0] |= (gain >> 5) & 0x1;
    send_buf[1] = (gain << 3) & 0xF8;
    send_buf[1] |= (RF_CMD_CODE_RF_GAIN >> 3) & 0x07;
    send_buf[2] = RF_CMD_CODE_RF_GAIN;

    if(send_packet(fd,send_buf,bits_len, NULL, 0))
    	perror("spi send packet failed.\n");
    
}

void rf_set_frequency(int fd, uint8_t channel, uint32_t freq)
{
    uint8_t send_buf[10] = {0};
    uint8_t bits_len;
	uint8_t flag = 0x15;
	uint8_t check = 0;
	
	printf("func:%s\n", __FUNCTION__);
    bits_len = 38;
    
    send_buf[0] = channel << 3;
    send_buf[0] |= (RF_CHANNEL_CS << 1) & 0x07;
    send_buf[0] |= (freq >> 24) & 0x1;
    send_buf[1] = (freq >> 16) & 0xFF;
    send_buf[2] = (freq >> 8) & 0xFF;
    send_buf[3] = freq & 0xFF;
    send_buf[4] = RF_CMD_CODE_FREQUENCY;
	
    if(send_packet(fd,send_buf,bits_len, NULL, 0))
    	printf("spi send packet failed.\n");
    
}

void check_rf_handshake(int fd, uint8_t ch)
{
    uint8_t send_buf[10] = {0};
    uint8_t recv_buf[10] = {0}; 
    uint8_t bits_len,recv_len;

	printf("func:%s\n", __FUNCTION__);
    recv_len = 1;
    bits_len = 13;

    send_buf[0] = ch << 3;
    send_buf[0] |= (RF_CHANNEL_CS << 1) & 0x07;
    send_buf[0] |= (RF_CMD_CODE_HANDSHAKE >> 5) & 0x1;
    send_buf[1] = RF_CMD_CODE_HANDSHAKE;
	
    if(send_packet(fd,send_buf,bits_len, recv_buf, recv_len))
    	printf("spi send packet failed.\n");
    
}


static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [option]\n"
	"		   -c channel [1~4]\n"
	"		   -m mode [0,1,2]\n"
	"		   -s rf gain [0,1,...30]\n "
	"		   -z mf gain [0,1,...30]\n "
	"		   \n", prog);
	exit(1);
}


int main(int argc, char **argv)
{ 
	int spidevfd = 0;
	uint8_t ch = 0, mode = 0, rf_gain = 0, mf_gain = 0;
    uint32_t freq = 0, bw = 0;

	spidevfd = spi_dev_init(SPI_DEV_NAME);
	if(spidevfd <= 0)
		printf("spi dev init failed.\n");


	
	int opt;
	while ((opt = getopt(argc, argv, "c:m:s:z:f:b:")) != -1){
		switch (opt){
			case 'c':
				ch = atoi(optarg);
			case 'm':
				mode = atoi(optarg);
				break;
			case 's':
				rf_gain = atoi(optarg);
				break;
			case 'z':
				mf_gain = atoi(optarg);
				break;
		    case 'f':
				freq = atoi(optarg);
				break;
			case 'b':
				bw = atoi(optarg);
				break;
			default: /* '?' */
				usage(argv[0]);
		}
	}

	printf("====>input ch= %d, mode= %d, rf_gain= %d, mf_gain = %d, freq = %u\n", ch, mode, rf_gain, mf_gain, freq);
	check_rf_handshake(spidevfd, ch);
	printf("====>get temperature:\n");
	rf_get_temperature(spidevfd, ch);
	printf("====>set mode:\n");
	rf_set_mode(spidevfd, ch, mode);
	printf("====>set rf gain:\n");
	rf_set_rf_gain(spidevfd, ch, rf_gain);
	printf("====>set mf gain:\n");
	rf_set_mf_gain(spidevfd, ch, mf_gain);
	printf("====>set freq: %ukhz\n", freq);
	rf_set_frequency(spidevfd, ch, freq);
	printf("====>set bw: %uhz\n", freq);
	rf_set_bw(spidevfd, ch, bw);
	printf("====>get para:\n");
	rf_get_para(spidevfd, ch);

    return 0;
}

