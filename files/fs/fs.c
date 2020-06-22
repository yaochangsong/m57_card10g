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


static void _thread_data_init(void)
{
    memset(&tp, 0, sizeof(struct _thread_pool));
}
static int timespec_check(struct timespec *t)
{
    if ((t->tv_nsec < 0) || (t->tv_nsec >= 1000000000))
        return -1;
    return 0;
}

static void timespec_sub(struct timespec *t1, struct timespec *t2)
{
    if (timespec_check(t1) < 0) {
        fprintf(stderr, "invalid time #1: %lld.%.9ld.\n",
        	(long long)t1->tv_sec, t1->tv_nsec);
        return;
    }
    if (timespec_check(t2) < 0) {
        fprintf(stderr, "invalid time #2: %lld.%.9ld.\n",
        	(long long)t2->tv_sec, t2->tv_nsec);
        return;
    }
    t1->tv_sec -= t2->tv_sec;
    t1->tv_nsec -= t2->tv_nsec;
    if (t1->tv_nsec >= 1000000000) {
        t1->tv_sec++;
        t1->tv_nsec -= 1000000000;
    } else if (t1->tv_nsec < 0) {
        t1->tv_sec--;
        t1->tv_nsec += 1000000000;
    }
}


static int _write_disk(char *filename, size_t size)
{
    #define     WRITE_UNIT_BYTE 8388608
    void *user_mem = NULL;
    size_t pagesize = 0;
    int rc;
    struct timespec ts_start, ts_end;
    float speed = 0;
    uint32_t total_MB = 0, reminder_MB = 0;
    uint32_t reminder = 0, count;
    
   if(size == 0)
        return -1;
    count = size / WRITE_UNIT_BYTE;
    reminder = size % WRITE_UNIT_BYTE;
    printf_note("filesize:%uByte, count=%u\n", size, count);

    total_MB = (size / (1024 * 1024));
    reminder_MB = (size % (1024 * 1024));
    printf_note(" write:filesize:%luByte, block size=%d, count=%dByte, total=%u.%uMB\n", size, WRITE_UNIT_BYTE, count, total_MB, reminder_MB);
    pagesize=getpagesize();
    posix_memalign((void **)&user_mem, pagesize /*alignment */ , WRITE_UNIT_BYTE + pagesize);
    if (!user_mem) {
        fprintf(stderr, "OOM %lu.\n", pagesize);
        return -1;
    }

    int out_fd = open(filename, O_RDWR | O_CREAT | O_TRUNC | O_DIRECT | O_SYNC, 0666);
    if (out_fd < 0) {
        printf("open %s fail\n", filename);
        return -1;
    }

    rc = clock_gettime(CLOCK_MONOTONIC, &ts_start);
    while (count--)
    {
        rc = write(out_fd, user_mem, WRITE_UNIT_BYTE);
        if (rc < 0){
            perror("write file");
            goto done;
        }
        else if(rc != WRITE_UNIT_BYTE)
        {
         printf("%s, Write 0x%x != 0x%x.\n", filename, rc, WRITE_UNIT_BYTE);
        }
    }
    if(reminder > 0){
        rc = write(out_fd, user_mem, reminder);
        if (rc < 0){
            perror("write file");
            goto done;
        }else if(rc != WRITE_UNIT_BYTE){
            printf("%s, Write 0x%x != 0x%x.\n", filename, rc, WRITE_UNIT_BYTE);
        }
    }
    sync();
    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    timespec_sub(&ts_end, &ts_start);
    float total_t = ts_end.tv_sec + (float)(ts_end.tv_nsec / 1000000000.0);
    fprintf(stdout,"total time: %.5f sec\n", total_t);
    speed = (float)(total_MB / total_t);
    fprintf(stdout,"speed: %.2f MBps\n", speed);
done:
    free(user_mem);
    close(out_fd);
    return 0;
}

static int _read_disk(char *filename, int size)
{
    void *user_mem = NULL;
    size_t pagesize = 0;
    int rc;
    struct timespec ts_start, ts_end;
    float speed = 0;
    uint32_t total_MB = 0;
    struct stat input_stat;
    int count = 0;

    pagesize=getpagesize();
    posix_memalign((void **)&user_mem, pagesize /*alignment */ , size + pagesize);
    if (!user_mem) {
        fprintf(stderr, "OOM %lu.\n", pagesize);
        return -1;
    }

    int in_fd = open(filename, O_RDONLY | O_DIRECT | O_SYNC, 0666);
    if (in_fd < 0) {
        printf("open %s fail\n", filename);
        return -1;
    }

    if (fstat(in_fd, &input_stat) < 0) {
        perror("Unable to get file statistics");
        rc = 1;
        goto done;
    }

    count = input_stat.st_size / size;
    total_MB = input_stat.st_size / (1024 * 1024);
    printf("test read: block size=%d, count=%d, total=%u MB\n", size, count, total_MB);

    rc = clock_gettime(CLOCK_MONOTONIC, &ts_start);
    while (count--)
    {
        rc = read(in_fd, user_mem, size);
        if (rc < 0)
        {
            perror("read file");
            goto done;
        }
        else if(rc != size)
        {
            printf("%s, Write 0x%x != 0x%x.\n", filename, rc, size);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    timespec_sub(&ts_end, &ts_start);
    float total_t = ts_end.tv_sec + (float)(ts_end.tv_nsec / 1000000000.0);
    fprintf(stdout,"total time: %.5f sec\n", total_t);
    speed = (float)(total_MB / total_t);
    fprintf(stdout,"speed: %.2f MBps\n", speed);

done:
    free(user_mem);
    close(in_fd);
    return 0;
}



static bool _fs_disk_valid(void)
{
    #define DISK_NODE_NAME "/dev/nvme0"
    //if(access(DISK_NODE_NAME)){
    //    printf_note("Disk[%s] is not valid\n", DISK_NODE_NAME);
   //     return false
    //}
    return true;
}


static int _fs_format(void)
{
    if(disk_is_valid == false)
        return -1;
    return safe_system("mkfs.ext3    /dev/sda6");
}

static int _fs_mkdir(char *dirname)
{
    if(disk_is_valid == false)
        return -1;
    if(dirname)
        mkdir(dirname, 0751);
    
    return 0;
}

static ssize_t _fs_dir(char *dirname, void **data)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp = opendir(dirname))==NULL){
        perror("opendir error");
        exit(1);
    }
    while((dirp = readdir(dp))!=NULL){
        if((strcmp(dirp->d_name,".")==0)||(strcmp(dirp->d_name,"..")==0))
            continue;
        printf("%6d:%-19s %5s\n",dirp->d_ino,dirp->d_name,(dirp->d_type==DT_DIR)?("(DIR)"):(""));
    }

    closedir(dp);
    return 0;
}


void *_fs_start_save_file_thread(void *arg)
{
    typedef int16_t adc_t;
    adc_t *ptr = NULL;
    ssize_t nread = 0; 
    char filename[256];
    int stateval;
    
    printf_note("start save filename: %s\n", arg);
#if 0
    nread = _ctx->ops.read_adc_data(&ptr);
    if(nread > 0){
        ret = _write_disk(filename, nread);
    }
#endif
    sleep(1);

}

/* act = 1: start save file; act = 0: stop save file */
static ssize_t _fs_save_file(char *filename, int act)
{
    #define     _START_SAVE     1
    #define     _STOP_SAVE      0
    int ret = -1, i, found = 0, cval;

    if(disk_is_valid == false)
        return -1;
    if(filename == NULL)
            return -1;
    
    if(act == _START_SAVE){
        ret = pthread_create_detach_loop(NULL, _fs_start_save_file_thread, filename, filename);
        if(ret != 0){
            perror("pthread cread save file thread!!");
        }
    }else{  /* stop save file */
        pthread_cancel_by_name(filename);
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
    //_fs_mkdir("/run/media/");
}

