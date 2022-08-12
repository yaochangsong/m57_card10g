#ifndef _FS_NITIFIER_H
#define _FS_NITIFIER_H

#include <stdbool.h>
#include <sys/statfs.h>
#include "../lib/libubox/uloop.h"

typedef enum _file_notify_status {
    FNS_NEW = 0,    /* 新文件 */
    FNS_STORE = 1,  /* 分片文件存储中 */
    FNS_FINISH = 2, /* 分片文件存储完成 */
    FNS_UN_OVER = 3,/* 文件存储异常完成 */
    FNS_OVER = 4,   /* 文件正常完成 */
}file_notify_status;

/* 文件节点属性 */
struct fs_node{
    struct list_head list;
    int ch;
    char *filename;
    uint64_t duration_time;
    uint64_t nbyte;
    file_notify_status status;     /* 文件存储状态 */
    struct timeval start_time;
    struct timeval end_time;
    bool send_new;                  /* 是否发送新建文件 */
};


struct fs_notifier {
    /* 完成分片文件链表 */
    struct list_head flist;
    struct fs_node *fnode;
    pthread_mutex_t lock;
    int flist_num;
    time_t timeout_ms;
    struct timeval start_time;
    struct uloop_timeout timeout;
    void *args;
};

extern int fs_notifier_init(int ch, char *filename, struct fs_notifier *args);
extern int fs_notifier_exit(int ch, char *filename, struct fs_notifier *args, bool exit_ok);
extern int fs_notifier_create_node(int ch, char *filename, struct fs_notifier  *args);
extern void fs_notifier_update_file_size(struct fs_notifier *args, uint32_t size_byte);
extern int fs_notifier_add_node_to_list(int ch, char *filename, struct fs_notifier *args);


#endif
