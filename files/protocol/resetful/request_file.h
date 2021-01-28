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

#define FILE_PATH_MAX_LEN 256
#define REQUEST_FILESIZE_BUFFER_LEN  512

typedef enum _request_read_status {
    FILE_READ_IDLE = 0x0,
    FILE_READ_BUSY = 0x1,
}request_read_status;


struct disk_file_info{
    uint32_t st_blocks;             /* number of blocks allocated -文件所占块数*/
    uint32_t st_blksize;            /* blocksize for filesystem I/O -系统块的大小*/
    uint64_t st_size;               /* total size, in bytes -文件大小，字节为单位*/
    time_t     ctime;               /* create time 创建时间   */
    uint32_t buffer_rx_len;         /*接收缓存区大小 */
    char  file_path[FILE_PATH_MAX_LEN];
}__attribute__ ((packed));

struct file_request_read{
    uint8_t *read_buffer_pointer;        /* File buffer memory pointer */
    size_t read_buffer_len;              /* File buffer memory length */
    size_t read_buffer_offset;           /* File buffer byte offset size，缓冲区读取偏移 */
    uint32_t st_blkbg_size;             /* 系统数据块组大小 */
    uint64_t st_size;                   /* 请求文件大小 */
    uint64_t offset_size;               /* 文件读取偏移 */
    bool is_buffer_has_data;            /* 缓冲区是否有数据 */
    char  file_path[FILE_PATH_MAX_LEN];
};


extern int file_download(struct uh_client *cl, char *filename);
extern int file_startstore(struct uh_client *cl, void *arg);
extern int file_stopstore(struct uh_client *cl, void *arg);
extern int file_search(struct uh_client *cl, void *arg);
extern int file_start_backtrace(struct uh_client *cl, void *arg);
extern int file_stop_backtrace(struct uh_client *cl, void *arg);
extern int file_delete(struct uh_client *cl, void *arg);
extern void file_handle_init(void);


#endif
