#ifndef NET_SOCKET_H
#define NET_SOCKET_H
#include "config.h"


#define MAX_RECEIVE_DATA_LEN  1024
#define MAX_SEND_DATA_LEN  1024
#define MAX_CLINET_NUM  1000



struct net_tcp_client {
    struct list_head list;
    struct net_tcp_server *srv;
    struct ustream *us;
    struct ustream_fd sfd;
    struct uloop_timeout timeout;
    struct sockaddr_in peer_addr;
    struct dispatch dispatch;
    bool connection_close;
    int response_length;
    void (*free)(struct net_tcp_client *cl);
    void (*send_error)(struct net_tcp_client *cl, int code, const char *summary, const char *fmt, ...);
    void (*send_header)(struct net_tcp_client *cl, int code, const char *summary, int length);
    void (*append_header)(struct net_tcp_client *cl, const char *name, const char *value);
    void (*header_end)(struct net_tcp_client *cl);
    void (*redirect)(struct net_tcp_client *cl, int code, const char *fmt, ...);
    void (*request_done)(struct net_tcp_client *cl);
    
    void (*send)(struct net_tcp_client *cl, const void *data, int len);
    void (*printf)(struct net_tcp_client *cl, const char *format, ...);
    void (*vprintf)(struct net_tcp_client *cl, const char *format, va_list arg);

    const char *(*get_version)(struct net_tcp_client *cl);
    const char *(*get_peer_addr)(struct net_tcp_client *cl);
    int (*get_peer_port)(struct net_tcp_client *cl);
    const char *(*get_header)(struct net_tcp_client *cl, const char *name);
    const char *(*get_body)(struct net_tcp_client *cl, int *len);
};

struct net_tcp_server {
    struct uloop_fd fd;
    struct list_head clients;
    int nclients;
    void (*free)(struct net_tcp_server *srv);
    void (*on_accept)(struct net_tcp_client *cl);
    int (*on_request)(struct net_tcp_client *cl);
};

struct net_tcp_server *tcp_server_new(const char *host, int port);

#endif
