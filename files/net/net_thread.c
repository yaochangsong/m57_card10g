#include "config.h"
#include "net_thread.h"

struct net_thread_context *net_thread_ctx = NULL;

struct thread_con_wait {
    pthread_mutex_t count_lock;
    pthread_cond_t count_cond;
    unsigned int count;
}*con_wait;

static inline int _get_thread_index(void)
{
    static int index = 0;
    int r_index = 0;

    r_index = index;
    if(index ++ > 32)
        index = 0;
    
    return r_index;
}
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

static void _net_thread_con_over(struct thread_con_wait *wait)
{
    pthread_mutex_lock(&wait->count_lock);
    wait->count++;
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
    struct thread_con_wait *wait = calloc(1, sizeof(*wait));
    if (!wait){
        printf_err("zalloc error!!\n");
        exit(1);
    }
    pthread_mutex_init(&wait->count_lock, NULL);
    pthread_cond_init(&wait->count_cond, NULL);
    wait->count = 0;
    
    return wait;
}

static void _net_thread_con_wait(struct thread_con_wait *wait, int num)
{
    if(num <= 0 || wait == NULL)
        return;
    
    printf_note("Wait[%d] to finish consume %d\n", num, wait->count);
    pthread_mutex_lock(&wait->count_lock);
    while (wait->count < num)
    	pthread_cond_wait(&wait->count_cond, &wait->count_lock);
    pthread_mutex_unlock(&wait->count_lock);
    _net_thread_con_stopped(wait);
}


static int _net_thread_wait(void *args)
{
    struct net_thread_context *ptr = args;
    struct net_thread_m *ptd = &ptr->thread;
    printf_note("%s thread is %s\n",  ptd->name, ptd->pwait.is_wait == true ? "PAUSE" : "RUN");
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE , NULL);
    pthread_mutex_lock(&ptd->pwait.t_mutex);
    while(ptd->pwait.is_wait == true)
        pthread_cond_wait(&ptd->pwait.t_cond, &ptd->pwait.t_mutex);
    pthread_mutex_unlock(&ptd->pwait.t_mutex);
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE , NULL);
    return 0;
}

static int _net_thread_con_nofity(struct net_tcp_client *cl)
{
    struct net_thread_m *ptd = &cl->section.thread->thread;
    if(ptd == NULL)
        return -1;
    
    if(pthread_check_alive_by_tid(ptd->tid) == false){
        printf_note("[%s]pthread is not running\n",  ptd->name);
        return -1;
    }
    pthread_mutex_lock(&ptd->pwait.t_mutex);
    ptd->pwait.is_wait = false;
    pthread_cond_signal(&ptd->pwait.t_cond);
    pthread_mutex_unlock(&ptd->pwait.t_mutex);
    printf_note("[%s]notify thread %s\n", ptd->name, ptd->pwait.is_wait == true ? "PAUSE" : "RUN");

    return 0;
}

static inline int _get_hash_id(void *args)
{
    int id = 0;
    
    return id;
}

static int  data_dispatcher(void *args, int hid)
{
    struct net_thread_context *ctx = args;
    struct spm_context *spm_ctx = ctx->args;
    struct spm_run_parm *arg = spm_ctx->run_args[1];
    struct net_tcp_client *cl = ctx->thread.client;
    
    int index = hid;
    int vec_cnt = arg->xdma_disp.type[index]->vec_cnt;

    printf_note("vec_cnt=%d, hid=%d\n", vec_cnt, hid);
    if(arg->xdma_disp.type[index] == NULL || vec_cnt == 0)
        return -1;

    if(vec_cnt > 0){
       // printf_note("tcp send hid:%d\n", hid);
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
    if(first){
        con_wait = _net_thread_con_init();
        first = false;
    }
    #if 0
    /* thread 0 deal */
    struct net_thread_context *ctx = net_thread_ctx;
    struct net_tcp_client *cl = ctx->thread.client;
    net_hash_for_each(cl->section.hash, data_dispatcher, ctx);
    #endif
    int num = tcp_client_do_for_each(_net_thread_con_nofity);
    printf_note("broadcast clinet num: %d\n", num);
    _net_thread_con_wait(con_wait, num);
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
    _net_thread_con_over(con_wait);
    /* when deal over ,it's need  pause and wait cond_signal */
    pthread_mutex_lock(&ptd->pwait.t_mutex);
    ptd->pwait.is_wait = true;
    pthread_mutex_unlock(&ptd->pwait.t_mutex);
    return 0;
}

static int  _net_thread_exit(void *arg)
{
    struct net_thread_context *ctx = arg;
    ctx->thread_count --;

    printf_note("thread[%s] exit!\n", ctx->thread.name);
    return 0;
}


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

    printf_note("client: %s:%d create thread\n", client->get_peer_addr(client), client->get_peer_port(client));
    ctx->thread.name = strdup(_get_thread_name(client->get_peer_port(client)));
    ctx->thread.client = cl;
    ctx->args = get_spm_ctx();
    _net_thread_wait_init(ctx);
    //if(t_index == 0){
        /* thread 0 use main thread */
    //    return ctx;
    //}
    ctx->thread_count ++;
    ret =  pthread_create_detach (NULL,NULL, _net_thread_main_loop, _net_thread_exit,  ctx->thread.name , ctx, ctx, &tid);
    if(ret != 0){
        perror("pthread err");
        ctx->thread_count --;
        safe_free(ctx);
        return NULL;
    }
    ctx->thread.tid = tid;
    return ctx;

}

