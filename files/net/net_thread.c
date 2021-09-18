#include "config.h"
#include "net_thread.h"

struct net_thread_context *net_thread_ctx = NULL;

struct thread_con_wait {
    pthread_mutex_t count_lock;
    pthread_cond_t count_cond;
    volatile unsigned int count;
}*con_wait;

static char *_get_thread_name(int index)
{
    static char buffer[64];

    snprintf(buffer, sizeof(buffer) -1, "thread_port_%d", index);
    return buffer;
}

static void _net_thread_wait_init(struct net_thread_context *ctx)
{
    if(ctx){
        pthread_cond_init(&ctx->thread.pwait.t_cond, NULL);
        pthread_mutex_init(&ctx->thread.pwait.t_mutex, NULL);
        ctx->thread.pwait.is_wait = true;
    }
}

static void _net_thread_con_over(struct thread_con_wait *wait, struct net_thread_m *ptd)
{
    printf_note("thread consume over start\n");
    pthread_mutex_lock(&wait->count_lock);
    wait->count++;
    printf_note("thread[%s] consume over, %u\n", ptd->name, wait->count);
    pthread_cond_signal(&wait->count_cond);
    pthread_mutex_unlock(&wait->count_lock);
}

static void _net_thread_con_stopped(struct thread_con_wait *wait)
{
    pthread_mutex_lock(&wait->count_lock);
    wait->count = 0;
    pthread_mutex_unlock(&wait->count_lock);
}


static void *_net_thread_con_init(void)
{
    static int has_init = 0;
    static struct thread_con_wait *wait = NULL;
    
    if(has_init)
        return wait;
    
    wait = calloc(1, sizeof(*wait));
    if (!wait){
        printf_err("zalloc error!!\n");
        exit(1);
    }
    pthread_mutex_init(&wait->count_lock, NULL);
    pthread_cond_init(&wait->count_cond, NULL);
    wait->count = 0;
    has_init = 1;
    
    return wait;
}

static void _net_thread_con_wait_timeout(struct thread_con_wait *wait, int num, int timeout_ms)
{
    if(num <= 0 || wait == NULL)
        return;

    struct timeval now;
    struct timespec outtime;
    int ret = 0;
    uint64_t us;

    printf_info("Wait[%d] to finish consume %d\n", num, wait->count);
    pthread_mutex_lock(&wait->count_lock);
    while (wait->count < num){
        printf_note("Wait[%d]!= %d\n", num, wait->count);
        //pthread_cond_wait(&wait->count_cond, &wait->count_lock);
        #if 1
        gettimeofday(&now, NULL);
        outtime.tv_sec = now.tv_sec +  timeout_ms/1000;
        us = now.tv_usec + 1000 * (timeout_ms % 1000);
        us = us % 1000000;
        outtime.tv_nsec = us * 1000;
        ret = pthread_cond_timedwait(&wait->count_cond, &wait->count_lock, &outtime);
        if(ret != 0){
            printf_warn(">>>>>>wait thread timeout!!\n");
        }
        #endif
    }
    printf_info("Wait[%d] consume over>>> %d\n", num, wait->count);
    wait->count = 0;
    pthread_mutex_unlock(&wait->count_lock);
    //_net_thread_con_stopped(wait);
}


static int _net_thread_wait(void *args)
{
    struct net_thread_context *ptr = args;
    struct net_thread_m *ptd = &ptr->thread;
    printf_note(" thread is PAUSE\n");

    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE , NULL);
    pthread_mutex_lock(&ptd->pwait.t_mutex);
    while(ptd->pwait.is_wait == true){
        pthread_cond_wait(&ptd->pwait.t_cond, &ptd->pwait.t_mutex);
    }
    ptd->pwait.is_wait = true;
    pthread_mutex_unlock(&ptd->pwait.t_mutex);
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE , NULL);
    return 0;
}

static int _net_thread_con_nofity(struct net_tcp_client *cl)
{
    struct net_thread_m *ptd = &cl->section.thread->thread;
    if(ptd == NULL)
        return -1;
    
   // if(pthread_check_alive_by_tid(ptd->tid) == false){
   //     printf_note("[%s]pthread is not running\n",  ptd->name);
   //     return -1;
   // }
    pthread_mutex_lock(&ptd->pwait.t_mutex);
    ptd->pwait.is_wait = false;
    //printf_note("[%s]notify thread %s\n", ptd->name, ptd->pwait.is_wait == true ? "PAUSE" : "RUN");
    pthread_cond_signal(&ptd->pwait.t_cond);
    pthread_mutex_unlock(&ptd->pwait.t_mutex);
    printf_note("[%s]notify thread %s[%d]\n", ptd->name,  "RUN", ptd->pwait.is_wait);
    return 0;
}

static int  data_dispatcher(void *args, int hid)
{
    struct net_thread_context *ctx = args;
    struct spm_context *spm_ctx = ctx->args;
    struct spm_run_parm *arg = spm_ctx->run_args[1];
    struct net_tcp_client *cl = ctx->thread.client;
    
    int index = hid;
    if(index > MAX_XDMA_DISP_TYPE_NUM)
        return -1;
    
    int vec_cnt = arg->xdma_disp.type[index]->vec_cnt;

    //printf_note("vec_cnt=%d, hid=%d\n", vec_cnt, hid);
    if(arg->xdma_disp.type[index] == NULL || vec_cnt == 0)
        return -1;

    if(vec_cnt > 0){
        //printf_note("tcp send hid:%d, %s\n", hid, ctx->thread.name);
        send_vec_data_to_client(cl, arg->xdma_disp.type[index]->vec, vec_cnt);
    }

    return 0;
}

static inline void _net_thread_dispatcher_refresh(void *args)
{
    struct spm_run_parm *arg = args;
    
    arg->xdma_disp.type_num = 0;
    for(int i = 0; i < MAX_XDMA_DISP_TYPE_NUM; i++){
        arg->xdma_disp.type[i]->vec_cnt = 0;
    }
}


void  net_thread_con_broadcast(void *args)
{
    static bool first = true;
    static struct thread_con_wait *wait;
    struct net_tcp_client *cl0 = NULL;
    if(first){
        con_wait = _net_thread_con_init();
        first = false;
    }
    
    int notify_num = tcp_client_do_for_each(_net_thread_con_nofity, (void **)&cl0);
   //printf_note("dispatcher: %s:%d\n", cl0->get_peer_addr(cl0), cl0->get_peer_port(cl0));
    //if(cl0)
     //   net_hash_for_each(cl0->section.hash, data_dispatcher, cl0->section.thread);
    
    if(notify_num > 0){
        printf_note("broadcast clinet num: %d\n", notify_num);
        _net_thread_con_wait_timeout(con_wait, notify_num, 10);
    }
    _net_thread_dispatcher_refresh(args);
}



static int _net_thread_main_loop(void *arg)
{
    struct net_thread_context *ctx = arg;
    struct spm_context *spm_ctx = ctx->args;
    struct net_thread_m *ptd = &ctx->thread;
    struct net_tcp_client *cl = ctx->thread.client;

    /* thread wait until receive start data consume */
    _net_thread_wait(ctx);
    printf_note("thread[%s] receive start consume\n", ptd->name);
    net_hash_for_each(cl->section.hash, data_dispatcher, arg);
    _net_thread_con_over(con_wait, ptd);
    return 0;
}

static int  _net_thread_exit(void *arg)
{
    struct net_thread_context *ctx = arg;
    struct net_tcp_client *cl = ctx->thread.client;

    printf_note("thread[%s] exit!\n", ctx->thread.name);
    safe_free(ctx->thread.name);
    if(cl){
        safe_free(cl->section.thread);
        printf_note("thread free\n");
    }
    return 0;
}

static int _net_thread_close(void *client)
{
    struct net_tcp_client *cl = client;
    
    if(pthread_check_alive_by_tid(cl->section.thread->thread.tid) == true){
        pthread_cancel_by_tid(cl->section.thread->thread.tid);
        printf_note("thread exit!\n");
    }
    usleep(10);
    return 0;
}

static const struct net_thread_ops nt_ops = {
    .close = _net_thread_close,
};


struct net_thread_context * net_thread_create_context(void *cl)
{
    struct net_thread_context *ctx = calloc(1, sizeof(*ctx));
    if (!ctx){
        printf_err("zalloc error!!\n");
        exit(1);
    }
    int ret = -1;
    pthread_t tid = 0;
    struct net_tcp_client *client = cl;

    ctx->thread.name = strdup(_get_thread_name(client->get_peer_port(client)));
    ctx->thread.client = cl;
    ctx->args = get_spm_ctx();
    _net_thread_wait_init(ctx);
    printf_note("client: %s:%d create thread\n", client->get_peer_addr(client), client->get_peer_port(client));

    ret =  pthread_create_detach (NULL,NULL, _net_thread_main_loop, _net_thread_exit,  ctx->thread.name , ctx, ctx, &tid);
    if(ret != 0){
        perror("pthread err");
        safe_free(ctx);
        return NULL;
    }
    ctx->thread.tid = tid;
    ctx->ops = &nt_ops;
    return ctx;

}


