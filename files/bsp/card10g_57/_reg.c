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


void *memmap(int fd_dev, off_t phr_addr, int length)
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
#define MAP_SIZE (32*1024UL)



static int fpga_memmap(int fd_dev, FPGA_CONFIG_REG *fpga_reg)
{
    void *tmp_addr = NULL;
    int i = 0;

    fpga_reg->system = memmap(fd_dev, FPGA_SYSETM_BASE, MAP_SIZE); 
    //fpga_reg->system = memmap(fd_dev, 0, MAP_SIZE); 
    if (!fpga_reg->system)
    {
        printf("mmap failed, NULL pointer!\n");
        return -1;
    }
    printf_note("virtual address:%p, physical address:0x%x, %p, %p\n", fpga_reg->system, FPGA_SYSETM_BASE, &fpga_reg->system->fpga_status, &fpga_reg->system->channel_sel);

    for(i = 0; i < MAX_FPGA_CARD_SLOT_NUM; i++){
        fpga_reg->status[i] = (STATUS_REG *)((uint8_t *)fpga_reg->system + FPGA_STAUS_OFFSET + CONFG_REG_LEN * i);
        if (!fpga_reg->status[i])
        {
            printf("mmap failed, NULL pointer!\n");
            return -1;
        }
        printf_note("virtual address:%p \n", fpga_reg->status[i]);
    }

    return 0;
}

static int fpga_unmemmap(FPGA_CONFIG_REG *fpga_reg)
{
    int i = 0;
    int ret = 0;

    ret = munmap(fpga_reg->system, MAP_SIZE); 
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
        printf_warn("%s, open error\n", FPGA_REG_DEV);
        return;
    }
    
    fpga_memmap(fd_fpga, &_fpga_reg);
    
    fpga_reg = &_fpga_reg;
    printf("FPGA version:%x\n", fpga_reg->system->version);
    fpga_reg->system->system_reset = 1;
    usleep(100);
    fpga_init_flag = true;
}


void fpga_io_close(void)
{
    //fpga_unmemmap(fpga_reg);
    fpga_init_flag = false;
}


