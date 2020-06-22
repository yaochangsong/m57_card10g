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

struct fs_ops {
    bool (*fs_disk_valid)(void);
    int (*fs_format)(void);
    int (*fs_mkdir)(char *);
    ssize_t (*fs_dir)(char *,void **);
    ssize_t (*fs_save_file)(char *, int);
    ssize_t (*fs_read_file)(char *);
    int (*fs_close)(void);
};

struct fs_context {
    const struct fs_ops *ops;
};

extern void fs_init(void);
extern struct fs_context *get_fs_ctx(void);
extern struct fs_context * fs_create_context(void);
#endif

