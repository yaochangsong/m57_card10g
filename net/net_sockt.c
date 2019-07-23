/******************************************************************************
*  Copyright 2019, Showay Technology Dev Co.,Ltd.
*  ---------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Showay Technology Dev Co.,Ltd. (C) 2019
******************************************************************************/
/*****************************************************************************     
*  Rev 1.0   06 July 2019   yaochangsong
*  Initial revision.
******************************************************************************/

#include "config.h"

struct net_tcp_server *g_srv;

static inline void tcp_ustream_read_cb(struct ustream *s, int bytes)
{
    uint8_t str[MAX_RECEIVE_DATA_LEN];
    int len;
    struct net_tcp_client *cl = container_of(s, struct net_tcp_client, sfd.stream);

    printf_debug("data from: %s:%d\n", cl->get_peer_addr(cl), cl->get_peer_port(cl));
    //str = ustream_get_read_buf(s, &len);
    memset(str, 0 ,sizeof(str));
    len =ustream_read(s, str, MAX_RECEIVE_DATA_LEN);
    if (!str || !len)
        return;
    poal_handle_request(cl, str,  len);
}

static void tcp_ustream_write_cb(struct ustream *s, int bytes)
{
     printf_debug("tcp_ustream_write_cb[%d]bytes\n",bytes);
}

void tcp_free(struct net_tcp_client *cl)
{
    printf_debug("tcp_free\n");
    if (cl) {
        uloop_timeout_cancel(&cl->timeout);
        ustream_free(&cl->sfd.stream);
        shutdown(cl->sfd.fd.fd, SHUT_RDWR);
        close(cl->sfd.fd.fd);
        list_del(&cl->list);
        cl->srv->nclients--;

       // if (cl->srv->on_client_free)
       // cl->srv->on_client_free(cl);

        free(cl);
    }
}


void tcp_client_notify_state(struct net_tcp_client *cl)
{
    struct ustream *us = cl->us;

    if (!us->write_error) {
        if (!us->eof || us->w.data_bytes)
            return;
    }

    tcp_free(cl);
}

static inline void tcp_notify_state(struct ustream *s)
{
    struct net_tcp_client *cl = container_of(s, struct net_tcp_client, sfd.stream);

    tcp_client_notify_state(cl);
}



static inline const char *tcp_get_peer_addr(struct net_tcp_client *cl)
{
    return inet_ntoa(cl->peer_addr.sin_addr);
}

static inline int tcp_get_peer_port(struct net_tcp_client *cl)
{
    return ntohs(cl->peer_addr.sin_port);
}

static inline void tcp_keepalive_cb(struct uloop_timeout *timeout)
{
    printf_debug("keepalive_cb\n");
}



static void tcp_accept_cb(struct uloop_fd *fd, unsigned int events)
{
    struct net_tcp_server *srv = container_of(fd, struct net_tcp_server, fd);

    struct net_tcp_client *cl;
    unsigned int sl;
    int sfd;
    struct sockaddr_in addr;

    sl = sizeof(addr);
    sfd = accept(srv->fd.fd, (struct sockaddr *)&addr, &sl);
    if (sfd < 0) {
        printf_err("accept");
        return;
    }

    cl = calloc(1, sizeof(struct net_tcp_client));
    if (!cl) {
        printf_err("calloc");
        goto err;
    }


    memcpy(&cl->peer_addr, &addr, sizeof(addr));
    printf_debug("connect: %s", inet_ntoa(cl->peer_addr.sin_addr));
   
    cl->us = &cl->sfd.stream;
    cl->us->notify_read = tcp_ustream_read_cb;
    cl->us->notify_write = tcp_ustream_write_cb;
    cl->us->notify_state = tcp_notify_state;

    cl->us->string_data = true;
    ustream_fd_init(&cl->sfd, sfd);

    cl->timeout.cb = tcp_keepalive_cb;
    uloop_timeout_set(&cl->timeout, 30 * 1000);

    list_add(&cl->list, &srv->clients);
    cl->srv = srv;
    cl->srv->nclients++;

    cl->free = tcp_free;
    //cl->vprintf = uh_vprintf;
    //cl->chunk_send = uh_chunk_send;
    //cl->chunk_printf = uh_chunk_printf;

    cl->get_peer_addr = tcp_get_peer_addr;
    cl->get_peer_port = tcp_get_peer_port;
    printf_info("New connection from: %s:%d\n", cl->get_peer_addr(cl), cl->get_peer_port(cl));

    return;
err:
    close(sfd);

}


int tcp_active_send_all_client(uint8_t *data, int len)
{
    struct net_tcp_client *cl_list, *list_tmp;
    list_for_each_entry_safe(cl_list, list_tmp, &g_srv->clients, list){
            printf_debug("Find ipaddree on list:%s, port=%d\n",  cl_list->get_peer_addr(cl_list), cl_list->get_peer_port(cl_list));
            ustream_write(cl_list->us, data, len, true);
    }
}

struct net_tcp_server *tcp_server_new(const char *host, int port)
{
    struct net_tcp_server *srv;
    int sock;
    
    printf_debug("tcp_server_new\n");
    sock = usock(USOCK_TCP | USOCK_SERVER | USOCK_IPV4ONLY, host, usock_port(port));
    if (sock < 0) {
        uh_log_err("usock");
        return NULL;
    }
    printf_debug("sock=%d\n", sock);
    srv = calloc(1, sizeof(struct net_tcp_server));
    if (!srv) {
        uh_log_err("calloc");
        goto err;
    }
    
    srv->fd.fd = sock;
    srv->fd.cb = tcp_accept_cb;
   
    uloop_fd_add(&srv->fd, ULOOP_READ);
    
    INIT_LIST_HEAD(&srv->clients);
    g_srv = srv;

    return srv;
    
err:
    close(sock);
    return NULL;
}

