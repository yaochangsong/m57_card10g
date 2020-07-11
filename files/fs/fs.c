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

struct fs_context *fs_ctx = NULL;
static bool disk_is_valid = false;
static void  *disk_buffer_ptr  = NULL;



static int get_frequency_band_from_str(const char *str,uint32_t *band,uint64_t *freq)
{
    int cnt = 0,first = 1;
    int fre_con1 = 0,fre_con2 = 0;
    int band_con1 = 0,band_con2 = 0;
    int con1_start = 1,con2_start = 1;
    int con3_start = 1,con4_start = 1;
    int freq_unit = 1;//1--G 0--M
    uint64_t frq_data = 0;
    double  frq_tmp ;
    uint32_t band_data = 0;

    char fre_str[20] = "0";
    char band_str[20] = "0";
    printf_note("str:%s\n",str);
    
    if(str==NULL)
        return -1;
    
    while(1)
    {
        /*
        if(*(str+cnt) != '\0')
            printf("data:%c\n",*(str+cnt)); */
        
        if(*(str+cnt) == '\0')
            break;
        
        if(*(str+cnt) == 'F' && con1_start == 1)
        {
            fre_con1 = cnt;
            con1_start = 0;
            printf_note("fre_con1:%d\n",fre_con1);
        }
        
        if((*(str+cnt) == 'G' || *(str+cnt) == 'M' )&& con2_start == 1)
        {
            fre_con2 = cnt;
            con2_start = 0;
            if(*(str+cnt) == 'M')
                freq_unit = 0;
            printf_note("fre_con2:%d\n",fre_con2);
        }


        if(*(str+cnt) == 'B' && con3_start == 1)
        {
            band_con1 = cnt;
            con3_start = 0;
            printf_note("band_con1:%d\n",band_con1);
        }
        
        if(*(str+cnt) == 'M' && con4_start == 1 && con3_start == 0)
        {
            band_con2 = cnt;
            con4_start = 0;
            printf_note("band_con2:%d\n",band_con2);
        }

        cnt++;

    }

    if( fre_con2 > fre_con1)
    {
        memcpy(fre_str,str+fre_con1 + 1 ,fre_con2 - fre_con1 -1);
        //frq_data = atoi(fre_str);
        frq_tmp = atof(fre_str);
        //frq_data = frq_tmp 
        if(freq_unit ==1 )
            frq_data = frq_tmp * pow(10,9);
        else
            frq_data = frq_tmp * pow(10,6);

        *freq = frq_data;
        printf_note("fre_str:%s frq_data:%lu frq_tmp:%lf\n",fre_str,frq_data,frq_tmp);
    }
    else
    {
        printf_err("get frequency error!\n");
        return -1;
    }

    if( band_con2 > band_con1)
    {
        memcpy(band_str,str+band_con1 + 1 ,band_con2 - band_con1 -1);
        band_data = atoi(band_str);
        *band = band_data * pow(10,6);
        printf_note("band_str:%s band_data:%u\n",band_str,band_data);
    }
    
    else
    {
        printf_err("get band error!\n");
        return -1;
    }
    
    return 0;
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

    //posix_memalign((void **)&user_mem, 4096 /*alignment */ , size + 4096);
   // posix_memalign((void **)&user_mem, 4096 /*alignment */ , WRITE_UNIT_BYTE + 4096);
    //printf_note("fd=%d, pdata=%p, %p, size=0x%x\n",fd, pdata,user_mem,  size);

    //if(size > DMA_BUFFER_SIZE){
    //    printf_warn("size is too big: %u\n", size);
    //    size = DMA_BUFFER_SIZE;
    //}
   // memcpy(disk_buffer_ptr, pdata, size);
    //printf_note("pdata=%p, size = 0x%x\n", pdata, size);
    //rc = write(fd, user_mem, WRITE_UNIT_BYTE);
    //rc = write(fd, disk_buffer_ptr, size);
    rc = write(fd, pdata, size);
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

char *fs_get_root_dir(void)
{
    return "/run/media/nvme0n1/data";
}


bool _fs_disk_valid(void)
{
    #define DISK_NODE_NAME "/run/media/nvme0n1"
    if(access(DISK_NODE_NAME, F_OK)){
        printf_note("Disk node[%s] is not valid\n", DISK_NODE_NAME);
        return false;
    }
    return true;
}

bool _fs_disk_info(struct statfs *diskInfo)
{
    #define DISK_NODE_NAME "/run/media/nvme0n1"
    
    if(access(DISK_NODE_NAME, F_OK)){
        printf_note("Disk node[%s] is not valid\n", DISK_NODE_NAME);
        return false;
    }

	if(statfs(DISK_NODE_NAME, diskInfo))
	{
		printf_note("Disk node[%s] is unknown filesystem\n", DISK_NODE_NAME);
        return false;
	}
    
    return true;
}

static inline int _fs_format(void)
{
	#define DISK_DEVICE_NAME "/dev/nvme0n1"
	#define DISK_NODE_NAME "/run/media/nvme0n1"
    char cmd[128];
	int ret;

    snprintf(cmd, sizeof(cmd), "umount %s",DISK_NODE_NAME);
    ret = safe_system(cmd);
    if(!ret)
    	printf_err("unmount %s failed\n", DISK_NODE_NAME);
	snprintf(cmd, sizeof(cmd), "mkfs.ext2 %s",DISK_DEVICE_NAME);
	ret = safe_system(cmd);    
	if(!ret)
    	printf_err("mkfs.ext2 %s failed\n", DISK_NODE_NAME);
	snprintf(cmd, sizeof(cmd), "mount %s %s",DISK_DEVICE_NAME, DISK_NODE_NAME);
	ret = safe_system(cmd);
	if(!ret)
    	printf_err("mount %s %s failed\n", DISK_DEVICE_NAME, DISK_NODE_NAME);
    return ret;
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

static inline int _fs_delete(char *filename)
{
    char path[PATH_MAX];
    int ret = 0;
    
    if(disk_is_valid == false)
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
    
    if(disk_is_valid == false)
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


#define THREAD_NAME "FS_SAVE_FILE"
static int disk_save_file_fd = -1;

static ssize_t _fs_start_save_file(char *filename)
{
    #define     _START_SAVE     1
    #define     _STOP_SAVE      0
    int ret = -1, i, found = 0, cval;
    
    
    char path[512];
    
    if(disk_is_valid == false)
        return -1;
    if(filename == NULL)
            return -1;
    
    io_set_enable_command(ADC_MODE_ENABLE, -1, -1, 0);
    snprintf(path, sizeof(path), "%s/%s", fs_get_root_dir(), filename);
    disk_save_file_fd = _write_fs_disk_init(path);
    printf("start save file: %s\n", path);
    ret = pthread_create_detach_loop(NULL, _fs_start_save_file_thread, THREAD_NAME, &disk_save_file_fd);
    if(ret != 0){
        perror("pthread cread save file thread!!");
    }
    
    return ret;
}

static ssize_t _fs_stop_save_file(char *filename)
{
    io_set_enable_command(ADC_MODE_DISABLE, -1, -1, 0);
    printf("stop save file : %s\n", THREAD_NAME);
    pthread_cancel_by_name(THREAD_NAME);
    _write_disk_close(disk_save_file_fd);
}

#define THREAD_FS_BACK_NAME "FS_BACK_FILE"

static ssize_t _fs_start_read_raw_file_loop(void *arg)
{
    ssize_t nread = 0; 
    int  fd;
    fd = *(int *)arg;
    
    nread = get_spm_ctx()->ops->back_running_file(STREAM_ADC, fd);
    if(nread == 0){
        printf_note(">>>>>>>read over!!\n");
        pthread_exit_by_name(THREAD_FS_BACK_NAME);
    }
    return nread;
}

static void _fs_read_exit_callback(void *arg){  
    int fd;
    fd = *(int *)arg;
    printf_note("fs read exit, fd= %d\n", fd);
    if(fd > 0)
        close(fd);
}


static ssize_t _fs_start_read_raw_file(char *filename)
{
    int ret = -1;
    static int file_fd;
    char path[512];
    uint32_t band;
    uint64_t freq;
    int ch = 0;
    if(disk_is_valid == false)
        return -1;
#if 1
    if(filename == NULL)
        return -1;
    //get_frequency_band_from_str(filename,&band,&freq);
    //printf_note("wirte freq:%lu band:%u !\n",freq,band);
    //config_write_data(EX_MID_FREQ_CMD,  EX_MID_FREQ, ch, &freq);
    //config_write_data(EX_MID_FREQ_CMD,  EX_BANDWITH, ch, &band);
    snprintf(path, sizeof(path), "%s/%s", fs_get_root_dir(), filename);
    file_fd = open(path, O_RDONLY, 0666);
    if (file_fd < 0) {
        printf_err("open % fail\n", path);
        return -1;
    } 
    printf_note("start read file: %s, file_fd=%d\n", path, file_fd);
    io_start_backtrace_file(NULL);
    ret =  pthread_create_detach (NULL, _fs_start_read_raw_file_loop, _fs_read_exit_callback,  
                                THREAD_FS_BACK_NAME, &file_fd, &file_fd);
    if(ret != 0){
        perror("pthread cread save file thread!!");
    }
#endif
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

