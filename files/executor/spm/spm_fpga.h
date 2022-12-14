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
*  Rev 1.0   21 Feb. 2020   yaochangsong
*  Initial revision.
******************************************************************************/
#ifndef _SPM_FPGA_H
#define _SPM_FPGA_H

#define DMA_FFT_DEV   "/dev/dma_fft"
#define DMA_BIQ_DEV  "/dev/dma_biq"
//#define DMA_NIQ_DEV   "/dev/dma_niq"
#define DMA_NIQ_DEV   "/dev/dma_niq"


#define DMA_MAX_BLOCK (3)
#define DMA_BUFFER_16M_SIZE (16 * 1024 * 1024)
#define DMA_BUFFER_64M_SIZE (64 * 1024 * 1024)
#define DMA_BUFFER_128M_SIZE (128 * 1024 * 1024)

#define DMA_IQ_BUFFER_SIZE DMA_BUFFER_16M_SIZE
typedef enum _read_status_
{
    READ_BUFFER_STATUS_OK = 0x0,//读数据成功
    READ_BUFFER_STATUS_FAIL = 0x1, //读数据一般错误
    READ_BUFFER_STATUS_OVERRUN = 0x2, //缓冲区满，有数据被覆盖
    READ_BUFFER_STATUS_PENDING = 0x3, //没有可读数据
}read_status;

typedef enum _write_status_
{
    WRITE_BUFFER_STATUS_OK = 0x0,//写数据成功
    WRITE_BUFFER_STATUS_FAIL = 0x1, //读数据一般错误
    WRITE_BUFFER_STATUS_EMPTY = 0x2, //写缓冲区空，DMA已传输完所有数据，可能会造成输出不连续
    WRITE_BUFFER_STATUS_FULL = 0x3, //缓冲区满，用户程序要等待DMA传输数据
}write_status;


typedef struct _block_info_
{
    unsigned int offset; //可读数据的偏移值，该值加上mmap得到的地址即为数据的虚拟地址
    unsigned int length; //可读数据的长度，单位字节
}block_info;

typedef struct _ready_info_
{
    read_status status;//当前读数据状态，定义如下：
    unsigned char block_num;//数据块个数
    block_info blocks[DMA_MAX_BLOCK];
}read_info;

typedef struct _write_info_ 
{
    write_status status;//当前读数据状态
    unsigned char block_num;//数据块个数
    block_info blocks[DMA_MAX_BLOCK];
}write_info;

typedef enum _dma_status_ {
    DMA_STATUS_IDLE = 0,   //DMA空闲
    DMA_STATUS_BUSY = 1,    //DMA忙，运行中
    DMA_STATUS_INITIALIZING = 2, //DMA启动过程中
    DMA_STATUS_PRE_WRITE = 3, //写准备状态，先将DMA buffer填满在启动DMA，避免取到无效数据
}dma_status;

typedef enum _dma_dire_ {
    DMA_S2MM = 0,   //设备到内存方向
    DMA_MM2S = 1,   //内存到设备方向
}dma_dire;

typedef struct _ioctl_dma_status_ {
    dma_dire dir;
    dma_status status;
}ioctl_dma_status;


typedef enum _dma_mode_ {
    DMA_MODE_ONCE = 0,         //单次工作模式
    DMA_MODE_CONTINUOUS = 1,   //连续工作模式
}dma_mode;

//DMA启动参数结构体
typedef struct _IOCTL_DMA_START_PARA_
{
    dma_mode mode;  //DMA 工作模式
    unsigned int trans_len; //单次工作模式下一次传输的数据长度，其他模式下无意义。
}IOCTL_DMA_START_PARA;

typedef enum _dma_tx_rx_
{
    DMA_READ = 0,
    DMA_WRITE = 1,
}dma_tx_rx;


struct _spm_stream {
    char *devname;      /* 流设备节点名称 */
    int id;             /* 频谱流类型描述符 */
    int ch;             /* 主通道，如果没有写-1 */
    uint8_t *ptr;       /* 频谱流数据buffer指针 */
    uint32_t len;       /* 频谱流数据buffer长度 */
    char *name;         /* 频谱流名称 */
    int rd_wr;
    enum stream_type type;
};



typedef enum _IOCTL_CMD_ 
{
    DMA_ASYN_READ_START = 0x1,
    DMA_ASYN_READ_STOP,
    DMA_GET_ASYN_READ_INFO,
    DMA_SET_ASYN_READ_INFO,
    DMA_ASYN_WRITE_START,
    DMA_ASYN_WRITE_STOP,
    DMA_GET_ASYN_WRITE_INFO,
    DMA_SET_ASYN_WRITE_INFO,    
    DMA_ASYN_WRITE_FLUSH,
    DMA_GET_STATUS,
    DMA_INIT_BUFFER,
    DMA_DUMP_REGS,
}FPGA_IOCTL_CMD;


#define XWDMA_IOCTL_MAGIC  'k'
#define IOCTL_DMA_ASYN_READ_START      _IOW(XWDMA_IOCTL_MAGIC, DMA_ASYN_READ_START, unsigned int)
#define IOCTL_DMA_GET_ASYN_READ_INFO      _IOW(XWDMA_IOCTL_MAGIC, DMA_GET_ASYN_READ_INFO, unsigned int)
#define IOCTL_DMA_SET_ASYN_READ_INFO   _IOW(XWDMA_IOCTL_MAGIC, DMA_SET_ASYN_READ_INFO, unsigned int)
#define IOCTL_DMA_ASYN_READ_STOP   _IOW(XWDMA_IOCTL_MAGIC, DMA_ASYN_READ_STOP, unsigned int)
#define IOCTL_DMA_ASYN_WRITE_START      _IOW(XWDMA_IOCTL_MAGIC, DMA_ASYN_WRITE_START, unsigned int)
#define IOCTL_DMA_GET_ASYN_WRITE_INFO      _IOW(XWDMA_IOCTL_MAGIC, DMA_GET_ASYN_WRITE_INFO, unsigned int)
#define IOCTL_DMA_SET_ASYN_WRITE_INFO   _IOW(XWDMA_IOCTL_MAGIC, DMA_SET_ASYN_WRITE_INFO, unsigned int)
#define IOCTL_DMA_ASYN_WRITE_STOP   _IOW(XWDMA_IOCTL_MAGIC, DMA_ASYN_WRITE_STOP, unsigned int)
#define IOCTL_DMA_ASYN_WRITE_FLUSH   _IOW(XWDMA_IOCTL_MAGIC, DMA_ASYN_WRITE_FLUSH, unsigned int)
#define IOCTL_DMA_GET_STATUS	_IOW(XWDMA_IOCTL_MAGIC, DMA_GET_STATUS, unsigned int)
#define IOCTL_DMA_INIT_BUFFER	_IOW(XWDMA_IOCTL_MAGIC, DMA_INIT_BUFFER, unsigned int)
#define IOCTL_DMA_DUMP_REGS   _IOW(XWDMA_IOCTL_MAGIC, DMA_DUMP_REGS, unsigned int)


extern struct spm_context * spm_create_fpga_context(void);
extern int spm_send_niq_data(void *data, size_t len, void *arg);
extern int spm_send_biq_data(int ch, void *data, size_t len, void *arg);

#endif
