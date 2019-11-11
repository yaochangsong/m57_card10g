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

bool file_startstore(const char *path)
{
    printf_warn("startstore\n");
    return true;
}

bool file_stopstore(const char *path)
{
    printf_warn("stopstore\n");
    return true;
}

bool file_search(const char *path)
{
    printf_warn("search\n");
    return true;
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

    filename = cl->dispatch.file.path;
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
   
    const char *path, *filename;
    const char *file_path, *cmd;
    int body_len = 0;

    printf_warn("-------------------------------\n");
    printf_warn("get_url %s\n", cl->get_url(cl));
    printf_warn("accept %s\n", path);
    printf_warn("get_query %s\n", cl->get_query(cl));

    /*path format:  http://192.168.1.111/file/{cmd}?ch=1&subch=1&filename=channel001.iq 
        or          http://192.168.1.111/file/{filename}?ch=1&subch=1
    */
    path = cl->get_path(cl);
    if(path ==NULL)
        return UH_REQUEST_CONTINUE;

    /* Check if the request path is a file path  */
    file_path =  strchr(path, '/')+1;
    printf_warn("file path is:%s, %s,%d\n", file_path, HTTP_FILE_PATH,sizeof(HTTP_FILE_PATH));
    if(strncmp(file_path, HTTP_FILE_PATH, sizeof(HTTP_FILE_PATH)-1)){
        printf_warn("file_path is:%s\n", file_path);
        return UH_REQUEST_CONTINUE;
    }

    /* Check cmd or filename */
    cmd = strrchr(path, '/')+1;
    if(cmd == NULL)
        return UH_REQUEST_DONE;

    if(strrchr(cmd, '.')){
        /* is filename */
        strncpy(cl->dispatch.file.path, cmd, sizeof(cl->dispatch.file.path));
        cl->dispatch.file.path[sizeof(cl->dispatch.file.path) -1] = 0;
        cl->dispatch.file.cmd = BLK_FILE_DOWNLOAD_CMD;
        printf_warn("download the filename is:%s\n", cmd);
        printf_warn("cl->dispatch.file.path=%s\n\n", cl->dispatch.file.path);
        return UH_REQUEST_CONTINUE;
    }
        /* is cmd */
    if(!strcmp(cmd, HTTP_DOWNLOAD_FILE_CMD)){
        cl->dispatch.file.cmd = BLK_FILE_DOWNLOAD_CMD;
        printf_warn("download file\n");
    }else if(!strcmp(cmd, HTTP_START_STORE_FILE_CMD)){
        cl->dispatch.file.cmd = BLK_FILE_START_STORE_CMD;
        printf_warn("startstore file\n");
    }else if(!strcmp(cmd, HTTP_STOP_STORE_FILE_CMD)){
        cl->dispatch.file.cmd = BLK_FILE_STOP_STORE_CMD;
        printf_warn("stopstore\n");
    }else if(!strcmp(cmd, HTTP_SEARCH_FILE_CMD)){
        cl->dispatch.file.cmd = BLK_FILE_SEARCH_CMD;
        printf_warn("search file\n");
    }else if(!strcmp(cmd, HTTP_START_BACKTRACE_FILE_CMD)){
        cl->dispatch.file.cmd = BLK_FILE_START_BACKTRACE_CMD;
        printf_warn("start backtrace file\n");
    }else if(!strcmp(cmd, HTTP_STOP_BACKTRACE_FILE_CMD)){
        cl->dispatch.file.cmd = BLK_FILE_STOP_BACKTRACE_CMD;
        printf_warn("stop backtrace file\n");
    }else if(!strcmp(cmd, HTTP_DELETE_FILE_CMD)){
        cl->dispatch.file.cmd = BLK_FILE_DELETE_CMD;
        printf_warn("delete file\n");
    }

     /* get file name*/
    filename = cl->get_var(cl, "filename");
    if(filename == NULL)
        return UH_REQUEST_DONE;
    strncpy(cl->dispatch.file.path, filename, sizeof(cl->dispatch.file.path));
    cl->dispatch.file.path[sizeof(cl->dispatch.file.path) -1] = 0;
    printf_warn("cl->dispatch.file.path=%s\n", cl->dispatch.file.path);

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
