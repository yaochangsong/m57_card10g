#ifndef __NET_H_H
#define __NET_H_H

enum srv_net_index{
    SRV_1G_NET =0,
    SRV_10G_NET = 1,
    SRV_MAX_NET = 2,
};

struct net_ifname_t{
    enum srv_net_index index;
    char *ifname;
};

union _cmd_srv{
    struct net_tcp_server *tcpsvr;
    struct uh_server *uhsvr;
};

union _data_srv{
    struct net_udp_server *udpsrv;
    struct net_tcp_server *tcpsrv;
};

struct net_server{
    int number;
    union _cmd_srv cmd[SRV_MAX_NET];
    union _data_srv data[SRV_MAX_NET];
    struct uh_server *http_server;
};

extern int server_init(void);
extern int get_use_ifname_num(void);
extern void *get_cmd_srv(int index);
extern void *get_data_srv(int index);
extern void *get_http_srv(int index);

#endif

