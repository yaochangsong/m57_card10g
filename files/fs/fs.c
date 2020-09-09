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


#define DEV_NAME    "/dev/nvme0n1"
#define MOUNT_DIR   "/run/media/nvme0n1"
#define ROOT_DIR    "/run/media/nvme0n1/data"

struct fs_context *fs_ctx = NULL;
static bool disk_is_valid = false;
static void  *disk_buffer_ptr  = NULL;
volatile bool disk_is_format = false;
volatile int disk_error_num = DISK_CODE_OK;

#define THREAD_FS_BACK_NAME "FS_BACK_FILE"
#define THREAD_NAME         "FS_SAVE_FILE"

struct push_arg{
    struct timeval ct;
    uint64_t nbyte;
    uint64_t count;
    int  fd;
    char *name;
};
static double difftime_us_val(const struct timeval *start, const struct timeval *end)
{
    double d;
    time_t s;
    suseconds_t u;
    s = end->tv_sec - start->tv_sec;
    u = end->tv_usec - start->tv_usec;
    d = s;
    d *= 1000000.0;//1 �� = 10^6 ΢��
    d += u;
    return d;
}
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
    //posix_memalign((void **)&disk_buffer_ptr, 4096 /*alignment */ , DMA_BUFFER_SIZE + 4096);
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
    
    rc = write(fd, pdata, size);
    if (rc < 0){
        perror("write file");
    }
    sync();
    return 0;
}


char *fs_get_root_dir(void)
{
    return ROOT_DIR;
}


bool _fs_disk_valid(void)
{
    if(access(MOUNT_DIR, F_OK)){
        disk_error_num = DISK_CODE_NOT_FOUND;
        printf_note("Disk node[%s] is not valid\n", MOUNT_DIR);
        return false;
    }
    return true;
}

bool _fs_disk_info(struct statfs *diskInfo)
{
    
    if(access(MOUNT_DIR, F_OK)){
        disk_error_num = DISK_CODE_NOT_FOUND;
        printf_note("Disk node[%s] is not valid\n", MOUNT_DIR);
        return false;
    }

	if(statfs(MOUNT_DIR, diskInfo))
	{
        disk_error_num = DISK_CODE_NOT_FORAMT;
		printf_note("Disk node[%s] is unknown filesystem\n", MOUNT_DIR);
        return false;
	}
    
    return true;
}

static disk_err_code _fs_get_err_code(void)
{
    return disk_error_num;
}

static inline int _fs_format(void)
{
    char cmd[128];
	int ret, err_no = 0;
	disk_error_num = DISK_CODE_FORAMT;
    if(disk_is_format){
        printf_warn("disk is format now\n");
        return DISK_CODE_FORAMT;
    }
        
    disk_is_format = true;
    snprintf(cmd, sizeof(cmd), "/etc/disk.sh format");
    ret = safe_system(cmd);
    if(!ret){
        err_no++;
        printf_err("format failed\n");
    } 
    disk_is_format = false;
    if(err_no > 0)
        disk_error_num = DISK_CODE_ERR;
    else
        disk_error_num = DISK_CODE_OK;
    return ret;
}

static inline int _fs_mkdir(char *dirname)
{
    if(disk_is_valid == false || disk_is_format == true)
        return -1;
    if(dirname && access(dirname, F_OK)){
        printf("mkdir %s\n", dirname);
        mkdir(dirname, 0755);
    }
    
    return 0;
}

static inline int _fs_delete(char *filename)
{
    char path[PATH_MAX];
    int ret = 0;
    
    if(disk_is_valid == false || disk_is_format == true)
        return -1;

    if(filename == NULL)
        return -1;
    
    sprintf(path, "%s/%s", fs_get_root_dir(), filename);
    path[PATH_MAX] = 0;

    if(remove(path) == 0 ){
        printf_note("Removed %s.\n", path);
    }
    else{
        perror("remove");
        ret = -1;
    }
        
    return ret;
}

static inline int _fs_find(char *filename,  int (*callback) (char *,void *, void *), void *args)
{
    char path[PATH_MAX];
    int ret = 0;
    DIR *dp;
    struct dirent *dirp;
    struct stat stats;
    char *dirname;
    
    if(disk_is_valid == false || disk_is_format == true)
        return -1;

    if(filename == NULL)
        return -1;

    dirname = fs_get_root_dir();
    if((dp = opendir(dirname))==NULL){
        perror("opendir error");
        return -1;
    }
    while((dirp = readdir(dp))!=NULL){
        if((strcmp(dirp->d_name,".")==0)||(strcmp(dirp->d_name,"..")==0))
            continue;
        printf_debug("%6d:%-19s %5s\n",dirp->d_ino,dirp->d_name,(dirp->d_type==DT_DIR)?("(DIR)"):(""));
        if(strncmp(filename, dirp->d_name, strlen(dirp->d_name))){
            continue;
        }
        snprintf(path,PATH_MAX, "%s/%s", dirname, dirp->d_name);
        path[PATH_MAX-1] = 0;
        printf_debug("path=%s,d_name=%s,PATH_MAX=%d\n", path, dirp->d_name, PATH_MAX);
        if (stat(path, &stats))
            continue;
        if(callback)
            callback(dirp->d_name, &stats, args);
        //printf_note("PATH_MAX:%d, (%s)st_size:%llu, st_blocks:%u, st_mode:%x, st_mtime=0x%x\n", PATH_MAX, dirp->d_name, (off_t)stats.st_size, stats.st_blocks, stats.st_mode, stats.st_mtime);
        break;
    }
    closedir(dp);
        
    return ret;
}

static ssize_t _fs_dir(char *dirname, int (*callback) (char *,void *, void *), void *args)
{
    DIR *dp;
    struct dirent *dirp;
    struct stat stats;
    char path[PATH_MAX];
    if(disk_is_valid == false || disk_is_format == true)
        return -1;
    if(dirname == NULL)
        dirname = fs_get_root_dir();
    if((dp = opendir(dirname))==NULL){
        perror("opendir error");
        return -1;
    }
    while((dirp = readdir(dp))!=NULL){
        if((strcmp(dirp->d_name,".")==0)||(strcmp(dirp->d_name,"..")==0))
            continue;
        printf_debug("%6d:%-19s %5s\n",dirp->d_ino,dirp->d_name,(dirp->d_type==DT_DIR)?("(DIR)"):(""));
        snprintf(path,PATH_MAX, "%s/%s", dirname, dirp->d_name);
        path[PATH_MAX-1] = 0;
        printf_debug("path:%s, dirp->d_name = %s, PATH_MAX=%d\n", path, dirp->d_name, PATH_MAX);
        if (stat(path, &stats))
            continue;
        if(callback)
            callback(dirp->d_name, &stats, args);
        printf_debug("PATH_MAX:%d, (%s)st_size:%llu, st_blocks:%u, st_mode:%x, st_mtime=0x%x\n", PATH_MAX, dirp->d_name, (off_t)stats.st_size, stats.st_blocks, stats.st_mode, stats.st_mtime);
    }
    closedir(dp);
    return 0;
}

static void _thread_exit_callback(void *arg){  
    struct timeval beginTime, endTime;
    uint64_t nbyte;
    float speed = 0;
    double  diff_time_us = 0;
    float diff_time_s = 0;
    int fd = -1;
    struct push_arg *pargs;
    pargs = (struct push_arg *)arg;
    memcpy(&beginTime, &pargs->ct, sizeof(struct timeval));
    nbyte = pargs->nbyte;
    fd = pargs->fd;
    fprintf(stderr, ">>start time %ld.%.9ld., nbyte=%llu, fd=%d\n",beginTime.tv_sec, beginTime.tv_usec, nbyte, fd);
    gettimeofday(&endTime, NULL);
    fprintf(stderr, ">>end time %ld.%.9ld.\n",endTime.tv_sec, endTime.tv_usec);
    diff_time_us = difftime_us_val(&beginTime, &endTime);
    diff_time_s = diff_time_us/1000000.0;
    printf(">>diff us=%fus, %fs\n", diff_time_us, diff_time_s);
     speed = (float)((nbyte / (1024 * 1024)) / diff_time_s);
     fprintf(stdout,"speed: %.2f MBps, count=%llu\n", speed, pargs->count);
    if(pargs->name && !strcmp(pargs->name, THREAD_FS_BACK_NAME)){
        printf_note("Stop Backtrace, Stop Psd!!!\n");
        io_stop_backtrace_file(NULL);
        executor_set_command(EX_ENABLE_CMD, PSD_MODE_DISABLE, 0, NULL);
    }
    if(fd > 0)
        close(fd);
    if(arg)
        safe_free(arg);

}  
static int _fs_start_save_file_thread(void *arg)
{
    typedef int16_t adc_t;
    adc_t *ptr = NULL;
    ssize_t nread = 0; 
    int ret;
    struct push_arg *p_args;
    struct spm_context *_ctx;
    p_args = (struct push_arg *)arg;
    _ctx = get_spm_ctx();
    nread = _ctx->ops->read_adc_data(&ptr);
    if(nread > 0){
        p_args->nbyte += nread;
        p_args->count ++;
        ret = _write_disk_run(p_args->fd, nread, ptr);
        _ctx->ops->read_adc_over_deal(&nread);
    }else{
        usleep(1000);
        printf("read_adc_data err:%d\n", nread);
    }
    return nread;

}

static ssize_t _fs_start_save_file(char *filename)
{
    #define     _START_SAVE     1
    #define     _STOP_SAVE      0
    int ret = -1;
    static int fd = 0;
    char path[512];
    struct timeval beginTime;
    struct push_arg *p_args;
    
    if(disk_is_valid == false || disk_is_format == true)
        return -1;
    if(filename == NULL)
        return -1;
    
    io_set_enable_command(ADC_MODE_ENABLE, -1, -1, 0);
    snprintf(path, sizeof(path), "%s/%s", fs_get_root_dir(), filename);
    printf("start save file: %s\n", path);
        
    if(path == NULL)
        return -1;
    fd = open(path, O_RDWR | O_CREAT | O_TRUNC | O_DIRECT | O_SYNC, 0666);
    if (fd < 0) {
        printf_err("open %s fail\n", path);
        return -1;
    }
    printf_note("fd = %d\n", fd);
    p_args = safe_malloc(sizeof(struct push_arg));
    gettimeofday(&beginTime, NULL);
    fprintf(stderr, "#######start time %ld.%.9ld.\n",beginTime.tv_sec, beginTime.tv_usec);
    memcpy(&p_args->ct, &beginTime, sizeof(struct timeval));
    p_args->nbyte = 0;
    p_args->count = 0;
    p_args->fd = fd;
    p_args->name=THREAD_NAME;
    ret =  pthread_create_detach (NULL, _fs_start_save_file_thread, _thread_exit_callback,  
                                THREAD_NAME, p_args, p_args);
    if(ret != 0){
        perror("pthread save file thread!!");
        safe_free(p_args);
        exit(1);
    }

    return ret;
}

static ssize_t _fs_stop_save_file(char *filename)
{
    io_set_enable_command(ADC_MODE_DISABLE, -1, -1, 0);
    printf("stop save file : %s\n", THREAD_NAME);
    pthread_cancel_by_name(THREAD_NAME);
}

static ssize_t _fs_start_read_raw_file_loop(void *arg)
{
    ssize_t nread = 0; 
    struct push_arg *p_args;
    p_args = arg;
    
    nread = get_spm_ctx()->ops->back_running_file(STREAM_ADC_WRITE, p_args->fd);
    p_args->count ++;
    if(nread > 0){
         p_args->nbyte += nread;
    }

    if(nread == 0){
        printf_note(">>>>>>>read over!!\n");
        pthread_exit_by_name(THREAD_FS_BACK_NAME);
    }
    return nread;
}

static void _fs_read_exit_callback(void *arg){  
    struct push_arg *p_args;
    p_args = (struct push_arg *)arg;
    
    printf_note("fs read exit, fd= %d\n", p_args->fd);
    if(p_args->fd > 0)
        close(p_args->fd);
    if(arg)
        safe_free(arg);
}


static ssize_t _fs_start_read_raw_file(char *filename)
{
    int ret = -1;
    static int file_fd;
    char path[512];
    uint32_t band;
    uint64_t freq;
    int ch = 0;
    struct timeval beginTime;
    if(disk_is_valid == false || disk_is_format == true)
        return -1;

    if(filename == NULL)
        return -1;
    snprintf(path, sizeof(path), "%s/%s", fs_get_root_dir(), filename);
    file_fd = open(path, O_RDONLY | O_DIRECT | O_SYNC, 0666);
    if (file_fd < 0) {
        printf_err("open % fail\n", path);
        return -1;
    } 
    printf_note("start read file: %s, file_fd=%d\n", path, file_fd);
    io_start_backtrace_file(NULL);
    struct push_arg *p_args;
    p_args = safe_malloc(sizeof(struct push_arg));
    gettimeofday(&beginTime, NULL);
    fprintf(stderr, "#######start time %ld.%.9ld.\n",beginTime.tv_sec, beginTime.tv_usec);
   // p_args->ct = &beginTime;
    memcpy(&p_args->ct, &beginTime, sizeof(struct timeval));
    p_args->nbyte = 0;
    p_args->count = 0;
    p_args->fd= file_fd;
    p_args->name=THREAD_FS_BACK_NAME;
    ret =  pthread_create_detach (NULL, _fs_start_read_raw_file_loop, _thread_exit_callback,  
                                THREAD_FS_BACK_NAME, p_args, p_args);
    if(ret != 0){
        perror("pthread read file thread!!");
        safe_free(p_args);
        io_stop_backtrace_file(NULL);
    }

    return 0;
}

static ssize_t _fs_stop_read_raw_file(char *filename)
{
    if(disk_is_valid == false)
        return -1;
    pthread_cancel_by_name(THREAD_FS_BACK_NAME);
    io_stop_backtrace_file(NULL);
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
    .fs_disk_info = _fs_disk_info,
    .fs_disk_valid = _fs_disk_valid,
    .fs_format = _fs_format,
    .fs_mkdir = _fs_mkdir,
    .fs_dir = _fs_dir,
    .fs_delete = _fs_delete,
    .fs_find = _fs_find,
    .fs_start_save_file = _fs_start_save_file,
    .fs_stop_save_file = _fs_stop_save_file,
    .fs_start_read_raw_file = _fs_start_read_raw_file,
    .fs_stop_read_raw_file = _fs_stop_read_raw_file,
	.fs_get_err_code = _fs_get_err_code,
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

struct fs_context *get_fs_ctx_ex(void)
{
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
    _fs_mkdir(fs_get_root_dir());
    _fs_dir(fs_get_root_dir(), NULL, NULL);
}

