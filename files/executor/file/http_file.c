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

#include "http_file.h"
#include "config.h"
#include "protocol/http/file.h"

#define BUFFER_LEN  512

struct file_read{
    char *ptotalbuffer;
    char *pfile;
    int file_len;
    int buffer_total_len;
};
struct file_read gfile;

static uint8_t *file_http_get_file_buffer()
{
    //mmap
    return gfile.pfile;
}

int file_http_read_shutdown(char *filename)
{
    int64_t len = 0;
    //ioctl(filename, );//notify kernel read file data one by one
    return len;
}

int file_http_reload_buffer(void)
{
    static int offset = 0;
    offset += gfile.file_len;
    if(offset > gfile.buffer_total_len){
        offset = 0;
        printf_warn("reload buffer over\n");
        return 0;
    }
    memcpy(gfile.pfile, gfile.ptotalbuffer+offset, gfile.file_len);
    printf_warn("--------------offset=%d, reload offset =%d,gfile.buffer_total_len=%d\n", offset, gfile.file_len, gfile.buffer_total_len);
    return gfile.file_len;
}

int file_http_read(char *path, uint8_t *buf, int len)
{
    int read_result_len = 0, read_len = 0;
    char *ptr_head;
    static int offset = 0;
    static int buffer_total_len = 0;
    
    
    if(buf == NULL)
        return -1;

loop:
    printf_warn("read offset=%d\n", offset);
    if(offset == 0){
        buffer_total_len = file_http_reload_buffer();
        if(buffer_total_len <= 0){
            buffer_total_len = 0;
            offset = 0;
            printf_warn("read over buffer_total_len=%d*********\n", buffer_total_len);
            return 0;
        }
    }

    printf_warn("buffer_total_len=%d\n", buffer_total_len);
    ptr_head = file_http_get_file_buffer();
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
        printf_warn("reset offset = %d, read_result_len=%d\n", offset, read_result_len);
        offset = 0;
        read_result_len = 0;
        /* start reload buffer */
        //buffer_total_len = file_http_reload_buffer();
        //if(file_http_reload_buffer>0)
        goto loop;
    }
    printf_warn("offset = %d, read_result_len=%d\n", offset, read_result_len);
    return read_result_len;
}


struct path_info *file_http_fill_path_info(struct uh_client *cl, const char *path)
{
    static struct path_info p;
    char *filename;
    
    if(path == NULL)
        return;

    filename = strrchr(path, '/')+1;
    if(filename == NULL){
        return;
    }
    printf_warn("filename=%s\n", filename);
    /* get file info */
    p.stat.st_mode = S_IFBLK|S_IRUSR|S_IROTH;   /*  区块装置,文件所有者具可读取权限,其他用户具可读取权限 */
    p.stat.st_mtime = 0;                /* time of last modification -最近修改时间*/   
    p.stat.st_atime = 0;                /* time of last access -最近存取时间*/
    p.stat.st_ctime = 0;                /* 创建时间   */
    p.stat.st_blocks = 1;               /* number of blocks allocated -文件所占块数*/ 
    p.stat.st_blksize = 4096;           /* blocksize for filesystem I/O -系统块的大小*/
    p.stat.st_size = BUFFER_LEN;        /* total size, in bytes -文件大小，字节为单位*/ 
    p.phys=filename;
    p.name=filename;
    p.info = filename;

    return &p;
}


int file_http_on_request(struct uh_client *cl)
{
   
    const char *path = cl->get_path(cl);
    int body_len = 0;
    char *cmd;

    if(cl->get_query(cl) == NULL)
        return UH_REQUEST_CONTINUE;
    
    printf_warn("get_url %s\n", cl->get_url(cl));
    printf_warn("accept %s\n", cl->get_path(cl));
    printf_warn("get_query %s\n", cl->get_query(cl));
    
    cmd = cl->get_var(cl, "cmd");
    if(cmd == NULL)
        return UH_REQUEST_CONTINUE;

    if(!strcmp(cmd, HTTP_QUERY_READ_FILE_CMD)){
        cl->dispatch.file.cmd = BLK_FILE_READ_CMD;
        printf_warn("ReadFile\n");
    }else if(!strcmp(cmd, HTTP_QUERY_DELETE_FILE_CMD)){
        cl->dispatch.file.cmd = BLK_FILE_DELETE_CMD;
        printf_warn("DeleteFile\n");
    }else if(!strcmp(cmd, HTTP_QUERY_ADD_FILE_CMD)){
        cl->dispatch.file.cmd = BLK_FILE_ADD_CMD;
        printf_warn("AddFile\n");
    }else if(!strcmp(cmd, HTTP_QUERY_BACKTRACE_FILE_CMD)){
        cl->dispatch.file.cmd = BLK_FILE_BACKTRACE_CMD;
        printf_warn("BackTraceFile\n");
    }else if(!strcmp(cmd, HTTP_QUERY_SEARCH_FILE_CMD)){
        cl->dispatch.file.cmd = BLK_FILE_SEARCH_CMD;
        printf_warn("SearchFile\n");
    }

    return UH_REQUEST_CONTINUE;
}

void file_http_init(void)
{
    char text[]={1,2,3,4,5,6,7,8,9,10,11,12,13,14};
    
    
    gfile.ptotalbuffer = malloc(1024);
    memset(gfile.ptotalbuffer, 'a', 1024);
    gfile.buffer_total_len = 1024;

    gfile.pfile = malloc(BUFFER_LEN);
    memset(gfile.pfile, 0, BUFFER_LEN);
   // memcpy(gfile.pfile, text, sizeof(text));
    gfile.file_len = BUFFER_LEN;//sizeof(text);
    //printf_warn("%s, %d\n", gfile.pfile, gfile.file_len);
}
