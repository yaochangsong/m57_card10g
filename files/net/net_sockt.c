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
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/if_link.h>


struct net_tcp_server *g_srv;

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
    printf_info("tcp_free:");
    printf_info(": %s:%d\n", cl->get_peer_addr(cl), cl->get_peer_port(cl));
    if (cl) {
        uloop_timeout_cancel(&cl->timeout);
        ustream_free(&cl->sfd.stream);
        shutdown(cl->sfd.fd.fd, SHUT_RDWR);
        close(cl->sfd.fd.fd);
        list_del(&cl->list);
        cl->srv->nclients--;
        executor_tcp_disconnect_notify(cl);
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

static inline void tcp_keepalive_cb(struct uloop_timeout *timeout)
{
    
    struct net_tcp_client *cl = container_of(timeout, struct net_tcp_client, timeout);
    printf_note("keepalive: find %s:%d disconnect; free\n", cl->get_peer_addr(cl), cl->get_peer_port(cl));
    tcp_free(cl);
}

/* 网络连接时，保活定时器更新；此操作为TCP客户端连接 */
void update_tcp_keepalive(struct net_tcp_client *cl)
{
    if(&cl->timeout && (sizeof(struct uloop_timeout) == sizeof(cl->timeout))){
        uloop_timeout_set(&cl->timeout, TCP_CONNECTION_TIMEOUT * 1000);
    } 
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
    uloop_timeout_set(&cl->timeout, TCP_CONNECTION_TIMEOUT * 1000);

    list_add(&cl->list, &srv->clients);
    cl->srv = srv;
    cl->srv->nclients++;

    cl->free = tcp_free;
    //cl->vprintf = uh_vprintf;
    //cl->chunk_send = uh_chunk_send;
    //cl->chunk_printf = uh_chunk_printf;

    cl->get_peer_addr = tcp_get_peer_addr;
    cl->get_peer_port = tcp_get_peer_port;
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


int tcp_active_send_all_client(uint8_t *data, int len)
{
    struct net_tcp_client *cl_list, *list_tmp;
    list_for_each_entry_safe(cl_list, list_tmp, &g_srv->clients, list){
            printf_debug("Find ipaddree on list:%s, port=%d\n",  cl_list->get_peer_addr(cl_list), cl_list->get_peer_port(cl_list));
            ustream_write(cl_list->us, data, len, true);
    }
}

bool tcp_find_client(struct sockaddr_in *addr)
{
    struct net_tcp_client *cl_list, *list_tmp;
    list_for_each_entry_safe(cl_list, list_tmp, &g_srv->clients, list){
        if(memcmp(&cl_list->peer_addr.sin_addr, &addr->sin_addr, sizeof(addr->sin_addr)) == 0){
            return true;
        }
    }
    return false;
}

bool tcp_get_peer_addr_port(void *cl, struct sockaddr_in *_peer_addr)
{
    struct net_tcp_client *client = (struct net_tcp_client *)cl;
    printf_note("TCP data connection from: %s:%d\n", client->get_peer_addr(client), client->get_peer_port(client));
    memcpy(_peer_addr, &client->peer_addr, sizeof(struct sockaddr_in));
    printf_note("_peer_addr  connection from: %s:%d\n", inet_ntoa(_peer_addr->sin_addr), ntohs(_peer_addr->sin_port));
    return true;
}

uint32_t tcp_get_addr(void)
{
    struct net_tcp_client *cl_list, *list_tmp;
    list_for_each_entry_safe(cl_list, list_tmp, &g_srv->clients, list){
       return  cl_list->peer_addr.sin_addr.s_addr;
    }
    return 0;
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

