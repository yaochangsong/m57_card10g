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


typedef enum _disk_err_code {

    DISK_CODE_OK = 0,
    DISK_CODE_NOT_FOUND = 1,
    DISK_CODE_FORAMT = 2,
    DISK_CODE_ERR = 3,
    DISK_CODE_NOT_FORAMT = 4,
}disk_err_code;

struct fs_ops {
	bool (*fs_disk_info)(struct statfs *diskInfo);
    bool (*fs_disk_valid)(void);
    int (*fs_format)(void);
    int (*fs_mkdir)(char *);
    int (*fs_delete)(char *);
    int (*fs_find)(char *,int (*callback) (char *,void *, void *), void *);
    ssize_t (*fs_dir)(char *,  int(*callback) (char *, void *, void *), void *);
    ssize_t (*fs_start_save_file)(int, char *);
    ssize_t (*fs_stop_save_file)(int, char *);
    ssize_t (*fs_start_read_raw_file)(int,char *);
    ssize_t (*fs_stop_read_raw_file)(int,char *);
    disk_err_code (*fs_get_err_code)(void);
    int (*fs_close)(void);
};

struct fs_context {
    const struct fs_ops *ops;
};

struct fs_notify_status {
    char *filename;
    size_t filesize;
    time_t duration_time;
    time_t timeout_ms;
    struct timeval start_time;
    struct uloop_timeout timeout;
    void *args;
};

extern void fs_init(void);
extern char *fs_get_root_dir(void);
extern struct fs_context *get_fs_ctx(void);
extern struct fs_context *get_fs_ctx_ex(void);
extern struct fs_context * fs_create_context(void);
extern void fs_file_list(char *filename, struct stat *stats, void *arg_array);
#endif

