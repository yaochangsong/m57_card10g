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
#include <sys/stat.h>
#include "../lib/libubox/uloop.h"
#include "fs_notifier.h"

#define _FS_EX_ATTR_DOWNLOAD_KEY   "user.download"
#define _FS_EX_ATTR_BACKTRACE_KEY  "user.backtrace"
#define _FS_EX_ATTR_PARAM_FREQ_KEY "user.paramfreq"
#define _FS_EX_ATTR_PARAM_BW_KEY   "user.parambw"
#define _FS_EX_ATTR_PARAM_FFTSIZE_KEY   "user.paramfftsize"
#define _FS_EX_ATTR_PARAM_SAMPLE_RATE_KEY "user.paramsampleRate"

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

typedef enum _fs_ops_func_index {
    FS_OPS_INDEX_SAVE = 0,
    FS_OPS_INDEX_BACK = 1,
    FS_OPS_INDEX_MAX = 2,
}fs_ops_func_index;

struct fs_save_sample_args{
    uint32_t bindwidth;
    uint16_t sample_time;
    uint16_t cache_time;
    uint64_t sample_size;
}__attribute__ ((packed));

struct fs_sampler{
    struct fs_save_sample_args args;
    sample_policy_code sample_policy;
    struct uloop_timeout timeout;
};

struct fs_progress_t{
    #define FS_PROGRESS_NOTIFY_TIME_MS 1000
    struct uloop_timeout timeout;
    int val;
    size_t offset;
    bool is_finish;
    bool is_exit;
};

struct thread_wait_t{
    pthread_cond_t t_cond;
    pthread_mutex_t t_mutex;
    volatile bool is_wait;
};

struct push_arg{
    struct timeval ct;
    uint64_t total_size;
    uint64_t nbyte;
    uint64_t split_nbyte;
    uint64_t count;
    int  fd;
    int split_num;
    int ch;
    int ops_index;
    char *thread_name;
    char *filename;
    uint32_t args;
    struct list_head file_list;
    struct fs_notifier notifier;
    struct fs_sampler sampler;
#if defined(CONFIG_FS_PROGRESS)
    struct fs_progress_t  progress;
#endif
    struct fs_context *pctx;
};

#include "config.h"
#define FS_OPS_CHANNEL  MAX_RADIO_CHANNEL_NUM

struct fs_thread_m{
    pthread_t tid;
    void *args;
#ifdef  CONFIG_FS_PAUSE
    struct thread_wait_t pwait;
#endif
};

struct fs_context {
    const struct fs_ops *ops;
    struct fs_thread_m fs_thread[FS_OPS_CHANNEL][FS_OPS_INDEX_MAX];
    void *client;
};


struct fs_ops {
    char *(*fs_root_dir)(void);
	bool (*fs_disk_info)(struct statfs *diskInfo);
    bool (*fs_disk_valid)(void);
    int (*fs_format)(void);
    int (*fs_mkdir)(char *);
    int (*fs_delete)(char *);
    int (*fs_find)(const char *, const char *,void (*callback) (char *,struct stat *, void *, void *), void *);
    int (*fs_find_rec)(char *, char *, void (*callback) (char *,struct stat *, void *, void *), void *);
    int (*fs_get_filesize)(const char *filepath, size_t *size);
    ssize_t (*fs_dir)(char *,  void(*callback) (char *, struct stat *, void *, void *), void *);
    ssize_t (*fs_list)(char *,  void(*callback) (char *, struct stat *, void *), void *);
    ssize_t (*fs_start_save_file)(int, char *, void*, struct fs_context *);
    ssize_t (*fs_stop_save_file)(int, char *, struct fs_context *);
    ssize_t (*fs_pause_save_file)(int, bool, struct fs_context *);
    ssize_t (*fs_start_read_raw_file)(int,char *, struct fs_context *);
    ssize_t (*fs_stop_read_raw_file)(int,char *, struct fs_context *);
    ssize_t (*fs_pause_read_raw_file)(int,bool, struct fs_context *);
    ssize_t (*fs_set_raw_file_offset)(int, char*, size_t, struct fs_context *);
    disk_err_code (*fs_get_err_code)(void);
    int (*fs_file_download_cb)(const char *);
    int (*fs_get_file_status)(const char *);
    int (*fs_create_default_path)(int, char *, size_t, void *);
    void *(*fs_get_xattr)(int, const char *, char *, void *,size_t);
    int (*fs_close)(struct fs_context *);
};



extern void fs_init(void);
extern char *fs_get_root_dir(void);
extern struct fs_context *get_fs_ctx(void);
extern struct fs_context *get_fs_ctx_ex(void);
extern struct fs_context * fs_create_context(void);
extern void fs_file_list(char *filename, struct stat *stats, void *arg_array);
#endif

