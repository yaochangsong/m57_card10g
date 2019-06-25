#include <stdlib.h>
#include <unistd.h>
#include "server.h"

static void uh_accept_cb(struct uloop_fd *fd, unsigned int events)
{
    struct uh_server *srv = container_of(fd, struct uh_server, fd);

    uh_accept_client(srv, srv->ssl);
}

int un_server_open(const char *host, int port)
{
    int sock = usock(USOCK_TCP | USOCK_SERVER | USOCK_IPV4ONLY, host, usock_port(port));
    if (sock < 0) {
        uh_log_err("usock");
        return sock;
    }

    return sock;
}

static void uh_server_free(struct uh_server *srv)
{
    struct uh_client *cl, *tmp;
    
    if (srv) {
        close(srv->fd.fd);
        uloop_fd_delete(&srv->fd);

        list_for_each_entry_safe(cl, tmp, &srv->clients, list)
            cl->free(cl);

    }
}


void un_server_init(struct uh_server *srv, int sock)
{   
    srv->fd.fd = sock;
    srv->fd.cb = uh_accept_cb;
    uloop_fd_add(&srv->fd, ULOOP_READ);

    INIT_LIST_HEAD(&srv->clients);
    
    srv->free = uh_server_free;

}


struct uh_server *un_server_new(const char *host, int port)
{
    struct uh_server *srv;
    int sock;

    sock = un_server_open(host, port);
    if (sock < 0)
        return NULL;

    srv = calloc(1, sizeof(struct uh_server));
    if (!srv) {
        uh_log_err("calloc");
        goto err;
    }

    un_server_init(srv, sock);

    return srv;
    
err:
    close(sock);
    return NULL;
}


