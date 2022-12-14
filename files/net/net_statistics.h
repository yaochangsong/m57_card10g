#ifndef _NET_STATISTICS_H_H
#define  _NET_STATISTICS_H_H
#include "net_tcp.h"
#include "net_thread.h"


struct net_statistics_uplink_info{
    volatile uint64_t read_bytes;
    volatile uint64_t route_bytes;
    volatile uint64_t send_bytes;
    volatile uint64_t send_ok_bytes;
    volatile uint64_t send_err_bytes;
    volatile uint64_t forward_pkgs;
    volatile uint64_t route_err_pkgs;
    volatile uint64_t over_run_count;
    volatile uint64_t read_speed_bps;
};

struct net_statistics_downlink_info{
    volatile uint64_t cmd_pkgs;
    volatile uint64_t data_pkgs;
    volatile uint64_t err_pkgs;
};

struct net_statistics_chip_status{
    volatile int link_status;
    volatile int load_status;
    volatile uint64_t load_count;
};

struct net_statistics_client{
    int port;
    volatile uint64_t send;
    volatile uint64_t send_err;
    void *args;
};

struct device_status_st{
    int code;
    char *info;
};

struct net_statistics_info{
    struct net_statistics_uplink_info uplink[MAX_XDMA_NUM];
    struct net_statistics_downlink_info downlink;
    struct net_statistics_chip_status chip[MAX_FPGA_CARD_SLOT_NUM];
    struct device_status_st device_status;
};
extern struct net_statistics_info net_statistics;

#define DEVICE_STATUS_WAIT_LINK     6
#define DEVICE_STATUS_LINK_OK       5
#define DEVICE_STATUS_WAIT_LOAD     4
#define DEVICE_STATUS_LOAD_OK       3
#define DEVICE_STATUS_LOADING       2
#define DEVICE_STATUS_OK            1
#define DEVICE_STATUS_CHECK_CARD    0
#define DEVICE_STATUS_NO_CARD      -1
#define DEVICE_STATUS_LOAD_ERR     -2
#define DEVICE_STATUS_LINK_ERR     -3
#define DEVICE_STATUS_UNKOWN_ERR   -255

static inline uint64_t ns_downlink_get_loadok_count(int slotid);
static inline void ns_downlink_set_loadok_count(int slotid, int count);

static inline  char *_get_device_status_info(int errCode)
{
    static struct _err_code {
        int code;
        char *message;
    } _msg[] ={
        {DEVICE_STATUS_WAIT_LINK,           "?????????"},
        {DEVICE_STATUS_LINK_OK,             "????????????"},
        {DEVICE_STATUS_WAIT_LOAD,           "?????????"},
        {DEVICE_STATUS_LOAD_OK,             "????????????"},
        {DEVICE_STATUS_LOADING,             "?????????"},
        {DEVICE_STATUS_OK,                  "Ok"},
        {DEVICE_STATUS_CHECK_CARD,          "??????????????????"},
        {DEVICE_STATUS_NO_CARD,             "??????????????????"},
        {DEVICE_STATUS_LOAD_ERR,            "????????????"},
        {DEVICE_STATUS_LINK_ERR,            "????????????"},
        {DEVICE_STATUS_UNKOWN_ERR,          "????????????"},
    };
    for (int i = 0; i < ARRAY_SIZE(_msg); i++){
        if(_msg[i].code == errCode)
            return _msg[i].message;
    }
    return "Unkonw Error";
}

static inline int config_get_device_status(char **info)
{
    if(info)
        *info = net_statistics.device_status.info;
    return net_statistics.device_status.code;
}

/*
3: ????????????
2: ?????????
1?????????,
0: ?????????,
-1?????????????????????,
-2???bit??????????????????,
-3: link??????
-255??? ????????????

*/
static inline  int config_set_device_status(int status, int id)
{
    static char buffer[128] = {0};

    net_statistics.device_status.code = status;
    if(id >= 0)
        snprintf(buffer, sizeof(buffer) - 1, "[0x%x], %s", id, _get_device_status_info(status));
    //printf_note("%s\n", buffer);
    net_statistics.device_status.info = buffer;
    return 0;
}


static inline void statistics_client_send_add(struct net_statistics_client *stat, uint32_t bytes)
{
    if(stat)
        stat->send += bytes;
}

static inline void statistics_client_send_err_add(struct net_statistics_client *stat, uint32_t bytes)
{
    if(stat)
        stat->send_err += bytes;
}

static inline uint64_t statistics_client_get_send_ok(struct net_tcp_client *cl)
{
    struct net_statistics_client *stat = (struct net_statistics_client *)cl->section.thread->thread.statistics;
    if(stat)
        return stat->send;
    return 0;
}

static inline uint64_t statistics_client_get_send_err(struct net_tcp_client *cl)
{
    struct net_statistics_client *stat = (struct net_statistics_client *)cl->section.thread->thread.statistics;
    if(stat)
        return stat->send_err;
    else
        return 0;
}

/* ???????????????????????? */
static inline void ns_uplink_set_speed(int ch, uint64_t speed)
{
    net_statistics.uplink[ch].read_speed_bps = speed;
}

static inline uint64_t ns_uplink_get_speed(int ch)
{
    return net_statistics.uplink[ch].read_speed_bps;
}


/* ????????????????????? */
static inline void ns_uplink_add_read_bytes(int ch, uint32_t bytes)
{
    net_statistics.uplink[ch].read_bytes += bytes;
}
static inline uint64_t ns_uplink_get_read_bytes(int ch)
{
    return net_statistics.uplink[ch].route_bytes;
}


/* ????????????????????? */
static inline void ns_uplink_add_route_bytes(int ch, uint32_t bytes)
{
    net_statistics.uplink[ch].route_bytes += bytes;
}
static inline uint64_t ns_uplink_get_route_bytes(int ch)
{
    return net_statistics.uplink[ch].route_bytes;
}


/* ???????????????????????? */
static inline void ns_uplink_add_forward_pkgs(int ch, uint32_t pkgs)
{
    net_statistics.uplink[ch].forward_pkgs += pkgs;
}
static inline uint64_t ns_uplink_get_forward_pkgs(int ch)
{
    return net_statistics.uplink[ch].forward_pkgs;
}


/* ???????????????????????? */
static inline void ns_uplink_add_route_err_pkgs(int ch,uint32_t pkgs)
{
    if(ch >= MAX_XDMA_NUM)
        return;
    net_statistics.uplink[ch].route_err_pkgs += pkgs;
}
static inline uint64_t ns_downlink_get_route_err_pkgs(int ch)
{
    if(ch >= MAX_XDMA_NUM)
        return 0;
    return net_statistics.uplink[ch].route_err_pkgs;
}

/* ???????????????????????? */
static inline void ns_uplink_add_over_run_cnt(int ch,uint32_t cnt)
{
    if(ch >= MAX_XDMA_NUM)
        return;
    net_statistics.uplink[ch].over_run_count += cnt;
}
static inline uint64_t ns_downlink_get_over_run_cnt(int ch)
{
    if(ch >= MAX_XDMA_NUM)
        return 0;
    return net_statistics.uplink[ch].over_run_count;
}


/* ?????????????????? */
static inline void ns_downlink_add_cmd_pkgs(uint32_t pkgs)
{
    net_statistics.downlink.cmd_pkgs += pkgs;
}
static inline uint64_t ns_downlink_get_cmd_pkgs(void)
{
    return net_statistics.downlink.cmd_pkgs;
}


/* ?????????????????? */
static inline void ns_downlink_add_data_pkgs(uint32_t pkgs)
{
    net_statistics.downlink.data_pkgs += pkgs;
}
static inline uint64_t ns_downlink_get_data_pkgs(void)
{
    return net_statistics.downlink.data_pkgs;
}


/* ?????????????????? */
static inline void ns_downlink_add_err_pkgs(uint32_t pkgs)
{
    net_statistics.downlink.err_pkgs += pkgs;
}

static inline uint64_t ns_downlink_get_err_pkgs(void)
{
    return net_statistics.downlink.err_pkgs;
}

/* ????????????????????????*/
static inline int ns_downlink_get_loadbit_result(int slotid)
{
    if(slotid > MAX_FPGA_CARD_SLOT_NUM)
        return -1;

    return net_statistics.chip[slotid].load_status;
}


/* ???????????????????????????
    OK(2): ??????(????????????)
    WAIT
 */
static inline char *ns_downlink_get_loadbit_str_result(int slotid)
{
    static char buffer[64];
    if(slotid > MAX_FPGA_CARD_SLOT_NUM)
        return "null";
    
    if((DEVICE_STATUS_LOAD_OK == net_statistics.chip[slotid].load_status) && (ns_downlink_get_loadok_count(slotid) > 0)){
        snprintf(buffer, sizeof(buffer)-1, "%s(%"PRIu64")",
                _get_device_status_info(net_statistics.chip[slotid].load_status), 
                ns_downlink_get_loadok_count(slotid));
        return buffer;
    }else{
        return _get_device_status_info(net_statistics.chip[slotid].load_status);
    }
}


/* ???????????????????????? */
static inline void ns_downlink_set_loadbit_result(int slotid, int status)
{
    if(slotid > MAX_FPGA_CARD_SLOT_NUM)
        return;

    if(status == DEVICE_STATUS_LOAD_OK){
        ns_downlink_set_loadok_count(slotid, 1);
        net_statistics.chip[slotid].load_status = status;
    }
    else  if(status == DEVICE_STATUS_WAIT_LOAD){
        ns_downlink_set_loadok_count(slotid, -1);
        if(ns_downlink_get_loadok_count(slotid) == 0)
            net_statistics.chip[slotid].load_status = status;
    } else{
        net_statistics.chip[slotid].load_status = status;
    }
}

/* ????????????link??????, 0: 0k,  !=0 false */
static inline int ns_downlink_get_link_result(int slotid)
{
    if(slotid > MAX_FPGA_CARD_SLOT_NUM)
        return -1;
    
    return net_statistics.chip[slotid].link_status;
}

/* ?????????????????????????????? */
static inline void ns_downlink_set_loadok_count(int slotid, int count)
{
    if(slotid > MAX_FPGA_CARD_SLOT_NUM)
        return;
    if(count < 0 && net_statistics.chip[slotid].load_count == 0)
        return;
        
    net_statistics.chip[slotid].load_count += count;
}
static inline uint64_t ns_downlink_get_loadok_count(int slotid)
{
    if(slotid > MAX_FPGA_CARD_SLOT_NUM)
        return 0;

    return net_statistics.chip[slotid].load_count;
}


static inline char *ns_downlink_get_link_str_result(int slotid)
{
    if(slotid > MAX_FPGA_CARD_SLOT_NUM)
        return "null";
    
    return _get_device_status_info(net_statistics.chip[slotid].link_status);
}


/* ????????????link?????? */
static inline void ns_downlink_set_link_result(int slotid, int status)
{
    if(slotid > MAX_FPGA_CARD_SLOT_NUM)
        return;

    net_statistics.chip[slotid].link_status = status;
}

extern void net_statistics_init(void);
extern struct net_statistics_client *net_statistics_client_create_context(int port);
extern uint64_t statistics_get_all_client_send_ok(void);
extern uint64_t statistics_get_all_client_send_err(void);

#endif
