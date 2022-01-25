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
#include "parse_cmd.h"
#include "../../utils/memshare.h"
#if 0
#include "../../log/log.h"
#include "protocol/http/file.h"
#include "request.h"
#include "request_file.h"
#include "../../bsp/io.h"
#include "utils/memshare.h"

#endif
extern int file_read_attr_info(const char *name, void *info);

static struct request_info http_req_cmd[] = {
    /* 磁盘文件操作 */
#if defined(CONFIG_FS_XW)
    {"GET",     "/file/@filename",               DISPATCH_DOWNLOAD_CMD,    cmd_file_download},
#endif
    {"DELETE",  "/file",                                -1,                cmd_file_delete},      //单个文件删除命令
    {"POST",    "/file/delete",                         -1,                cmd_file_batch_delete},//批量文件删除命令
    {"GET",     "/file/autiostore/@ch/@enable",         -1,                cmd_file_auto_store},    //自动存储文件
    {"GET",     "/file/store/@ch/@enable",              -1,                cmd_file_store},         //存储文件
    {"GET",     "/file/pause_store/@ch/@enable",        -1,                cmd_file_pause_store},
    {"GET",     "/file/pause_backtrace/@ch/@enable",    -1,                cmd_file_pause_backtrace},
    {"GET",     "/file/backtrace/@ch/@enable",          -1,                cmd_file_backtrace},     //回溯文件
    {"GET",     "/file/backtrace/offset/@ch/@value",    -1,                cmd_file_backtrace_offset},  //文件指定位置回溯
    {"GET",     "/file/list",                           -1,                cmd_file_list},       //获取根目录下文件列表
    {"GET",     "/file/find/@filename",                 -1,                cmd_file_find},       //根据当前目录查找当前目录文件
    {"GET",     "/file/findrec/@filename",              -1,                cmd_file_find_rec},   //根据当前目录递归查找文件
    {"GET",     "/file/dir",                            -1,                cmd_file_dir},        //根据当前目录显示当前目录和文件信息
    {"GET",     "/file/size",                           -1,                cmd_file_size},       //根据文件名称获取文件大小
    {"GET",     "/file/download_cb",                    -1,                cmd_file_download_cb},//文件下载完成回调
    {"GET",     "/folder",                              -1,                cmd_folder_create},   //文件夹新建
    {"DELETE",  "/folder",                              -1,                cmd_folder_delete},   //文件夹删除
    {"GET",     "/disk/format",                         -1,                cmd_disk_format},     //磁盘格式化
    
    /* 模式参数设置 */
    {"POST",    "/mode/mutiPoint/@ch",                  -1,                cmd_muti_point},
    {"POST",    "/mode/multiBand/@ch",                  -1,                cmd_multi_band},
    {"POST",    "/demodulation/@ch/@subch",             -1,                cmd_demodulation},
    {"POST",    "/nddc/@ch/@subch",                     -1,                cmd_demodulation},
    {"POST",    "/bddc/@ch",                            -1,                cmd_bddc},
    /* 中频参数设置 */
    {"PUT",     "/if/@ch/@subch/@type/@value",          -1,                cmd_if_single_value_set},
    {"PUT",     "/if/@ch/@subch/@type/@value/@value2",  -1,                cmd_if_single_value_set},
    {"PUT",     "/if/@ch/@type/@value",                 -1,                cmd_if_single_value_set},
    {"POST",    "/if/@ch/@subch",                       -1,                cmd_if_multi_value_set},
    {"POST",    "/if/@ch",                              -1,                cmd_if_multi_value_set},
    /* 射频参数设置 */
    {"PUT",     "/rf/@ch/@subch/@type/@value",          -1,                cmd_rf_single_value_set},
    {"PUT",     "/rf/@ch/@type/@value",                 -1,                cmd_rf_single_value_set},
    {"POST",    "/rf/@ch/@subch",                       -1,                cmd_rf_multi_value_set},
    {"POST",    "/rf/@ch",                              -1,                cmd_rf_multi_value_set},
    /* 网络参数 */
    {"POST",    "/net",                                 -1,                cmd_netset},
    {"GET",     "/net",                                 -1,                cmd_netget},
    {"POST",    "/net/@ch/client",                      -1,                cmd_net_client},
    {"POST",    "/net/@ch/client/@type",                -1,                cmd_net_client_type},
    /* 使能控制 */
    {"PUT",     "/enable/@ch/@subch/@type/@value",      -1,                cmd_subch_enable_set},
    {"PUT",     "/enable/@ch/@type/@value",             -1,                cmd_ch_enable_set},
    /* 心跳 */
    {"GET",     "/ping",                                -1,                cmd_ping},
    /* 状态 */
    {"GET",     "/status/device/softversion",           -1,                cmd_get_softversion},
    {"GET",     "/status/device/fpgaInfo",              -1,                cmd_get_fpga_info},
    {"GET",     "/status/device/all",                   -1,                cmd_get_all_info},
    {"GET",     "/status/device/selfcheckInfo",         -1,                cmd_get_selfcheck_info},
    /* 控制参数 */
    /* 时间源 */
    {"PUT",     "/timesource/@value",                   -1,                cmd_timesource_set},
    /* 射频电源开关 */
    {"PUT",     "/rf/power/@value",                     -1,                cmd_rf_power_onoff},
    /* 射频AGC工作模式 */
    {"PUT",     "/rf/agcCtrlMode/@value",               -1,                cmd_rf_agcmode},
    /* 设备编码 */
    {"PUT",     "/devicecode/@value",                   -1,                cmd_set_devicecode},
};

#define URL_SEPARATOR "/"
/* 比较模板url和请求url参数个数是否相等 */
static bool url_param_num_is_equal(const char * url, const char * url_format)
{
    char * saveptr = NULL, * cur_word = NULL, * url_cpy = NULL, * url_cpy_addr = NULL;
    char * saveptr_prefix = NULL, * cur_word_format = NULL, * url_format_cpy = NULL, * url_format_cpy_addr = NULL;
    int url_format_param_num = 0, url_param_num= 0;
    int ret = false;
    if(url == NULL || url_format == NULL)
        return -1;
    url_cpy = url_cpy_addr = strdup(url);
    url_format_cpy = url_format_cpy_addr = strdup(url_format);
    cur_word = strtok_r( url_cpy, URL_SEPARATOR, &saveptr );
    while (cur_word != NULL){
        cur_word = strtok_r( NULL, URL_SEPARATOR, &saveptr );
        url_param_num ++;
    }
    cur_word_format = strtok_r( url_format_cpy, URL_SEPARATOR, &saveptr_prefix );
    while (cur_word_format != NULL){
        cur_word_format = strtok_r( NULL, URL_SEPARATOR, &saveptr_prefix );
        url_format_param_num ++;
    }
    if(url_format_param_num == url_param_num){
        ret =  true;
    }
    printf_debug("url_format_param_num=%d, url_param_num=%d\n", url_format_param_num, url_param_num);
    free(url_cpy_addr);
    free(url_format_cpy_addr);
    url_cpy_addr = NULL;
    url_format_cpy_addr = NULL;
    return ret;
}

/* 分析模板url和请求url是否一致，若一直获取参数 */
int parse_format_url(struct uh_client *cl, const char * url, const char * url_format)
{
    char * saveptr = NULL, * cur_word = NULL, * url_cpy = NULL, * url_cpy_addr = NULL;
    char * saveptr_prefix = NULL, * cur_word_format = NULL, * url_format_cpy = NULL, * url_format_cpy_addr = NULL;
    char concat_url_param[512];
    char *s;
    int index = 0;
    int ret = 0;
    url = cl->get_path(cl);
    if(url == NULL || url_format == NULL)
        return -1;
    if(url_param_num_is_equal(url, url_format) == false){
        return -1;
    }
    url_cpy = url_cpy_addr = strdup(url);
    url_format_cpy = url_format_cpy_addr = strdup(url_format);
    printf_debug("url_format_cpy=%s\n",url_format_cpy);
    s = strchr(url_format_cpy, ':');
    if (s == NULL) {
        s = strchr(url_format_cpy, '@');
        /* 在url_format中没有变量参数 */
        if (s == NULL) {
            if (strcmp(url_cpy, url_format_cpy) == 0){
                ret = 0;    /* 匹配 */
            }else{
                ret = -1;    /* 未匹配 */
            }
            goto exit;
        }
    }
    cur_word = strtok_r( url_cpy, URL_SEPARATOR, &saveptr );
    cur_word_format = strtok_r( url_format_cpy, URL_SEPARATOR, &saveptr_prefix );
    while (cur_word_format != NULL && cur_word != NULL){
        printf_debug("cur_word=%s, cur_word_format=%s\n",cur_word, cur_word_format);
        if (strcmp(cur_word, cur_word_format) != 0) {
            /* 判断模板是否有参数 */
            if (cur_word_format[0] == ':' || cur_word_format[0] == '@'){
                //printf_note("%s=%s\n",cur_word_format+1, cur_word);
                /* 判断参数是否为文件，filename */
                if(!strcmp(cur_word_format+1, "filename")){
                    /* 判断是否为有效文件名称 */
                    if(strchr(cur_word, '.') == NULL){
                        printf_debug("%s is command\n", cur_word);
                        ret = -1;
                        break;
                    }
                }
                sprintf(concat_url_param, "%s,%s", cur_word_format+1, cur_word);
                cl->parse_resetful_var(cl, concat_url_param);
            }else{
                ret = -1;
                break;
            }
        }
        cur_word = strtok_r( NULL, URL_SEPARATOR, &saveptr );
        cur_word_format = strtok_r( NULL, URL_SEPARATOR, &saveptr_prefix );
    }
    url_cpy = url_cpy_addr = strdup(url);
exit:
    free(url_cpy_addr);
    free(url_format_cpy_addr);
    url_cpy_addr = NULL;
    url_format_cpy_addr = NULL;
    return ret;
}

size_t refill_buffer_file(void)
{
    struct stat stats;
    char path_phys[] = "/spectrum.xml";
    
    if (!stat(path_phys, &stats)){
        printf_note("(%s)st_size:%lu, st_blocks:%lu, st_mode:%x\n", path_phys, stats.st_size, stats.st_blocks, stats.st_mode);
        read_file(memshare_get_dma_rx_base(), stats.st_size, path_phys);
    }
    return stats.st_size;

}

#ifdef CONFIG_FS_XW
ssize_t http_request_fill_path_info(struct uh_client *cl, const char *filename, struct path_info *pi)
{
    struct path_info *p;
    struct disk_file_info fi;
    //char *filename;
    int ret;
    
    if(filename == NULL){
        return -1;
    }
    printf_info("filename=%s\n", filename);
    /* get file info */
    ret = file_read_attr_info(filename, (void *)&fi);
    if((ret = http_err_code_check(ret)) != 0){
        return ret;
    }  
    printf_note("ret =%d,file_path=%s, st_ctime=%s, st_blocks=%x,st_blksize=%x,st_size=%lx\n", 
        ret, fi.file_path, asctime(gmtime(&fi.ctime)), fi.st_blocks,fi.st_blksize,fi.st_size);

    p = pi;
    p->stat.st_mode = S_IFBLK|S_IRUSR|S_IROTH;   /*  区块装置,文件所有者具可读取权限,其他用户具可读取权限 */
    p->stat.st_mtime = fi.ctime;                /* time of last modification -最近修改时间*/   
    p->stat.st_atime = fi.ctime;                /* time of last access -最近存取时间*/
    p->stat.st_ctime = fi.ctime;                /* 创建时间   */
    p->stat.st_blocks = fi.st_blocks;               /* number of blocks allocated -文件所占块数*/ 
    p->stat.st_blksize = fi.st_blksize;           /* blocksize for filesystem I/O -系统块的大小*/
    p->stat.st_size = fi.st_size;        /* total size, in bytes -文件大小，字节为单位*/ 
    p->phys=filename;
    p->name=filename;
    p->info = filename;

    return 0;
}
#endif

int http_on_request(struct uh_client *cl)
{
    const char *path, *filename = NULL;
    int ret = -1;
    char *err_msg= NULL, *content=NULL;
    path = cl->get_path(cl);
    if(path ==NULL)
        return UH_REQUEST_DONE;

    printf_info("accept path: %s,query=%s\n", path, cl->get_query(cl));
    
    /* Check the request path  */
    for(int i = 0; i<sizeof(http_req_cmd)/sizeof(struct request_info); i++){
        if(strcmp(cl->get_method(cl), http_req_cmd[i].method)){
            continue;
        }
        if(parse_format_url(cl, path, http_req_cmd[i].path) == 0){
            if(http_req_cmd[i].dispatch_cmd == -1){
                 if(http_req_cmd[i].action){
                    ret = http_req_cmd[i].action(cl, (void **)&err_msg, (void **)&content);
                    printf_info("action result: %d, %s\n", ret, err_msg);
                    if(err_msg){
                        cl->send_json(cl, ret, err_msg, content);
                        if(RESP_CODE_EXECMD_REBOOT == ret){
                            printf_warn("Platform Exit, Wait Reboot For Watchdog!\n");
                            exit(0);
                        }
                    }
                 }
                 cl->update_keepalive(cl);
                 return UH_REQUEST_DONE;
            }else{
                cl->dispatch.cmd = http_req_cmd[i].dispatch_cmd;
                return UH_REQUEST_CONTINUE;
            }
        }
    }
    cl->dispatch.cmd = 0;
    return UH_REQUEST_CONTINUE;
}



void http_request_action(struct uh_client *cl)
{
    int found = 0;
    char *err_msg= NULL;
    int ret = -1;
    for(int i = 0; i<ARRAY_SIZE(http_req_cmd); i++){
        if(cl->dispatch.cmd == http_req_cmd[i].dispatch_cmd){
            ret = http_req_cmd[i].action(cl, (void **)&err_msg, (void **)NULL);
            printf_info("action result: %d, %s\n", ret, err_msg);
            found = 1;
            break;
        }
    }
    if(found == 0){
        printf_warn("request action not found [cmd=%d]\n",cl->dispatch.cmd);
    }
}

bool http_dispatch_requset_handle(struct uh_client *cl, const char *path)
{
    struct path_info pi;
    ssize_t err;
    const char *filename =NULL;
    switch(cl->dispatch.cmd)
    {
        case DISPATCH_DOWNLOAD_CMD:
            filename = cl->get_restful_var(cl, "filename");
            if(filename == NULL)
                return false;
            /* 判断是否为有效文件 */
            if(strchr(filename, '.') == NULL){
                return false;
            }
            printf_note("dispatch.cmd=%d, filename=%s\n",cl->dispatch.cmd, filename);
#if defined(CONFIG_FS_XW)
            err = http_request_fill_path_info(cl, filename, &pi);
            if(err != 0){
                printf_warn("err code=%d\n",err);
                return false;
            }
            if(file_read_attr(filename) == -1){
                printf_err("file download error\n");
                return false;
            }
            lseek(xwfs_get_fd(), 0, SEEK_SET);
            uh_blk_file_response_header(cl, &pi); 
 #endif
            http_request_action(cl);
            break;
    }
    
    return true;
}


#ifdef CONFIG_FS_XW
void http_requset_init(void)
{
    file_handle_init();
    //refill_buffer_file();
}
#endif

