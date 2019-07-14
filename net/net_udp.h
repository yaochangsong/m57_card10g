#ifndef _NET_UDP_H_H
#define _NET_UDP_H_H

struct net_udp_client {
    struct list_head list;
    struct net_udp_server *srv;

    struct sockaddr_in peer_addr;
    bool connection_close;
    int response_length;
    int tag;
    void (*free)(struct net_udp_client *cl);
    void (*send_error)(struct net_udp_client *cl, int code, const char *summary, const char *fmt, ...);
    void (*send)(struct net_udp_client *cl, const void *data, int len);
    const char *(*get_version)(struct net_udp_client *cl);
    const char *(*get_peer_addr)(struct net_udp_client *cl);
    int (*get_peer_port)(struct net_udp_client *cl);
};


struct net_udp_server {
    struct uloop_fd fd;
    struct list_head clients;
    int nclients;
    void (*free)(struct net_udp_server *srv);
    void (*on_accept)(struct net_udp_client *cl);
    int (*on_request)(struct net_udp_client *cl);
};

extern struct net_udp_server *udp_server_new(const char *host, int port);

#endif

