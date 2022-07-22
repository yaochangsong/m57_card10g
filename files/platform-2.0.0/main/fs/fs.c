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
#include <sys/xattr.h>
#include "fs.h"
#include "../log/log.h"
#include "../utils/utils.h"
#include "../executor/spm/spm.h"
#include "config.h"
#include "wav.h"
#include "../bsp/io.h"
#include "../protocol/resetful/data_frame.h"




#define DEV_NAME    "/dev/nvme0n1"
#define MOUNT_DIR   "/run/media/nvme0n1"
#if defined(CONFIG_ARCH_ARM)
#define ROOT_DIR    "/run/media/nvme0n1/data"
#else
#define ROOT_DIR    "/home"
#endif

struct fs_context *fs_ctx = NULL;
static bool disk_is_valid = false;
static void  *disk_buffer_ptr  = NULL;
volatile bool disk_is_format = false;
volatile int disk_error_num = DISK_CODE_OK;
volatile bool disk_is_full_alert = false;


static inline void _fs_init_(void);
static int _fs_save_exit(void *args);
static ssize_t _fs_stop_save_file(int ch, char *filename, struct fs_context *ctx);
static int _fs_create_xattr(int ch, const char *path, void *args);
static int _fs_exit(struct fs_context *ctx);
static int _fs_read_exit(void *args);



#define MAX_THREAD_NAME_LEN 64

static const char *const fs_ops_thread_table[] = {
    [FS_OPS_INDEX_SAVE] = "FS_SAVE_FILE",
    [FS_OPS_INDEX_BACK] = "FS_BACK_FILE",
};


static double difftime_us_val(const struct timeval *start, const struct timeval *end)
{
    double d;
    time_t s;
    suseconds_t u;
    s = end->tv_sec - start->tv_sec;
    u = end->tv_usec - start->tv_usec;
    d = s;
    d *= 1000000.0;
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

static int _fs_get_filesize(const char *filepath, size_t *size)
{
    char *dirname;
    char path[PATH_MAX] = {0};
    struct stat stats;
    
    if(disk_is_valid == false || disk_is_format == true)
        return -1;

    if(filepath == NULL)
        return -1;

    dirname = fs_get_root_dir();
    snprintf(path,PATH_MAX, "%s/%s", dirname, filepath);
    if(stat(path, &stats) < 0){
        printf_warn("invalid path: %s\n", path);
        return -1;
    }
    if(size)
        *size = stats.st_size;
    return 0;
}

/* 当前目录查找文件 */
static inline int _fs_find(const char *dir, const char *filename,  void (*callback) (char *,struct stat *, void *, void *), void *args)
{
    char path[PATH_MAX];
    int ret = 0;
    DIR *dp;
    struct dirent *dirp;
    struct stat stats;
    uint8_t find = 0;
    if(disk_is_valid == false || disk_is_format == true)
        return -1;

    if(filename == NULL)
        return -1;
    
    if(chdir(fs_get_root_dir()) == -1){
        printf_warn("dir: %s error\n", fs_get_root_dir());
        return -1;
    }
    if(dir == NULL || !strcmp(dir, "/")){
        dir = ".";
    }

    if((dp = opendir(dir))==NULL){
        printf("%s\n", dir);
        perror("opendir error");
        return -1;
    }
    while((dirp = readdir(dp))!=NULL){
        if(!(strncmp(dirp->d_name,".", 1))||(!strncmp(dirp->d_name, "..", 2)))
            continue;
        printf_debug("%6ld:%-19s %5s\n",dirp->d_ino,dirp->d_name,(dirp->d_type==DT_DIR)?("(DIR)"):(""));
        if(strncmp(filename, dirp->d_name, strlen(filename))){
            continue;
        }
        snprintf(path,PATH_MAX - 1, "%s/%s", dir, dirp->d_name);
        path[PATH_MAX-1] = 0;
        printf_debug("path=%s,d_name=%s,PATH_MAX=%d\n", path, dirp->d_name, PATH_MAX);
        if (stat(path, &stats))
            continue;
        if(callback)
            callback(path, &stats, args, dp);
            //callback(dirp->d_name, &stats, args);
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

/* 递归查找 */
static int _fs_find_rec(char *path, char *filename,  void (*callback) (char *,struct stat *, void *, void *), void *args)
{
    DIR *d = NULL;
    struct dirent *dp = NULL;
    struct stat st;
    char p[PATH_MAX] = {0};
    
    if(disk_is_valid == false || disk_is_format == true)
        return -1;
    
    memset(p, 0 , sizeof(p));
    if(path == NULL || !strcmp(path, "/")){
        path = ".";
    }

    if(chdir(fs_get_root_dir()) == -1){
        printf_warn("dir: %s error\n", fs_get_root_dir());
        return -1;
    }
    if(stat(path, &st) < 0 || !S_ISDIR(st.st_mode)){
        printf_warn("invalid path: %s\n", path);
        return -1;
    }

    if(!(d = opendir(path))){
        printf_err("opendir: %s error\n", path);
        return -1;
    }
    while((dp = readdir(d)) != NULL){
        if(!(strncmp(dp->d_name,".", 1))||(!strncmp(dp->d_name, "..", 2)))
            continue;

        snprintf(p, sizeof(p) - 1, "%s/%s", path, dp->d_name);
        stat(p, &st);
        if(!S_ISDIR(st.st_mode)){
            if(!strcmp(dp->d_name, filename)){
                if(callback)
                    callback(p, &st, args, dp);
                //break;
            }
        } else{
            _fs_find_rec(p, filename, callback, args);
        }
    }

    closedir(d);
    return 0;
}


static ssize_t _fs_list(char *dirname, void (*callback) (char *,struct stat *, void *), void *args)
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

static ssize_t _fs_dir(char *dirname, void (*callback) (char *,struct stat *, void *, void *), void *args)
{
    DIR *dp;
    struct dirent *dirp;
    struct stat stats;
    char path[PATH_MAX];
    char dir[PATH_MAX];
    if(disk_is_valid == false || disk_is_format == true)
        return -1;

    if(chdir(fs_get_root_dir()) == -1){
        printf_warn("dir: %s error\n", fs_get_root_dir());
        return -1;
    }
    
    if(dirname == NULL || !strcmp(dirname, "/"))
        dirname = ".";
    
    printf_note("dirname=%s\n", dirname);
    if((dp = opendir(dirname))==NULL){
        perror("opendir error");
        return -1;
    }
    while((dirp = readdir(dp))!=NULL){
        if(!(strncmp(dirp->d_name,".", 1))||(!strncmp(dirp->d_name, "..", 2))){
                continue;
        }
        printf_debug("%6ld:%-19s %5s\n",dirp->d_ino,dirp->d_name,(dirp->d_type==DT_DIR)?("(DIR)"):(""));
        snprintf(path,PATH_MAX-1, "%s/%s", dirname, dirp->d_name);
        path[PATH_MAX-1] = 0;
        printf_debug("path:%s, dirp->d_name = %s, PATH_MAX=%d\n", path, dirp->d_name, PATH_MAX);
        if (stat(path, &stats))
            continue;
        if(callback)
            callback(path, &stats, args, dirp);
        printf_debug("PATH_MAX:%d, (%s)st_size:%"PRIu64", st_blocks:%lu, st_mode:%x, st_mtime=0x%lx\n", PATH_MAX, dirp->d_name, (off_t)stats.st_size, stats.st_blocks, stats.st_mode, stats.st_mtime);
    }
    closedir(dp);
    return 0;
}


static int _fs_get_default_dirname(int ch, void *args, char *dirname, size_t dirlen)
{
#if defined(FS_FILENAME_RULE_TF713)
    time_t now ;
    struct tm *tm_now ;
    time(&now) ;
    tm_now = localtime(&now) ;
    snprintf(dirname, dirlen -1,"%04d%02d%02d",\
        tm_now->tm_year+1900, tm_now->tm_mon+1, tm_now->tm_mday);
    return 0;
#else
    return -1;
#endif
}

static int _fs_get_default_filename(int ch, void *args, char *filename, size_t len)
{
#if defined(FS_FILENAME_RULE_TF713)
    /*20170725083129IQ******0001_F1350000000_B0062500000_S0200000000_1_1.dat*/
    char path1[NAME_MAX];
    time_t now ;
    struct tm *tm_now ;
    char *device_code = config_get_devicecode();
    uint8_t ddc_ch = 1;
    uint32_t bw = 0;
    uint64_t m_freq = 0;
    uint64_t sample_rate = 0;
#ifdef CONFIG_BSP_DEMO
        m_freq = MHZ(1000);
        bw = MHZ(120);
        sample_rate = io_get_raw_sample_rate(ch, m_freq, bw);
#else
        struct spm_context *_ctx = get_spm_ctx();
        if(_ctx){
            bw = _ctx->pdata->channel[ch].multi_freq_point_param.ddc.bandwidth;
            m_freq = _ctx->pdata->channel[ch].multi_freq_point_param.ddc.middle_freq;
            sample_rate = io_get_raw_sample_rate(ch, m_freq, bw);
        }
#endif

    time(&now) ;
    tm_now = localtime(&now) ;
    snprintf(path1, sizeof(path1) -1,"%04d%02d%02d%02d%02d%02d",
        tm_now->tm_year+1900, tm_now->tm_mon+1, tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);

    snprintf(filename, len -1, "%sIQ%s_F%010"PRIu64"_B%010"PRIu32"_S%010"PRIu64"_%02d_%02d.dat", \
            path1, device_code, m_freq, bw, sample_rate, ch, ddc_ch);
    return 0;
#else
    return -1;
#endif
}

static inline int _fs_create_dir(char *dirname)
{
    int ret = -1;
    char path[PATH_MAX];

    if(dirname && strlen(dirname) > 0){
        snprintf(path, sizeof(path) - 1, "%s/%s", fs_get_root_dir(), dirname);
        ret = _fs_mkdir(path);
    }

    return ret;
}

static int _fs_create_default_path(int ch, char *path, size_t path_len, void *args)
{
    char dir[NAME_MAX], filename[NAME_MAX];
    int ret = 0;
    
    if(path == NULL)
        return -1;
    
    ret = _fs_get_default_dirname(ch, args, dir, sizeof(dir));
    if(ret != 0)
        return -1;
    printf_note("dir: %s\n", dir);
    
    ret = _fs_create_dir(dir);
    if(ret != 0)
        return -1;
    
    ret = _fs_get_default_filename(ch, args, filename, sizeof(filename));
    if(ret != 0)
        return -1;
    printf_note("filename: %s\n", filename);
    
    snprintf(path, path_len -1, "%s/%s", dir, filename);
    printf_note("path: %s\n", path);
    
    return 0;
}
static int _fs_download_over_callback(const char * filepath)
{
    int ret = 0, status = 1;
    char path[PATH_MAX+1];
    snprintf(path, PATH_MAX-1, "%s/%s", fs_get_root_dir(), filepath);
    ret = setxattr(path, _FS_EX_ATTR_DOWNLOAD_KEY, &status, 4, 0);
    printf_note("path:%s, ret=%d\n", path, ret);
    if(ret != 0)
        return -1;
    else
        return 0;
}

static int _fs_get_download_status(const char *path)
{
    int ret = -1;
    int val = 0;
    if(path == NULL || strlen(path) == 0)
        return -1;
    ret = getxattr(path, _FS_EX_ATTR_DOWNLOAD_KEY, &val, 4);
    if(ret == -1)
        val = 0;
    printf_debug("%s, %d， %d\n", path,  val, ret);
    return val;
}


static int _fs_set_backtrace_status(char *filepath)
{
    char path[PATH_MAX+1];
    int ret = -1;
    int val = 0, status = 1;
    if(filepath == NULL || strlen(filepath) == 0)
        return -1;
    snprintf(path, PATH_MAX-1, "%s/%s", fs_get_root_dir(), filepath);
    ret = setxattr(path, _FS_EX_ATTR_BACKTRACE_KEY, &status, 4, 0);
    printf_note("path:%s, ret=%d\n", path, ret);
    if(ret == -1)
         return -1;
    return 0;
}

static int _fs_get_backtrace_status(const char *path)
{
    int ret = -1;
    int val = 0;
    
    if(path == NULL || strlen(path) == 0)
        return -1;
    ret = getxattr(path, _FS_EX_ATTR_BACKTRACE_KEY, &val, 4);
    if(ret == -1)
        val = 0;
    printf_debug("%s, %d， %d\n", path,  val, ret);
    return val;
}

static int _fs_get_file_status(const char *path)
{
    /*
    0：正常状态
    1：文件已下载
    2：文件已回溯
    3：文件下载且已回溯
    */
    int status = 0, download = 0, back = 0;
    download = (_fs_get_download_status(path) == 0 ? 0 : 1);
    back = (_fs_get_backtrace_status(path) == 0 ? 0 : 1) << 1;
    status = download|back;
    printf_debug("download=%d, back=%d, status=%d\n", download, back, status);
    return status;
}

static int _fs_set_param_xattr(const char *path, const char *name, void *val, size_t size)
{
    int ret = -1;

    if(path == NULL || strlen(path) == 0)
        return -1;
    
    ret = setxattr(path, name, val, size, 0);
    printf_note("path:%s, name=%s, ret=%d, size=%ld\n", path, name,  ret, size);
    if(ret == -1)
         return -1;
    return 0;
}

static int _fs_get_param_xattr(const char *path, const char *name, void *val, size_t size)
{
    char fpath[PATH_MAX+1];
    int ret = -1;

    if(path == NULL || strlen(path) == 0)
        return -1;
    snprintf(fpath, PATH_MAX-1, "%s/%s", fs_get_root_dir(), path);
    ret = getxattr(fpath, name, val, size);
    printf_note("%s, %d\n", fpath, ret);
    return ret;
}

/* 
    @path: absolute path
*/
static int _fs_create_xattr(int ch, const char *path, void *args)
{
    uint32_t bw = 0;
    uint64_t m_freq = 0;
    uint64_t sample_rate = 0;
#ifdef CONFIG_BSP_DEMO
    m_freq = MHZ(1000);
    bw = MHZ(120);
    sample_rate = io_get_raw_sample_rate(ch, m_freq, bw);
#else
    struct spm_context *_ctx = get_spm_ctx();
    if(_ctx){
        bw = _ctx->pdata->channel[ch].multi_freq_point_param.ddc.bandwidth;
        m_freq = _ctx->pdata->channel[ch].multi_freq_point_param.ddc.middle_freq;
        sample_rate = io_get_raw_sample_rate(ch, m_freq, bw);
    }
#endif
    _fs_set_param_xattr(path, _FS_EX_ATTR_PARAM_FREQ_KEY, &m_freq,     sizeof(m_freq));
    _fs_set_param_xattr(path, _FS_EX_ATTR_PARAM_BW_KEY  , &bw,  sizeof(bw));
    _fs_set_param_xattr(path, _FS_EX_ATTR_PARAM_SAMPLE_RATE_KEY  , &sample_rate,  sizeof(sample_rate));
    return 0;
}

/* 
    @ch: channel
    @path: relative path
    @name: attr name
    @val:  attr value
    @size: attr value size length
*/
static void *_fs_get_xattr(int ch, const char *path, char *name, void *val, size_t size)
{
    char *attr_name = NULL;
    if(!strcmp(name, "midFreq")){
        attr_name = _FS_EX_ATTR_PARAM_FREQ_KEY;
    }
    else if(!strcmp(name, "bandwidth")){
        attr_name = _FS_EX_ATTR_PARAM_BW_KEY;
    }
    if(_fs_get_param_xattr(path, attr_name, val, size) == -1){
        printf_warn("[%s]Can't find attr name: %s\n", path, attr_name);
    }
    return val;
}



static char *_fs_get_save_thread_name(int ch)
{
    static char name[FS_OPS_CHANNEL][MAX_THREAD_NAME_LEN];
    if(ch >= FS_OPS_CHANNEL)
        return NULL;

    snprintf(name[ch], MAX_THREAD_NAME_LEN, "%s_ch%d", fs_ops_thread_table[FS_OPS_INDEX_SAVE], ch);
    return name[ch];
}

static char *_fs_get_backtrace_thread_name(int ch)
{
    static char name[FS_OPS_CHANNEL][MAX_THREAD_NAME_LEN];
    if(ch >= FS_OPS_CHANNEL)
        return NULL;

    snprintf(name[ch], MAX_THREAD_NAME_LEN, "%s_ch%d", fs_ops_thread_table[FS_OPS_INDEX_BACK], ch);
    return name[ch];
}
static void _thread_exit_callback(void *arg){  
    struct timeval beginTime, endTime;
    uint64_t nbyte, filesize = 0;
    float speed = 0;
    double  diff_time_us = 0;
    float diff_time_s = 0;
    int fd = -1, ch = 0;
    char command[256];
    struct push_arg *pargs;
    pargs = (struct push_arg *)arg;
#if defined(CONFIG_FS_PAUSE)
    pthread_mutex_unlock(&pargs->pctx->fs_thread[pargs->ch][pargs->ops_index].pwait.t_mutex);
#endif
    memcpy(&beginTime, &pargs->ct, sizeof(struct timeval));
    nbyte = pargs->nbyte;
    fd = pargs->fd;
    ch = pargs->ch;
    fprintf(stderr, ">>start time %zd.%.9ld., nbyte=%"PRIu64", fd=%d\n",beginTime.tv_sec, beginTime.tv_usec, nbyte, fd);
    gettimeofday(&endTime, NULL);
    fprintf(stderr, ">>end time %zd.%.9ld.\n",endTime.tv_sec, endTime.tv_usec);
    diff_time_us = difftime_us_val(&beginTime, &endTime);
    diff_time_s = diff_time_us/1000000.0;
    printf(">>diff us=%fus, %fs\n", diff_time_us, diff_time_s);
     speed = (float)((nbyte / (1024 * 1024)) / diff_time_s);
     fprintf(stdout,"speed: %.2f MBps, count=%"PRIu64"\n", speed, pargs->count);
    if(pargs->thread_name && !strcmp(pargs->thread_name, _fs_get_backtrace_thread_name(ch))){
        printf_note("Stop Backtrace, Stop Psd!!!\n");
        io_stop_backtrace_file(&ch);
        executor_set_command(EX_FFT_ENABLE_CMD, PSD_MODE_DISABLE, ch, NULL);
        _fs_read_exit(pargs);
    }
    else if(pargs->thread_name && !strcmp(pargs->thread_name,  _fs_get_save_thread_name(ch))) {
    #if defined(CONFIG_FS_FORMAT_WAV)
        struct spm_context *ctx = get_spm_ctx();
        uint32_t sample_rate = 0;
        if(ctx){
            struct spm_run_parm *run_para = ctx->run_args[pargs->ch];
            uint64_t middle_freq = run_para->m_freq;
            sample_rate = io_get_raw_sample_rate(pargs->ch, middle_freq, pargs->sampler.args.bindwidth);
        }
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
#if defined(CONFIG_FS_FILENAME_RULE_ACT)
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
#else
    _fs_create_default_path(p_args->ch, path, FILE_NAME_LEN,  args);
#endif
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
            #if defined(CONFIG_FS_FORMAT_WAV)
            struct spm_context *ctx = get_spm_ctx();
            uint32_t sample_rate = 0;
            if(ctx){
                struct spm_run_parm *run_para = ctx->run_args[p_args->ch];
                uint64_t middle_freq = run_para->m_freq;
                sample_rate = io_get_raw_sample_rate(p_args->ch, middle_freq, p_args->sampler.args.bindwidth);
            }
            wav_write_header(p_args->fd, sample_rate, p_args->split_nbyte);
            #endif
            close(p_args->fd);
        }
#if defined(CONFIG_FS_NOTIFIER)
        fs_notifier_add_node_to_list(p_args->ch, p_args->filename, &p_args->notifier);
#endif
        
        p_args->split_num++;
        p_args->split_nbyte = 0;
        p_args->filename = _fs_split_file_rename(p_args);
#if defined(CONFIG_FS_NOTIFIER)
        fs_notifier_create_node(p_args->ch, p_args->filename, &p_args->notifier);
#endif
        snprintf(path, sizeof(path), "%s/%s", fs_get_root_dir(), p_args->filename);
        printf_debug("path:%s\n", path);
        fd = open(path, O_RDWR | O_CREAT | O_TRUNC | O_DIRECT | O_SYNC, 0666);
        if (fd < 0) {
            printf_err("open %s fail\n", path);
            return -1;
        }
        _fs_create_xattr(p_args->ch, path, &p_args->sampler.args);
        p_args->fd = fd;
#if defined(CONFIG_FS_FORMAT_WAV)
        wav_write_header_before(fd);
    #if defined(CONFIG_FS_NOTIFIER)
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


static void _fs_thread_args_push(int ch, struct fs_context *ctx, pthread_t tid, int index, void *args)
{
    if(ctx && tid > 0){
        if(index >= FS_OPS_INDEX_MAX){
            printf_warn("fs thread index is too big[%d]\n", index);
            return;
        }
        printf_note("push tid: %lu, index: %d\n", tid, index);
        ctx->fs_thread[ch][index].tid = tid;
        ctx->fs_thread[ch][index].args = args;
    }
}

static void _fs_thread_wait_init(int ch, struct fs_context *ctx, int index)
{
#if defined(CONFIG_FS_PAUSE)
    if(ctx && index < FS_OPS_INDEX_MAX){
        pthread_cond_init(&ctx->fs_thread[ch][index].pwait.t_cond, NULL);
        pthread_mutex_init(&ctx->fs_thread[ch][index].pwait.t_mutex, NULL);
        ctx->fs_thread[ch][index].pwait.is_wait = false;
    }
#endif
}

static int _fs_thread_wait(void *args)
{
    int ret = 0;
#if defined(CONFIG_FS_PAUSE)
    struct push_arg *ptr = args;
    int ch = ptr->ch;
    int index = ptr->ops_index;
    struct fs_thread_m *ptd = &ptr->pctx->fs_thread[ch][index];

    printf_info("[ch:%d]%s thread is %s\n", ch, fs_ops_thread_table[index], ptd->pwait.is_wait == true ? "PAUSE" : "RUN");
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE , NULL);
    pthread_mutex_lock(&ptd->pwait.t_mutex);
    while(ptd->pwait.is_wait == true)
        pthread_cond_wait(&ptd->pwait.t_cond, &ptd->pwait.t_mutex);
    pthread_mutex_unlock(&ptd->pwait.t_mutex);
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE , NULL);
#endif
    return ret;
}

static int _fs_thread_wait_or_no_nofity(int ch, int index, bool wait, struct fs_context *ctx)
{
    int ret = -1;
#if defined(CONFIG_FS_PAUSE)
    struct fs_thread_m *ptd = &ctx->fs_thread[ch][index];
    if(pthread_check_alive_by_tid(ptd->tid) == false){
        printf_note("[ch:%d,%s]pthread is not running\n", ch, fs_ops_thread_table[index]);
        return -1;
    }
    if(wait)
        ptd->pwait.is_wait = true;
    else
        ptd->pwait.is_wait = false;
    pthread_cond_signal(&ptd->pwait.t_cond);
    printf_note("[ch:%d,%s]notify thread %s\n", ch, fs_ops_thread_table[index], ptd->pwait.is_wait == true ? "PAUSE" : "RUN");
    ret = 0;
#endif
    return ret;
}


static int _fs_start_save_file_thread(void *arg)
{
    void *ptr = NULL;
    ssize_t nread = 0; 
    int ret = 0, ch;
    uint8_t is_write = 0;
    struct push_arg *p_args;
    struct spm_context *_ctx;
    p_args = (struct push_arg *)arg;
#if defined(CONFIG_FS_SAVE_PAUSE)
    _fs_thread_wait(arg);
#endif
    _ctx = get_spm_ctx();
    if(_ctx){
        ch = p_args->ch;
        if(_ctx->ops->read_adc_data)
            nread = _ctx->ops->read_adc_data(ch, &ptr);
        /* disk is full */
        if(disk_is_full_alert == true || ret == -1){
            if(disk_is_full_alert)
                printf_warn("disk is full, save thread exit\n");
            if(ret == -1)
                printf_warn("write file error, save thread exit\n");
            if(_ctx->ops->read_adc_over_deal)
                _ctx->ops->read_adc_over_deal(ch, &nread);
            //_fs_stop_save_file(ch, NULL, NULL);
            pthread_exit_by_name(_fs_get_save_thread_name(ch));
            return -1;
        }
    }
    if(nread > 0){
        p_args->nbyte += nread;
        p_args->count ++;
        if((ret = _fs_split_file(p_args)) == 0){
            ret = _write_disk_run(p_args->fd, nread, ptr);
            if(_ctx && _ctx->ops->read_adc_over_deal){
                _ctx->ops->read_adc_over_deal(ch, &nread);
                is_write = 1;
            }
        } 
#if defined(CONFIG_FS_NOTIFIER)
        fs_notifier_update_file_size(&p_args->notifier, nread);
#endif
        p_args->split_nbyte += nread;
        if(_fs_sample_size_full(p_args)){
            if(_ctx && _ctx->ops->read_adc_over_deal && is_write == 0){
                _ctx->ops->read_adc_over_deal(ch, &nread);
            }
            //_fs_stop_save_file(ch, NULL, NULL);
            pthread_exit_by_name(_fs_get_save_thread_name(ch));
            return -1;
        }
    }else{
        usleep(1000);
        //printf("read_adc_data err:%zd\n", nread);
    }
    return (int)nread;

}

static void _fs_sample_time_out(struct uloop_timeout *t)
{
    struct push_arg *p_args = container_of(t, struct push_arg, sampler.timeout);
    if(p_args == NULL)
        return;
    
    printf_note("Save File Stop!sample_time:%d\n", p_args->sampler.args.sample_time);
    _fs_stop_save_file(p_args->ch, NULL, NULL);
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
    
#if defined(CONFIG_FS_NOTIFIER)
    p_args->notifier.args = &p_args->sampler;
    fs_notifier_init(p_args->ch, p_args->filename, &p_args->notifier);
#endif
    if(p_args->sampler.sample_policy == FSP_TIMER){
        _fs_sample_timer_init(args);
    }
    
#if defined(CONFIG_FS_FORMAT_WAV)
    wav_write_header_before(p_args->fd);
    #if defined(CONFIG_FS_NOTIFIER)
    fs_notifier_update_file_size(&p_args->notifier, 512);
    #endif
#endif

#if defined(CONFIG_FS_SAVE_PAUSE)
    _fs_thread_wait_init(p_args->ch, p_args->pctx, p_args->ops_index);
#endif
    return 0;
}

static int _fs_save_exit(void *args)
{   
     struct push_arg *p_args = args;
     if(p_args == NULL)
        return -1;
     
#if defined(CONFIG_FS_NOTIFIER)
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

static ssize_t _fs_start_save_file(int ch, char *filename, void *args, struct fs_context *ctx)
{
    int ret = -1;
    static int fd = 0;
    char path[512];
    struct timeval beginTime;
    struct push_arg *p_args;
    pthread_t tid = 0;

    if(disk_is_valid == false || disk_is_format == true || disk_is_full_alert == true)
        return -1;
    if(filename == NULL || ch >= MAX_RADIO_CHANNEL_NUM)
        return -1;

    if(pthread_check_alive_by_name(_fs_get_save_thread_name(ch)) == true){
        printf_warn("thread %s is busy\n", _fs_get_save_thread_name(ch));
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", fs_get_root_dir(), filename);
    printf("start save file: %s\n", path);
        
    if(path == NULL)
        return -1;
    fd = open(path, O_RDWR | O_CREAT | O_TRUNC | O_DIRECT | O_SYNC, 0666);
    if (fd < 0) {
        printf_err("open %s fail\n", path);
        return -1;
    }
    _fs_create_xattr(ch, path, args);
    printf_debug("fd = %d\n", fd);
    p_args = safe_malloc(sizeof(struct push_arg));
    gettimeofday(&beginTime, NULL);
    fprintf(stderr, "#######start time %zd.%.9ld.\n",beginTime.tv_sec, beginTime.tv_usec);
    memcpy(&p_args->ct, &beginTime, sizeof(struct timeval));
    p_args->nbyte = 0;
    p_args->count = 0;
    p_args->split_num = 0;
    p_args->split_nbyte = 0;
    p_args->fd = fd;
    p_args->ch = ch;
    p_args->thread_name = _fs_get_save_thread_name(ch);
    p_args->pctx = ctx;
    p_args->ops_index = FS_OPS_INDEX_SAVE;
    printf_note("thread name=%s\n", p_args->thread_name);
    p_args->filename = safe_strdup(filename);
    if(args != NULL){
        memcpy(&p_args->sampler.args, args, sizeof(struct fs_save_sample_args));
        printf_note("bindwidth=%u,sample_size=%"PRIu64" sample_time=%d, cache_time=%d\n", 
                p_args->sampler.args.bindwidth, p_args->sampler.args.sample_size, p_args->sampler.args.sample_time, p_args->sampler.args.cache_time);
    }
    p_args->sampler.sample_policy = _fs_get_sample_policy(p_args);
    printf_note("sample_policy=%d\n", p_args->sampler.sample_policy);
    
    io_set_enable_command(ADC_MODE_ENABLE, ch, -1, 0);
    ret =  pthread_create_detach (NULL,_fs_save_init, _fs_start_save_file_thread, _thread_exit_callback,  
                                p_args->thread_name, p_args, p_args, &tid);
    if(ret != 0){
        perror("pthread save file thread!!");
        safe_free(p_args->filename);
        safe_free(p_args);
        exit(1);
    }
    if(ctx){
        _fs_thread_args_push(ch, ctx, tid, p_args->ops_index, p_args);
    }

    return ret;
}


static ssize_t _fs_pause_save_file(int ch, bool pause, struct fs_context *ctx)
{
    if(pause)
        io_set_enable_command(ADC_MODE_DISABLE, ch, -1, 0);
    else
        io_set_enable_command(ADC_MODE_ENABLE, ch, -1, 0);
    
    return _fs_thread_wait_or_no_nofity(ch, FS_OPS_INDEX_SAVE, pause, ctx);
}

static ssize_t _fs_pause_read_raw_file(int ch, bool pause, struct fs_context *ctx)
{
    if(pause)
        io_start_backtrace_file(&ch);
    else
        io_stop_backtrace_file(&ch);
    
    return _fs_thread_wait_or_no_nofity(ch, FS_OPS_INDEX_BACK, pause, ctx);
}

static ssize_t _fs_set_raw_file_offset(int ch, char *path, size_t offset, struct fs_context *ctx)
{
    struct push_arg *p_args;
    off_t ret = -1;
    if(ctx){
        p_args = ctx->fs_thread[ch][FS_OPS_INDEX_BACK].args;
        if(p_args && p_args->fd > 0){
            printf_note("[ch:%d]set offset: %s, fd:%d, offset:%lu\n", ch, path, p_args->fd, offset);
            ret = lseek(p_args->fd, offset, SEEK_SET);
            if(ret == -1){
                perror("lseek error!");
                return -1;
            }
        }else{
            return -1;
        }
    }
    return ret;
}


static ssize_t _fs_stop_save_file(int ch, char *filename, struct fs_context *ctx)
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
#if defined(CONFIG_FS_BACK_PAUSE)
    _fs_thread_wait(arg);
#endif
#if defined(CONFIG_ARCH_ARM)
    if(get_spm_ctx() && get_spm_ctx()->ops->back_running_file){
        nread = get_spm_ctx()->ops->back_running_file(ch, STREAM_ADC_WRITE, p_args->fd);
    }
#else /* X86 TEST */
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    nread = read(p_args->fd, buffer, sizeof(buffer));
    if(nread < 0){
        perror("read file");
    }
    usleep(10);
#endif
    p_args->count ++;
    if(nread > 0){
         p_args->nbyte += nread;
    }
#if defined(CONFIG_FS_PROGRESS)
    p_args->progress.offset = lseek(p_args->fd, 0, SEEK_CUR);
#endif

    if(nread == 0){
        printf_note(">>>>>>>read over!!\n");
        pthread_exit_by_name(_fs_get_backtrace_thread_name(ch));
    }
    return (int)nread;
}

#if defined(CONFIG_FS_PROGRESS)
static void _fs_progress_time_out(struct uloop_timeout *t)
{
    int timer_ms = 0;
    struct push_arg *p_args = container_of(t, struct push_arg, progress.timeout);
    if(p_args == NULL || p_args->pctx == NULL){
        printf_note("EXIT!!!!!!!!!!!\n");
        return;
    }
    
    if(p_args){
        if(p_args->total_size > 0){
            p_args->progress.val = (p_args->progress.offset * 100) / p_args->total_size;
            printfn("PROGRESS:%d%%, %"PRIu64", offset:%"PRIu64"\n", p_args->progress.val, p_args->total_size, p_args->progress.offset);
#if defined(CONFIG_PROTOCOL_XW)
            xw_send_cmd_data(p_args->pctx->client, CMD_CODE_BACKTRACE_PROGRESS, p_args);
#endif
            if(p_args->progress.val == 100){
                p_args->progress.is_finish = true;
            }
        }
        
        struct fs_progress_t  *progress = &p_args->progress;
        timer_ms = FS_PROGRESS_NOTIFY_TIME_MS;
        uloop_timeout_set(&progress->timeout, timer_ms);
    }
    

}

/* 
    进度定时器初始化
*/
static int _fs_progress_timer_init(struct push_arg *p_args)
{
    int timer_ms = 0;
    if(p_args == NULL)
        return -1;
    
    struct fs_progress_t  *progress = &p_args->progress;
    progress->timeout.cb = _fs_progress_time_out;
    progress->is_finish = false;
    progress->is_exit = false;
    timer_ms = FS_PROGRESS_NOTIFY_TIME_MS;
    uloop_timeout_set(&progress->timeout, timer_ms);
    printf_note("Start Progress Timer %d.mSec\n", FS_PROGRESS_NOTIFY_TIME_MS);
    return 0;
}

static int _fs_progress_timer_exit(void *args)
{
    struct push_arg *p_args = args;
    uint32_t timeout_ms = 10000; /* Time out 10S */
    
    if(p_args == NULL)
        return -1;
    
    struct fs_progress_t  *progress= &p_args->progress;
    if(progress == NULL)
        return -1;
    
    /* Wait util the progress report is completed or TimeOut or client is quit*/
    do{
        usleep(1000);
        timeout_ms --;
    }while(p_args->progress.is_finish == false && timeout_ms > 0 && progress->is_exit == false);
    
    uloop_timeout_cancel(&progress->timeout);
    if(progress->is_exit)
        printf_warn("Progress Timer Exit! Client is quit\n");
    else
        printf_note("Progress Timer Exit [%u]%s\n", timeout_ms, p_args->progress.is_finish == true ? "Finish": "TimeOut");
    return 0;
}

#endif

static int _fs_read_init(void *args)
{
    struct push_arg *p_args = args;
    
    if(p_args == NULL)
        return -1;

#if defined(CONFIG_FS_BACK_PAUSE)
    _fs_thread_wait_init(p_args->ch, p_args->pctx, p_args->ops_index);
#endif

#if defined(CONFIG_FS_PROGRESS)
     _fs_progress_timer_init(p_args);
#endif

    return 0;
}

static int _fs_read_exit(void *args)
{   
     struct push_arg *p_args = args;
     if(p_args == NULL)
        return -1;
     
#if defined(CONFIG_FS_PROGRESS)
    _fs_progress_timer_exit(p_args);
#endif
    _fs_set_backtrace_status(p_args->filename);

    return 0;
}



static ssize_t _fs_start_read_raw_file(int ch, char *filename, struct fs_context *ctx)
{
    int ret = -1;
    int file_fd;
    char path[512];
    struct timeval beginTime;
    pthread_t tid = 0;
    if(disk_is_valid == false || disk_is_format == true)
        return -1;

    if(filename == NULL || ch >= MAX_RADIO_CHANNEL_NUM)
        return -1;

    if(pthread_check_alive_by_name(_fs_get_backtrace_thread_name(ch)) == true){
        printf_warn("thread %s is busy\n", _fs_get_backtrace_thread_name(ch));
        return -1;
    }
    snprintf(path, sizeof(path), "%s/%s", fs_get_root_dir(), filename);
#if defined(CONFIG_ARCH_ARM)
    file_fd = open(path, O_RDONLY | O_DIRECT | O_SYNC, 0666);
#else
    file_fd = open(path, O_RDONLY, 0666);
#endif
    if (file_fd < 0) {
        printf_err("open %s fail\n", path);
        return -1;
    } 
    printf_note("start read file: %s, file_fd=%d\n", path, file_fd);
    
    struct push_arg *p_args;
    p_args = safe_malloc(sizeof(struct push_arg));
    gettimeofday(&beginTime, NULL);
    fprintf(stderr, "#######start time %zd.%.9ld.\n",beginTime.tv_sec, beginTime.tv_usec);
    memcpy(&p_args->ct, &beginTime, sizeof(struct timeval));
    p_args->filename = strdup(filename);
    p_args->nbyte = 0;
    p_args->count = 0;
    p_args->fd = file_fd;
    p_args->ch = ch;
    _fs_get_filesize(filename, &p_args->total_size);
    p_args->thread_name = _fs_get_backtrace_thread_name(ch);
    p_args->pctx = ctx;
    p_args->ops_index = FS_OPS_INDEX_BACK;
#if defined(CONFIG_FS_FORMAT_WAV)
    wav_backtrace_before(file_fd);
#endif
    io_start_backtrace_file(&ch);
    ret =  pthread_create_detach (NULL,_fs_read_init, _fs_start_read_raw_file_loop, _thread_exit_callback,  
                                p_args->thread_name , p_args, p_args, &tid);
    if(ret != 0){
        perror("pthread read file thread!!");
        safe_free(p_args);
        io_stop_backtrace_file(&ch);
        return -1;
    }
    if(ctx){
        _fs_thread_args_push(ch, ctx, tid, p_args->ops_index, p_args);
    }

    return 0;
}

static ssize_t _fs_stop_read_raw_file(int ch, char *filename, struct fs_context *ctx)
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
    .fs_root_dir = fs_get_root_dir,
    .fs_disk_info = _fs_disk_info,
    .fs_disk_valid = _fs_disk_valid,
    .fs_format = _fs_format,
    .fs_mkdir = _fs_create_dir,
    .fs_dir = _fs_dir,
    .fs_list = _fs_list,
    .fs_delete = _fs_delete,
    .fs_find = _fs_find,
    .fs_find_rec = _fs_find_rec,
    .fs_get_filesize = _fs_get_filesize,
    .fs_start_save_file = _fs_start_save_file,
    .fs_stop_save_file = _fs_stop_save_file,
    .fs_pause_save_file = _fs_pause_save_file,
    .fs_start_read_raw_file = _fs_start_read_raw_file,
    .fs_stop_read_raw_file = _fs_stop_read_raw_file,
    .fs_pause_read_raw_file = _fs_pause_read_raw_file,
    .fs_set_raw_file_offset = _fs_set_raw_file_offset,
    .fs_get_err_code = _fs_get_err_code,
    .fs_file_download_cb = _fs_download_over_callback,
    .fs_get_file_status = _fs_get_file_status,
    .fs_create_default_path = _fs_create_default_path,
    .fs_get_xattr = _fs_get_xattr,
    .fs_close = _fs_exit,
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
                    #ifdef CONFIG_PROTOCOL_ACT
                    akt_send_alert(1);
                    #endif
                    //send error
                } else if(disk_error_num == DISK_CODE_NOT_FORAMT){
                    #ifdef CONFIG_PROTOCOL_ACT
                    akt_send_alert(2);
                    #endif
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
                #ifdef CONFIG_PROTOCOL_ACT
                akt_send_alert(0);
                #endif
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

static int _fs_exit(struct fs_context *ctx)
{
    struct push_arg *parg;
    if(ctx){
        for(int ch = 0; ch < FS_OPS_CHANNEL; ch++){
            for(int i = 0; i < FS_OPS_INDEX_MAX; i++){
                if(pthread_check_alive_by_tid(ctx->fs_thread[ch][i].tid) == true){
                    pthread_cancel_by_tid(ctx->fs_thread[ch][i].tid);
#if defined(CONFIG_FS_PROGRESS)
                    parg = ctx->fs_thread[ch][i].args;
                    parg->progress.is_exit = true;
#endif
                }
                ctx->fs_thread[ch][i].tid = 0;
            }
        }
        printf_note("free fs thread args\n");
        safe_free(ctx);
    }
    return 0;
}

static inline void _fs_init_(void) 
{
    static bool fs_has_inited = false;
    
    if(fs_has_inited == true)
        return;
    fs_ctx = fs_create_context();
#if defined(CONFIG_ARCH_ARM)
    _fs_mkdir(fs_get_root_dir());
#endif
    _fs_dir(NULL, NULL, NULL);
    fs_has_inited = true;
}


void fs_init(void)
{
#if defined(CONFIG_ARCH_ARM)
    fs_disk_start_check();
    disk_is_valid = _fs_disk_valid();
    if(disk_is_valid == false)
        return;
#else
    disk_is_valid = true;
#endif
    printf_note("FS Init\n");
    _fs_init_();
}

