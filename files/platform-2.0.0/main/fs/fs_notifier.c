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
#include "fs_notifier.h"
#include "../log/log.h"
#include "../utils/utils.h"
#include "../executor/spm/spm.h"

#ifdef CONFIG_PROTOCOL_ACT
void akt_send_file_status(void *data, size_t data_len, void *args);
#endif
extern uint32_t config_get_disk_file_notifier_timeout(void);



static double difftime_us_val(const struct timeval *start, const struct timeval *end)
{
    double d;
    time_t s;
    suseconds_t u;
    s = end->tv_sec - start->tv_sec;
    u = end->tv_usec - start->tv_usec;
    d = s;
    d *= 1000000.0;//1 �� = 10^6 ΢��
    d += u;
    return d;
}

static struct str_status {
    file_notify_status code;
    char *str;
}str_status_st;

static struct str_status status_s[] ={
    {FNS_NEW,                           "*NEW*"},
    {FNS_STORE,                         "*STORE*"},
    {FNS_FINISH,                        "*FINISH*"},
    {FNS_UN_OVER,                       "*ABNOLMAL*"},
    {FNS_OVER,                          "*OVER*"},
};

static inline char *get_str_status(file_notify_status code)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(status_s); i++){
        if(status_s[i].code == code)
            return status_s[i].str;
    }

    return "undefined";
}


/* 定时发送中断 */
static void _fs_notifier_time_out(struct uloop_timeout *timeout)
{
    struct timeval *beginTime, endTime;
    
    struct fs_notifier *fsn = container_of(timeout, struct fs_notifier, timeout);
    struct fs_node node;
    
    /* 1: 发送完成分片文件链表 */
    struct fs_node *flist, *file_list_tmp;
    list_for_each_entry_safe(flist, file_list_tmp, &fsn->flist, list){
        node.ch = flist->ch;
        node.duration_time = flist->duration_time;
        node.filename = flist->filename;
        node.nbyte = flist->nbyte;
        /* send finish file */
        node.status = flist->status;
#ifdef CONFIG_PROTOCOL_ACT
        akt_send_file_status(&node, sizeof(struct fs_node), fsn);
#endif
        printf_note("list file[ch=%d, file=%s, status=%s, time=%zd]\n", 
                    node.ch,node.filename, get_str_status(node.status), node.duration_time);
        
        list_del(&flist->list);
        if(fsn->flist_num > 0)
            fsn->flist_num--;
        safe_free(flist->filename);
        safe_free(flist);
    }
    
    /* 2: 发送运行文件节点属性 */
    double  diff_time_us = 0;
    
    if(fsn->fnode == NULL){
        printf_note("fnode null\n");
        goto exit;
    }
    
    pthread_mutex_lock(&fsn->lock);
    gettimeofday(&fsn->fnode->end_time, NULL);
    diff_time_us = difftime_us_val(&fsn->fnode->start_time, &fsn->fnode->end_time);
    node.ch = fsn->fnode->ch;
    node.duration_time = diff_time_us/1000000;
    node.filename = fsn->fnode->filename;
    node.nbyte = fsn->fnode->nbyte;
    node.status = fsn->fnode->status;
    pthread_mutex_unlock(&fsn->lock);
#ifdef CONFIG_PROTOCOL_ACT
    akt_send_file_status(&node, sizeof(struct fs_node), fsn);
#endif
    printf_note("file [ch=%d, file=%s, status=%s, time=%zd]\n", 
        node.ch,node.filename, get_str_status(node.status), node.duration_time);
    
exit:
    uloop_timeout_set(timeout, fsn->timeout_ms); 
}

/* 
    完成一个分片文件后加入链表
    更新文件完成时间等属性值
*/
int fs_notifier_add_node_to_list(int ch, char *filename, struct fs_notifier *args)
{
    struct fs_node *node;
    node = args->fnode;
    
    pthread_mutex_lock(&args->lock);
    gettimeofday(&node->end_time, NULL);
    node->duration_time = difftime_us_val(&node->start_time, &node->end_time)/1000000;
    node->status = FNS_FINISH;
    list_add(&args->fnode->list, &args->flist);
    args->flist_num ++;
    pthread_mutex_unlock(&args->lock);
	return 0;
    
}

void fs_notifier_update_file_size(struct fs_notifier *args, uint32_t size_byte)
{
    //pthread_mutex_lock(&args->lock);
    args->fnode->nbyte += size_byte;
    //pthread_mutex_unlock(&args->lock);
}




int fs_notifier_create_node(int ch, char *filename, struct fs_notifier  *args)
{
    if(args == NULL)
        return -1;

    pthread_mutex_lock(&args->lock);
    struct fs_node *node = calloc(1, sizeof(struct fs_node));
    if(node == NULL){
        pthread_mutex_unlock(&args->lock);
        return -1;
    }
        

    memset(node, 0,  sizeof(struct fs_node));
    node->filename = safe_strdup(filename);
    node->status = FNS_NEW;
    node->ch = ch;
    node->send_new = false;
    gettimeofday(&node->start_time, NULL);
    gettimeofday(&node->end_time, NULL);
    args->fnode = node;

    /* 发送新文件状态通知： send new file */
#ifdef CONFIG_PROTOCOL_ACT
    akt_send_file_status(node, sizeof(struct fs_node), args);
#endif
    printf_note("file %s: %s\n", node->filename, get_str_status(node->status));
    node->send_new = true;
    /* 文件状态进入存储状态 */
    node->status = FNS_STORE;
    
    pthread_mutex_unlock(&args->lock);


    return 0;
}


int fs_notifier_init(int ch, char *filename, struct fs_notifier *args)
{
    uint32_t timer_ms = 0;
    if(args == NULL)
        return -1;

    /* 创建状态上报定时器 */
    struct fs_notifier  *fsn = args;
    fsn->timeout.cb = _fs_notifier_time_out;
    timer_ms = config_get_disk_file_notifier_timeout();
    args->timeout_ms = timer_ms;
    uloop_timeout_set(&fsn->timeout, timer_ms);
    printf_note("notifer timer start %d.mSec\n", timer_ms);
    
    /* 初始化分片文件链表 */
    INIT_LIST_HEAD(&args->flist);
    /* 初始化文件节点属性 */
    pthread_mutex_init(&args->lock,  NULL);
    fs_notifier_create_node(ch, filename, args);

    return 0;
}

int fs_notifier_exit(int ch, char *filename, struct fs_notifier *args, bool exit_ok)
{
    struct fs_node fnode;
    uint32_t timeout_ms = config_get_disk_file_notifier_timeout()+1000;/* +1000ms */
    if(args == NULL)
        return -1;

    do{
        usleep(1000);
        timeout_ms -- ;
    }while(args->flist_num > 0 && timeout_ms > 0);
    if(timeout_ms <= 0){
        printf_note("exit! wait failed!!!\n");
    }
    uloop_timeout_cancel(&args->timeout);
    pthread_mutex_lock(&args->lock);
    if(args->fnode){
        if(exit_ok)
            args->fnode->status = FNS_OVER;
        else
            args->fnode->status = FNS_UN_OVER;
#ifdef CONFIG_PROTOCOL_ACT
        akt_send_file_status(args->fnode, sizeof(struct fs_node), args);
#endif
        printf_note("file: %s, %s\n", args->fnode->filename, get_str_status(args->fnode->status));
        safe_free(args->fnode->filename);
        safe_free(args->fnode);
    }else {
        fnode.ch = ch;
        fnode.filename = filename;
        fnode.status = FNS_OVER;
#ifdef CONFIG_PROTOCOL_ACT
        akt_send_file_status(&fnode, sizeof(struct fs_node), args);
#else
        fnode = fnode;  /* for warm*/
#endif
        printf_note("file: %s, %s", filename, get_str_status(args->fnode->status));
    }
    pthread_mutex_unlock(&args->lock);
    printf_note("send file status over\n");

    return 0;
}



