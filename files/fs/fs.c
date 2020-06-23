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
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <math.h>
#include <dirent.h>
#include <pthread.h>
#include <stdbool.h>
#include "fs.h"
#include "../log/log.h"
#include "../utils/utils.h"
#include "../executor/spm/spm.h"
#include "config.h"

#define DMA_BUFFER_SIZE (16 * 1024 * 1024)

static struct _thread_tables{
    char *name;
    pthread_t pid;
    int num;
};

#define THREAD_MAX_NUM 10
static struct _thread_pool{
    struct _thread_tables tb[THREAD_MAX_NUM];
    int index;
};
static struct _thread_pool tp;

struct fs_context *fs_ctx = NULL;
static bool disk_is_valid = false;


static inline void _thread_data_init(void)
{
    memset(&tp, 0, sizeof(struct _thread_pool));
}

static void  *disk_buffer_ptr  = NULL;
static inline int _write_fs_disk_init(char *path)
{
    int fd;
    if(path == NULL)
        return -1;

    fd = open(path, O_RDWR | O_CREAT | O_TRUNC | O_DIRECT | O_SYNC, 0666);
    //fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        printf_err("open %s fail\n", path);
        return -1;
    }
    posix_memalign((void **)&disk_buffer_ptr, 4096 /*alignment */ , DMA_BUFFER_SIZE + 4096);
    return fd;
}
static inline  int _write_disk_run(int fd, size_t size, void *pdata)
{
    #define     WRITE_UNIT_BYTE 8388608
    size_t pagesize = 0;
    void *user_mem = NULL;
    int rc;

    if(size == 0 || pdata == NULL || fd <= 0)
        return -1;

    //posix_memalign((void **)&user_mem, 4096 /*alignment */ , size + 4096);
   // posix_memalign((void **)&user_mem, 4096 /*alignment */ , WRITE_UNIT_BYTE + 4096);
    //printf_note("fd=%d, pdata=%p, %p, size=0x%x\n",fd, pdata,user_mem,  size);

    if(size > DMA_BUFFER_SIZE){
        printf_warn("size is too big: %u\n", size);
        size = DMA_BUFFER_SIZE;
    }
    memcpy(disk_buffer_ptr, pdata, size);
    //printf_note("pdata=%p, size = 0x%x\n", pdata, size);
    //rc = write(fd, user_mem, WRITE_UNIT_BYTE);
    rc = write(fd, disk_buffer_ptr, size);
    if (rc < 0){
        perror("write file");
    }
    sync();
    return 0;
}

static inline  void _write_disk_close(int fd)
{
    if(fd > 0)
        close(fd);
}


static inline bool _fs_disk_valid(void)
{
    #define DISK_NODE_NAME "/dev/nvme0"
    //if(access(DISK_NODE_NAME)){
    //    printf_note("Disk[%s] is not valid\n", DISK_NODE_NAME);
   //     return false
    //}
    return true;
}


static inline int _fs_format(void)
{
    if(disk_is_valid == false)
        return -1;
    return safe_system("mkfs.ext3    /dev/sda6");
}

static inline int _fs_mkdir(char *dirname)
{
    if(disk_is_valid == false)
        return -1;
    if(dirname && access(dirname, F_OK)){
        printf("mkdir %s\n", dirname);
        mkdir(dirname, 0755);
    }
    
    return 0;
}

static  inline char *_fs_get_root_dir(void)
{
    return "/run/media/nvme0n1/data";
}



static ssize_t _fs_dir(char *dirname, void **data)
{
    DIR *dp;
    struct dirent *dirp;
    struct stat stats;
    char path[PATH_MAX];
    if((dp = opendir(dirname))==NULL){
        perror("opendir error");
        return -1;
    }
    while((dirp = readdir(dp))!=NULL){
        if((strcmp(dirp->d_name,".")==0)||(strcmp(dirp->d_name,"..")==0))
            continue;
        printf("%6d:%-19s %5s\n",dirp->d_ino,dirp->d_name,(dirp->d_type==DT_DIR)?("(DIR)"):(""));
        snprintf(path,PATH_MAX, "%s/%s", dirname, dirp->d_name);
        path[PATH_MAX] = 0;
        printf_note("path:%s, dirp->d_name = %s, PATH_MAX=%d\n", path, dirp->d_name, PATH_MAX);
        if (stat(path, &stats))
            continue;
        printf_note("PATH_MAX:%d, (%s)st_size:%llu, st_blocks:%u, st_mode:%x, st_mtime=0x%x\n", PATH_MAX, dirp->d_name, (off_t)stats.st_size, stats.st_blocks, stats.st_mode, stats.st_mtime);
    }
    closedir(dp);
    return 0;
}

static int _fs_start_save_file_thread(void *arg)
{
    typedef int16_t adc_t;
    adc_t *ptr = NULL;
    ssize_t nread = 0; 
    int ret, fd;
#if 1
    fd = *(int *)arg;
    struct spm_context *_ctx;
    _ctx = get_spm_ctx();
    nread = _ctx->ops->read_adc_data(&ptr);
    if(nread > 0){
        ret = _write_disk_run(fd, nread, ptr);
        _ctx->ops->read_adc_over_deal(&nread);
    }else{
        usleep(1000);
        printf("read_adc_data err:%d\n", nread);
    }
    //usleep(1000);
#else
    sleep(1);
#endif
    return nread;

}


/* act = 1: start save file; act = 0: stop save file */
static ssize_t _fs_save_file(char *filename, int act)
{
    #define     _START_SAVE     1
    #define     _STOP_SAVE      0
    int ret = -1, i, found = 0, cval;
    static int fd = -1;
    #define THREAD_NAME "FS_SAVE_FILE"
    char path[512];
    
    if(disk_is_valid == false)
        return -1;
    if(filename == NULL)
            return -1;
    
    if(act == _START_SAVE){
        io_set_enable_command(ADC_MODE_ENABLE, -1, -1, 0);
        snprintf(path, sizeof(path), "%s/%s", _fs_get_root_dir(), filename);
        fd = _write_fs_disk_init(path);
        printf("start save file: %s\n", path);
        ret = pthread_create_detach_loop(NULL, _fs_start_save_file_thread, THREAD_NAME, &fd);
        if(ret != 0){
            perror("pthread cread save file thread!!");
        }
    }else{  /* stop save file */
        io_set_enable_command(ADC_MODE_DISABLE, -1, -1, 0);
        printf("stop save file : %s\n", THREAD_NAME);
        pthread_cancel_by_name(THREAD_NAME);
        _write_disk_close(fd);
    }
    return ret;
}

static ssize_t _fs_read_file(char *filename)
{
    if(disk_is_valid == false)
        return -1;
    return 0;
}

static int _fs_close(void)
{
    if(disk_is_valid == false)
        return -1;
    safe_free(fs_ctx);
    return 0;
}

static const struct fs_ops _fs_ops = {
    .fs_disk_valid = _fs_disk_valid,
    .fs_format = _fs_format,
    .fs_mkdir = _fs_mkdir,
    .fs_dir = _fs_dir,
    .fs_save_file = _fs_save_file,
    .fs_read_file = _fs_read_file,
    .fs_close = _fs_close,
};


struct fs_context *get_fs_ctx(void)
{
    if(disk_is_valid == false)
        return NULL;
    if(fs_ctx == NULL){
        fs_ctx = fs_create_context();
    }
    return fs_ctx;
}

struct fs_context * fs_create_context(void)
{
    unsigned int len;
    struct fs_context *ctx = calloc(1, sizeof(*ctx));
    if (!ctx){
        printf_err("zalloc error!!\n");
        exit(1);
    }
    ctx->ops = &_fs_ops;
    return ctx;
}

void fs_init(void)
{
    disk_is_valid = _fs_disk_valid();
    if(disk_is_valid == false)
        return;
    fs_ctx = fs_create_context();
    pthread_bmp_init();
    _fs_mkdir(_fs_get_root_dir());
    _fs_dir(_fs_get_root_dir(), NULL);
}

