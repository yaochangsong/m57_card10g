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
*  Rev 1.0   9 June. 2020   yaochangsong
*  Initial revision.
******************************************************************************/
#include "config.h"
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <getopt.h>
#include "clk_adc.h"
#include "clk_adc_spi.h"
#include "../rf/spi/rf_spi.h"
#include "clk_adc_conf.h"
extern bool config_get_is_internal_clock(void);



static struct rf_spi_node_info spi_node[] ={
    /* name path               function code    pin  spifd      pinfd  info */
#ifdef PETALINUX_VER_2019_1
    /* petalinux2019.1 */
    //{"/dev/spidev2.0",     SPI_FUNC_RF,     8,   -1,      -1,  "spi rf"},
    {"/dev/spidev1.0",     SPI_FUNC_CLOCK,  8,   -1,      -1,  "spi clock 7044 chip"},
    {"/dev/spidev1.1",     SPI_FUNC_AD,     8,   -1,      -1,  "spi ad 9690 chip"},
#else
    /* petalinux2016.4 */
    //{"/dev/spidev32765.0",     SPI_FUNC_RF,     8,   -1,      -1,  "spi rf"},
    {"/dev/spidev32766.0",     SPI_FUNC_CLOCK,  8,   -1,      -1,  "spi clock 7044 chip"},
    {"/dev/spidev32766.1",     SPI_FUNC_AD,     8,   -1,      -1,  "spi ad 9690 chip"},
#endif
    {NULL,                     SPI_FUNC_NULL,    -1,   -1,      -1,  NULL},
};


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


static int ca_spi_clock_init_before(void)
{
    uint8_t send_buf[8];
    int spi_fd, ret =0;
    int found = 0, index = 0;
    uint32_t array_len = 3;
    bool internal_clock = config_get_is_internal_clock();
    for(int i = 0; i< ARRAY_SIZE(spi_node); i++){
        printf_note("spi_node: %d, func_code:%d\n", i, spi_node[i].func_code);
        if(spi_node[i].func_code == SPI_FUNC_CLOCK){
            spi_fd = spi_node[i].fd;
            index = i;
            found = 1;
        }
    }
    if(found == 0 || spi_fd == -1){
        printf_err("SPI CLOCK init Faild!found = %d\n", found);
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

static int ca_spi_clock_init_after(void)
{
    uint8_t send_buf[8];
    int spi_fd, ret =0;
    int found = 0;
	//int	__attribute__((unused))index = 0;
    uint32_t array_len = 3;
    
    for(int i = 0; i< ARRAY_SIZE(spi_node); i++){
        if(spi_node[i].func_code == SPI_FUNC_CLOCK){
            spi_fd = spi_node[i].fd;
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

static int ca_spi_clock_check(void)
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


static int ca_spi_adc_init(void)
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


static int ca_spi_init(void)
{
    int ret = -1;
    uint8_t status = 0;

    printf_note("clock adc spi init...\n");
    ret = ca_spi_clock_init_before();
    /* 等待时钟稳定 */
    usleep(10000);
    ret = ca_spi_adc_init();
    status = ((ret >= 0) ? 0 : 1);
    printf_note("AD status:%s\n", status == 0 ? "OK" : "Faild");
    config_save_cache(EX_STATUS_CMD, EX_AD_STATUS, -1, &status);
    /* 复位时钟 */
    ret = ca_spi_clock_init_after();
    usleep(10000);
    ret = ca_spi_clock_check();
    status = ((ret == 0) ? 0 : 1);
    printf_note("Clock status:%s\n", status == 0 ? "OK" : "Faild");
    config_save_cache(EX_STATUS_CMD, EX_CLK_STATUS, -1, &status);
	return 0;
}

static bool spi_has_inited = false;
static int _spi_init(void)
{
    struct rf_spi_node_info *ptr = spi_node;
    uint8_t mode = 0;
    uint32_t speed = 4000000;

    if(spi_has_inited == true)
        return 0;
    for(int i = 0; i< ARRAY_SIZE(spi_node); i++){
        if(ptr[i].name != NULL){
            ptr[i].fd =  open(ptr[i].name,O_RDWR);
            if(ioctl(ptr[i].fd, SPI_IOC_WR_MODE,&mode) <0){
                printf_err("[%s]fd:%d,can't set spi mode\n",ptr[i].name, ptr[i].fd);
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
    spi_has_inited = true;
    return 0;
}


static int ca_spi_close(void)
{
    return -1;
}


static const struct clock_adc_ops ca_ctx_ops = {
    .init = ca_spi_init,
    .close = ca_spi_close,
};


struct clock_adc_ctx * clock_adc_spi_cxt(void)
{
    int ret = -ENOMEM;
    
     _spi_init();
    struct clock_adc_ctx *ctx = calloc(1, sizeof(*ctx));
    if (!ctx)
        goto err_set_errno;
    
    ctx->ops = &ca_ctx_ops;
    
err_set_errno:
    errno = -ret;
    return ctx;

}

