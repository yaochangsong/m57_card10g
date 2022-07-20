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
    int type;
    int index;
    struct executor_mode_table *mode;
    struct executor_thread_wait pwait;
    volatile bool reset;
};

struct executor_thread_ops {
    int (*close)(void *);
};

/* 通道数 */
#define EXEC_CHANNEL_NUM MAX_RADIO_CHANNEL_NUM

/* FFT线程数 */
#define EXEC_THREAD_FFT_NUM  EXEC_CHANNEL_NUM
/* 窄带IQ线程数 */
#define EXEC_THREAD_NIQ_NUM  1
/* 宽带IQ线程数 */
#define EXEC_THREAD_BIQ_NUM  2
/* FFT分发线程数 */
#define EXEC_THREAD_FFT_DISP_NUM  1
/* 总线程数 */
#define EXEC_THREAD_NUM (EXEC_THREAD_FFT_NUM + EXEC_THREAD_NIQ_NUM + EXEC_THREAD_BIQ_NUM + EXEC_THREAD_FFT_DISP_NUM)

/* 线程类型 */
enum {
    EXEC_THREAD_TYPE_FFT,
    EXEC_THREAD_TYPE_NIQ,
    EXEC_THREAD_TYPE_BIQ,
    EXEC_THREAD_TYPE_FFT_DIST,
    EXEC_THREAD_TYPE_MAX
};

struct executor_context {
    void *client;
    struct executor_thread_m thread[EXEC_THREAD_TYPE_MAX][EXEC_THREAD_NUM];
    struct executor_thread_ops *ops;
    void *args;
};


extern void executor_thread_init(void);
extern struct executor_context *get_executor_ctx(void);
extern int executor_fft_work_nofity(void *cl, int ch, int mode, bool enable);
extern int executor_niq_work_nofity(void *cl, int ch, int subch, bool enable);
#endif
