#ifndef _NET_THREAD_H
#define _NET_THREAD_H

#define MAX_NET_THREAD 32

struct net_thread_wait{
    pthread_cond_t t_cond;
    pthread_mutex_t t_mutex;
    volatile bool is_wait;
};

struct net_thread_m{
    pthread_t tid;
    char *name;
    void *args;
    void *client;
    void *statistics;
    int prio;
    struct net_thread_wait pwait;
};

struct thread_consume_fin {
    pthread_mutex_t count_lock;
    pthread_cond_t count_cond;
    unsigned int count;
};

struct net_thread_consume{
    int num;
    int id;
};

struct net_thread_ops {
    int (*close)(void *);
};



struct net_thread_context {
    struct net_thread_m thread;
 //   int t_index;
    void *args;
    //int thread_count;
    struct net_thread_consume consum[MAX_XDMA_DISP_TYPE_NUM];
    struct thread_consume_fin fin_con;
    struct net_thread_ops *ops;
};

extern void  net_thread_con_broadcast(int ch, void *args);
extern struct net_thread_context * net_thread_create_context(void *cl);
extern uint64_t get_in_statistics_byte(int ch);
extern uint64_t get_out_statistics_byte(int ch);
extern uint64_t get_read_statistics_byte(int ch);
extern uint64_t get_ok_out_statistics_byte(int ch);
extern uint64_t  get_send_bytes_by_type(int ch, int type, int id);

#endif

