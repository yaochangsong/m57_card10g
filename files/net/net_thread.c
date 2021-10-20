#include "config.h"
#include "net_thread.h"


static void *_net_thread_con_init(void);
struct net_thread_context *net_thread_ctx = NULL;

#define _NOTIFY_MAX_NUM 2
volatile void *channel_param[_NOTIFY_MAX_NUM] = {NULL};

struct thread_con_wait {
    pthread_mutex_t count_lock;
    pthread_cond_t count_cond;
    volatile unsigned int count;
    volatile unsigned int thread_count;
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

static void _net_thread_count_add(void)
{
    struct thread_con_wait *wait;
    if(con_wait == NULL)
       con_wait = _net_thread_con_init();
    wait = con_wait;

    pthread_mutex_lock(&wait->count_lock);
    wait->thread_count++;
    pthread_mutex_unlock(&wait->count_lock);
}

static void _net_thread_count_sub(void)
{
    struct thread_con_wait *wait;
    if(con_wait == NULL)
       con_wait = _net_thread_con_init();
    wait = con_wait;

    pthread_mutex_lock(&wait->count_lock);
    if(wait->thread_count > 0)
        wait->thread_count--;
    pthread_mutex_unlock(&wait->count_lock);
}

static uint32_t _net_thread_count_get(void)
{
    struct thread_con_wait *wait;
    uint32_t count = 0;
    if(con_wait == NULL)
       con_wait = _net_thread_con_init();
    wait = con_wait;

    pthread_mutex_lock(&wait->count_lock);
    count = wait->thread_count;
    pthread_mutex_unlock(&wait->count_lock);
    return count;
}



static void _net_thread_con_over(struct thread_con_wait *wait, struct net_thread_m *ptd)
{
    printf_info("thread consume over start\n");
    pthread_mutex_lock(&wait->count_lock);
    wait->count++;
    //printf_note("thread[%s] consume over, %u\n", ptd->name, wait->count);
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
        printf_info("Wait[%d]!= %d\n", num, wait->count);
        pthread_cond_wait(&wait->count_cond, &wait->count_lock);
        #if 0
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
    printf_info(" thread is PAUSE\n");

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
    printf_info("[%s]notify thread %s\n", ptd->name,  "RUN");
    return 0;
}

static void *_create_buffer(size_t len)
{
    void *buffer = NULL;
    int pagesize = 0;
    printf_note("Create buffer: %lu\n", len);
    pagesize=getpagesize();
    posix_memalign((void **)&buffer, pagesize /*alignment */ , len + pagesize);
    if (!buffer) {
        fprintf(stderr, "OOM %u.\n", pagesize);
        exit(1);
    }
    return buffer;
}

static inline int _get_channel_by_prio(int prio)
{
    int ch = 0;
#ifdef PRIO_CHANNEL_EN
    ch = (prio == 0 ? 0 : 1);
#endif
    return ch;
}

static inline int _get_prio_by_channel(int ch)
{
    int prio = 0;
#ifdef PRIO_CHANNEL_EN
    prio = (ch == 0 ? 0 : 1);
#endif
    return prio;
}

#ifdef CONFIG_NET_STATISTICS_ENABLE
static struct hash_type {
    int type;
    int mask;
    int num;
    int offset;
}_hash_type_table[] = {
    {HASHMAP_TYPE_SLOT, CARD_SLOT_MASK, GET_HASHMAP_SLOT_ADD_OFFSET},
    {HASHMAP_TYPE_CHIP, CARD_CHIP_MASK, GET_HASHMAP_CHIP_ADD_OFFSET},
    {HASHMAP_TYPE_FUNC, CARD_FUNC_MASK, GET_HASHMAP_FUNC_ADD_OFFSET},
    {HASHMAP_TYPE_PRIO, CARD_PRIO_MASK, GET_HASHMAP_PROI_ADD_OFFSET},
};

int get_valid_hashid(int ch, int type, int id, int (*f)(int))
{
    struct spm_run_parm *arg = NULL;
     arg = channel_param[ch];
    int hash_table[MAX_XDMA_DISP_TYPE_NUM] = {0}, count = 0;
    for(int i = 0 ; i < MAX_XDMA_DISP_TYPE_NUM; i++){
        if(arg->xdma_disp.type[i]->statistics.bytes != 0)
            hash_table[count++] = i;
    }
    for(int i = 0; i < ARRAY_SIZE(_hash_type_table); i++){
        if(type == _hash_type_table[i].type){
            for(int j = 0; j < count; j++){
                //if(hash_table[j] == 0)
                if((_hash_type_table[i].mask & hash_table[j]) != 0){
                    f(hash_table[j]);
                }
            }
        }
    }
    return 0;
}
#endif


uint64_t get_in_statistics_byte(int ch)
{
    struct spm_run_parm *arg = NULL;
    arg = channel_param[ch];
    if(arg == NULL)
        return 0;
    
    return arg->xdma_disp.inout.in_bytes;
}

uint64_t get_out_statistics_byte(int ch)
{
    struct spm_run_parm *arg = NULL;
    arg = channel_param[ch];
    if(arg == NULL)
        return 0;
    
    return arg->xdma_disp.inout.out_bytes;
}

uint64_t get_ok_out_statistics_byte(int ch)
{
    struct spm_run_parm *arg = NULL;
    arg = channel_param[ch];
    if(arg == NULL)
        return 0;
    
    return arg->xdma_disp.inout.out_seccess_bytes;
}





uint64_t get_read_statistics_byte(int ch)
{
    struct spm_run_parm *arg = NULL;
    arg = channel_param[ch];
    if(arg == NULL)
        return 0;

    return arg->xdma_disp.inout.read_bytes;
}


static size_t _data_send(struct net_tcp_client *cl, void *data, size_t len)
{
    #define THREAD_SEND_MAX_BYTE 8196 //8388608//262144//131072
    int index = 0, r = 0;
    size_t sbyte = 0, reminder = 0;
    char *pdata = NULL;
    size_t send_len = 0;   
    
    index = len / THREAD_SEND_MAX_BYTE;
    sbyte = index * THREAD_SEND_MAX_BYTE;
    reminder = len - sbyte;
    pdata = (char *)data;

    for(int i = 0; i<index; i++){
        r = tcp_send_data_to_client(cl->sfd.fd.fd,   pdata,  THREAD_SEND_MAX_BYTE);
        pdata += THREAD_SEND_MAX_BYTE;
        if(r > 0)
            send_len += r;
    }
    if(reminder > 0){
        r = tcp_send_data_to_client(cl->sfd.fd.fd,   pdata,  reminder);
        if(r > 0)
            send_len += r;
    }

    return send_len;
}

static void print_array(uint8_t *ptr, ssize_t len)
{
    if(ptr == NULL || len <= 0)
        return;
    
    for(int i = 0; i< len; i++){
        printf("%02x ", *ptr++);
    }
    printf("\n");
}


static int  data_dispatcher(void *args, int hid, int prio)
{
    struct net_thread_context *ctx = args;
    struct spm_context *spm_ctx = ctx->args;
    struct spm_run_parm *arg = NULL;//spm_ctx->run_args[0];
    struct net_tcp_client *cl = ctx->thread.client;
    int ch = 0, r = 0, r0 = 0;
    ch = _get_channel_by_prio(prio);
    arg = channel_param[ch];
    int index = hid;
    if(index > MAX_XDMA_DISP_TYPE_NUM || arg == NULL){
        printf_note("error!\n");
        return -1;
    }
        
    
    int vec_cnt = arg->xdma_disp.type[index]->vec_cnt;

    if(vec_cnt > 0)
        printf_note("send index=%d, vec_cnt=%d, %d, hid=%d, ch=%d, _prio=%d, [%ld, %ld]\n", index, vec_cnt,arg->xdma_disp.type[index]->vec_cnt,  hid, ch, prio, 
                arg->xdma_disp.type[index]->vec[0].iov_len, arg->xdma_disp.type[index]->vec[0].iov_len);
    if(arg->xdma_disp.type[index] == NULL || vec_cnt == 0){
        printf_debug("hash id %d is null,vec_cnt=%d\n", index, vec_cnt);
        return -1;
    }
        
#if 1
    if(vec_cnt > 0){
       
        //r = send_vec_data_to_client(cl, arg->xdma_disp.type[index]->vec, vec_cnt);
        //if(r > 0)  
        //     arg->xdma_disp.inout.out_seccess_bytes += r; 
#if   CONFIG_NET_STATISTICS_ENABLE
        for(int i = 0; i < vec_cnt; i++){
            arg->xdma_disp.type[index]->statistics.bytes += arg->xdma_disp.type[index]->vec[i].iov_len;
            arg->xdma_disp.inout.out_bytes += arg->xdma_disp.type[index]->vec[i].iov_len;
            printf_info("len:%lu\n", arg->xdma_disp.type[index]->vec[i].iov_len);
            //r = tcp_send_data_to_client(cl->sfd.fd.fd,  
            //                            arg->xdma_disp.type[index]->vec[i].iov_base,  
            //                            arg->xdma_disp.type[index]->vec[i].iov_len);
            //if(index == 525)
            //    print_array(arg->xdma_disp.type[index]->vec[i].iov_base, 32);
            r = _data_send(cl, arg->xdma_disp.type[index]->vec[i].iov_base, arg->xdma_disp.type[index]->vec[i].iov_len);
            if(r != arg->xdma_disp.type[index]->vec[i].iov_len)
                printf_warn("overun: send len:%d/%lu\n", r, arg->xdma_disp.type[index]->vec[i].iov_len);
            if(r > 0)  
                arg->xdma_disp.inout.out_seccess_bytes += r; 
            r0 += r;
        }
         printf_note("[%d]send hash id: %d, vec_cnt=%d, bytes:%d\n", cl->get_peer_port(cl), index, vec_cnt, r0);
        
#endif
    }
#else
    char *buffer[16] = {NULL};
    static struct iovec vec[16];
    static int _first = 0;
    if(_first == 0){
        for(int i = 0; i < 16; i++){
            buffer[i] = _create_buffer(0x400000);
            vec[i].iov_base = buffer[i];
            vec[i].iov_len = 0x400000;
        }
        _first = 1;
    }
    send_vec_data_to_client(cl, vec, 2);
   //for(int i = 0; i < vec_cnt; i++)
   //     tcp_send_data_to_client(cl->sfd.fd.fd,  vec[i].iov_base,  vec[i].iov_len);
#endif
    return 0;
}

static inline void _net_thread_dispatcher_refresh(void *args)
{
    struct spm_run_parm *arg = args;
    
    arg->xdma_disp.type_num = 0;
    for(int i = 0; i < MAX_XDMA_DISP_TYPE_NUM; i++){
        //if(i == 41 || i == 45 || i == 681)
        //    printf_note("%d, vec_cnt=%lu\n", i, arg->xdma_disp.type[i]->vec_cnt);
        arg->xdma_disp.type[i]->vec_cnt = 0;
       // arg->xdma_disp.type[i]->vec_cnt = 0;
    }
}

void  net_thread_con_broadcast(int ch, void *args)
{
    static bool first = true;
    static struct thread_con_wait *wait;
    struct net_tcp_client *cl0 = NULL;
    int prio = 0;
    if(first){
        con_wait = _net_thread_con_init();
        first = false;
    }
    prio = _get_prio_by_channel(ch);
    int notify_num = tcp_client_do_for_each(_net_thread_con_nofity, (void **)&cl0, prio);
    notify_num = min(_net_thread_count_get(), notify_num);
   //printf_note("dispatcher: %s:%d\n", cl0->get_peer_addr(cl0), cl0->get_peer_port(cl0));
    //if(cl0)
     //   net_hash_for_each(cl0->section.hash, data_dispatcher, cl0->section.thread);
    if(ch >= _NOTIFY_MAX_NUM)
        ch = _NOTIFY_MAX_NUM-1;
    channel_param[ch] = args;
   
    if(notify_num > 0){
        printf_debug("broadcast clinet num: %d\n", notify_num);
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
    printf_debug("thread[%s] receive start consume\n", ptd->name);
    net_hash_for_each(cl->section.hash, data_dispatcher, arg);
    _net_thread_con_over(con_wait, ptd);
    return 0;
}

static int  _net_thread_exit(void *arg)
{
    struct net_thread_context *ctx = arg;
    struct net_tcp_client *cl = ctx->thread.client;

    printf_note("thread[%s] exit!\n", ctx->thread.name);
    _net_thread_count_sub();
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
        printf_note("thread close!\n");
    }
    usleep(10);
    return 0;
}

static const struct net_thread_ops nt_ops = {
    .close = _net_thread_close,
};

static int _thread_init(void *args)
{
    static int cpu_index = 0;
    long cpunum = sysconf(_SC_NPROCESSORS_CONF);
    printf_note("bind cpu: %d\n", cpu_index);
    //thread_bind_cpu(cpu_index);
    cpu_index++;
    if(cpu_index >= cpunum)
        cpu_index = 0;
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

    ctx->thread.name = strdup(_get_thread_name(client->get_peer_port(client)));
    ctx->thread.client = cl;
    ctx->args = get_spm_ctx();
    _net_thread_wait_init(ctx);
    printf_note("client: %s:%d create thread\n", client->get_peer_addr(client), client->get_peer_port(client));
    _net_thread_count_add();
    ret =  pthread_create_detach (NULL,_thread_init, _net_thread_main_loop, _net_thread_exit,  ctx->thread.name , ctx, ctx, &tid);
    if(ret != 0){
        perror("pthread err");
        _net_thread_count_sub();
        safe_free(ctx);
        return NULL;
    }
    ctx->thread.tid = tid;
    ctx->ops = &nt_ops;
    return ctx;

}
