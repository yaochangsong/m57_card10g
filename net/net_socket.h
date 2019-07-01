#ifndef NET_SOCKET_H
#define NET_SOCKET_H
#include "../http/uhttpd.h"
#include "../http/client.h"

struct server {
    struct uloop_fd fd;
    struct list_head clients;
    void (*free)(struct uh_server *srv);
    void (*on_accept)(struct uh_client *cl);
    int (*on_request)(struct uh_client *cl);
};

struct server *tcp_server_new(const char *host, int port);

#endif

