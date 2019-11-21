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
*  Rev 1.0   09 Nov 2019   yaochangsong
*  Initial revision.
******************************************************************************/
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <sys/stat.h>
#include "config.h"
#include "log/log.h"
#include "file.h"
#include "request_file.h"


struct file_request_read req_readx;

static char *file_get_read_buffer_pointer()
{
   return  memshare_get_dma_rx_base();
}


static int file_reload_buffer(char *filename)
{
    printf_note("filename:%s\n", filename);
    return io_set_refresh_disk_file_buffer((void*)filename);
}



int file_startstore(struct uh_client *cl, void *arg)
{
    printf_note("startstore\n");
    sleep(5);
    return true;
}

int file_stopstore(struct uh_client *cl, void *arg)
{
    printf_note("stopstore\n");
    sleep(1);
    return true;
}

int file_search(struct uh_client *cl, void *arg)
{
    printf_note("search\n");
    sleep(1);
    return true;
}

int file_start_backtrace(struct uh_client *cl, void *arg)
{
    printf_warn("start_backtrace\n");
    sleep(1);
    return true;
}

int file_stop_backtrace(struct uh_client *cl, void *arg)
{
    printf_note("stop backtrace\n");
    sleep(1);
    return true;
}

int file_delete(struct uh_client *cl, void *arg)
{
    printf_note("file_delete\n");
    sleep(1);
    return true;
}


int file_read(const char *filename, uint8_t *buf, int len)
{
    int ret;
    struct file_request_read *fr=&req_readx;
    
    if(buf == NULL)
        return -1;

loop:
    printf_note("read offset=%d\n", fr->read_offset);
    if(fr->read_offset == 0){
        ret = file_reload_buffer(filename);
        if(ret < 0){
            fr->read_offset = 0;
            printf_err("read err %d\n", ret);
            return ret;
        }else if(ret == 0){
            printf_note("read over\n");
            return 0;
        }
    }
    memset(buf, 0, len);
    if(len > fr->read_buffer_len - fr->read_offset){
        len =  fr->read_buffer_len - fr->read_offset;
    }
    if(fr->read_offset < fr->read_buffer_len){
        memcpy(buf, fr->read_buffer_pointer + fr->read_offset, len);
        fr->read_offset += len;
        ret = len;
    }else{
        /*  read buffer over, reload buffer */
        fr->read_offset = 0;
        goto loop;
    }
    
    return ret;
}

int file_read_attr_info(const char *name, void *info)
{
    if(name == NULL || info == NULL)
        return -1;
    strcpy((char *)info, name);
    return io_get_read_file_info(info);
}



int file_download(struct uh_client *cl, void *arg)
{
    
    static char buf[256];
    char *path = cl->dispatch.file.path;
    int r, i;

    printf_info("download:path=%s\n", path);
    while (cl->us->w.data_bytes < 256) {
        r = file_read(path, buf, sizeof(buf));
        printf_info("r=%d\n", r);
        if (r < 0) {
            printf_warn("read error\n");
            if (errno == EINTR)
                continue;
            uh_log_err("read");
        }

        if (r <= 0) {
            printf_info("request_done\n");
            cl->request_done(cl);
            return;
        }
        
        cl->send(cl, buf, r);
    }
    return true;
}

void file_handle_init(void)
{
#if 0
    char text[]={1,2,3,4,5,6,7,8,9,10,11,12,13,14};
    
    
    gfile.ptotalbuffer = malloc(1024);
    memset(gfile.ptotalbuffer, 'a', 1024);
    gfile.buffer_total_len = 1024;

    gfile.pfile = malloc(REQUEST_FILESIZE_BUFFER_LEN);
    memset(gfile.pfile, 0, REQUEST_FILESIZE_BUFFER_LEN);
   // memcpy(gfile.pfile, text, sizeof(text));
    gfile.file_len = REQUEST_FILESIZE_BUFFER_LEN;//sizeof(text);
    //printf_warn("%s, %d\n", gfile.pfile, gfile.file_len);
        char buf[256];
    int fd ;
        struct disk_file_info fi;
    int ret;
    char filename[256]="test.txt";
    printf_warn("filename=%s\n", filename);
    ret = io_get_read_file_info(filename, (void *)&fi, sizeof(struct disk_file_info));
    printf_warn("ret=%d\n", ret);
    printf_warn("fi.ctime=%u, fi.file_path=%s,fi.read_cnt=%u, fi.st_blksize=%u, fi.st_blocks=%u, fi.st_size=%llu\n", 
                fi.ctime, fi.file_path,fi.read_cnt, fi.st_blksize, fi.st_blocks, fi.st_size);
    //fd = read(io_get_fd(), buf, 256),
    //printf_warn("read %d:%s\n", fd,buf);
    
#endif
    
    struct disk_file_info fi;
    int ret;
    char filename[64]="test.txt";
    char buf[256];
    memshare_init();
    ret = file_read_attr_info(filename, (void *)&fi);
    printf_warn("ret=%d\n", ret);
    printf_warn("fi.ctime=%u, fi.file_path=%s,fi.read_cnt=%u, fi.st_blksize=%u, fi.st_blocks=%u, fi.st_size=%llu\n", 
                fi.ctime, fi.file_path,fi.read_cnt, fi.st_blksize, fi.st_blocks, fi.st_size);
    req_readx.read_buffer_pointer=file_get_read_buffer_pointer();
    req_readx.read_buffer_len =  fi.buffer_len;
    req_readx.read_offset = 0;
    lseek(io_get_fd(), 0, SEEK_SET);
    file_read(filename, buf, sizeof(buf));
    printfe("buf = %s\n", buf);
}
