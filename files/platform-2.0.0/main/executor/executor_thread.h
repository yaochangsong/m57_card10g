#ifndef _EXECUTOR_V2_H_H
#define _EXECUTOR_V2_H_H

struct executor_mode_table{
    int mode;
    const char *name;
    bool (*exec)(uint8_t ch, int mode, void *args);
};

struct  executor_thread_wait{
    pthread_cond_t t_cond;
    pthread_mutex_t t_mutex;
    volatile bool is_wait;
};


struct executor_thread_m{
    pthread_t tid;
    int ch;
    char *name;
    void *args;
    struct executor_mode_table *mode;
    struct executor_thread_wait pwait;
    volatile bool reset;
};

struct executor_thread_ops {
    int (*close)(void *);
};

struct executor_context {
#define _EXEC_THREAD_NUM 4
    void *client;
    struct executor_thread_m thread[_EXEC_THREAD_NUM];
    struct executor_thread_ops *ops;
    void *args;
};

extern void executor_thread_init(void);
extern struct executor_context *get_executor_ctx(void);
extern int executor_fft_work_nofity(void *cl, int ch, int mode, bool enable);

#endif
