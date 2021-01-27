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
*  Rev 1.0   19 June. 2020   yaochangsong
*  Initial revision.
******************************************************************************/
#ifndef _FS_H
#define _FS_H

#include <stdbool.h>
#include <sys/statfs.h>
#include "../lib/libubox/uloop.h"
#include "fs_notifier.h"


typedef enum _disk_err_code {

    DISK_CODE_OK = 0,
    DISK_CODE_NOT_FOUND = 1,
    DISK_CODE_FORAMT = 2,
    DISK_CODE_ERR = 3,
    DISK_CODE_NOT_FORAMT = 4,
}disk_err_code;

typedef enum _file_sample_policy {
    FSP_TIMER = 0,  /* ?¡§¨º¡À¡ä?¡ä¡é */
    FSP_SIZE = 1,   /* ???¡§¡ä¨®D?¡ä?¡ä¡é */
    FSP_FREE = 2,   /* ¡Á?¨®¨¦¡ä?¡ä¡é */
}sample_policy_code;
struct fs_ops {
	bool (*fs_disk_info)(struct statfs *diskInfo);
    bool (*fs_disk_valid)(void);
    int (*fs_format)(void);
    int (*fs_mkdir)(char *);
    int (*fs_delete)(char *);
    int (*fs_find)(char *,int (*callback) (char *, struct stat *, size_t *), void *);
    ssize_t (*fs_dir)(char *,  void(*callback) (char *, struct stat *, void *), void *);
    ssize_t (*fs_start_save_file)(int, char *, void*);
    ssize_t (*fs_stop_save_file)(int, char *);
    ssize_t (*fs_start_read_raw_file)(int,char *);
    ssize_t (*fs_stop_read_raw_file)(int,char *);
    disk_err_code (*fs_get_err_code)(void);
    int (*fs_close)(void);
};

struct fs_save_sample_args{
    uint32_t bindwidth;
    uint16_t sample_time;
    uint64_t sample_size;
}__attribute__ ((packed));

struct fs_sampler{
    struct fs_save_sample_args args;
    sample_policy_code sample_policy;
    struct uloop_timeout timeout;
};
struct push_arg{
    struct timeval ct;
    uint64_t nbyte;
    uint64_t split_nbyte;
    uint64_t count;
    int  fd;
    int split_num;
    int ch;
    char *thread_name;
    char *filename;
    uint32_t args;
    struct list_head file_list;
    struct fs_notifier notifier;
    struct fs_sampler sampler;
};


struct fs_context {
    const struct fs_ops *ops;
};



extern void fs_init(void);
extern char *fs_get_root_dir(void);
extern struct fs_context *get_fs_ctx(void);
extern struct fs_context *get_fs_ctx_ex(void);
extern struct fs_context * fs_create_context(void);
extern void fs_file_list(char *filename, struct stat *stats, void *arg_array);
#endif

