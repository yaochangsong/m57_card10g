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
#include "io_fpga.h"

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

int fpga_memmap(int fd_dev, FPGA_CONFIG_REG *fpga_reg)
{
    void *base_addr = NULL;
    int i = 0;

    fpga_reg->system = memmap(fd_dev, FPGA_SYSETM_BASE, SYSTEM_CONFG_4K_LENGTH); 
    if (!fpga_reg->system)
    {
        printf("mmap failed, NULL pointer!\n");
        return -1;
    }
    printf("virtual address:%p, physical address:0x%x\n", fpga_reg->system, FPGA_SYSETM_BASE);

    fpga_reg->rfReg = (uint8_t *)fpga_reg->system +CONFG_REG_LEN;
    if (!fpga_reg->rfReg)
    {
        printf("mmap failed, NULL pointer!\n");
        return -1;
    }
    printf("virtual address:%p, physical address:0x%x\n", fpga_reg->rfReg, FPGA_RF_BASE);

    fpga_reg->adcReg = memmap(fd_dev, FPGA_ADC_BASE, SYSTEM_CONFG_REG_LENGTH); 
    if (!fpga_reg->adcReg)
    {
        printf("mmap failed, NULL pointer!\n");
        return -1;
    }
    printf("virtual address:%p, physical address:0x%x\n", fpga_reg->adcReg, FPGA_ADC_BASE);

    fpga_reg->signal = memmap(fd_dev, FPGA_SIGNAL_BASE, SYSTEM_CONFG_REG_LENGTH); 
    if (!fpga_reg->signal)
    {
        printf("mmap failed, NULL pointer!\n");
        return -1;
    }
    printf("virtual address:%p, physical address:0x%x\n", fpga_reg->signal, FPGA_SIGNAL_BASE);

    fpga_reg->broad_band = memmap(fd_dev, FPGA_BRAOD_BAND_BASE, SYSTEM_CONFG_REG_LENGTH); 
    if (!fpga_reg->broad_band)
    {
        printf("mmap failed, NULL pointer!\n");
        return -1;
    }
    printf("virtual address:%p, physical address:0x%x\n", fpga_reg->broad_band, FPGA_BRAOD_BAND_BASE);


    fpga_reg->narrow_band[0] = memmap(fd_dev, FPGA_NARROR_BAND_BASE, SYSTEM_CONFG_REG_LENGTH * NARROW_BAND_CHANNEL_MAX_NUM); 
    if (!fpga_reg->narrow_band[0])
    {
        printf("mmap failed, NULL pointer!\n");
        return -1;
    }
    printf("virtual address:%p, physical address:0x%x\n", fpga_reg->narrow_band[0], FPGA_NARROR_BAND_BASE);

    
    for (i = 1; i < NARROW_BAND_CHANNEL_MAX_NUM; ++i){
        fpga_reg->narrow_band[i] = (void *)fpga_reg->narrow_band[0] + NARROW_BAND_REG_LENGTH * i;
    }
    return 0;
}

int fpga_unmemmap(FPGA_CONFIG_REG *fpga_reg)
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
    

    ret = munmap(fpga_reg->broad_band, SYSTEM_CONFG_REG_LENGTH); 
    if (ret)
    {
    	perror("munmap");
        return -1;
    }

    ret = munmap(fpga_reg->narrow_band, SYSTEM_CONFG_REG_LENGTH); 
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
    int fd_fpga;
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
    printf_note("FPGA version:%x\n", fpga_reg->system->version);
    /* 寄存器默认参数设置 */
    /* for fpga clock 213.333mhz,1 ms convert times 213333 */
   // fpga_reg->system->time_pulse = 213333*1;
   // fpga_reg->system->data_path_reset = 1;
   // fpga_reg->broad_band->enable = 0xff;
   // fpga_reg->broad_band->signal_carrier = 0;
    fpga_reg->system->system_reset = 1;
    usleep(100);
    fpga_reg->signal->data_path_reset = 1;
    usleep(100);
    fpga_reg->broad_band->enable = 0xff;
    usleep(100);
    fpga_reg->broad_band->band = 0; //200Mhz
    usleep(100);
    fpga_reg->broad_band->signal_carrier = 0;
    usleep(100);
    fpga_init_flag = true;
}

void fpga_io_close(void)
{
    fpga_unmemmap(fpga_reg);
    fpga_init_flag = false;
}

