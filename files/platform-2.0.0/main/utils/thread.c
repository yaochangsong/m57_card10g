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
*  Rev 1.0   22 June 2020   yaochangsong
*  Initial revision.
******************************************************************************/
#include <unistd.h> 
#include <stdlib.h>
#include "bitops.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include "errno.h"
#include "utils.h"


#define MAX_TEST_BITS 32

static DECLARE_BITMAP(thread_bmp, MAX_TEST_BITS);

struct thread_table_t{
    pthread_t tid;
    char *name;
    unsigned long index;
};

struct thread_bitmaps_t{
    struct thread_table_t thread_t[MAX_TEST_BITS];
    const unsigned long *bitmap;
};

struct thread_args{
    char *name;
    void *arg_exit;
    void *arg_cb;
    int (*init_callback)(void *);
    ssize_t (*callback)(void *);
    void (*exit_callback)(void *);
};


struct thread_bitmaps_t tbmp;

void pthread_bmp_init(void)
{
    memset(&tbmp, 0, sizeof(struct thread_bitmaps_t));
    tbmp.bitmap = thread_bmp;
    bitmap_zero(tbmp.bitmap, MAX_TEST_BITS);
}

void *thread_loop_cb (void *s)
{
   // thread_bind_cpu(1);
    int stateval;
    int ret;
    struct timeval beginTime, endTime;
    struct thread_args args;
    
    memcpy(&args, s, sizeof(struct thread_args));
    if(s)
        free(s);
    pthread_detach(pthread_self());
    
    pthread_cleanup_push(args.exit_callback, args.arg_exit);

    if(args.init_callback)
        args.init_callback(args.arg_cb);
    
    while(1){
        stateval = pthread_setcancelstate (PTHREAD_CANCEL_DISABLE , NULL);
        if (stateval != 0){
            printf("set cancel state failure\n");
        }
        if(args.callback){
            args.callback(args.arg_cb);
        }
        stateval  = pthread_setcancelstate (PTHREAD_CANCEL_ENABLE , NULL);
        pthread_testcancel();
    }
    pthread_cleanup_pop(1);
    
    return (void *)0;
}

int pthread_create_detach (const pthread_attr_t *attr, int (*init_callback) (void *),
                                        ssize_t (*start_routine) (void *), void (*exit_callback) (void *), 
                                        char *name, void *arg_cb,void *arg_exit, pthread_t *tid)
{

    struct thread_args *args;
    args = (struct thread_args *)calloc(1, sizeof(*args));
    if(args == NULL)
        return -1;
    args->name = name;
    args->init_callback = init_callback;
    args->arg_exit = arg_exit;
    args->arg_cb = arg_cb;
    args->callback = start_routine;
    args->exit_callback = exit_callback;
    int err, i;
    unsigned long   tid_index = 0;
    if(bitmap_weight(thread_bmp, MAX_TEST_BITS) >= MAX_TEST_BITS){
        printf("thread number %zd = max number %d\n", bitmap_weight(thread_bmp, MAX_TEST_BITS), MAX_TEST_BITS);
        return -1;
    }
    for(i = 0; i< ARRAY_SIZE(tbmp.thread_t); i++){
        if(tbmp.thread_t[i].name &&  !strcmp(tbmp.thread_t[i].name, name)){
            printf("[%s]thread is running\n", name);
            return -1;
        }
    }

    tid_index = find_first_zero_bit(thread_bmp, MAX_TEST_BITS);
    tbmp.thread_t[tid_index].name = strdup(name);

    err = pthread_create (&tbmp.thread_t[tid_index].tid , attr , thread_loop_cb , args);
    if (err != 0){
        printf("can't create thread: %s\n", strerror(err));
        return -1;
    }
    *tid = tbmp.thread_t[tid_index].tid; 
    set_bit(tid_index, thread_bmp);
    usleep(100);
    
    return 0;
}



int pthread_cancel_by_name(char *name)
{
    int i, cval, j=0;

    for(i = 0; i< ARRAY_SIZE(tbmp.thread_t); i++){
        if(tbmp.thread_t[i].name &&  !strcmp(tbmp.thread_t[i].name, name) && tbmp.thread_t[i].tid > 0){
           // printf("name=%s, index=%d, tid=%u\n", tbmp.thread_t[i].name, i, tbmp.thread_t[i].tid);
            cval = pthread_cancel(tbmp.thread_t[i].tid);
            if(cval !=0){
                printf("######cancel[%s] thread failure\n", name);
                return -1;
            }else{
                printf("######cancel[%s] thread ok[tid:%lu]\n", name, tbmp.thread_t[i].tid);
                if(tbmp.thread_t[i].name){
                    safe_free((void *)tbmp.thread_t[i].name);
                    tbmp.thread_t[i].name = NULL;
                    tbmp.thread_t[i].tid = 0;
                }
                clear_bit(i, thread_bmp);
                return 0;
            }
        }
    }
    return -1;
}

int pthread_cancel_by_tid(pthread_t tid)
{
    int i, cval;

    if(tid == 0)
        return -1;
    for(i = 0; i< ARRAY_SIZE(tbmp.thread_t); i++){
        if(tbmp.thread_t[i].tid == tid){
           // printf("name=%s, index=%d, tid=%u\n", tbmp.thread_t[i].name, i, tbmp.thread_t[i].tid);
            cval = pthread_cancel(tid);
            if(cval !=0){
                printf("######cancel[tid %lu] thread failure\n", tid);
                return -1;
            }else{
                printf("######cancel[%s,tid=%lu] thread ok\n", tbmp.thread_t[i].name, tid);
                if(tbmp.thread_t[i].name){
                    safe_free((void *)tbmp.thread_t[i].name);
                    tbmp.thread_t[i].name = NULL;
                    tbmp.thread_t[i].tid = 0;
                }
                clear_bit(i, thread_bmp);
                return 0;
            }
        }
    }
    printf("######can't find [tid %lu] thread in bitbmp, maybe have exited!\n", tid);
    cval = pthread_cancel(tid);
    if(cval !=0){
        printf("######cancel[tid %lu] thread failure\n", tid);
        return -1;
    }else 
        printf("######cancel[tid=%lu] thread ok\n", tid);

    return 0;
}


int pthread_exit_by_name(char *name)
{
    int i;
    for(i = 0; i< ARRAY_SIZE(tbmp.thread_t); i++){
        if(tbmp.thread_t[i].name &&  !strcmp(tbmp.thread_t[i].name, name) && tbmp.thread_t[i].tid > 0){
                printf("######[%s] thread exit\n", name);
                if(tbmp.thread_t[i].name){
                    safe_free((void *)tbmp.thread_t[i].name);
                    tbmp.thread_t[i].name = NULL;
                    tbmp.thread_t[i].tid = 0;
                }
                clear_bit(i, thread_bmp);
                pthread_exit(0);
                return 0;
        }
    }
    printf("NOT find thread %s\n", name);
    return -1;
}


bool pthread_check_alive_by_name(char *name)
{
    int i, kill_rc;
    bool is_alive = false;
    
    for(i = 0; i< ARRAY_SIZE(tbmp.thread_t); i++){
        if(tbmp.thread_t[i].name &&  !strcmp(tbmp.thread_t[i].name, name) && (tbmp.thread_t[i].tid > 0)){
                kill_rc = pthread_kill(tbmp.thread_t[i].tid, 0);
                if(kill_rc == ESRCH || kill_rc == EINVAL){
                    printf("[%s]thread not exists\n", name);
                }else{
                    is_alive = true;
                }
                break;
        }
    }
    return is_alive;
}

bool pthread_check_alive_by_tid(pthread_t tid)
{
    int i, kill_rc;
    bool is_alive = false;

    for(i = 0; i< ARRAY_SIZE(tbmp.thread_t); i++){
        if(tbmp.thread_t[i].tid == tid && tid > 0){
                kill_rc = pthread_kill(tid, 0);
                if(kill_rc == ESRCH || kill_rc == EINVAL){
                    printf("[tid:%lu]thread not exists\n", tid);
                }else{
                    is_alive = true;
                }
                break;
        }
    }
    return is_alive;
}


ssize_t thread_test (void *s)
{
    int i;
    i = *(int *)s;
    printf("my is test thread[%d]\n", i);
    sleep(1);
    return 0;
}

void pthread_init(void)
{
    pthread_bmp_init();
}

int main_thread_test(void)
{
    int i = 1, j, k,l,m,n,o;
    pthread_bmp_init();
    i = 1;
    pthread_create_detach(NULL,NULL, thread_test, NULL, "thead name test1", &i, NULL, NULL);
    //pthread_create_detach(NULL, thread_test, NULL, "thead name test1", &i, NULL);
    j = 2;
    pthread_create_detach(NULL,NULL, thread_test, NULL, "thead name test2", &j, NULL, NULL);
    //pthread_cancel_by_name("thead name test1");
    k = 3;
    pthread_create_detach(NULL,NULL, thread_test, NULL, "thead name test3", &k, NULL, NULL);
    l=4;
    pthread_create_detach(NULL,NULL, thread_test, NULL, "thead name test4", &l, NULL, NULL);
    //pthread_cancel_by_name("thead name test2");
    m=5;
    pthread_create_detach(NULL,NULL, thread_test, NULL, "thead name test5", &m, NULL, NULL);
    n=6;
    pthread_create_detach(NULL,NULL, thread_test, NULL, "thead name test6", &n, NULL, NULL);
    o=7;
    pthread_create_detach(NULL,NULL, thread_test, NULL, "thead name test7", &o, NULL, NULL);

    sleep(5);
    pthread_cancel_by_name("thead name test1");
    pthread_cancel_by_name("thead name test2");
    pthread_cancel_by_name("thead name test3");
    pthread_cancel_by_name("thead name test4");
    pthread_cancel_by_name("thead name test5");
    pthread_cancel_by_name("thead name test6");
    pthread_cancel_by_name("thead name test7");
    printf("main exit1\n");
    return 0;
}
