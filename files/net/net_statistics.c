#include "config.h"
#include "net_statistics.h"

struct net_statistics_info net_statistics;


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
    config_set_device_status(DEVICE_STATUS_CHECK_CARD, 0);
    sleep(2);
    while(1){
        is_check = io_get_xdma_fpga_status();
        printf_note("XDMA FPGA Status: %s!!\n", is_check == true ? "OK" : "False");
        if(is_check == false){
            if(try_count++ > 10){
                try_count = 0;
                //device Not Find FPGA Card
                config_set_device_status(DEVICE_STATUS_NO_CARD, 0);
            }
        } else if((config_get_device_status(NULL) == DEVICE_STATUS_CHECK_CARD) || 
                    (config_get_device_status(NULL) == DEVICE_STATUS_LOAD_OK) || 
                    (config_get_device_status(NULL) == DEVICE_STATUS_NO_CARD))
            //if the device   is in checking  or load ok or not find card status, then set the device is ok status
            config_set_device_status(DEVICE_STATUS_OK, 0);
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
    for(int i = 0; i < MAX_FPGA_CARD_SLOT_NUM; i++){
        net_statistics.chip[i].load_status = DEVICE_STATUS_WAIT_LOAD;
        net_statistics.chip[i].link_status = DEVICE_STATUS_WAIT_LINK;
    }
    device_status_loop();
}
