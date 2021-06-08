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
*  Rev 1.0   14 Sep 2020   yaochangsong
*  Paring data through function callback.
******************************************************************************/
#include "config.h"
#include <pthread.h>
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

int get_ifa_name_by_ip(char *ipaddr, char *ifa_name)
{
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if(ipaddr == NULL || ifa_name == NULL)
        return -1;
    
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return -1;
    }

    /* Walk through linked list, maintaining head pointer so we
       can free list later */

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;


        /* For an AF_INET* interface address, display the address */
        if (family == AF_INET || family == AF_INET6) {
            s = getnameinfo(ifa->ifa_addr,
                    (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                          sizeof(struct sockaddr_in6),
                    host, NI_MAXHOST,
                    NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                goto exit;
            }
            
            //printf("\t\taddress: <%s>, %s\n", host, ipaddr);
            if(!strcmp(host, ipaddr)){
                strcpy(ifa_name, ifa->ifa_name);
                break;
            }

        } 
    }
exit:
    freeifaddrs(ifaddr);
    return 0;
}

static bool tcp_client_header_cb(struct net_tcp_client *cl, char *buf, int len)
{
    bool stat;
    int code = 0;
    int head_len = 0;

    stat = cl->srv->on_header(cl, buf, len, &head_len, &code);
    if(stat == false){
        ustream_consume(cl->us, len);
        cl->state = NET_TCP_CLIENT_STATE_DONE;
        cl->srv->send_error(cl, code, NULL);
        cl->request_done(cl);
        return false;
    }
    ustream_consume(cl->us, head_len);
    cl->state = NET_TCP_CLIENT_STATE_DATA;
    return stat;
}

static void tcp_dispatch_done(struct net_tcp_client *cl)
{
    
    if (cl->dispatch.free)
        cl->dispatch.free(cl);

    memset(&cl->dispatch, 0, sizeof(struct tcp_dispatch));
}

static void tcp_data_free(struct net_tcp_client *cl)
{
    struct tcp_dispatch *d = &cl->dispatch;
    if(d->body){
        free(d->body);
        d->body = NULL;
    }
        
}

static void tcp_post_done(struct net_tcp_client *cl)
{
    int code = 0;
    if(cl->srv->on_execute){
        cl->srv->on_execute(cl, &code);
    }
    
    cl->srv->send_error(cl, code, NULL);
    
    if(cl->request_done)
        cl->request_done(cl);
}


static int tcp_load_data(struct net_tcp_client *cl, const char *data, int len)
{
    struct tcp_dispatch *d = &cl->dispatch;

    
    #if 0
    if (len > NET_DATA_BUF_SIZE) {
        printf_note("realloc: %p, %d\n", d->body, MAX_RECEIVE_DATA_LEN);
        d->body = realloc(d->body, MAX_RECEIVE_DATA_LEN);
        if (!d->body) {
            cl->srv->send_error(cl, 500, "No memory");
            return 0;
        }
    }
    #endif
    //printf_note("post len :%d, %p, len:%d\n", d->post_len, d->body , len);
    if(len > 0)
        memcpy(d->body + d->post_len, data, len);

    d->post_len += len;
    if (d->post_len > MAX_RECEIVE_DATA_LEN){
        printf_err("post len is too long:%d\n", d->post_len);
        goto err;
    }

#if 0
    int i;
    printfd("rec data: ");
    for(i = 0; i< len; i++){
        printfd("%02x ", d->body[i]);
    }
    printfd("\n");
#endif
    return len;
err:
    cl->srv->send_error(cl, 413, NULL);
    return 0;
}



static void tcp_client_poll_data(struct net_tcp_client *cl)
{
    struct tcp_dispatch *d = &cl->dispatch;
    struct net_tcp_request *r = &cl->request;
    char *buf;
    int len;
    int content_length = r->content_length;
    
    if(r->remain_length > 0)
        content_length = r->remain_length;
    
    if (cl->state == NET_TCP_CLIENT_STATE_DONE)
        return;
    d->post_data = tcp_load_data;
    d->post_done = tcp_post_done;
    d->free = tcp_data_free;
    if(d->body == NULL)
        d->body = calloc(1, MAX_RECEIVE_DATA_LEN);
    if (!d->body)
        cl->srv->send_error(cl, 500, "Internal Server Error,No memory");
    while (1) {
        int cur_len;
        buf = ustream_get_read_buf(cl->us, &len);
        if (!buf || !len){
             if(content_length){
                r->remain_length = content_length;
                return;
             }
             break;
        }
            

        if (!d->post_data)
            return;
        cur_len = min(content_length, len);
        if (content_length) {
            if (d->post_data)
                cur_len = d->post_data(cl, buf, cur_len);
            content_length -= cur_len;
            ustream_consume(cl->us, cur_len);
            continue;
        }else{
             int end_len;
             if(cl->srv->on_end){
                end_len = cl->srv->on_end(cl, buf, len);
                ustream_consume(cl->us, end_len);
             }
            break;
        }
    }
    if (!content_length && cl->state != NET_TCP_CLIENT_STATE_DONE) {
        if (cl->dispatch.post_done)
            cl->dispatch.post_done(cl);
    }else{
        if(cl->request_done)
            cl->request_done(cl);
    }

}

static inline bool tcp_client_data_cb(struct net_tcp_client *cl, char *buf, int len)
{
    tcp_client_poll_data(cl);
    return true;
}

static void tcp_client_request_done(struct net_tcp_client *cl)
{
    cl->state = NET_TCP_CLIENT_STATE_HEADER;
    tcp_dispatch_done(cl);
    safe_free(cl->request.header);
    safe_free(cl->response.data);
    tcp_data_free(cl);
    memset(&cl->request, 0, sizeof(cl->request));
    memset(&cl->response, 0, sizeof(cl->response));
}


typedef bool (*tcp_read_cb_t)(struct net_tcp_client *cl, char *buf, int len);
static tcp_read_cb_t tcp_read_cbs[] = {
    [NET_TCP_CLIENT_STATE_HEADER] = tcp_client_header_cb,
    [NET_TCP_CLIENT_STATE_DATA] = tcp_client_data_cb,
};


void tcp_client_read_cb(struct net_tcp_client *cl)
{
    struct ustream *us = cl->us;
    
    char *str;
    int len;
    
    do {
        str = ustream_get_read_buf(us, &len);
        if (!str || !len){
            return;
        }

        if (cl->state >= ARRAY_SIZE(tcp_read_cbs) || !tcp_read_cbs[cl->state]){
            return;
        }

        if (!tcp_read_cbs[cl->state](cl, str, len)) {
            if (len == us->r.buffer_len && cl->state != NET_TCP_CLIENT_STATE_DATA)
                cl->srv->send_error(cl, 413, NULL);
            break;
        }
    } while(1);
}

static inline void tcp_ustream_read_cb(struct ustream *s, int bytes)
{
    struct net_tcp_client *cl = container_of(s, struct net_tcp_client, sfd.stream);
    tcp_client_read_cb(cl);
}

static void tcp_ustream_write_cb(struct ustream *s, int bytes)
{
     printf_debug("tcp_ustream_write_cb[%d]bytes\n",bytes);
}

void tcp_free(struct net_tcp_client *cl)
{
    printf_note("tcp_free:");
    printf_note(": %s:%d\n", cl->get_peer_addr(cl), cl->get_peer_port(cl));
    struct net_tcp_server *srv;
    srv = cl->srv;
    pthread_mutex_lock(&srv->tcp_client_lock);
    if (cl) {
        uloop_timeout_cancel(&cl->timeout);
        ustream_free(&cl->sfd.stream);
        shutdown(cl->sfd.fd.fd, SHUT_RDWR);
        close(cl->sfd.fd.fd);
        list_del(&cl->list);
        cl->srv->nclients--;
        executor_net_disconnect_notify(&cl->peer_addr);
        free(cl);
    }
    pthread_mutex_unlock(&srv->tcp_client_lock);
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

static inline const char *get_serv_addr(struct net_tcp_client *cl)
{
    return inet_ntoa(cl->serv_addr.sin_addr);
}

static inline int tcp_get_peer_port(struct net_tcp_client *cl)
{
    return ntohs(cl->peer_addr.sin_port);
}

static inline int get_serv_port(struct net_tcp_client *cl)
{
    return ntohs(cl->serv_addr.sin_port);
}

static inline void tcp_keepalive_cb(struct uloop_timeout *timeout)
{
    
    struct net_tcp_client *cl = container_of(timeout, struct net_tcp_client, timeout);
    uloop_timeout_set(&cl->timeout, TCP_CONNECTION_TIMEOUT * 1000);
    printf_note("tcp_keepalive_probes = %d\n", cl->tcp_keepalive_probes);
#ifdef  SUPPORT_PROTOCAL_M57
    m57_send_heatbeat((void *)cl);
#endif
    if(++cl->tcp_keepalive_probes >= TCP_MAX_KEEPALIVE_PROBES){
        //printf_note("keepalive: find %s:%d disconnect; free\n", cl->get_peer_addr(cl), cl->get_peer_port(cl));
       // tcp_free(cl);
    }
}

/* 网络连接时，保活定时器更新；此操作为TCP客户端连接 */
void update_tcp_keepalive(void *client)
{
    struct net_tcp_client *cl = (struct net_tcp_client *)client;
    if(&cl->timeout && (sizeof(struct uloop_timeout) == sizeof(cl->timeout))){
        uloop_timeout_set(&cl->timeout, TCP_CONNECTION_TIMEOUT * 1000);
        cl->tcp_keepalive_probes = 0;
    } 
}

static int  tcp_send(struct net_tcp_client *cl, const void *data, int len)
{
    int s;
    s = ustream_write(cl->us, data, len, true);
    return s;
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

    cl->tcp_keepalive_probes = 0;
    cl->timeout.cb = tcp_keepalive_cb;
    uloop_timeout_set(&cl->timeout, TCP_CONNECTION_TIMEOUT * 1000);

    list_add(&cl->list, &srv->clients);
    cl->srv = srv;
    cl->srv->nclients++;

    cl->free = tcp_free;
    cl->get_peer_addr = tcp_get_peer_addr;
    cl->get_peer_port = tcp_get_peer_port;
    cl->send = tcp_send;
    cl->request_done = tcp_client_request_done;
    printf_note("New connection from: %s:%d\n", cl->get_peer_addr(cl), cl->get_peer_port(cl));
    
    cl->section.hash = net_hash_new();

    socklen_t serv_len = sizeof(cl->serv_addr); 
    getsockname(sfd, (struct sockaddr *)&cl->serv_addr, &serv_len); 
    cl->get_serv_addr = get_serv_addr;
    cl->get_serv_port = get_serv_port;
    printf_note("New connection Serv: %s:%d\n", cl->get_serv_addr(cl), cl->get_serv_port(cl));
    return;
err:
    close(sfd);

}

#if 0
void tcp_active_send_all_client_(uint8_t *data, int len)
{
    struct net_tcp_client *cl_list, *list_tmp;
    struct net_tcp_server *srv = (struct net_tcp_server *)get_cmd_srv(SRV_1G_NET);//(struct net_tcp_server *)net_get_tcp_srv_ctx();
    struct net_tcp_server *srv10g = (struct net_tcp_server *)net_get_10g_tcp_srv_ctx();

    if(srv == NULL && srv10g == NULL)
        return;
    pthread_mutex_lock(&srv->tcp_client_lock);
    if(srv != NULL){
        list_for_each_entry_safe(cl_list, list_tmp, &srv->clients, list){
            printf_debug("1g send status to %s:%d\n", cl_list->get_peer_addr(cl_list), cl_list->get_peer_port(cl_list));
            cl_list->send(cl_list,data, len);
        }
    }
    if(srv10g != NULL){
        list_for_each_entry_safe(cl_list, list_tmp, &srv10g->clients, list){
                printf_debug("10g send status to %s:%d\n", cl_list->get_peer_addr(cl_list), cl_list->get_peer_port(cl_list));
                cl_list->send(cl_list,data, len);
        }
    }

    pthread_mutex_unlock(&srv->tcp_client_lock);
}
#endif

void tcp_active_send_all_client(uint8_t *data, int len)
{
    struct net_tcp_client *cl_list, *list_tmp;
    union _cmd_srv *cmdsrv;
    
    for(int i = 0; i < get_use_ifname_num(); i++){
        cmdsrv = (union _cmd_srv *)get_cmd_server(i);
        if(cmdsrv == NULL)
            return;
        struct net_tcp_server *srv = (struct net_tcp_server *)cmdsrv->tcpsvr;
        if(srv == NULL)
            return;

        pthread_mutex_lock(&srv->tcp_client_lock);
        list_for_each_entry_safe(cl_list, list_tmp, &srv->clients, list){
            printf_debug("1g send status to %s:%d\n", cl_list->get_peer_addr(cl_list), cl_list->get_peer_port(cl_list));
            cl_list->send(cl_list,data, len);
        }
        pthread_mutex_unlock(&srv->tcp_client_lock);
    }
}


bool tcp_get_peer_addr_port(void *cl, struct sockaddr_in *_peer_addr)
{
    struct net_tcp_client *client = (struct net_tcp_client *)cl;
    printf_note("TCP data connection from: %s:%d\n", client->get_peer_addr(client), client->get_peer_port(client));
    memcpy(_peer_addr, &client->peer_addr, sizeof(struct sockaddr_in));
    printf_note("_peer_addr  connection from: %s:%d\n", inet_ntoa(_peer_addr->sin_addr), ntohs(_peer_addr->sin_port));
    return true;
}

struct net_tcp_server *tcp_server_new(const char *host, int port)
{
    #define TCP_SEND_BUF 2*1024*1024
    struct net_tcp_server *srv;
    int sock, _err;
    
    sock = usock(USOCK_TCP | USOCK_SERVER | USOCK_IPV4ONLY, host, usock_port(port));
    if (sock < 0) {
        uh_log_err("usock");
        return NULL;
    }
    printf_debug("sock=%d\n", sock);
    int opt = 6;
    _err = setsockopt(sock, SOL_SOCKET, SO_PRIORITY, &opt, sizeof(opt));
    if(_err){
        printf_err("setsockopt SO_PRIORITY failure \n");
    }
    int defrcvbufsize = -1;
    int nSnd_buffer = TCP_SEND_BUF;
    socklen_t optlen = sizeof(defrcvbufsize);
    if(getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &defrcvbufsize, &optlen) < 0)
    {
        printf("getsockopt error\n");
        return NULL;
    }
    printf_info("Current tcp default send buffer size is:%d\n", defrcvbufsize);
    if(setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &nSnd_buffer, optlen) < 0)
    {
        printf("set tcp recive buffer size failed.\n");
        return NULL;
    }
    if(getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &defrcvbufsize, &optlen) < 0)
    {
        printf("getsockopt error\n");
        return NULL;
    }
    printf_info("Now set tcp send buffer size to:%dByte\n",defrcvbufsize);
    srv = calloc(1, sizeof(struct net_tcp_server));
    if (!srv) {
        uh_log_err("calloc");
        goto err;
    }
    srv->fd.fd = sock;
    srv->fd.cb = tcp_accept_cb;
    pthread_mutex_init(&srv->tcp_client_lock,  NULL);
   
    uloop_fd_add(&srv->fd, ULOOP_READ);
    
    INIT_LIST_HEAD(&srv->clients);

    return srv;
    
err:
    close(sock);
    return NULL;
}

