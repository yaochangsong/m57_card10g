#include "config.h"
#include "net_statistics.h"

struct net_statistics_info net_statistics;
#if 0
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

static bool _of_match_type_id(int hash_id, int type_id, int offset, int mask)
{
    if((((hash_id & mask) >> offset) & (type_id != 0)) || 
        (type_id == 0 && ((hash_id & mask) == 0)))
        return true;
    else
        return false;
}

uint64_t  get_send_bytes_by_type(int ch, int type, int id)
{
    struct spm_run_parm *arg = NULL;
     arg = channel_param[ch];
    int hash_index[MAX_XDMA_DISP_TYPE_NUM] = {0}, count = 0, _index = 0;
    uint64_t sum_bytes = 0;

    if(arg == NULL)
        return 0;
    
    for(int i = 0 ; i < MAX_XDMA_DISP_TYPE_NUM; i++){
        if(arg->xdma_disp.type[i]->statistics.send_bytes != 0)
            hash_index[count++] = i;
    }
    
    for(int i = 0; i < ARRAY_SIZE(_hash_type_table); i++){
        if(type == _hash_type_table[i].type){
            for(int j = 0; j < count; j++){
                _index = hash_index[j] ;
                if(_of_match_type_id(_index,  id, _hash_type_table[i].offset, _hash_type_table[i].mask))
                    sum_bytes += arg->xdma_disp.type[_index]->statistics.send_bytes;
            }
        }
    }
    return sum_bytes;
}
#endif

static int _statistics_client_send_ok(struct net_tcp_client *cl, void* bytes)
{
    uint64_t *_bytes = bytes;

    struct net_statistics_client *stat = cl->section.thread->thread.statistics;
    *_bytes += stat->send;
    
    return 0;
}

uint64_t statistics_get_all_client_send_ok(void)
{
    uint64_t bytes = 0;
    tcp_client_do_for_each(_statistics_client_send_ok, NULL, -1, &bytes);
    
    return bytes;
}

static int _statistics_client_send_err(struct net_tcp_client *cl, void* bytes)
{
    uint64_t *_bytes = bytes;

    struct net_statistics_client *stat = cl->section.thread->thread.statistics;
    *_bytes += stat->send_err;
    
    return 0;
}

uint64_t statistics_get_all_client_send_err(void)
{
    uint64_t bytes = 0;
    tcp_client_do_for_each(_statistics_client_send_err, NULL, -1, &bytes);
    
    return bytes;
}

struct net_statistics_client *net_statistics_client_create_context(int port)
{
    struct net_statistics_client *ctx = calloc(1, sizeof(*ctx));
    if (!ctx){
        printf_err("zalloc error!!\n");
        exit(1);
    }
    
    ctx->port = port;
    ctx->args = &net_statistics;
    return ctx;
}

void *device_status_check_thread(void *s)
{
    bool is_check = false;
    int try_count = 0;
    //device start check FPGA Card
    config_set_device_status(0, 0);
    sleep(2);
    while(1){
        is_check = io_get_xdma_fpga_status();
        printf_note("XDMA FPGA Status: %s!!\n", is_check == true ? "OK" : "False");
        if(is_check == false){
            if(try_count++ > 10){
                try_count = 0;
                //device Not Find FPGA Card
                config_set_device_status(-1, 0);
            }
        } else if((config_get_device_status(NULL) == 0) || 
                    (config_get_device_status(NULL) == 3) || 
                    (config_get_device_status(NULL) == -1))
            //if the device   is in checking  or load ok or not find card status, then set the device is ok status
            config_set_device_status(1, 0);
        sleep(3);
    }
}

int device_status_loop(void)
{
    pthread_t tid;
    int err;
    err = pthread_create (&tid , NULL , device_status_check_thread , NULL);
    if (err != 0){
        printf("can't create thread: %s\n", strerror(err));
        return -1;
    }
    pthread_detach(tid);
    return 0;
}

void net_statistics_init(void)
{
    memset(&net_statistics, 0, sizeof(net_statistics));
    device_status_loop();
}
