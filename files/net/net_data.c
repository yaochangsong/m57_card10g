#include "config.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/if_link.h>
#include "../protocol/http/file.h"
#include "net_data.h"

#if 1
struct net_tcp_client *tcp_get_datasrv_client(char *ipaddr)
{
    struct net_tcp_client *cl_list, *list_tmp;
    struct net_tcp_server *srv = (struct net_tcp_server *)net_get_data_srv_ctx();
    list_for_each_entry_safe(cl_list, list_tmp, &srv->clients, list){
            printf_debug("Find ipaddree on list:%s, port=%d\n",  cl_list->get_peer_addr(cl_list), cl_list->get_peer_port(cl_list));
            return cl_list;
    }
    printf_note("client is null\n");
    return NULL;
}
#endif


static  void tcp_send_raw_data_cancel(struct net_tcp_client *cl)
{
    printf_note("cancel send data:%d\n", cl->dispatch.cmd);
    cl->dispatch.cmd = 0;
    printf_note("cancel send data over\n");
}
static  bool is_tcp_send_raw_data_cancel(struct net_tcp_client *cl)
{
    if(cl->dispatch.cmd == 0)
        return true;
    else
        return false;
}

static void tcp_ustream_write_cb(struct ustream *s, int bytes)
{
    struct net_tcp_client *cl = container_of(s, struct net_tcp_client, sfd.stream);
    if (cl->dispatch.write_cb)
        cl->dispatch.write_cb(cl);
}

static inline const char *tcp_get_peer_addr(struct net_tcp_client *cl)
{
    return inet_ntoa(cl->peer_addr.sin_addr);
}

static inline const char *get_serv_addr(struct net_tcp_client *cl)
{
    return inet_ntoa(cl->serv_addr.sin_addr);
}

static inline int tcp_get_peer_port(struct net_tcp_client *cl)
{
    return ntohs(cl->peer_addr.sin_port);
}

static inline const char *get_serv_port(struct net_tcp_client *cl)
{
    return ntohs(cl->serv_addr.sin_port);
}

static void tcp_free(struct net_tcp_client *cl)
{
    printf_note("tcp_free:");
    printf_note(": %s:%d\n", cl->get_peer_addr(cl), cl->get_peer_port(cl));
    if (cl) {
       // uloop_timeout_cancel(&cl->timeout);
        ustream_free(&cl->sfd.stream);
        shutdown(cl->sfd.fd.fd, SHUT_RDWR);
        close(cl->sfd.fd.fd);
        list_del(&cl->list);
        cl->srv->nclients--;
        free(cl);
    }
}

static void tcp_client_notify_state(struct net_tcp_client *cl)
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

static void tcp_raw_data_write_free(struct net_tcp_client *cl)
{
    printf_note("data write free...[fd=%d]\n", cl->dispatch.file.fd);
    if(cl->dispatch.file.fd)
        close(cl->dispatch.file.fd);
}

static void tcp_dispatch_done(struct net_tcp_client *cl)
{
    
    if (cl->dispatch.free)
        cl->dispatch.free(cl);

    memset(&cl->dispatch, 0, sizeof(struct dispatch));
}

static void tcp_data_client_request_done(struct net_tcp_client *cl)
{
    cl->us->eof = true;
    ustream_state_change(cl->us);
    tcp_dispatch_done(cl);
    safe_free(cl->request.header);
    safe_free(cl->response.data);
    memset(&cl->request, 0, sizeof(cl->request));
    memset(&cl->response, 0, sizeof(cl->response));
   // tcp_free(cl);
}


static void tcp_file_data_write_loop(struct net_tcp_client *cl)
{
    static char buf[4096];
    int fd = cl->dispatch.file.fd;
    int r = 0;
    void *ptr;
    usleep(1);
    while (cl->us->w.data_bytes < 256) {
        r = read(fd, buf, sizeof(buf));
        if (r < 0) {
            if (errno == EINTR)
                continue;
            uh_log_err("read");
            printf_err("read\n");
        }
        if (r <= 0) {
            cl->request_done(cl);
            return;
        }
        cl->send(cl, buf, r);
    }
}


static void tcp_raw_data_write_loop(struct net_tcp_client *cl)
{
    int fd = cl->dispatch.file.fd;
    int r = 0,s = 0;
    void *ptr;
    usleep(1);
    while (cl->us->w.data_bytes < 256) {
        if(cl->srv->read_raw_data)
            r = cl->srv->read_raw_data(&ptr);
        if (r < 0) {
            if (errno == EINTR)
                continue;
            uh_log_err("read");
        }

        if (r <= 0) {
            printf_note("request_done:r=%d\n", r);
            cl->request_done(cl);
            return;
        }
        if(cl->send){
            s = cl->send(cl, ptr, r);
            if(s == 0){
                cl->request_done(cl);
                return;
            }
                
        }

        if(cl->send_over)
            cl->send_over(&r);
        #if 0
        if(cl->is_send_raw_data_cancel){
            if(cl->is_send_raw_data_cancel(cl)){
                cl->request_done(cl);
                return;
            }
        }
        #endif
    }
}

static void tcp_raw_data_write_cb(struct net_tcp_client *cl, const char *path, 
                                        size_t (*callback) (void **), int (*callback_over) (size_t *))
{
    int fd;
    if(path != NULL){
        struct path_info *pi;
        printf_note("read path:%s\n", path);
        fd = open(path, O_RDONLY);
        if (fd < 0){
            printf_err("error\n");
            return;
        }
        cl->dispatch.file.fd = fd;
        cl->dispatch.write_cb = tcp_file_data_write_loop;
        cl->dispatch.free = tcp_raw_data_write_free;
        printf_warn("from: %s:%d\n", cl->get_peer_addr(cl), cl->get_peer_port(cl));
        tcp_file_data_write_loop(cl);
    }else if(callback != NULL && callback_over != NULL){
        printf_warn("==>from: %s:%d, %p\n", cl->get_peer_addr(cl), cl->get_peer_port(cl), callback_over);
        cl->dispatch.cmd = 1;
        cl->send_over = callback_over;
        cl->srv->read_raw_data = callback;
        cl->dispatch.write_cb = tcp_raw_data_write_loop;
        cl->dispatch.free = tcp_raw_data_write_free;
        tcp_raw_data_write_loop(cl);
    }

}

static int tcp_send(struct net_tcp_client *cl, const void *data, int len)
{
    int s;
    s = ustream_write(cl->us, data, len, true);
}


static void tcp_data_accept_cb(struct uloop_fd *fd, unsigned int events)
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
 //   cl->us->notify_read = tcp_ustream_read_cb;
    cl->us->notify_write = tcp_ustream_write_cb;
    cl->us->notify_state = tcp_notify_state;

    cl->us->string_data = true;
    ustream_fd_init(&cl->sfd, sfd);

   // cl->timeout.cb = tcp_keepalive_cb;
  //  uloop_timeout_set(&cl->timeout, TCP_CONNECTION_TIMEOUT * 1000);

    list_add(&cl->list, &srv->clients);
    cl->srv = srv;
    cl->srv->nclients++;

    cl->free = tcp_free;
    cl->get_peer_addr = tcp_get_peer_addr;
    cl->get_peer_port = tcp_get_peer_port;
    cl->send = tcp_send;
    cl->send_raw_data = tcp_raw_data_write_cb;
    cl->request_done = tcp_data_client_request_done;
    cl->send_raw_data_cancel = tcp_send_raw_data_cancel;
    cl->is_send_raw_data_cancel = is_tcp_send_raw_data_cancel;
    printf_note("New connection from: %s:%d\n", cl->get_peer_addr(cl), cl->get_peer_port(cl));

    socklen_t serv_len = sizeof(cl->serv_addr); 
    getsockname(sfd, (struct sockaddr *)&cl->serv_addr, &serv_len); 
    cl->get_serv_addr = get_serv_addr;
    cl->get_serv_port = get_serv_port;
    printf_note("New connection Serv: %s:%d\n", cl->get_serv_addr(cl), cl->get_serv_port(cl));
    return;
err:
    close(sfd);

}


struct net_tcp_server *tcp_data_server_new(const char *host, int port)
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
    srv->fd.cb = tcp_data_accept_cb;
   
    uloop_fd_add(&srv->fd, ULOOP_READ);
    
    INIT_LIST_HEAD(&srv->clients);

    return srv;
    
err:
    close(sock);
    return NULL;
}

