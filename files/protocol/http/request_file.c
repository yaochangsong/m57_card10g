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

int http_err_code_check(int ret)
{
    if(ret == 0){
        return 0;
    }
    if(ret == -EBUSY){
        printf_warn("Disk is busy now\n");
    }else if(ret == -ENOMEM || ret == EFAULT){
        printf_warn("No memory or Bad address:%d\n", ret);
    }else if(ret == -EEXIST){
        printf_warn("File exists\n");
    }else if(ret == -ENOENT){
        printf_warn("No such file\n");
    }
    return -1;
}


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


ssize_t readn(int fd, void *ptr, size_t n)
{
    size_t nleft;
    ssize_t nread;

    nleft = n;
    while(nleft > 0){
        if((nread = read(fd, ptr, nleft)) < 0){
            if(nleft == n)
                return -1;
            else
                break;
        }else if(nread == 0){
            break;
        }
        nleft -= nread;
        ptr += nread;
    }
    return (n - nleft);  /* return >=0 */
}

ssize_t readmem(void *ptr, size_t n)
{
    struct file_request_read *fr=&req_readx;
    ssize_t nleft;

    /* Check if buffer has data */
    if(fr->is_buffer_has_data == false)
        return -1;
    if(n > fr->read_buffer_len - fr->read_buffer_offset){
        /* The remain length of the read is greater than
           the remaining length of the buffer  */
        n = fr->read_buffer_len - fr->read_buffer_offset;
    }
    
    memcpy(ptr, fr->read_buffer_pointer + fr->read_buffer_offset, n);
    fr->read_buffer_offset += n;

    nleft = fr->read_buffer_len - fr->read_buffer_offset;
    printf_note("nleft=%d, read_buffer_len=0x%x, read_buffer_offset=0x%x\n", nleft, fr->read_buffer_len, fr->read_buffer_offset);
    if(nleft <= 0){
        fr->read_buffer_offset = 0;
        return 0; /* read buffer over */
    }
    return n;
}

int file_read_attr_info(const char *name, void *info)
{
    int ret = 0;
    struct disk_file_info *fi;
    if(name == NULL || info == NULL)
        return -1;
    strcpy((char *)info, name);
    ret = io_get_read_file_info(info);
    fi = (struct disk_file_info *)info;
    return ret;
}


int file_read_attr(const char *filename)
{
    struct disk_file_info fi;
    struct file_request_read *fr=&req_readx;
    int ret;
    
    ret = file_read_attr_info(filename, (void *)&fi);
    if(http_err_code_check(ret)== -1){
        return -1;
    }
    printf_note("file_path:%s,buffer_len:0x%x,st_size=0x%llx, st_blocks=%x\n", 
                fi.file_path,fi.buffer_rx_len,fi.st_size, fi.st_blocks);
    strcpy(fr->file_path,fi.file_path);
    fr->read_buffer_pointer = file_get_read_buffer_pointer();
    fr->read_buffer_len =  fi.buffer_rx_len;
    fr->read_buffer_offset = 0;
    fr->offset_size = 0;
    fr->st_size = fi.st_size;
    fr->is_buffer_has_data = false;
    lseek(io_get_fd(), 0, SEEK_SET);
    return 0;
}


/* before call file_read, we must read file attr */
static ssize_t file_read(const char *filename, uint8_t *ptr, int n)
{
    struct file_request_read *fr=&req_readx;
    int ret = 0;
    ssize_t nread;
    int64_t nleft;
    
    nleft = fr->st_size - fr->offset_size;
    printf_note("n=%d, nleft=%llx, st_size:0x%llx, offset_size:0x%llx\n", 
        n, nleft, fr->st_size, fr->offset_size);
    if(nleft <= 0){
        return 0;   /* read file over */
    }
    if(n > nleft){
        n = nleft;
    }
loop:
    if((nread = readmem(ptr, n)) <= 0){
        ret = file_reload_buffer(filename);
        if(ret < 0){
            printf_err("read file err %d\n", ret);
            return ret;
        }else if(ret == 0){
            printf_note("read file over\n");
            return 0;
        }else{
            fr->is_buffer_has_data = true;
            printf_note("reading\n");
            goto loop;
        }
    }
    fr->offset_size +=  nread;
    if(fr->offset_size >= fr->st_size){
        return 0; /* read file over */
    }
    return nread;
}

int file_download(struct uh_client *cl, void *arg)
{
    
    static char buf[4096];
    char *filename = cl->dispatch.file.filename;
    int r, i;

    printf_info("download:name=%s, cmd=%d\n", filename, cl->dispatch.cmd);
    if(file_read_attr(filename) == -1){
        printf_err("file download error\n");
        return -1;
    }
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
            return 0;
        }
        
        cl->send(cl, buf, r);
    }
    printf_info("w.data_bytes:%d\n", cl->us->w.data_bytes);
    return 0;
}

void file_handle_init(void)
{
    memshare_init();
    memset(&req_readx, 0, sizeof(struct file_request_read));
}
