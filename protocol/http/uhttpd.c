/*
 * Copyright (C) 2017 Jianhui Zhao <jianhuizhao329@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#include <stdlib.h>
#include <unistd.h>
#include "../../libubox/usock.h"
#include "../../libubox/avl-cmp.h"

#include "uhttpd.h"
//#include "uh_ssl.h"
#include "../../log/log.h"
 
static void uh_set_docroot(struct uh_server *srv, const char *docroot)
{
    free(srv->docroot);
    srv->docroot = strdup(docroot);
}

static void uh_set_index_file(struct uh_server *srv, const char *index_file)
{
    free(srv->index_file);
    srv->index_file = strdup(index_file);
}

static void uh_server_free(struct uh_server *srv)
{
    struct uh_client *cl, *tmp;
    
    if (srv) {
        close(srv->fd.fd);
        uloop_fd_delete(&srv->fd);

        list_for_each_entry_safe(cl, tmp, &srv->clients, list)
            cl->free(cl);

        //uh_ssl_free();
        free(srv->docroot);
        free(srv->index_file);
    }
}

static void uh_accept_cb(struct uloop_fd *fd, unsigned int events)
{
    struct uh_server *srv = container_of(fd, struct uh_server, fd);

    uh_accept_client(srv, srv->ssl);
}

int uh_server_open(const char *host, int port)
{
    int sock = usock(USOCK_TCP | USOCK_SERVER | USOCK_IPV4ONLY, host, usock_port(port));
    if (sock < 0) {
        uh_log_err("usock");
        return sock;
    }

    return sock;
}

void uh_server_init(struct uh_server *srv, int sock)
{
    srv->docroot = strdup(".");
    srv->index_file = strdup("index.html");

    srv->fd.fd = sock;
    srv->fd.cb = uh_accept_cb;
    uloop_fd_add(&srv->fd, ULOOP_READ);

    INIT_LIST_HEAD(&srv->clients);
    
    srv->free = uh_server_free;
    srv->set_docroot = uh_set_docroot;
    srv->set_index_file = uh_set_index_file;

#if (UHTTPD_SSL_SUPPORT)
    srv->ssl_init = uh_ssl_init;
#endif  
}

struct uh_server *uh_server_new(const char *host, int port)
{
    struct uh_server *srv;
    int sock;

    sock = uh_server_open(host, port);
    if (sock < 0)
        return NULL;

    srv = calloc(1, sizeof(struct uh_server));
    if (!srv) {
        uh_log_err("calloc");
        goto err;
    }

    uh_server_init(srv, sock);

    return srv;
    
err:
    close(sock);
    return NULL;
}
