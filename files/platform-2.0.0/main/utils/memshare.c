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
*  Rev 1.0   21 Nov 2019   yaochangsong
*  Initial revision.
******************************************************************************/
#include "config.h"
#include "memshare.h"
#include <sys/mman.h>

static int mem_share_fd = -1;
void *map_base = NULL;


int memshare_init(void)
{
    if (mem_share_fd > 0) 
        return -1;

    mem_share_fd= open("/dev/mem", O_RDWR|O_SYNC);
    if (mem_share_fd == -1){
        printf_err("open /dev/mem error.\n");
        return -1;
    }

    map_base = mmap(NULL, DMA_DDR_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mem_share_fd, DMA_ADDR_START);

    if (map_base == 0){
        printf_err("memshare init failed!\n");
        return -1;
    }
    printf_info("Memory mapped at address %p.\n", map_base);
    //memset(memshare_get_dma_rx_base(), 0, DMA_SSD_RX_SIZE);
    return 0;
}

void * memshare_get_dma_rx_base(void)
{
    if(map_base == NULL)
        return NULL;
    else
        //return (map_base);
        return (map_base + DMA_IQ_SIZE+DMA_FFT_SIZE);
}
