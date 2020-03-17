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

int fpga_memmap(int fd_dev, FPGA_CONFIG_REG *fpga_reg)
{
    void *base_addr = NULL;
    int i = 0;

    fpga_reg->system = memmap(fd_dev, FPGA_REG_BASE, SYSTEM_CONFG_REG_LENGTH + BROAD_BAND_REG_LENGTH); 
    if (!fpga_reg->system){
        printf_err("mmap failed, NULL pointer!\n");
        return -1;
    }

    printf_note("virtual address:0x%x, physical address:0x%x\n", fpga_reg->system, FPGA_REG_BASE);
    fpga_reg->broad_band = (void*)fpga_reg->system + BROAD_BAND_REG_OFFSET; 
    printf("broad_band:0x%x \n", fpga_reg->broad_band);
    
    fpga_reg->narrow_band[0] = memmap(fd_dev, NARROW_BAND_REG_BASE, NARROW_BAND_REG_LENGTH * NARROW_BAND_CHANNEL_MAX_NUM);
    if (!fpga_reg->narrow_band[0]){
        printf_err("mmap failed, NULL pointer!\n");
        return -1;
    }
    printf_note("virtual address:0x%x, physical address:0x%x\n", fpga_reg->narrow_band[0], NARROW_BAND_REG_BASE);
    for (i = 1; i < NARROW_BAND_CHANNEL_MAX_NUM; ++i){
        fpga_reg->narrow_band[i] = (void *)fpga_reg->narrow_band[0] + NARROW_BAND_REG_LENGTH * i;

    }

    return 0;
}

int fpga_unmemmap(FPGA_CONFIG_REG *fpga_reg)
{
    int i = 0;
    int ret = 0;

    ret = munmap(fpga_reg->system, SYSTEM_CONFG_REG_LENGTH + BROAD_BAND_REG_LENGTH); 
    if (ret){
        perror("munmap");
        return -1;
    }

    ret = munmap(fpga_reg->narrow_band[0], NARROW_BAND_REG_LENGTH * NARROW_BAND_CHANNEL_MAX_NUM);
    if (ret){
        perror("munmap");
        return -1;
    }

    return 0;
}

static FPGA_CONFIG_REG *fpga_reg;
FPGA_CONFIG_REG *get_fpga_reg(void)
{
    return fpga_reg;
}


void fpga_io_init(void)
{
    int fd_fpga;
    static FPGA_CONFIG_REG _fpga_reg;

    fd_fpga = open(FPGA_REG_DEV, O_RDWR|O_SYNC);
    if (fd_fpga == -1){
        printf_warn("%s, open error", FPGA_REG_DEV);
        return (-1);
    }
    
    fpga_memmap(fd_fpga, &_fpga_reg);
    fpga_reg = &_fpga_reg;
    printf_note("FPGA version:%x\n", fpga_reg->system->version);
    /* 寄存器默认参数设置 */
    /* for fpga clock 213.333mhz,1 ms convert times 213333 */
    fpga_reg->system->time_pulse = 213333*1;
    fpga_reg->system->data_path_reset = 1;
    fpga_reg->broad_band->enable = 0xff;
    fpga_reg->broad_band->signal_carrier = 0;
}

void fpga_io_close(void)
{
    fpga_unmemmap(fpga_reg);
}

