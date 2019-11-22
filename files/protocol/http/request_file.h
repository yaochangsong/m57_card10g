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
*  Rev 1.0   09 Nov 2019   yaochangsong
*  Initial revision.
******************************************************************************/
#ifndef _REQUEST_FILE_H_
#define _REQUEST_FILE_H_
#include <time.h>

#define REQUEST_FILESIZE_BUFFER_LEN  512

typedef enum _request_read_status {
    FILE_READ_IDLE = 0x0,
    FILE_READ_BUSY = 0x1,
}request_read_status;


struct disk_file_info{
    uint8_t  file_path[64];
    uint32_t st_blocks;             /* number of blocks allocated -文件所占块数*/
    uint32_t st_blksize;            /* blocksize for filesystem I/O -系统块的大小*/
    uint64_t st_size;               /* total size, in bytes -文件大小，字节为单位*/
    time_t     ctime;               /* create time 创建时间   */
    uint32_t read_cnt;                 /* 读取缓存总次数 */
    uint32_t buffer_len;               /*缓存区大小 */
};

struct file_request_read{
    uint8_t  file_path[64];
    uint8_t *read_buffer_pointer;        /* File buffer memory pointer */
    size_t read_buffer_len;              /* File buffer memory length */
    size_t read_offset;                  /* File buffer byte offset size */
    uint8_t read_flags;                  /* 0: idle, 1: busy */
};



extern int file_download(struct uh_client *cl, void *arg);
extern int file_startstore(struct uh_client *cl, void *arg);
extern int file_stopstore(struct uh_client *cl, void *arg);
extern int file_search(struct uh_client *cl, void *arg);
extern int file_start_backtrace(struct uh_client *cl, void *arg);
extern int file_stop_backtrace(struct uh_client *cl, void *arg);
extern int file_delete(struct uh_client *cl, void *arg);
extern void file_handle_init(void);


#endif
