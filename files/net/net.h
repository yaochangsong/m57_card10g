#ifndef __NET_H_H
#define __NET_H_H

enum net_listen_data_type {
    NET_LISTEN_TYPE_XDMA = 0,
    NET_LISTEN_TYPE_MAX,
};

enum net_data_type {
    NET_DATA_TYPE_XDMA= 0,
    NET_DATA_TYPE_FFT = 1,
    NET_DATA_TYPE_BIQ = 2,
    NET_DATA_TYPE_NIQ = 3,
    NET_DATA_TYPE_AUDIO = 4,
    NET_DATA_TYPE_MAX,
};

union _cmd_srv{
    struct net_tcp_server *tcpsvr;
    struct uh_server *uhsvr;
};

union _data_srv{
    struct net_udp_server *udpsrv[NET_DATA_TYPE_MAX];
    struct net_tcp_server *tcpsrv[NET_DATA_TYPE_MAX];
};

struct net_server{
    int number;
    struct uh_server *http_server;
    union _cmd_srv cmd[2];
    union _data_srv data[2];
};

extern int server_init(void);
extern int get_use_ifname_num(void);
extern void *get_cmd_server(int index);
extern void *get_http_server(int index);
extern void *get_data_server(int index, int type);

#endif

