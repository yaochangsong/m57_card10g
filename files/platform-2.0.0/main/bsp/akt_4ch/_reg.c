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

pthread_mutex_t rf_param_mutex[MAX_RF_NUM] = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};


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

static int fpga_memmap(int fd_dev, FPGA_CONFIG_REG *fpga_reg)
{
    void *base_addr = NULL;
    int i = 0;

    fpga_reg->z7base = memmap(fd_dev, FPGA_Z7_REG_BASE, SYSTEM_CONFG_4K_LENGTH);
    if (!fpga_reg->z7base)
    {
        printf("mmap failed, NULL pointer!\n");
        return -1;
    }
    printf_note("[z7base]virtual address:%p, physical address:0x%x\n", fpga_reg->z7base, FPGA_Z7_REG_BASE);

    #if 0
    //audio
    fpga_reg->audioReg = memmap(fd_dev, FPGA_AUDIO_BASE, AUDIO_REG_LENGTH); 
    if (!fpga_reg->audioReg)
    {
        printf("mmap failed, NULL pointer!\n");
        return -1;
    }
    printf_note("[audio]virtual address:%p, physical address:0x%x\n", fpga_reg->audioReg, FPGA_AUDIO_BASE);
    #else
    //audio
    fpga_reg->audioReg = (AUDIO_REG *)((uint8_t *)fpga_reg->z7base + CONFG_AUDIO_OFFSET);
    printf_note("[audio]virtual address:%p, physical address:0x%x\n", fpga_reg->audioReg, FPGA_AUDIO_BASE);
    #endif
    
    //system
    fpga_reg->system = memmap(fd_dev, FPGA_SYSETM_BASE, SYSTEM_CONFG_4K_LENGTH); 
    if (!fpga_reg->system)
    {
        printf("mmap failed, NULL pointer!\n");
        return -1;
    }
    printf_note("[system]virtual address:%p, physical address:0x%x\n", fpga_reg->system, FPGA_SYSETM_BASE);
    
    //adc
    fpga_reg->adcReg = memmap(fd_dev, FPGA_ADC_BASE, ADC_REG_LENGTH); 
    if (!fpga_reg->adcReg)
    {
        printf("mmap failed, NULL pointer!\n");
        return -1;
    }
    printf_note("[adc]virtual address:%p, physical address:0x%x\n", fpga_reg->adcReg, FPGA_ADC_BASE);

    //宽带ddc
    fpga_reg->bddc[0] = memmap(fd_dev, FPGA_DDC_BASE, SYSTEM_CONFG_4K_LENGTH); 
    if (!fpga_reg->bddc[0])
    {
        printf("mmap failed, NULL pointer!\n");
        return -1;
    }
    printf_note("[bddc 0]virtual address:%p, physical address:0x%x\n", fpga_reg->bddc[0], FPGA_DDC_BASE);
    for (i = 1; i < DDC_CHANNEL_MAX_NUM; ++i){
        fpga_reg->bddc[i] = (BROAD_BAND_REG *)((uint8_t *)fpga_reg->bddc[0] + DDC_REG_LENGTH * i);
        printf_note("[bddc %d]virtual address:%p, physical address:0x%x\n", i,fpga_reg->bddc[i], FPGA_DDC_BASE+DDC_REG_LENGTH*i);
        //fpga_reg->bddc[i] = (void *)fpga_reg->bddc[0] + DDC_REG_LENGTH * i;
    }
    
    #if 0
    //窄带ddc
    for (i = DDC_CHANNEL_MAX_NUM; i < DDC_CHANNEL_MAX_NUM*2; ++i){
        fpga_reg->nddc[i-DDC_CHANNEL_MAX_NUM] = (NARROW_BAND_REG *)((uint8_t *)fpga_reg->bddc[0] + DDC_REG_LENGTH * i);
    }
    printf_note("[nddc]virtual address:%p, physical address:0x%x\n", fpga_reg->nddc[0], FPGA_NDDC_BASE);
    #endif

    //fft组包
    fpga_reg->fft = memmap(fd_dev, FPGA_FFT_BASE, FFT_REG_LENGTH); 
    if (!fpga_reg->fft)
    {
        printf("mmap failed, NULL pointer!\n");
        return -1;
    }
    printf_note("[fft]virtual address:%p, physical address:0x%x\n", fpga_reg->fft, FPGA_FFT_BASE);

    //窄带ddc
    fpga_reg->nddc[0] = memmap(fd_dev, FPGA_NDDC_BASE, SYSTEM_CONFG_4K_LENGTH); 
    if (!fpga_reg->nddc[0])
    {
        printf("mmap failed, NULL pointer!\n");
        return -1;
    }
    printf_note("[nddc]virtual address:%p, physical address:0x%x\n", fpga_reg->nddc[0], FPGA_NDDC_BASE);
    for (i = 1; i < DDC_CHANNEL_MAX_NUM; ++i){
        fpga_reg->nddc[i] = (NARROW_BAND_REG *)((uint8_t *)fpga_reg->nddc[0] + DDC_REG_LENGTH * i);
    }
    return 0;
}

static int fpga_unmemmap(FPGA_CONFIG_REG *fpga_reg)
{
    int i = 0;
    int ret = 0;

    ret = munmap(fpga_reg->z7base, SYSTEM_CONFG_4K_LENGTH); 
    if (ret)
    {
    	perror("munmap");
        return -1;
    }
    
    ret = munmap(fpga_reg->system, SYSTEM_CONFG_4K_LENGTH); 
    if (ret)
    {
    	perror("munmap");
        return -1;
    }

    ret = munmap(fpga_reg->adcReg, ADC_REG_LENGTH); 
    if (ret)
    {
        perror("munmap");
        return -1;
    }

    ret = munmap(fpga_reg->bddc[0], SYSTEM_CONFG_4K_LENGTH); 
    if (ret)
    {
    	perror("munmap");
        return -1;
    }
    
    ret = munmap(fpga_reg->fft, FFT_REG_LENGTH); 
    if (ret)
    {
        perror("munmap");
        return -1;
    }

    ret = munmap(fpga_reg->nddc[0], SYSTEM_CONFG_4K_LENGTH); 
    if (ret)
    {
    	perror("munmap");
        return -1;
    }
    
    printf_note("munmap OK\n");
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
    printf_note("----FPGA mem init\n");
    fd_fpga = open(FPGA_REG_DEV, O_RDWR|O_SYNC);
    if (fd_fpga == -1){
        printf_warn("%s, open error", FPGA_REG_DEV);
        return;
    }
    printf_note("FPGA mem fd:%d\n", fd_fpga);
    fpga_memmap(fd_fpga, &_fpga_reg);
    fpga_reg = &_fpga_reg;
    printf_note("FPGA version:%x\n", fpga_reg->system->version);
    fpga_reg->system->system_reset = 1;
    printf_note("sys reset...\n");
    usleep(100);
    for(i = 0; i < MAX_RADIO_CHANNEL_NUM; i++){
        fpga_reg->bddc[i]->enable = 0x00;  //关闭所有fft
        usleep(100);
        fpga_reg->nddc[i]->enable = 0x00;
        usleep(100);
    }
    printf_note("==========[audio iic addr:%p][audio sample rate addr:%p]\n", &fpga_reg->audioReg->iic_ch_en, (uint8_t *)&fpga_reg->adcReg->audio_sample);
    fpga_init_flag = true;
    printf_note("fpga IO memmp ok!\n");
}

void fpga_io_close(void)
{
    fpga_unmemmap(fpga_reg);
    fpga_init_flag = false;
}

