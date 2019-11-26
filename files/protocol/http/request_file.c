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
    if(fr->read_flags == FILE_READ_IDLE){
        struct disk_file_info fi;
        ret = file_read_attr_info(filename, (void *)&fi);
        printf_note("file_path:%s,buffer_len:%u,read_cnt:%u,st_size=%lu,st_blocks=%d\n", 
            fi.file_path,fi.buffer_len, fi.read_cnt,fi.st_size, fi.st_blocks);
        strcpy(fr->file_path,fi.file_path);
        fr->read_buffer_pointer=file_get_read_buffer_pointer();
        fr->read_buffer_len =  fi.buffer_len;
        fr->read_offset = 0;
        lseek(io_get_fd(), 0, SEEK_SET);
        fr->read_flags = FILE_READ_BUSY;
    }else if(strcmp(fr->file_path, filename)){
        printf_note("file read [%s] is busy\n", fr->file_path);
        return -1;
    }

loop:
    printf_note("read_buffer_len=%u, read offset=%u,len=%d\n", fr->read_buffer_len, fr->read_offset, len);
    if(fr->read_offset == 0){
        ret = file_reload_buffer(filename);
        if(ret < 0){
            fr->read_flags = FILE_READ_IDLE;
            printf_err("read err %d\n", ret);
            return ret;
        }else if(ret == 0){
            fr->read_flags = FILE_READ_IDLE;
            printf_note("read over\n");
            return 0;
        }else{
            printf_note("reading\n");
        }
    }
    memset(buf, 0, len);
    if(len > fr->read_buffer_len - fr->read_offset){
        len =  fr->read_buffer_len - fr->read_offset;
        printf_note("cut len:%d\n", len);
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
    
    static char buf[4096];
    char *filename = cl->dispatch.file.filename;
    int r, i;

    printf_warn("download:name=%s, cmd=%d\n", filename, cl->dispatch.cmd);
    while (cl->us->w.data_bytes < 256) {
        r = file_read(filename, buf, sizeof(buf));
        printf_info("r=%d\n", r);
        if (r < 0) {
            printf_warn("read error\n");
            if (errno == EINTR)
                continue;
            uh_log_err("read");
        }

        if (r <= 0) {
            printf_warn("request_done:%d\n", cl->us->w.data_bytes);
            cl->request_done(cl);
            return;
        }
        
        cl->send(cl, buf, r);
    }
    printf_warn("w.data_bytes:%d\n", cl->us->w.data_bytes);
    return true;
}

void file_handle_init(void)
{
    memshare_init();
    memset(&req_readx, 0, sizeof(struct file_request_read));
}
