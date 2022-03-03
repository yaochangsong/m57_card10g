#include "config.h"
#include "net_thread.h"


struct net_thread_context *net_thread_ctx = NULL;

#define _NOTIFY_MAX_NUM MAX_PRIO_LEVEL
volatile void *channel_param[_NOTIFY_MAX_NUM] = {NULL};

struct thread_con_wait {
    pthread_mutex_t count_lock;
    pthread_cond_t count_cond;
    volatile unsigned int count;
    volatile unsigned int thread_count;
    int prio;   /* 0: low, 1: high */
}*con_wait[MAX_PRIO_LEVEL];

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

static void _net_thread_count_add(int level)
{
    if(unlikely(level >= MAX_PRIO_LEVEL) || con_wait[level] == NULL)
        return;
    
    struct thread_con_wait *wait = con_wait[level];

    pthread_mutex_lock(&wait->count_lock);
    wait->thread_count++;
    pthread_mutex_unlock(&wait->count_lock);
}

static void _net_thread_count_sub(int level)
{
    if(unlikely(level >= MAX_PRIO_LEVEL) || con_wait[level] == NULL)
        return;
    
    struct thread_con_wait *wait = con_wait[level];

    pthread_mutex_lock(&wait->count_lock);
    if(wait->thread_count > 0)
        wait->thread_count--;
    pthread_mutex_unlock(&wait->count_lock);
}

static uint32_t _net_thread_count_get(int level)
{
    uint32_t count = 0;
    if(unlikely(level >= MAX_PRIO_LEVEL) || con_wait[level] == NULL)
        return -1;
    
    struct thread_con_wait *wait = con_wait[level];

    pthread_mutex_lock(&wait->count_lock);
    count = wait->thread_count;
    pthread_mutex_unlock(&wait->count_lock);
    return count;
}



static void _net_thread_con_over(struct thread_con_wait *wait, struct net_thread_m *ptd)
{
    printf_debug("thread consume over start\n");
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


static void *_net_thread_con_init(int level)
{
    struct thread_con_wait *wait = NULL;

    wait = calloc(1, sizeof(*wait));
    if (!wait){
        printf_err("zalloc error!!\n");
        exit(1);
    }
    pthread_mutex_init(&wait->count_lock, NULL);
    pthread_cond_init(&wait->count_cond, NULL);
    wait->count = 0;
    wait->prio = level;
    
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

    printf_debug("Wait[%d] to finish consume %d\n", num, wait->count);
    pthread_mutex_lock(&wait->count_lock);
    while (wait->count < num){
        printf_debug("Wait[%d]!= %d\n", num, wait->count);
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
            break;
        }
        #endif
    }
    printf_debug("Wait[%d] consume over>>> %d\n", num, wait->count);
    wait->count = 0;
    pthread_mutex_unlock(&wait->count_lock);
    //_net_thread_con_stopped(wait);
}


static int _net_thread_wait(void *args)
{
    struct net_thread_context *ptr = args;
    struct net_thread_m *ptd = &ptr->thread;
    printf_debug(" thread is PAUSE\n");

    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE , NULL);
    pthread_mutex_lock(&ptd->pwait.t_mutex);
    while(ptd->pwait.is_wait == true){
        pthread_cond_wait(&ptd->pwait.t_cond, &ptd->pwait.t_mutex);
    }
    ptd->pwait.is_wait = true;
    pthread_mutex_unlock(&ptd->pwait.t_mutex);
    //pthread_setcancelstate (PTHREAD_CANCEL_ENABLE , NULL);
    pthread_setcancelstate (PTHREAD_CANCEL_DISABLE , NULL);
    return 0;
}

static int _net_thread_con_nofity(struct net_tcp_client *cl, void *args)
{
    if(unlikely(cl->section.thread == NULL))
        return -1;

    struct net_thread_m *ptd = &cl->section.thread->thread;
    if(unlikely(ptd == NULL))
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
    printf_debug("[%s]notify thread %s\n", ptd->name,  "RUN");
    return 0;
}

static void *_create_buffer(size_t len)
{
    void *buffer = NULL;
    int pagesize = 0;
    printf_debug("Create buffer: %lu\n", len);
    pagesize=getpagesize();
    posix_memalign((void **)&buffer, pagesize /*alignment */ , len + pagesize);
    if (!buffer) {
        fprintf(stderr, "OOM %u.\n", pagesize);
        exit(1);
    }
    return buffer;
}

static struct hash_type {
    int type;
    int mask;
    int offset;
}_hash_type_table[] = {
    {HASHMAP_TYPE_SLOT, CARD_SLOT_MASK, GET_HASHMAP_SLOT_ADD_OFFSET},
    {HASHMAP_TYPE_CHIP, CARD_CHIP_MASK, GET_HASHMAP_CHIP_ADD_OFFSET},
    {HASHMAP_TYPE_FUNC, CARD_FUNC_MASK, GET_HASHMAP_FUNC_ADD_OFFSET},
    {HASHMAP_TYPE_PRIO, CARD_PRIO_MASK, GET_HASHMAP_PROI_ADD_OFFSET},
};


/* 判断HASHID值（hash_id）中和对应位（type_id）是否为1（匹配） */
static bool _of_match_type_id(int hash_id, int type_id, int offset, int mask)
{
    if(((type_id != 0) && (((hash_id & mask) >> offset) & type_id) ) || 
        (type_id == 0 && ((hash_id & mask) == 0)))
        return true;
    else
        return false;
}

uint64_t  get_send_bytes_by_type(int ch, int type, int id)
{
    struct spm_run_parm *arg = NULL;
    uint64_t sum_bytes = 0;
    struct hash_entry *entry;
    struct hash_entry *tmp;
    struct cache_hash *hash = NULL;
    struct ikey_record *r;
    int key, rv;

    arg = channel_param[ch];
    if(arg == NULL)
        return 0;

    hash = arg->hash;
    rv = pthread_rwlock_rdlock(&(hash->cache_lock));
    if (rv)
        return 0;

    HASH_ITER(hh, hash->entries, entry, tmp) {
        if(type == HASHMAP_TYPE_SLOT && id == GET_SLOTID_BY_HASHID(entry->key)){
            r = entry->data;
            sum_bytes += spm_hash_get_sendbytes((void *)r);
        }
    }
    pthread_rwlock_unlock(&(hash->cache_lock));
    
    return sum_bytes;
}

uint64_t  get_send_bytes_by_type_ex(void *args,  int type, int id)
{
    struct spm_run_parm *arg = NULL;
     
    uint64_t sum_bytes = 0;
    struct hash_entry *entry;
    struct hash_entry *tmp;
    struct cache_hash *hash = args;
    struct ikey_record *r;
    int key, rv;
    rv = pthread_rwlock_rdlock(&(hash->cache_lock));
    if (rv)
        return 0;

    HASH_ITER(hh, hash->entries, entry, tmp) {
        for(int i = 0; i < ARRAY_SIZE(_hash_type_table); i++){
            if(type != _hash_type_table[i].type)
                continue;
            if(_of_match_type_id(entry->key, id, _hash_type_table[i].offset, _hash_type_table[i].mask)){
                r = entry->data;
                sum_bytes += spm_hash_get_sendbytes((void *)r);
            }
        }
    }
    pthread_rwlock_unlock(&(hash->cache_lock));

    return sum_bytes;
}



static ssize_t _data_send(struct net_tcp_client *cl, void *data, size_t len)
{
    #define THREAD_SEND_MAX_BYTE  131072 //8388608//262144//131072
    int index = 0, r = 0;
    size_t sbyte = 0, reminder = 0;
    char *pdata = NULL;
    size_t send_len = 0;   
    
    index = len / THREAD_SEND_MAX_BYTE;
    //sbyte = index * THREAD_SEND_MAX_BYTE;
    //reminder = len - sbyte;
    reminder = len % THREAD_SEND_MAX_BYTE;
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

static ssize_t _send_vec_entry(void *args, void *_vec, void *data)
{
    struct net_thread_context *ctx = args;
    struct net_tcp_client *cl = ctx->thread.client;
    struct iovec *vec = (struct iovec *)_vec;
    ssize_t r = 0;

    //printf_note("vec->iov_base=%p, vec->iov_len=%zu\n", vec->iov_base, vec->iov_len);
    if(vec == NULL || vec->iov_len == 0 || ctx == NULL || cl == NULL){
        printf_warn("invalid data!\n");
        return -1;
    }

    //统计每个hash key发送字节
    spm_hash_set_sendbytes(data, vec->iov_len);
    r = _data_send((struct net_tcp_client *)cl, vec->iov_base, vec->iov_len);
    //统计每个客户端线程发送成功字节
    if(r > 0)
        statistics_client_send_add(ctx->thread.statistics, r);
    if(r != vec->iov_len){
        printf_warn("overrun: send len:%ld/%zu\n", r, vec->iov_len);
    }
    /* 统计发送失败字节 */
    if((ssize_t)vec->iov_len - r > 0 && r >= 0){
        statistics_client_send_err_add(ctx->thread.statistics, vec->iov_len - r);
    }

    return r;
}


static int  _data_dispatcher(int key, void *data, void *vec, void *_ctx)
{
    struct net_thread_context *ctx = _ctx;
    struct net_tcp_client *cl = ctx->thread.client;
    int rv = 0;
    //printf_note("key:%d\n", key);
    if(unlikely(cl == NULL)){
        printf_note("client is null!\n");
        return -1;
    }

    rv = client_hash_do_for_each_key(cl, key, data, vec, _send_vec_entry, _ctx);

    return rv;
}


void  net_thread_con_broadcast(int ch, void *args)
{
    int prio = 0;
    channel_param[ch] = args;

    prio = _get_prio_by_channel(ch);
    int notify_num = tcp_client_do_for_each(_net_thread_con_nofity, NULL, prio, NULL);
    //printf_debug("ch:%d, notify_num=%d, prio=%d, %d\n", ch, notify_num, prio, _net_thread_count_get(prio));
    notify_num = min(_net_thread_count_get(prio), notify_num);
    if(ch >= _NOTIFY_MAX_NUM)
        ch = _NOTIFY_MAX_NUM-1;
    if(notify_num > 0){
        printf_debug("broadcast client num: %d, prio:%d\n", notify_num, prio);
        _net_thread_con_wait_timeout(con_wait[prio], notify_num, 2000);
    }
}


static int _net_thread_main_loop(void *arg)
{
    struct net_thread_context *ctx = arg;
    struct spm_context *spm_ctx = ctx->args;
    struct net_thread_m *ptd = &ctx->thread;
    struct net_tcp_client *cl = ctx->thread.client;
    int ch;

    /* thread wait until receive start data consume */
    _net_thread_wait(ctx);
    printf_debug("thread[%s] receive start consume, prio=%d\n", ptd->name, ctx->thread.prio);
    //TIME_ELAPSED(
    ch = _get_channel_by_prio(cl->section.thread->thread.prio);
    if(spm_ctx && spm_ctx->run_args[ch]){
        spm_hash_do_for_each(spm_ctx->run_args[ch]->hash, _data_dispatcher, arg);
    }
    //);
    _net_thread_con_over(con_wait[ctx->thread.prio], ptd);
    return 0;
}

void _net_thread_unlock(void *arg)
{
    struct net_thread_context *ptr = arg;
    struct net_thread_m *ptd = &ptr->thread;
    pthread_mutex_trylock(&(ptd->pwait.t_mutex));
    pthread_mutex_unlock(&ptd->pwait.t_mutex);
}


static int  _net_thread_exit(void *arg)
{
    struct net_thread_context *ctx = arg;
    struct net_tcp_client *cl = ctx->thread.client;

    printf_note("thread[%s] exit!\n", ctx->thread.name);
    _net_thread_count_sub(ctx->thread.prio);
    safe_free(ctx->thread.name);
    safe_free(ctx->thread.statistics);
    client_hash_unlock(cl);
    _net_thread_unlock(ctx);
    if(cl){
        safe_free(cl->section.thread);
        printf_debug("thread free\n");
    }
    return 0;
}

static int _net_thread_close(void *client)
{
    struct net_tcp_client *cl = client;
    m57_stop_load((void *)cl);
    if(pthread_check_alive_by_tid(cl->section.thread->thread.tid) == true){
        pthread_cancel_by_tid(cl->section.thread->thread.tid);
        printf_debug("thread close!\n");
    }
    usleep(10);
    return 0;
}

static int _net_thread_set_prio(struct net_tcp_client *client, int prio)
{
    if(!client || !client->section.thread)
        return -1;
    if(likely(client->section.prio < MAX_PRIO_LEVEL))
        client->section.thread->thread.prio = prio;
    else
        client->section.thread->thread.prio = 0;
    printf_note("set prio: %s\n", client->section.thread->thread.prio == 1 ? "Urgent" : "Normal");
    _net_thread_count_add(client->section.thread->thread.prio);
    return 0;
}

static const struct net_thread_ops nt_ops = {
    .close = _net_thread_close,
     .set_prio = _net_thread_set_prio,
};

static int _thread_init(void *args)
{
    struct net_thread_context *ctx = args;
    struct net_tcp_client *client = ctx->thread.client;
    static int cpu_index = 0, have_inited = 0;
    long cpunum = sysconf(_SC_NPROCESSORS_CONF);
    printf_debug("bind cpu: %d\n", cpu_index);
    //thread_bind_cpu(cpu_index);
    cpu_index++;
    if(cpu_index >= cpunum)
        cpu_index = 0;
    if(have_inited == 0){
        for(int i = 0; i < MAX_PRIO_LEVEL; i++){
            con_wait[i] = _net_thread_con_init(i);
        }
        have_inited = 1;
    }
    ctx->thread.statistics = net_statistics_client_create_context(client->get_peer_port(client));
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
    if(likely(client->section.prio < MAX_PRIO_LEVEL))
        ctx->thread.prio = client->section.prio;
    else
        ctx->thread.prio = 0;
    
    ctx->args = get_spm_ctx();
    _net_thread_wait_init(ctx);
    printf_note("client: %s:%d create thread\n", client->get_peer_addr(client), client->get_peer_port(client));
    ret =  pthread_create_detach (NULL,_thread_init, _net_thread_main_loop, _net_thread_exit,  ctx->thread.name , ctx, ctx, &tid);
    if(ret != 0){
        perror("pthread err");
        safe_free(ctx);
        return NULL;
    }
    //_net_thread_count_add(ctx->thread.prio);
    ctx->thread.tid = tid;
    ctx->ops = &nt_ops;
    return ctx;

}
