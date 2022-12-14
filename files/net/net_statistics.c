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
    for(int i = 0; i < MAX_XDMA_NUM; i++)
        tcp_client_do_for_each(_statistics_client_send_ok, NULL, i, &bytes);
    
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
    for(int i = 0; i < MAX_XDMA_NUM; i++)
        tcp_client_do_for_each(_statistics_client_send_err, NULL, i, &bytes);
    
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

static void _gettime(struct timeval *tv)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    tv->tv_sec = ts.tv_sec;
    tv->tv_usec = ts.tv_nsec / 1000;
}

static int _tv_diff(struct timeval *t1, struct timeval *t2)
{
    return
        (t1->tv_sec - t2->tv_sec) * 1000 +
        (t1->tv_usec - t2->tv_usec) / 1000;
}

static uint64_t _get_uplink_speed(int ch, int time_sec)
{
    if(ch >= MAX_XDMA_NUM || time_sec <= 0)
        return 0;
    
    uint64_t newdata = ns_uplink_get_read_bytes(ch);
    static uint64_t olddata[MAX_XDMA_NUM] = {[0 ... MAX_XDMA_NUM-1] = 0};
    uint64_t speed = 0;

    if(newdata > olddata[ch]){
        speed = (newdata - olddata[ch]) / time_sec;
    }
    olddata[ch] = newdata;
    
    return speed;
}

static bool _check_fpga_status(int timeout_ms)
{
    bool is_check = false;
    int try_count = 0;
    struct timeval start, now;

    _gettime(&start);
    while(1){
        is_check = io_get_xdma_fpga_status();
        if(is_check == false)
            printf_warn("XDMA FPGA Status: %s!!\r", is_check == true ? "OK" : "False");
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

        if(timeout_ms > 0){
            if(is_check == true){
                return true;
            }else{
                _gettime(&now);
                if(_tv_diff(&now, &start) > timeout_ms){
                    return is_check;
                }
                usleep(1000);
            }
        }else{
            sleep(3);
            for(int i = 0; i < MAX_XDMA_NUM; i++)
                ns_uplink_set_speed(i, _get_uplink_speed(i, 3));
        }
    }

}

void *device_status_check_thread(void *s)
{
    //loop forever
    printf_note("Device FPGA status check LOOP Thread start:\n");
    _check_fpga_status(-1);
    return NULL;

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
    printf_note("Device FPGA status check start:\n");
    config_set_device_status(DEVICE_STATUS_CHECK_CARD, 0);
    if(_check_fpga_status(5000) == true){
        config_set_device_status(DEVICE_STATUS_OK, 0);
        printf_note("Device FPGA status check OK:\n");
    }
    else{
        config_set_device_status(DEVICE_STATUS_NO_CARD, 0);
        printf_note("Device FPGA status check Failed:\n");
    }
    printf_note("Device FPGA status check end:\n");
    device_status_loop();
}
