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
#include "bitops.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include "utils.h"

#define MAX_TEST_BITS 20

static DECLARE_BITMAP(thread_bmp, MAX_TEST_BITS);

static struct thread_table_t{
    pthread_t tid;
    char *name;
    unsigned long index;
};

static struct thread_bitmaps_t{
    struct thread_table_t thread_t[MAX_TEST_BITS];
    const unsigned long *bitmap;
};

struct thread_args{
    char *name;
    void *arg;
    void *arg_cb;
    void *(*callback)(void *);
    void *(*exit_callback)(void *);
};


struct thread_bitmaps_t tbmp;

void pthread_bmp_init(void)
{
    memset(&tbmp, 0, sizeof(struct thread_bitmaps_t));
    tbmp.bitmap = thread_bmp;
    bitmap_zero(tbmp.bitmap, MAX_TEST_BITS);
}

double difftime_us_val(const struct timeval *start, const struct timeval *end)
{
    double d;
    time_t s;
    suseconds_t u;

    s = end->tv_sec - start->tv_sec;
    u = end->tv_usec - start->tv_usec;
    d = s;
    d *= 1000000.0;//1 秒 = 10^6 微秒
    d += u;

    return d;
}

struct push_arg{
    struct timeval *ct;
    uint64_t nbyte;
    uint64_t count;
};


static void thread_exit_callback(void *arg){  
    struct timeval *beginTime, endTime;
    uint64_t nbyte;
    float speed = 0;
    double  diff_time_us = 0;
    float diff_time_s = 0;

    struct push_arg *pargs;
    pargs = (struct push_arg *)arg;
    beginTime = pargs->ct;
    nbyte = pargs->nbyte;
    fprintf(stderr, ">>start time %ld.%.9ld., nbyte=%llu\n",beginTime->tv_sec, beginTime->tv_usec, nbyte);
    
    gettimeofday(&endTime, NULL);
    fprintf(stderr, ">>end time %ld.%.9ld.\n",endTime.tv_sec, endTime.tv_usec);

    diff_time_us = difftime_us_val(beginTime, &endTime);
    diff_time_s = diff_time_us/1000000.0;
    printf(">>diff us=%fus, %fs\n", diff_time_us, diff_time_s);
    
     speed = (float)((nbyte / (1024 * 1024)) / diff_time_s);
     fprintf(stdout,"speed: %.2f MBps, count=%llu\n", speed, pargs->count);
}  

void *thread_loop (void *s)
{
   // thread_bind_cpu(1);
    int stateval;
    int ret;
    struct timeval beginTime, endTime;
    struct push_arg p_args;
    struct thread_args args;
    
    memcpy(&args, s, sizeof(struct thread_args));
    
    pthread_detach(pthread_self());
    pthread_cleanup_push(thread_exit_callback,&p_args);
    gettimeofday(&beginTime, NULL);

    fprintf(stderr, "#######start time %ld.%.9ld.\n",beginTime.tv_sec, beginTime.tv_usec);
    p_args.ct = &beginTime;
    p_args.nbyte = 0;
    p_args.count = 0;
    while(1){
        stateval = pthread_setcancelstate (PTHREAD_CANCEL_DISABLE , NULL);
        if (stateval != 0){
            printf("set cancel state failure\n");
        }
        if(args.callback){
            ret = args.callback(args.arg);
            p_args.count ++;
        }
        if(ret > 0){
            p_args.nbyte += ret;
        }
        stateval  = pthread_setcancelstate (PTHREAD_CANCEL_ENABLE , NULL);
        pthread_testcancel();
    }
    pthread_cleanup_pop(1);
    return (void *)0;
}

int pthread_create_detach_loop (const pthread_attr_t *attr, 
                                        int (*start_routine) (void *), 
                                        char *name, void *arg)
{

    struct thread_args args;
    args.name = name;
    args.arg = arg;
    args.callback = start_routine;
    int err, i;
    unsigned long	tid_index = 0;
    if(bitmap_weight(thread_bmp, MAX_TEST_BITS) >= MAX_TEST_BITS){
        printf("thread number %d = max number %d\n", bitmap_weight(thread_bmp, MAX_TEST_BITS), MAX_TEST_BITS);
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

    err = pthread_create (&tbmp.thread_t[tid_index].tid , attr , thread_loop , &args);
    if (err != 0){
        printf("can't create thread: %s\n", strerror(err));
        return -1;
    }
    set_bit(tid_index, thread_bmp);
    usleep(100);
    
    return 0;
}

void *thread_loop_cb (void *s)
{
   // thread_bind_cpu(1);
    int stateval;
    int ret;
    struct timeval beginTime, endTime;
    struct push_arg p_args;
    struct thread_args args;
    
    memcpy(&args, s, sizeof(struct thread_args));
    
    pthread_detach(pthread_self());
    pthread_cleanup_push(args.exit_callback, args.arg_cb);
    gettimeofday(&beginTime, NULL);

    fprintf(stderr, "#######start time %ld.%.9ld.\n",beginTime.tv_sec, beginTime.tv_usec);

    while(1){
        stateval = pthread_setcancelstate (PTHREAD_CANCEL_DISABLE , NULL);
        if (stateval != 0){
            printf("set cancel state failure\n");
        }
        if(args.callback){
            ret = args.callback(args.arg);
            //p_args.count ++;
        }
        if(ret > 0){
           // p_args.nbyte += ret;
        }
        stateval  = pthread_setcancelstate (PTHREAD_CANCEL_ENABLE , NULL);
        pthread_testcancel();
    }
    pthread_cleanup_pop(1);
    return (void *)0;
}

int pthread_create_detach (const pthread_attr_t *attr, 
                                        int (*start_routine) (void *), int (*exit_callback) (void *), 
                                        char *name, void *arg_st, void *arg_cb)
{

    struct thread_args args;
    args.name = name;
    args.arg = arg_st;
    args.arg_cb = arg_cb;
    args.callback = start_routine;
    args.exit_callback = exit_callback;
    int err, i;
    unsigned long   tid_index = 0;
    if(bitmap_weight(thread_bmp, MAX_TEST_BITS) >= MAX_TEST_BITS){
        printf("thread number %d = max number %d\n", bitmap_weight(thread_bmp, MAX_TEST_BITS), MAX_TEST_BITS);
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

    err = pthread_create (&tbmp.thread_t[tid_index].tid , attr , thread_loop_cb , &args);
    if (err != 0){
        printf("can't create thread: %s\n", strerror(err));
        return -1;
    }
    set_bit(tid_index, thread_bmp);
    usleep(100);
    
    return 0;
}



int pthread_cancel_by_name(char *name)
{
    int i, cval, j=0;

    for(i = 0; i< ARRAY_SIZE(tbmp.thread_t); i++){
        if(tbmp.thread_t[i].name &&  !strcmp(tbmp.thread_t[i].name, name)){
           // printf("name=%s, index=%d, tid=%u\n", tbmp.thread_t[i].name, i, tbmp.thread_t[i].tid);
            cval = pthread_cancel(tbmp.thread_t[i].tid);
            if(cval !=0){
                printf("######cancel[%s] thread failure\n", name);
                return -1;
            }else{
                printf("######cancel[%s] thread ok\n", name);
                if(tbmp.thread_t[i].name){
                    free(tbmp.thread_t[i].name);
                    tbmp.thread_t[i].name = NULL;
                }
                clear_bit(i, thread_bmp);
                return 0;
            }
        }
    }
    return -1;
}


int thread_test (void *s)
{
    int i;
    i = *(int *)s;
    printf("my is test thread[%d]\n", i);
    sleep(1);
}

int main_thread_test(void)
{
    int i = 1, j, k,l,m,n,o;
    pthread_bmp_init();
    i = 1;
    pthread_create_detach_loop(NULL, thread_test, "thead name test1", &i);
    pthread_create_detach_loop(NULL, thread_test, "thead name test1", &i);
    j = 2;
    pthread_create_detach_loop(NULL, thread_test, "thead name test2", &j);
    //pthread_cancel_by_name("thead name test1");
    k = 3;
    pthread_create_detach_loop(NULL, thread_test, "thead name test3", &k);
    l=4;
    pthread_create_detach_loop(NULL, thread_test, "thead name test4", &l);
    //pthread_cancel_by_name("thead name test2");
    m=5;
    pthread_create_detach_loop(NULL, thread_test, "thead name test5", &m);
    n=6;
    pthread_create_detach_loop(NULL, thread_test, "thead name test6", &n);
    o=7;
    pthread_create_detach_loop(NULL, thread_test, "thead name test7", &o);

    sleep(5);
    pthread_cancel_by_name("thead name test1");
    pthread_cancel_by_name("thead name test2");
    pthread_cancel_by_name("thead name test3");
    pthread_cancel_by_name("thead name test4");
    pthread_cancel_by_name("thead name test5");
    pthread_cancel_by_name("thead name test6");
    pthread_cancel_by_name("thead name test7");
    printf("main exit1\n");
}
