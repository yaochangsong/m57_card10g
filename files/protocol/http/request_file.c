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


struct file_read{
    char *ptotalbuffer;
    char *pfile;
    int file_len;
    int buffer_total_len;
};
struct file_read gfile;

static char *file_get_file_buffer()
{
    //mmap
    return gfile.pfile;
}

static int file_read_shutdown(char *filename)
{
    int64_t len = 0;
    //ioctl(filename, );//notify kernel read file data one by one
    return len;
}

static int file_reload_buffer(char *filename)
{
    static int offset = 0;
    offset += gfile.file_len;
    if(offset > gfile.buffer_total_len){
        offset = 0;
        printf_note("reload buffer over\n");
        return 0;
    }
    memcpy(gfile.pfile, gfile.ptotalbuffer+offset, gfile.file_len);
    printf_note("filename:%s\n", filename);
    io_set_refresh_disk_file_buffer((void*)filename);
    printf_note("--------------offset=%d, reload offset =%d,gfile.buffer_total_len=%d\n", offset, gfile.file_len, gfile.buffer_total_len);
    sleep(1);
    return gfile.file_len;
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


int file_http_read(char *filename, uint8_t *buf, int len)
{
    int read_result_len = 0, read_len = 0;
    char *ptr_head;
    static int offset = 0;
    static int buffer_total_len = 0;
    
    if(buf == NULL)
        return -1;

loop:
    printf_note("read offset=%d\n", offset);
    if(offset == 0){
        buffer_total_len = file_reload_buffer(filename);
        if(buffer_total_len <= 0){
            buffer_total_len = 0;
            offset = 0;
            printf_note("read over buffer_total_len=%d*********\n", buffer_total_len);
            return 0;
        }
    }

    printf_info("buffer_total_len=%d\n", buffer_total_len);
    ptr_head = file_get_file_buffer();
    if(ptr_head == NULL){
        return -1;
    }

    memset(buf, 0, len);
    read_len = len;
    if(read_len > buffer_total_len - offset)
        read_len = buffer_total_len - offset;
    
    if(offset < buffer_total_len){
        memcpy(buf, ptr_head+offset, read_len);
        read_result_len = read_len;
        offset += read_len; 
    }else{
        /* read over, reset buffer offset */
        printf_info("reset offset = %d, read_result_len=%d\n", offset, read_result_len);
        offset = 0;
        read_result_len = 0;
        /* start reload buffer */
        //buffer_total_len = file_http_reload_buffer();
        //if(file_http_reload_buffer>0)
        goto loop;
    }
    printf_info("offset = %d, read_result_len=%d\n", offset, read_result_len);
    return read_result_len;
}


int file_download(struct uh_client *cl, void *arg)
{
    
    static char buf[256];
    char *path = cl->dispatch.file.path;
    int r, i;

    printf_info("download:path=%s\n", path);
    while (cl->us->w.data_bytes < 256) {
        r = file_http_read(path, buf, sizeof(buf));
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
    char text[]={1,2,3,4,5,6,7,8,9,10,11,12,13,14};
    
    
    gfile.ptotalbuffer = malloc(1024);
    memset(gfile.ptotalbuffer, 'a', 1024);
    gfile.buffer_total_len = 1024;

    gfile.pfile = malloc(REQUEST_FILESIZE_BUFFER_LEN);
    memset(gfile.pfile, 0, REQUEST_FILESIZE_BUFFER_LEN);
   // memcpy(gfile.pfile, text, sizeof(text));
    gfile.file_len = REQUEST_FILESIZE_BUFFER_LEN;//sizeof(text);
    //printf_warn("%s, %d\n", gfile.pfile, gfile.file_len);
}
