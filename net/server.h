#ifndef _SERVER_H
#define _SERVER_H

#include "client.h"

struct uh_server {
    struct uloop_fd fd;
    struct list_head clients;
    void (*free)(struct uh_server *srv);
}


#endif

