#include <stdlib.h>
#include <unistd.h>
#include <libubox/usock.h>
#include <libubox/avl-cmp.h>
#include <arpa/inet.h>
#include "net_socket.h"
#include "uhttpd.h"
#include "log.h"
#include "utils.h"



static inline void tcp_ustream_read_cb(struct ustream *s, int bytes)
{
    char str[1024];
    int len;
    struct uh_client *cl = container_of(s, struct uh_client, sfd.stream);

    printf("data from: %s:%d\n", cl->get_peer_addr(cl), cl->get_peer_port(cl));
    //str = ustream_get_read_buf(s, &len);
    len =ustream_read(s, str, 1024);
    if (!str || !len)
        return;
    printf("str = %s[%d]\n", str, len);
}

static void tcp_ustream_write_cb(struct ustream *s, int bytes)
{
     printf("tcp_ustream_write_cb[%d]bytes\n",bytes);
}

void tcp_free(struct uh_client *cl)
{
    printf("tcp_free\n");
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

void tcp_client_notify_state(struct uh_client *cl)
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
    struct uh_client *cl = container_of(s, struct uh_client, sfd.stream);

    tcp_client_notify_state(cl);
}



static inline const char *tcp_get_peer_addr(struct uh_client *cl)
{
    return inet_ntoa(cl->peer_addr.sin_addr);
}

static inline int tcp_get_peer_port(struct uh_client *cl)
{
    return ntohs(cl->peer_addr.sin_port);
}

static inline void tcp_keepalive_cb(struct uloop_timeout *timeout)
{
    printf("keepalive_cb\n");
}



static void tcp_accept_cb(struct uloop_fd *fd, unsigned int events)
{
    struct server *srv = container_of(fd, struct server, fd);

    struct uh_client *cl;
    unsigned int sl;
    int sfd;
    struct sockaddr_in addr;

    sl = sizeof(addr);
    sfd = accept(srv->fd.fd, (struct sockaddr *)&addr, &sl);
    if (sfd < 0) {
        uh_log_err("accept");
        return;
    }

    cl = calloc(1, sizeof(struct uh_client));
    if (!cl) {
        uh_log_err("calloc");
        goto err;
    }


    memcpy(&cl->peer_addr, &addr, sizeof(addr));
    printf("connect: %s", inet_ntoa(cl->peer_addr.sin_addr));
   
#if 1
    cl->us = &cl->sfd.stream;
    cl->us->notify_read = tcp_ustream_read_cb;
    cl->us->notify_write = tcp_ustream_write_cb;
    cl->us->notify_state = tcp_notify_state;

    cl->us->string_data = true;
    ustream_fd_init(&cl->sfd, sfd);

    cl->timeout.cb = tcp_keepalive_cb;
    uloop_timeout_set(&cl->timeout, 3 * 1000);
    printf("[%d]tcp_accept_cb\n", __LINE__);

    list_add(&cl->list, &srv->clients);
    cl->srv = srv;
    cl->srv->nclients++;

    cl->free = tcp_free;
    cl->vprintf = uh_vprintf;
    cl->chunk_send = uh_chunk_send;
    cl->chunk_printf = uh_chunk_printf;

    cl->get_peer_addr = tcp_get_peer_addr;
    cl->get_peer_port = tcp_get_peer_port;
    printf("New connection from: %s:%d\n", cl->get_peer_addr(cl), cl->get_peer_port(cl));

    char buf[]={0x11,0x12,0x11,0x12,0x11,0x12,0xaa,0x55,0x11,0x12,0x11,0x12,0x11,0x12,0xaa,0x55};
    //cl->chunk_printf(cl, buf);
   // cl->chunk_send(cl, buf, sizeof(buf));
   ustream_write(cl->us, buf, sizeof(buf), true);
   //cl->chunk_printf(cl, buf);
#endif
    return;
err:
    close(sfd);

}


struct server *tcp_server_new(const char *host, int port)
{
    struct server *srv;
    int sock;
    
    printf("tcp_server_new\n");
    sock = usock(USOCK_TCP | USOCK_SERVER | USOCK_IPV4ONLY, host, usock_port(port));
    if (sock < 0) {
        uh_log_err("usock");
        return sock;
    }
    printf("sock=%d\n", sock);
    srv = calloc(1, sizeof(struct server));
    if (!srv) {
        uh_log_err("calloc");
        goto err;
    }
    
    srv->fd.fd = sock;
    srv->fd.cb = tcp_accept_cb;
   
    uloop_fd_add(&srv->fd, ULOOP_READ);
    
    INIT_LIST_HEAD(&srv->clients);
    //srv->free = uh_server_free;


    return srv;
    
err:
    close(sock);
    return NULL;
}

