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
*  Rev 1.0   10 March 2019   bob yaochangsong
*  Initial revision.
******************************************************************************/
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <getopt.h>
#include "config.h"
#include "_reg.h"

void *memmap(int fd_dev, void *phr_addr, int length)
{
    void *base_addr = NULL;

    base_addr = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED, fd_dev, phr_addr);
 
    if (base_addr == NULL)
    {
        printf("mmap failed, NULL pointer!\n");
        return NULL;
    }

    return base_addr;
}

#define SYSTEM_CONFG_4K_LENGTH 0x1000


static int fpga_memmap(int fd_dev, FPGA_CONFIG_REG *fpga_reg)
{
    void *tmp_addr = NULL;
    int i = 0;

    fpga_reg->system = memmap(fd_dev, FPGA_SYSETM_BASE, SYSTEM_CONFG_4K_LENGTH); 
    if (!fpga_reg->system)
    {
        printf("mmap failed, NULL pointer!\n");
        return -1;
    }
    printf_debug("virtual address:%p, physical address:0x%x\n", fpga_reg->system, FPGA_SYSETM_BASE);

    tmp_addr = (uint8_t *)fpga_reg->system +CONFG_REG_LEN;
    for(i = 0; i < MAX_RF_NUM; i++){
        fpga_reg->rfReg[i] = (uint8_t *)tmp_addr +i*CONFG_REG_LEN;
        if (!fpga_reg->rfReg[i])
        {
            printf("mmap failed, NULL pointer!\n");
            return -1;
        }
        printf_debug("rf: %d virtual address:%p, physical address:0x%x\n", i, fpga_reg->rfReg[i], FPGA_SYSETM_BASE + (i+1)*CONFG_REG_LEN);
    }

    fpga_reg->audioReg = (uint8_t *)fpga_reg->system + CONFG_AUDIO_OFFSET;
    if (!fpga_reg->audioReg)
    {
        printf("mmap failed, NULL pointer!\n");
        return -1;
    }
    printf_debug("virtual address:%p, physical address:0x%x\n", fpga_reg->audioReg, FPGA_AUDIO_BASE);

    fpga_reg->adcReg = memmap(fd_dev, FPGA_ADC_BASE, SYSTEM_CONFG_REG_LENGTH); 
    if (!fpga_reg->adcReg)
    {
        printf("mmap failed, NULL pointer!\n");
        return -1;
    }
    printf_debug("virtual address:%p, physical address:0x%x\n", fpga_reg->adcReg, FPGA_ADC_BASE);

    fpga_reg->signal = memmap(fd_dev, FPGA_SIGNAL_BASE, SYSTEM_CONFG_REG_LENGTH); 
    if (!fpga_reg->signal)
    {
        printf("mmap failed, NULL pointer!\n");
        return -1;
    }
    printf_debug("virtual address:%p, physical address:0x%x\n", fpga_reg->signal, FPGA_SIGNAL_BASE);

    tmp_addr =  memmap(fd_dev, FPGA_BRAOD_BAND_BASE, SYSTEM_CONFG_4K_LENGTH); 
    if (!tmp_addr)
    {
        printf("mmap failed, NULL pointer!\n");
        exit(1);
    }
    for(i = 0; i < MAX_RADIO_CHANNEL_NUM; i++){
        fpga_reg->broad_band[i] = tmp_addr + BROAD_BAND_REG_OFFSET*i;
        printf_note("virtual address:%p, physical address:0x%x\n", fpga_reg->broad_band[i], FPGA_BRAOD_BAND_BASE+BROAD_BAND_REG_OFFSET*i);
    }

    fpga_reg->narrow_band[0] = memmap(fd_dev, FPGA_NARROR_BAND_BASE, SYSTEM_CONFG_REG_LENGTH * NARROW_BAND_CHANNEL_MAX_NUM); 
    if (!fpga_reg->narrow_band[0])
    {
        printf("mmap failed, NULL pointer!\n");
        return -1;
    }
    printf_debug("virtual address:%p, physical address:0x%x\n", fpga_reg->narrow_band[0], FPGA_NARROR_BAND_BASE);

    
    for (i = 1; i < NARROW_BAND_CHANNEL_MAX_NUM; ++i){
        fpga_reg->narrow_band[i] = (void *)fpga_reg->narrow_band[0] + NARROW_BAND_REG_LENGTH * i;
    }
    return 0;
}

static int fpga_unmemmap(FPGA_CONFIG_REG *fpga_reg)
{
    int i = 0;
    int ret = 0;

    ret = munmap(fpga_reg->system, SYSTEM_CONFG_4K_LENGTH); 
    if (ret)
    {
    	perror("munmap");
        return -1;
    }

    ret = munmap(fpga_reg->signal, SYSTEM_CONFG_REG_LENGTH); 
    if (ret)
    {
        perror("munmap");
        return -1;
    }
    
    for(i = 0; i < MAX_RADIO_CHANNEL_NUM; i++){
        ret = munmap(fpga_reg->broad_band[i], SYSTEM_CONFG_REG_LENGTH); 
        if (ret)
        {
        	perror("munmap");
            return -1;
        }
    }


    ret = munmap(fpga_reg->narrow_band[0], SYSTEM_CONFG_REG_LENGTH); 
    if (ret)
    {
    	perror("munmap");
        return -1;
    }

    return 0;
}


static FPGA_CONFIG_REG *fpga_reg = NULL;
static bool fpga_init_flag = false;


FPGA_CONFIG_REG *get_fpga_reg(void)
{
    if(fpga_init_flag == false)
        fpga_io_init();

    
    return fpga_reg;
}


void fpga_io_init(void)
{
    int fd_fpga, i;
    static FPGA_CONFIG_REG _fpga_reg;
    
    if(fpga_init_flag)
        return;

    fd_fpga = open(FPGA_REG_DEV, O_RDWR|O_SYNC);
    if (fd_fpga == -1){
        printf_warn("%s, open error", FPGA_REG_DEV);
        return;
    }
    
    fpga_memmap(fd_fpga, &_fpga_reg);
    fpga_reg = &_fpga_reg;
    printf("FPGA version:%x\n", fpga_reg->system->version);
    fpga_reg->system->system_reset = 1;
    usleep(100);
    fpga_reg->signal->data_path_reset = 1;
    usleep(100);
    for(i = 0; i < MAX_RADIO_CHANNEL_NUM; i++){
        fpga_reg->broad_band[i]->enable = 0xff;
        usleep(100);
        fpga_reg->broad_band[i]->band = 0; //200Mhz
        usleep(100);
        fpga_reg->broad_band[i]->signal_carrier = 0;
        usleep(100);
    }
    fpga_init_flag = true;
}

void fpga_io_close(void)
{
    fpga_unmemmap(fpga_reg);
    fpga_init_flag = false;
}

