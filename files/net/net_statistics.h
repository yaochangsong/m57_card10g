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
};

struct net_statistics_downlink_info{
    volatile uint64_t cmd_pkgs;
    volatile uint64_t data_pkgs;
    volatile uint64_t err_pkgs;
};

struct net_statistics_chip_status{
    volatile int link_status;
    volatile int load_status;
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


static inline  char *_get_device_status_info(int errCode)
{
    static struct _err_code {
        int code;
        char *message;
    } _msg[] ={
        {1,            "Ok"},
        {0,            "Starting"},
        {-1,           "Not Find FPGA Card"},
        {-2,           "Bit File Load Error"},
        {-3,           "Link Error"},
        {-255,         "Unkonw Error"},
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
1：成功,
0: 启动中,
-1：未检测到子卡,
-2：bit文件加载失败,
-3: link失败
-255： 未知错误

*/
static inline  int config_set_device_status(int status, int id)
{
    static char buffer[128] = {0};

    net_statistics.device_status.code = status;
    if(id >= 0)
        snprintf(buffer, sizeof(buffer) - 1, "[0x%x], %s", id, _get_device_status_info(status));
    printf_note("%s\n", buffer);
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


/* 上行读取字节数 */
static inline void ns_uplink_add_read_bytes(int ch, uint32_t bytes)
{
    net_statistics.uplink[ch].read_bytes += bytes;
}
static inline uint64_t ns_uplink_get_read_bytes(int ch)
{
    return net_statistics.uplink[ch].route_bytes;
}


/* 上行路由字节数 */
static inline void ns_uplink_add_route_bytes(int ch, uint32_t bytes)
{
    net_statistics.uplink[ch].route_bytes += bytes;
}
static inline uint64_t ns_uplink_get_route_bytes(int ch)
{
    return net_statistics.uplink[ch].route_bytes;
}


#if 0
/* 上行发送字节数 */
static inline void _ns_uplink_add_send_bytes(int ch,uint32_t bytes)
{
    net_statistics.uplink[ch].send_bytes += bytes;
}
static inline uint64_t _ns_uplink_get_send_bytes(int ch)
{
    return net_statistics.uplink[ch].send_bytes;
}

static inline void ns_uplink_add_send_ok_bytes(int ch,uint32_t bytes)
{
    net_statistics.uplink[ch].send_ok_bytes += bytes;
}
static inline uint64_t ns_uplink_get_send_ok_bytes(int ch)
{
    return net_statistics.uplink[ch].send_ok_bytes;
}

static inline void ns_uplink_add_send_bytes(int ch,uint32_t pre_bytes, uint32_t ok_bytes)
{
    net_statistics.uplink[ch].send_bytes    += pre_bytes;
    net_statistics.uplink[ch].send_ok_bytes    += ok_bytes;
}


/* 上行发送错误字节数 */
static inline uint64_t ns_uplink_get_send_err_bytes(int ch)
{
    if(net_statistics.uplink[ch].send_bytes - net_statistics.uplink[ch].send_ok_bytes < 0)
        return 0;
    return net_statistics.uplink[ch].send_bytes - net_statistics.uplink[ch].send_ok_bytes;
}
#endif


/* 上行路由转发报文 */
static inline void ns_uplink_add_forward_pkgs(int ch, uint32_t pkgs)
{
    net_statistics.uplink[ch].forward_pkgs += pkgs;
}
static inline uint64_t ns_uplink_get_forward_pkgs(int ch)
{
    return net_statistics.uplink[ch].forward_pkgs;
}


/* 上行路由错误报文 */
static inline void ns_uplink_add_route_err_pkgs(int ch,uint32_t pkgs)
{
    net_statistics.uplink[ch].route_err_pkgs += pkgs;
}
static inline uint64_t ns_downlink_get_route_err_pkgs(int ch)
{
    return net_statistics.uplink[ch].route_err_pkgs;
}


/* 下行命令报文 */
static inline void ns_downlink_add_cmd_pkgs(uint32_t pkgs)
{
    net_statistics.downlink.cmd_pkgs += pkgs;
}
static inline uint64_t ns_downlink_get_cmd_pkgs(void)
{
    return net_statistics.downlink.cmd_pkgs;
}


/* 下行数据报文 */
static inline void ns_downlink_add_data_pkgs(uint32_t pkgs)
{
    net_statistics.downlink.data_pkgs += pkgs;
}
static inline uint64_t ns_downlink_get_data_pkgs(void)
{
    return net_statistics.downlink.data_pkgs;
}


/* 下行错误报文 */
static inline void ns_downlink_add_err_pkgs(uint32_t pkgs)
{
    net_statistics.downlink.err_pkgs += pkgs;
}

static inline uint64_t ns_downlink_get_err_pkgs(void)
{
    return net_statistics.downlink.err_pkgs;
}

/* 获取芯片加载状态 */
static inline int ns_downlink_get_loadbit_result(int slotid)
{
    if(slotid > MAX_FPGA_CARD_SLOT_NUM)
        return -1;

    return net_statistics.chip[slotid].load_status;
}

/* 设置芯片加载状态 */
static inline void ns_downlink_set_loadbit_result(int slotid, int status)
{
    if(slotid > MAX_FPGA_CARD_SLOT_NUM)
        return;

    net_statistics.chip[slotid].load_status = status;
}

/* 获取芯片link状态, 0: 0k,  !=0 false */
static inline int ns_downlink_get_link_result(int slotid)
{
    if(slotid > MAX_FPGA_CARD_SLOT_NUM)
        return -1;
    
    return net_statistics.chip[slotid].link_status;
}

/* 设置芯片link状态 */
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
