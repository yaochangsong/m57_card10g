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

#ifndef MEMSHARE_H
#define MEMSHARE_H

#define SZ_512M 0x20000000
#define SZ_36M 0x02400000
#define SZ_32M 0x02000000
#define SZ_16M 0x01000000
#define SZ_8M  0x00800000
#define SZ_6M  0x00600000
#define SZ_5M  0x00500000
#define SZ_4M  0x00400000
#define SZ_3M  0x00300000
#define SZ_2M  0x00200000
#define SZ_1M  0x00100000
#define SZ_64K 0x00010000

/*
memory map:
    1.system memory map(FOR 1G):
    | Linux system use(0~512M)|   user(we) use 256M(512M~768M)       |  FPGA use 256M(768M~1G)        |
                                    -------------------------
                                            \|/
    2.use memory map:(大小请根据内核分配调整)
    |   IQ data(16M)   |   FFT data(16M)     |   SSD RX(32M)   |   SSD TX(32M)   |

*/
#define DMA_ADDR_START      SZ_512M   /* 512M */
#define DMA_DDR_SIZE        SZ_512M

#define DMA_IQ_SIZE         SZ_16M
#define DMA_FFT_SIZE        SZ_16M
#define DMA_SSD_RX_SIZE     SZ_32M
#define DMA_SSD_TX_SIZE     SZ_32M

extern int memshare_init(void);
extern void * memshare_get_dma_rx_base(void);
#endif
