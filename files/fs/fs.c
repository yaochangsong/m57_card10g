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
#include "wav.h"
#include "../bsp/io.h"



#define DEV_NAME    "/dev/nvme0n1"
#define MOUNT_DIR   "/run/media/nvme0n1"
#define ROOT_DIR    "/run/media/nvme0n1/data"

struct fs_context *fs_ctx = NULL;
static bool disk_is_valid = false;
static void  *disk_buffer_ptr  = NULL;
volatile bool disk_is_format = false;
volatile int disk_error_num = DISK_CODE_OK;
volatile bool disk_is_full_alert = false;


static inline void _fs_init_(void);
static int _fs_save_exit(void *args);
static ssize_t _fs_stop_save_file(int ch, char *filename);

#define SUPPORT_FS_NOTIFY 1
#define FS_OPS_CHANNEL  MAX_RADIO_CHANNEL_NUM
#define MAX_THREAD_NAME_LEN 64



#define THREAD_FS_BACK_NAME "FS_BACK_FILE"
#define THREAD_FS_SAVE_NAME "FS_SAVE_FILE"


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
        return -1;
    }
    sync();
    return 0;
}

void fs_file_list(char *filename, struct stat *stats, void *arg_array)
{
    cJSON *item = NULL;
    cJSON *array = (cJSON *)arg_array;
    if(stats == NULL || filename == NULL || array == NULL)
        return;
    cJSON_AddItemToArray(array, item = cJSON_CreateObject());
    cJSON_AddStringToObject(item, "filename", filename);
    cJSON_AddNumberToObject(item, "size", (off_t)stats->st_size);
    cJSON_AddNumberToObject(item, "createTime", stats->st_mtime);
}


char *fs_get_root_dir(void)
{
    return ROOT_DIR;
}


bool _fs_disk_valid(void)
{
    if(access(DEV_NAME, F_OK)){
        disk_error_num = DISK_CODE_NOT_FOUND;
        printf_note("SSD Disk [%s] check failed\n", DEV_NAME);
        return false;
    }
    return true;
}

bool _fs_disk_info(struct statfs *diskInfo)
{
    
    if(access(DEV_NAME, F_OK)){
        disk_error_num = DISK_CODE_NOT_FOUND;
        printf_note("SSD Disk [%s] check failed\n", DEV_NAME);
        return false;
    }

    if(statfs(MOUNT_DIR, diskInfo) != 0)
    {
        disk_error_num = DISK_CODE_NOT_FORAMT;
    	printf_note("Disk node[%s] is unknown filesystem\n", MOUNT_DIR);
        return false;
    }

    return true;
}

uint64_t _fs_get_file_size(char *filename)
{
    struct statfs sfs;
    char cmd[256];
    
    if(filename == NULL || strlen(filename) == 0)
        return 0;
    
    snprintf(cmd, sizeof(cmd)-1, "%s/%s", fs_get_root_dir(), filename);
    printf_note("cmd:%s\n", cmd);
    if(statfs(cmd, &sfs) != 0){
        printf_err("file not found: %s\n", cmd);
        return 0;
    }
    printf_note("fs size: %"PRIu64"\n", sfs.f_bsize * sfs.f_blocks);
    return (sfs.f_bsize * sfs.f_blocks);
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

static inline int _fs_find(char *filename,  int (*callback) (char *, struct stat *, void *), void *args)
{
    char path[PATH_MAX];
    int ret = 0;
    DIR *dp;
    struct dirent *dirp;
    struct stat stats;
    char *dirname;
    uint8_t find = 0;
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
        printf_debug("%6ld:%-19s %5s\n",dirp->d_ino,dirp->d_name,(dirp->d_type==DT_DIR)?("(DIR)"):(""));
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
        find = 1;
        break;
    }
    closedir(dp);
        
    if (find)
        ret = 0;
    else
        ret = -1;
    return ret;
}

static ssize_t _fs_dir(char *dirname, void (*callback) (char *,struct stat *, void *), void *args)
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
        printf_debug("%6ld:%-19s %5s\n",dirp->d_ino,dirp->d_name,(dirp->d_type==DT_DIR)?("(DIR)"):(""));
        snprintf(path,PATH_MAX, "%s/%s", dirname, dirp->d_name);
        path[PATH_MAX-1] = 0;
        printf_debug("path:%s, dirp->d_name = %s, PATH_MAX=%d\n", path, dirp->d_name, PATH_MAX);
        if (stat(path, &stats))
            continue;
        if(callback)
            callback(dirp->d_name, &stats, args);
        printf_debug("PATH_MAX:%d, (%s)st_size:%"PRIu64", st_blocks:%lu, st_mode:%x, st_mtime=0x%lx\n", PATH_MAX, dirp->d_name, (off_t)stats.st_size, stats.st_blocks, stats.st_mode, stats.st_mtime);
    }
    closedir(dp);
    return 0;
}

static char *_fs_get_save_thread_name(int ch)
{
    static char name[FS_OPS_CHANNEL][MAX_THREAD_NAME_LEN];
    if(ch >= FS_OPS_CHANNEL)
        return NULL;

    snprintf(name[ch], MAX_THREAD_NAME_LEN, "%s_ch%d", THREAD_FS_SAVE_NAME, ch);
    return name[ch];
}

static char *_fs_get_backtrace_thread_name(int ch)
{
    static char name[FS_OPS_CHANNEL][MAX_THREAD_NAME_LEN];
    if(ch >= FS_OPS_CHANNEL)
        return NULL;

    snprintf(name[ch], MAX_THREAD_NAME_LEN, "%s_ch%d", THREAD_FS_BACK_NAME, ch);
    return name[ch];
}
static int _thread_exit_callback(void *arg){  
    struct timeval beginTime, endTime;
    uint64_t nbyte, filesize = 0;
    float speed = 0;
    double  diff_time_us = 0;
    float diff_time_s = 0;
    int fd = -1, ch;
    char command[256];
    struct push_arg *pargs;
    pargs = (struct push_arg *)arg;
    memcpy(&beginTime, &pargs->ct, sizeof(struct timeval));
    nbyte = pargs->nbyte;
    fd = pargs->fd;
    ch = pargs->ch;
    fprintf(stderr, ">>start time %ld.%.9ld., nbyte=%"PRIu64", fd=%d\n",beginTime.tv_sec, beginTime.tv_usec, nbyte, fd);
    gettimeofday(&endTime, NULL);
    fprintf(stderr, ">>end time %ld.%.9ld.\n",endTime.tv_sec, endTime.tv_usec);
    diff_time_us = difftime_us_val(&beginTime, &endTime);
    diff_time_s = diff_time_us/1000000.0;
    printf(">>diff us=%fus, %fs\n", diff_time_us, diff_time_s);
     speed = (float)((nbyte / (1024 * 1024)) / diff_time_s);
     fprintf(stdout,"speed: %.2f MBps, count=%"PRIu64"\n", speed, pargs->count);
    if(pargs->thread_name && !strcmp(pargs->thread_name, _fs_get_backtrace_thread_name(ch))){
        printf_note("Stop Backtrace, Stop Psd!!!\n");
        io_stop_backtrace_file(&ch);
        executor_set_command(EX_ENABLE_CMD, PSD_MODE_DISABLE, ch, NULL);
    }
    else if(pargs->thread_name && !strcmp(pargs->thread_name,  _fs_get_save_thread_name(ch))) {
    #if defined(SUPPORT_WAV)
        struct spm_context *ctx = get_spm_ctx();
        struct spm_run_parm *run_para = ctx->run_args[pargs->ch];
        uint64_t middle_freq = run_para->m_freq;
        uint32_t sample_rate = io_get_raw_sample_rate(pargs->ch, middle_freq);
        wav_write_header(fd, sample_rate, pargs->split_nbyte);
    #endif
        _fs_save_exit(pargs);
    }

    if(fd > 0)
        close(fd);
    safe_free(pargs->filename);
    if(arg) {
        safe_free(arg);
    }
    return 0;
}  


static inline char *_fs_split_file_rename(void *args)
{
    
    #define FILE_NAME_LEN (256)
    #define _SPLIT_FILE_TAG_STR "_N"
    struct push_arg *p_args;
    char *ptr_tag, *filename;
    char part[8][64];
    char *path;
    int num;
    
    p_args = (struct push_arg *)args;
    path = calloc(1, FILE_NAME_LEN);
    if(path == NULL)
        goto exit;

    filename = p_args->filename;
    ptr_tag = strstr(filename, _SPLIT_FILE_TAG_STR);
    if(ptr_tag == NULL){
        sscanf(filename, "%[^_]_%[^_]_%[^_]_%[^_]_%[^_]_%[^_]", 
            part[0],part[1],part[2],part[3],part[4],part[5]);
        snprintf(path, FILE_NAME_LEN, "%s_%s_%s_%s_%s_TIQ.wav.%d", part[0],part[1],part[2],part[3],part[4], p_args->split_num);
        printf_debug("filename=%s\n",path);
        goto exit;
    }
    sscanf(filename, "%[^_]_%[^_]_%[^_]_%[^_]_%[^_]_%[^_]_%[^_]", 
            part[0],part[1],part[2],part[3],part[4],part[5],part[6]);
    //CH0_D20201218180001034_F70.000M_B80.000M_R409.600M_N0001_TIQ.wav
    snprintf(path, FILE_NAME_LEN, "%s_%s_%s_%s_%s"_SPLIT_FILE_TAG_STR"%04d_%s", part[0],part[1],part[2],part[3],part[4],p_args->split_num+1, part[6]);
    printf_debug("filename=%s\n", path);
    
exit:
    safe_free(p_args->filename);
    return path;
}

static inline int _fs_split_file(void *args)
{
    struct push_arg *p_args;

    p_args = (struct push_arg *)args;
    if((config_get_split_file_threshold() > 0) && (p_args->split_nbyte >= config_get_split_file_threshold())){
        int fd = 0;
        char *ptr = NULL, path[512];
        int offset = 0;
        if(p_args->fd > 0) {
            #if defined(SUPPORT_WAV)
            struct spm_context *ctx = get_spm_ctx();
            struct spm_run_parm *run_para = ctx->run_args[p_args->ch];
            uint64_t middle_freq = run_para->m_freq;
            uint32_t sample_rate = io_get_raw_sample_rate(p_args->ch, middle_freq);
            wav_write_header(p_args->fd, sample_rate, p_args->split_nbyte);
            #endif
            close(p_args->fd);
        }
#if defined(SUPPORT_FS_NOTIFY)
        fs_notifier_add_node_to_list(p_args->ch, p_args->filename, &p_args->notifier);
#endif
        
        p_args->split_num++;
        p_args->split_nbyte = 0;
        p_args->filename = _fs_split_file_rename(p_args);
#if defined(SUPPORT_FS_NOTIFY)
        fs_notifier_create_node(p_args->ch, p_args->filename, &p_args->notifier);
#endif
        snprintf(path, sizeof(path), "%s/%s", fs_get_root_dir(), p_args->filename);
        printf_debug("path:%s\n", path);
        fd = open(path, O_RDWR | O_CREAT | O_TRUNC | O_DIRECT | O_SYNC, 0666);
        if (fd < 0) {
            printf_err("open %s fail\n", path);
            return -1;
        }
        p_args->fd = fd;
#if defined(SUPPORT_WAV)
        wav_write_header_before(fd);
    #if defined(SUPPORT_FS_NOTIFY)
        fs_notifier_update_file_size(&p_args->notifier, 512);
    #endif
        p_args->split_nbyte += 512;
#endif
    }
    return 0;
}

static bool _fs_sample_size_full(struct push_arg *args)
{
    if(args->sampler.sample_policy == FSP_SIZE){
        if(args->nbyte >= args->sampler.args.sample_size){
            printf_note("filename: %s size %"PRIu64" is full,sample_size: %"PRIu64"\n",args->filename, args->nbyte, args->sampler.args.sample_size);
            return true;
        }
    }
    return false;
}
static int _fs_start_save_file_thread(void *arg)
{
    void *ptr = NULL;
    ssize_t nread = 0; 
    int ret = 0, ch;
    struct push_arg *p_args;
    struct spm_context *_ctx;
    p_args = (struct push_arg *)arg;
    _ctx = get_spm_ctx();
    ch = p_args->ch;
    nread = _ctx->ops->read_adc_data(ch, &ptr);
    /* disk is full */
    if(disk_is_full_alert == true || ret == -1){
        if(disk_is_full_alert)
            printf_warn("disk is full, save thread exit\n");
        if(ret == -1)
            printf_warn("write file error, save thread exit\n");
        _ctx->ops->read_adc_over_deal(ch, &nread);
        _fs_stop_save_file(ch, NULL);
        return -1;
    }
    if(nread > 0){
        p_args->nbyte += nread;
        p_args->count ++;
        if((ret = _fs_split_file(p_args)) == 0){
            ret = _write_disk_run(p_args->fd, nread, ptr);
            _ctx->ops->read_adc_over_deal(ch, &nread);
        } 
#if defined(SUPPORT_FS_NOTIFY)
        fs_notifier_update_file_size(&p_args->notifier, nread);
#endif
        p_args->split_nbyte += nread;
        if(_fs_sample_size_full(p_args)){
            _ctx->ops->read_adc_over_deal(ch, &nread);
            _fs_stop_save_file(ch, NULL);
            return -1;
        }
    }else{
        usleep(1000);
        printf("read_adc_data err:%ld\n", nread);
    }
    return (int)nread;

}

static void _fs_sample_time_out(struct uloop_timeout *t)
{
    struct push_arg *p_args = container_of(t, struct push_arg, sampler.timeout);
    if(p_args == NULL)
        return;
    
    _fs_stop_save_file(p_args->ch, NULL);
    printf_note("Save File Stop!sample_time:%d\n", p_args->sampler.args.sample_time);
}

/* 
存储定时器初始化
    若存储策略为定时存储，开始存储定时器 
*/
static int _fs_sample_timer_init(struct push_arg *p_args)
{
    int timer_ms = 0;
    if(p_args == NULL)
        return -1;
    
    struct fs_sampler  *fss = &p_args->sampler;
    fss->timeout.cb = _fs_sample_time_out;
    timer_ms = p_args->sampler.args.sample_time * 1000;
    uloop_timeout_set(&fss->timeout, timer_ms);
    printf_note("Save File: start Timer %d.Sec\n", p_args->sampler.args.sample_time);
    return 0;
}

/* 存储定时器退出 */
static int _fs_sample_timer_exit(void *args)
{
    struct push_arg *p_args = args;
    
    if(p_args == NULL)
        return -1;
    
    struct fs_sampler  *fss = &p_args->sampler;
    if(fss == NULL)
        return -1;
    
    uloop_timeout_cancel(&fss->timeout);
    return 0;
}

static int _fs_save_init(void *args)
{
    struct push_arg *p_args = args;
    
    if(p_args == NULL)
        return -1;
    
#if defined(SUPPORT_FS_NOTIFY)
    p_args->notifier.args = &p_args->sampler;
    fs_notifier_init(p_args->ch, p_args->filename, &p_args->notifier);
#endif
    if(p_args->sampler.sample_policy == FSP_TIMER){
        _fs_sample_timer_init(args);
    }
    
#if defined(SUPPORT_WAV)
    wav_write_header_before(p_args->fd);
    #if defined(SUPPORT_FS_NOTIFY)
    fs_notifier_update_file_size(&p_args->notifier, 512);
    #endif
#endif
    return 0;
}

static int _fs_save_exit(void *args)
{   
     struct push_arg *p_args = args;
     if(p_args == NULL)
        return -1;
     
#if defined(SUPPORT_FS_NOTIFY)
    bool exit_ok;
    if(disk_is_full_alert)
        exit_ok = false;    /* 异常退出 */
    else
        exit_ok = true;
    fs_notifier_exit(p_args->ch,p_args->filename, &p_args->notifier, exit_ok);
#endif
    if(p_args->sampler.sample_policy == FSP_TIMER){
        _fs_sample_timer_exit(args);
    }
    
    return 0;
}

static sample_policy_code _fs_get_sample_policy(struct push_arg *p_args)
{
    if(p_args == NULL)
        return FSP_FREE;

    if(p_args->sampler.args.sample_time > 0)
        return FSP_TIMER;

    if(p_args->sampler.args.sample_size > 0)
        return FSP_SIZE;

    return FSP_FREE;
}

static ssize_t _fs_start_save_file(int ch, char *filename, void *args)
{
    #define     _START_SAVE     1
    #define     _STOP_SAVE      0
    
    int ret = -1;
    static int fd = 0;
    char path[512];
    struct timeval beginTime;
    struct push_arg *p_args;

    if(disk_is_valid == false || disk_is_format == true || disk_is_full_alert == true)
        return -1;
    if(filename == NULL || ch >= MAX_RADIO_CHANNEL_NUM)
        return -1;

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
    p_args->split_num = 0;
    p_args->split_nbyte = 0;
    p_args->fd = fd;
    p_args->ch = ch;
    p_args->thread_name = _fs_get_save_thread_name(ch);
    printf_note("thread name=%s\n", p_args->thread_name);
    p_args->filename = safe_strdup(filename);
    if(args != NULL){
        memcpy(&p_args->sampler.args, args, sizeof(struct fs_save_sample_args));
        printf_note("bindwidth=%u,sample_size=%"PRIu64" sample_time=%u\n", 
                p_args->sampler.args.bindwidth, p_args->sampler.args.sample_size, p_args->sampler.args.sample_time);
    }
    p_args->sampler.sample_policy = _fs_get_sample_policy(p_args);

    io_set_enable_command(ADC_MODE_ENABLE, ch, -1, 0);
    ret =  pthread_create_detach (NULL,_fs_save_init, _fs_start_save_file_thread, _thread_exit_callback,  
                                p_args->thread_name, p_args, p_args);
    if(ret != 0){
        perror("pthread save file thread!!");
        safe_free(p_args->filename);
        safe_free(p_args);
        exit(1);
    }

    return ret;
}

static ssize_t _fs_stop_save_file(int ch, char *filename)
{
    io_set_enable_command(ADC_MODE_DISABLE, ch, -1, 0);
    printf_note("stop save file : %s\n",  _fs_get_save_thread_name(ch));
    pthread_cancel_by_name(_fs_get_save_thread_name(ch));
    return 0;
}

static int _fs_start_read_raw_file_loop(void *arg)
{
    ssize_t nread = 0; 
    int ch;
    struct push_arg *p_args;
    p_args = arg;
    ch = p_args->ch;
    nread = get_spm_ctx()->ops->back_running_file(ch, STREAM_ADC_WRITE, p_args->fd);
    p_args->count ++;
    if(nread > 0){
         p_args->nbyte += nread;
    }

    if(nread == 0){
        printf_note(">>>>>>>read over!!\n");
        pthread_exit_by_name(_fs_get_backtrace_thread_name(ch));
    }
    return (int)nread;
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


static ssize_t _fs_start_read_raw_file(int ch, char *filename)
{
    int ret = -1;
    static int file_fd;
    char path[512];
    uint32_t band;
    uint64_t freq;
    struct timeval beginTime;
    if(disk_is_valid == false || disk_is_format == true)
        return -1;

    if(filename == NULL || ch >= MAX_RADIO_CHANNEL_NUM)
        return -1;
    snprintf(path, sizeof(path), "%s/%s", fs_get_root_dir(), filename);
    file_fd = open(path, O_RDONLY | O_DIRECT | O_SYNC, 0666);
    if (file_fd < 0) {
        printf_err("open %s fail\n", path);
        return -1;
    } 
    printf_note("start read file: %s, file_fd=%d\n", path, file_fd);
    
    struct push_arg *p_args;
    p_args = safe_malloc(sizeof(struct push_arg));
    gettimeofday(&beginTime, NULL);
    fprintf(stderr, "#######start time %ld.%.9ld.\n",beginTime.tv_sec, beginTime.tv_usec);
   // p_args->ct = &beginTime;
    memcpy(&p_args->ct, &beginTime, sizeof(struct timeval));
    p_args->nbyte = 0;
    p_args->count = 0;
    p_args->fd = file_fd;
    p_args->ch = ch;
    p_args->thread_name = _fs_get_backtrace_thread_name(ch);
#if defined(SUPPORT_WAV)
    wav_backtrace_before(file_fd);
#endif
    io_start_backtrace_file(&ch);
    ret =  pthread_create_detach (NULL,NULL, _fs_start_read_raw_file_loop, _thread_exit_callback,  
                                p_args->thread_name , p_args, p_args);
    if(ret != 0){
        perror("pthread read file thread!!");
        safe_free(p_args);
        io_stop_backtrace_file(&ch);
    }

    return 0;
}

static ssize_t _fs_stop_read_raw_file(int ch, char *filename)
{
    if(disk_is_valid == false || ch >= MAX_RADIO_CHANNEL_NUM)
        return -1;
    
    pthread_cancel_by_name(_fs_get_backtrace_thread_name(ch));
    io_stop_backtrace_file(&ch);
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

void *fs_disk_check_thread(void *args)
{
    #define _SLOW_CHECK_TIME_INTERVAL_US  (5000000)
    #define _FAST_CHECK_TIME_INTERVAL_US  (100000)
    
    struct statfs sfs;
    bool disk_check = false;
    int check_time = _SLOW_CHECK_TIME_INTERVAL_US;
    uint64_t threshold_byte = 0, free_byte = 0;
    pthread_detach(pthread_self());
    sleep(1);
    
    while(1){
        do{
            disk_check = _fs_disk_info(&sfs);
            if(disk_check == false){
                if(disk_error_num == DISK_CODE_NOT_FOUND){
                    akt_send_alert(1);
                    //send error
                } else if(disk_error_num == DISK_CODE_NOT_FORAMT){
                    akt_send_alert(2);
                    //send format
                }
                usleep(check_time);
            }
        }while(disk_check == false);
            
        _fs_init_();
        /* now disk is ok */
        if((threshold_byte = config_get_disk_alert_threshold()) > 0){
            free_byte = sfs.f_bsize * sfs.f_bfree;
            if(threshold_byte >= free_byte){
                printf_note("%"PRIu64" >= threshold_byte[%"PRIu64"], send alert to client\n", free_byte, threshold_byte);
                disk_is_full_alert = true;
                akt_send_alert(0);
            }else {
                printf_debug("%"PRIu64" < threshold_byte[%"PRIu64"], disk not full\n", free_byte, threshold_byte);
                disk_is_full_alert =  false;
            }
        }
        check_time = _SLOW_CHECK_TIME_INTERVAL_US;
        for(int i = 0; i <FS_OPS_CHANNEL ; i++){
            if(pthread_check_alive_by_name(_fs_get_save_thread_name(i)) == true){
                check_time = _FAST_CHECK_TIME_INTERVAL_US;
            }
        }
        usleep(check_time);
    }
}

int fs_disk_start_check(void)
{
    int ret;
    pthread_t work_id;
    ret=pthread_create(&work_id, NULL, fs_disk_check_thread, NULL);
    if(ret!=0){
        perror("pthread cread check");
        return -1;
    }
    return 0;
}

static inline void _fs_init_(void) 
{
    static bool fs_has_inited = false;
    
    if(fs_has_inited == true)
        return;
    fs_ctx = fs_create_context();
    pthread_bmp_init();
    _fs_mkdir(fs_get_root_dir());
    _fs_dir(fs_get_root_dir(), NULL, NULL);
    fs_has_inited = true;
}


void fs_init(void)
{
    fs_disk_start_check();
    disk_is_valid = _fs_disk_valid();
    if(disk_is_valid == false)
        return;
    _fs_init_();
}

