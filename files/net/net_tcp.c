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
#include <netinet/tcp.h>
#include "../protocol/http/file.h"
#include "net_sub.h"
#include "../bsp/io.h"
#include "net_thread.h"



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
        cl->state = NET_TCP_CLIENT_STATE_DONE;
        //cl->srv->send_error(cl, code, NULL);
        ustream_consume(cl->us, len);
        cl->request_done(cl);
        return false;
    }
    if(cl->request.data_state == NET_TCP_DATA_WAIT){
        cl->state = NET_TCP_CLIENT_STATE_HEADER;
        cl->request_done(cl);
        return false;
    } else if(cl->request.data_state == NET_TCP_DATA_NO_DEAL){
        cl->state = NET_TCP_CLIENT_STATE_DONE;
        ustream_consume(cl->us, head_len);
        cl->request_done(cl);
        return false;
    }
    ustream_consume(cl->us, head_len);
    //cl->state = NET_TCP_CLIENT_STATE_DATA;
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
    if(cl->response.need_resp == true)
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
            //if (len == us->r.buffer_len && cl->state != NET_TCP_CLIENT_STATE_DATA)
               // cl->srv->send_error(cl, 413, NULL);
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
    printfn("\nSTART tcp_free:");
    printfn(": %s:%d\n", cl->get_peer_addr(cl), cl->get_peer_port(cl));
    struct net_tcp_server *srv;
    srv = cl->srv;
    
    if (cl) {
        pthread_mutex_lock(&cl->section.free_lock);
        cl->section.is_exitting = true;
        pthread_mutex_unlock(&cl->section.free_lock);
        pthread_mutex_lock(&srv->tcp_client_lock);
        /* close clinet dispatcher thread */
        if(cl->section.thread && cl->section.thread->ops->close)
            cl->section.thread->ops->close(cl);
        //executor_net_disconnect_notify(&cl->peer_addr);
        client_hash_delete(cl);
        uloop_timeout_cancel(&cl->timeout);
        ustream_free(&cl->sfd.stream);
        shutdown(cl->sfd.fd.fd, SHUT_RDWR);
        close(cl->sfd.fd.fd);
        list_del(&cl->list);
        cl->srv->nclients--;
        socket_bitmap_clear(cl->section.section_id);
        _safe_free_(cl);
        pthread_mutex_unlock(&srv->tcp_client_lock);
    }
    printfn("End tcp_free\n");
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
    printf_debug("tcp_keepalive_probes = %d\n", cl->tcp_keepalive_probes);
#ifdef  SUPPORT_PROTOCAL_M57
    m57_send_heatbeat((void *)cl);
#endif
#if 1
    char *ifname = config_get_ifname_by_addr(&cl->serv_addr);
    if(ifname != NULL){
        int32_t link = get_netlink_status(ifname);
            if(link == 0){
                printf_note("%s: down\n", ifname);
                cl->tcp_keepalive_probes++;
            } else{
                printf_debug("%s: up\n", ifname);
                cl->tcp_keepalive_probes = 0;
            }
    }
#endif
    if(cl->tcp_keepalive_probes > TCP_MAX_KEEPALIVE_PROBES){
        printf_note("keepalive: find %s:%d disconnect; free\n", cl->get_peer_addr(cl), cl->get_peer_port(cl));
        tcp_free(cl);
    }
}

/* ÍøÂçÁ¬½ÓÊ±£¬±£»î¶¨Ê±Æ÷¸üÐÂ£»´Ë²Ù×÷ÎªTCP¿Í»§¶ËÁ¬½Ó */
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

    cl->free = tcp_free;
    cl->get_peer_addr = tcp_get_peer_addr;
    cl->get_peer_port = tcp_get_peer_port;
    cl->send = tcp_send;
    cl->request_done = tcp_client_request_done;
    
    pthread_mutex_init(&cl->section.free_lock, NULL);
    printf_note("New connection from: %s:%d\n", cl->get_peer_addr(cl), cl->get_peer_port(cl));

    socklen_t serv_len = sizeof(cl->serv_addr); 
    getsockname(sfd, (struct sockaddr *)&cl->serv_addr, &serv_len); 
    cl->get_serv_addr = get_serv_addr;
    cl->get_serv_port = get_serv_port;
    printf_note("New connection Serv: %s:%d\n", cl->get_serv_addr(cl), cl->get_serv_port(cl));

    client_hash_create(cl);
    cl->section.thread = net_thread_create_context(cl);
    cl->section.section_id = socket_bitmap_find_index();
    if(cl->section.section_id >= MAX_CLINET_SOCKET_NUM)
        printf_warn("client: %s:%d. socket section id[%d] is use over[%d]!!\n", 
            cl->get_peer_addr(cl), cl->get_peer_port(cl), cl->section.section_id, MAX_CLINET_SOCKET_NUM);
    else
        socket_bitmap_set(cl->section.section_id);
    printf_note("section id=%d\n",cl->section.section_id);

    list_add(&cl->list, &srv->clients);
    cl->srv = srv;
    pthread_mutex_lock(&srv->tcp_client_lock);
    cl->srv->nclients++;
    pthread_mutex_unlock(&srv->tcp_client_lock);

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

static inline bool _is_idata_in_range(int indata, int dstdata, int range)
{
    if((indata - range <= dstdata) && (indata + range >= dstdata)){
        return true;
    }
    return false;
}

struct net_tcp_client *tcp_find_prio_client(void *client, int prio)
{
    struct net_tcp_client *cl = client;
    struct net_tcp_client *cl_list, *list_tmp;

    if(unlikely(cl == NULL))
        return NULL;
   // pthread_mutex_lock(&cl->section.free_lock);
   // if(cl->section.is_exitting == true){
   //     pthread_mutex_lock(&cl->section.free_lock);
   //     return NULL;
    //}
    //pthread_mutex_lock(&cl->section.free_lock);

    struct net_tcp_server *srv = cl->srv;
    list_for_each_entry_safe(cl_list, list_tmp, &srv->clients, list){
        printf_info("prio:%d, %d, %s:%d\n", cl_list->section.prio, prio, cl_list->get_peer_addr(cl_list), cl_list->get_peer_port(cl_list));
        if(cl->peer_addr.sin_addr.s_addr == cl_list->peer_addr.sin_addr.s_addr && 
            prio == cl_list->section.prio && 
            _is_idata_in_range(cl_list->get_peer_port(cl_list), cl->get_peer_port(cl), 1)){
            return cl_list;
        }
    }
    /* Not Find client */
    printf_info("NOT find client, try again\n");
    list_for_each_entry_safe(cl_list, list_tmp, &srv->clients, list){
        if(cl->peer_addr.sin_addr.s_addr == cl_list->peer_addr.sin_addr.s_addr && prio == cl_list->section.prio){
            return cl_list;
        }
    }
    return NULL;
}

struct net_tcp_client *tcp_find_rss_client(void *client, int id)
{
    struct net_tcp_client *cl = client;
    struct net_tcp_client *cl_list, *list_tmp;

    if(cl == NULL)
        return NULL;
   // pthread_mutex_lock(&cl->section.free_lock);
   // if(cl->section.is_exitting == true){
   //     pthread_mutex_lock(&cl->section.free_lock);
   //     return NULL;
    //}
    //pthread_mutex_lock(&cl->section.free_lock);

    struct net_tcp_server *srv = cl->srv;
    int port;
    list_for_each_entry_safe(cl_list, list_tmp, &srv->clients, list){
         port = cl_list->get_peer_port(cl_list);
        printf_note("%s:%d\n", cl_list->get_peer_addr(cl_list), port);
        if(cl->peer_addr.sin_addr.s_addr == cl_list->peer_addr.sin_addr.s_addr && (port % 4)  == (id % 4)){
            return cl_list;
        }
    }
    /* Not Find client */
    printf_warn("NOT find client, try again\n");
    list_for_each_entry_safe(cl_list, list_tmp, &srv->clients, list){
        if(cl->peer_addr.sin_addr.s_addr == cl_list->peer_addr.sin_addr.s_addr){
            return cl_list;
        }
    }
    return NULL;
}



int tcp_client_do_for_each(int (*f)(struct net_tcp_client *, void *), void **cl, int prio, void *args)
{
    struct net_tcp_client *cl_list, *list_tmp;
    union _cmd_srv *cmdsrv;
    int client_num = 0, ret = -1;
    
    for(int i = 0; i < get_use_ifname_num(); i++){
        cmdsrv = (union _cmd_srv *)get_cmd_server(i);
        if(cmdsrv == NULL){
            printf_note("cmd server is null\n");
            return client_num;
        }
            
        struct net_tcp_server *srv = (struct net_tcp_server *)cmdsrv->tcpsvr;
        if(srv == NULL){
            printf_note("tcp server is null\n");
            return client_num;
        }
        pthread_mutex_lock(&srv->tcp_client_lock);
        //list_for_each_entry_safe(cl_list, list_tmp, &srv->clients, list){
        list_for_each_entry_reverse(cl_list, &srv->clients, list){
        #if 0
            if(client_num == 0){
                *cl = cl_list;
                //client_num++;
            }else
        #endif
            {
#ifdef PRIO_CHANNEL_EN
                if(prio >= 0 && prio == cl_list->section.prio)
#endif
                {
                    ret = f(cl_list, args);
                    if(ret == 0){
                        client_num++;
                    }
                }
            }
        }
        pthread_mutex_unlock(&srv->tcp_client_lock);
    }
    return client_num;
}


int tcp_send_data_to_client(int fd, const char *buf, int buflen)
{
    ssize_t ret = 0, len;

    if (!buflen)
        return 0;

    while (buflen) {
        len = send(fd, buf, buflen, 0);

        if (len < 0) {
            //printf_note("[fd:%d]-send len : %ld, %d[%s], %d, %d, %d, %d\n", fd, len, errno, strerror(errno), EAGAIN, EWOULDBLOCK, ENOTCONN, EINTR);
            //if (errno == EINTR )
            //    continue;

            if ( errno == ENOTCONN || errno == EAGAIN || errno == EWOULDBLOCK)
                break;

            return -1;
        }

        ret += len;
        buf += len;
        buflen -= len;
    }
    
    return ret;
}


int send_vec_data_to_client(struct net_tcp_client *client, struct iovec *iov, int iov_len)
{    
    struct msghdr msgsent;
    int r=0;
    if(client == NULL)
        return -1;
    msgsent.msg_name = &client->peer_addr;
    msgsent.msg_namelen = sizeof(client->peer_addr);
    msgsent.msg_iovlen = iov_len;
    msgsent.msg_iov = iov;
    msgsent.msg_control = NULL;
    msgsent.msg_controllen = 0;
    //printf_note("send: %s:%d\n", client->get_peer_addr(client), client->get_peer_port(client));
    r = sendmsg(client->sfd.fd.fd, &msgsent, 0);
    if(r < 0){
        perror("sendmsg");
    }
    if(iov_len == 1 && r != iov->iov_len)
        printf_warn("err send:%d/%lu\n", r,iov->iov_len);
    return r;
}


int tcp_send_vec_data_uplink(struct iovec *iov, int iov_len, void *args)
{
    struct net_tcp_server *srv ;
    struct net_tcp_client *cl_list, *list_tmp;
    int ret = 0;
    union _cmd_srv *cmdsrv;
    char *ifname;

    struct net_sub_st *parg = args;
    if(parg == NULL)
        return -1;
    for(int i = 0; i < get_use_ifname_num()+1; i++){
        ifname = config_get_if_indextoname(i);
        if(!ifname || get_netlink_status(ifname) == -1)
            continue;
        cmdsrv = (union _cmd_srv *)get_cmd_server(i);
        srv = (struct net_tcp_server *)cmdsrv->tcpsvr;
        if(srv == NULL)
            continue;
        list_for_each_entry_safe(cl_list, list_tmp, &srv->clients, list){
            //if(net_hash_find(cl_list->section.hash, parg->chip_id, RT_CHIPID) && 
            //    net_hash_find(cl_list->section.hash, parg->func_id, RT_FUNCID)){
                    //printf_note("send to port: %d, 0x%x, 0x%x\n", cl_list->get_peer_port(cl_list), parg->chip_id, parg->func_id);
                    send_vec_data_to_client(cl_list, iov, iov_len);
            //    }
                
        }
    }
    return 0;
}

bool tcp_get_peer_addr_port(void *cl, struct sockaddr_in *_peer_addr)
{
    struct net_tcp_client *client = (struct net_tcp_client *)cl;
    printf_note("TCP data connection from: %s:%d\n", client->get_peer_addr(client), client->get_peer_port(client));
    memcpy(_peer_addr, &client->peer_addr, sizeof(struct sockaddr_in));
    printf_note("_peer_addr  connection from: %s:%d\n", inet_ntoa(_peer_addr->sin_addr), ntohs(_peer_addr->sin_port));
    return true;
}

int _set_socket_keepalive(int listenfd)
{
        int optval;
    socklen_t optlen = sizeof(optval);

        /* Check the status for the keepalive option */
    if(getsockopt(listenfd, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0) {
        perror("getsockopt()");
        close(listenfd);
        //exit(EXIT_FAILURE);
    }
    printf("SO_KEEPALIVE is %s\n", (optval ? "ON" : "OFF"));

    /* Set the option active */
    optval = 1;
    optlen = sizeof(optval);
    if(setsockopt(listenfd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
        perror("setsockopt()");
        close(listenfd);
        //exit(EXIT_FAILURE);
    }

    int keepIdle = 5;     //30ç§’æ²¡æœ‰æ•°æ®ä¸Šæ¥ï¼Œåˆ™å‘é€æŽ¢æµ‹åŒ…
    int keepInterval = 10;  //æ¯éš”10å‘æ•°ä¸€ä¸ªæŽ¢æµ‹åŒ…
    int keepCount = 3;      //å‘é€3ä¸ªæŽ¢æµ‹åŒ…ï¼Œæœªæ”¶åˆ°åé¦ˆåˆ™ä¸»åŠ¨æ–­å¼€è¿žæŽ¥
    setsockopt(listenfd, SOL_SOCKET, TCP_KEEPIDLE, (void *)&keepIdle, sizeof(keepIdle));
    setsockopt(listenfd, SOL_SOCKET,TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval));
    setsockopt(listenfd, SOL_SOCKET, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount));

    printf("SO_KEEPALIVE set on socket\n");

    /* Check the status again */
    if(getsockopt(listenfd, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0) {
        perror("getsockopt()");
        close(listenfd);
        //exit(EXIT_FAILURE);
    }
    printf("SO_KEEPALIVE is %s\n", (optval ? "ON" : "OFF"));
    return 0;
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
    /* keepalive */
    //_set_socket_keepalive(sock);
    #if 0
    int keepalive = 1; //å¼€å¯keepaliveå±žæ€§
    int keepidle = 5;
    int keepinterval = 5;
    int keepcount = 3;
    printf_note("KEEP Alive SET\n");
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepalive,sizeof(keepalive));
    setsockopt(sock, SOL_TCP, TCP_KEEPIDLE, (void*)&keepidle,sizeof(keepidle));
    setsockopt(sock, SOL_TCP, TCP_KEEPINTVL, (void*)&keepinterval,sizeof(keepinterval));
    setsockopt(sock, SOL_TCP, TCP_KEEPCNT, (void*)&keepcount,sizeof(keepcount));
    #endif
    /* end keepalive */
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

