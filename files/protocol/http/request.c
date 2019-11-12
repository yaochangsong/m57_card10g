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
#include "request.h"
#include "request_file.h"
#include "executor/io.h"





static struct request_info http_req_cmd[] = {
    {"/file/*.*",               BLK_FILE_DOWNLOAD_CMD,          file_download},
    {"/file/download",          BLK_FILE_DOWNLOAD_CMD,          file_download},
    {"/file/startstore",        BLK_FILE_START_STORE_CMD,       file_startstore},
    {"/file/stopstore",         BLK_FILE_STOP_STORE_CMD,        file_stopstore},
    {"/file/search",            BLK_FILE_SEARCH_CMD,            file_search},
    {"/file/startbacktrace",    BLK_FILE_START_BACKTRACE_CMD,   file_start_backtrace},
    {"/file/stopbacktrace",     BLK_FILE_STOP_BACKTRACE_CMD,    file_stop_backtrace},
    {"/file/delete",            BLK_FILE_DELETE_CMD,            file_delete},
};


struct path_info *http_request_fill_path_info(struct uh_client *cl, const char *path)
{
    static struct path_info p;
    struct disk_file_info fi;
    char *filename;
    
    if(path == NULL)
        return;

    filename = cl->dispatch.file.path;
    if(filename == NULL){
        return;
    }
    printf_warn("filename=%s\n", filename);
    /* get file info */
    io_get_read_file_info(filename, (void *)&fi);
    printf_warn("fi.st_ctime=%s, fi.st_blocks=%u,fi.st_blksize=%u,fi.st_size=%llu\n", asctime(gmtime(&fi.ctime)), fi.st_blocks,fi.st_blksize,fi.st_size);
    p.stat.st_mode = S_IFBLK|S_IRUSR|S_IROTH;   /*  区块装置,文件所有者具可读取权限,其他用户具可读取权限 */
    p.stat.st_mtime = fi.ctime;                /* time of last modification -最近修改时间*/   
    p.stat.st_atime = fi.ctime;                /* time of last access -最近存取时间*/
    p.stat.st_ctime = fi.ctime;                /* 创建时间   */
    p.stat.st_blocks = fi.st_blocks;               /* number of blocks allocated -文件所占块数*/ 
    p.stat.st_blksize = fi.st_blksize;           /* blocksize for filesystem I/O -系统块的大小*/
    p.stat.st_size = REQUEST_FILESIZE_BUFFER_LEN;        /* total size, in bytes -文件大小，字节为单位*/ 
    p.phys=filename;
    p.name=filename;
    p.info = filename;

    return &p;
}


int http_on_request(struct uh_client *cl)
{
    const char *path, *filename = NULL;

    path = cl->get_path(cl);
    if(path ==NULL)
        return UH_REQUEST_CONTINUE;

    printf_warn("accept path: %s\n", path);
    
    /* Check if the request path is a file request cmd */
    for(int i = 0; i<sizeof(http_req_cmd)/sizeof(struct request_info); i++){
        if(!strcmp(http_req_cmd[i].path, path)){
            cl->dispatch.cmd = http_req_cmd[i].cmd;
            filename = cl->get_var(cl, "filename");
            printf_info("http request cmd: path=%s, cmd=%d\n", path, cl->dispatch.cmd);
            break;
        }
        if(strrchr(path, '.') && strrchr(http_req_cmd[i].path, '.')){
            cl->dispatch.cmd = http_req_cmd[i].cmd;
            filename = strrchr(path, '/')+1;
            printf_info("http download source: path=%s,filename=%s, cmd=%d\n", path, filename, cl->dispatch.cmd);
            break;
        }
    }
    if(filename != NULL){
        strncpy(cl->dispatch.file.path, filename, sizeof(cl->dispatch.file.path));
        cl->dispatch.file.path[sizeof(cl->dispatch.file.path) -1] = 0;
        printf_info("cl->dispatch.file.path=%s\n",cl->dispatch.file.path);
    }
    return UH_REQUEST_CONTINUE;
}


int http_request_action(struct uh_client *cl)
{
    const char *path, *filename = NULL;
    path = cl->get_path(cl);

    printf_warn("path=%s\n",path);
    for(int i = 0; i<sizeof(http_req_cmd)/sizeof(struct request_info); i++){
        if(!strcmp(http_req_cmd[i].path, path)){
            filename = cl->get_var(cl, "filename");
            printf_warn("filename=%s\n",filename);
            http_req_cmd[i].action(cl, NULL);
            break;
        }
        if(strrchr(path, '.') && strrchr(http_req_cmd[i].path, '.')){
            filename = strrchr(path, '/')+1;
            printf_warn("filename=%s\n",filename);
            http_req_cmd[i].action(cl, NULL);
            break;
        }
    }
}

bool http_requset_handle_cmd(struct uh_client *cl, const char *path)
{
    struct path_info *pi;
    
    switch(cl->dispatch.cmd)
    {
        case BLK_FILE_DOWNLOAD_CMD:
            pi = http_request_fill_path_info(cl, path);
            uh_blk_file_response_header(cl, pi); 
            http_request_action(cl);
            break;
        case BLK_FILE_START_STORE_CMD:
            http_request_action(cl);
            break;
        case BLK_FILE_STOP_STORE_CMD:
        case BLK_FILE_SEARCH_CMD:
        case BLK_FILE_START_BACKTRACE_CMD:
        case BLK_FILE_STOP_BACKTRACE_CMD:
        case BLK_FILE_DELETE_CMD:
            http_request_action(cl);
            break;
    }
    
    return true;
}


void http_requset_init(void)
{
    file_handle_init();
}

