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
*  Rev 1.0   14 Sep 2020   yaochangsong
*  Analysis of sending and receiving network raw data
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
#include "../protocol/http/file.h"
#include "net_data.h"


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

static inline  int get_serv_port(struct net_tcp_client *cl)
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
    if(cl->dispatch.file.fd)
        close(cl->dispatch.file.fd);
}

static void tcp_dispatch_done(struct net_tcp_client *cl)
{
    
    if (cl->dispatch.free)
        cl->dispatch.free(cl);

    memset(&cl->dispatch, 0, sizeof(struct tcp_dispatch));
}

static void tcp_data_client_request_done(struct net_tcp_client *cl)
{
    cl->us->eof = true;
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
    #define ONCE_SEND_MAX_BYTE (4096)
    int fd = cl->dispatch.file.fd;
    int r = 0,s = 0;
    void *ptr;

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
            if(r > ONCE_SEND_MAX_BYTE)
                r = ONCE_SEND_MAX_BYTE;
            s = cl->send(cl, ptr, r);
            if(s == 0){
                cl->request_done(cl);
                return;
            }
        }

        if(cl->send_over)
            cl->send_over((size_t *)&r);
    }
}

/*
功能：使用TCP发送原始数据
    @cl: 发送客户端信息
    @path: 若不为空，发送文件地址
    @callback: 若不为空，读取数据函数指针
    @callback_over: 发送完毕一帧后，函数操作指针
    .
    如：通过TCP发送IQ数据给客户端
    client->send_raw_data(client, NULL,  get_spm_ctx()->ops->read_niq_data,  get_spm_ctx()->ops->read_iq_over_deal);
    通过TCP发送文件给客户端
    client->send_raw_data(client, "/etc/file/CH0_D20200909192511390_F1000.000M_B175.000M_R409.600M_TIQ.wav", NULL, NULL);

*/
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
    return s;
}

static void tcp_ustream_read_data_cb(struct ustream *s , int bytes)
{
    struct net_tcp_client *cl = container_of(s, struct net_tcp_client, sfd.stream);
    struct tcp_dispatch *d = &cl->dispatch;
    struct net_tcp_request *r = &cl->request;
    char *buf;
    int len;

    while (1) {
        int cur_len = 0;
        /* 根据数据长度；循环读取数据流 */
        buf = ustream_get_read_buf(cl->us, &len);
        if (!buf || !len)
            break;

        if (!d->post_data)
            return;

        if (d->post_data)
            cur_len = d->post_data(cl, buf, len);
        cur_len = min(cur_len, len);
        ustream_consume(cl->us, cur_len);
        continue;
    }
    if(cl->request_done)
        cl->request_done(cl);
}


void net_data_add_client(struct sockaddr_in *addr, int ch, int type)
{
#if (defined SUPPORT_DATA_PROTOCAL_TCP)
    struct net_tcp_server *srv ;
    struct net_tcp_client *cl_list, *list_tmp;
    int ret = 0;
    for(int i = 0; i < get_use_ifname_num(); i++){
        srv = (struct net_tcp_server *)get_data_server(i, type);
        if(srv == NULL)
            continue;
        list_for_each_entry_safe(cl_list, list_tmp, &srv->clients, list){
            if(addr->sin_addr.s_addr == cl_list->peer_addr.sin_addr.s_addr &&
                addr->sin_port == cl_list->peer_addr.sin_port){
                cl_list->section.type[type] = true;
                cl_list->section.ch = ch;
            }
        }
    }
#else
    udp_add_client_to_list(addr, ch, type);
#endif
}
void net_data_remove_client(struct sockaddr_in *addr, int ch, int type)
{
#if (defined SUPPORT_DATA_PROTOCAL_TCP)
    struct net_tcp_server *srv ;
    struct net_tcp_client *cl_list, *list_tmp;
    int ret = 0;
    for(int i = 0; i < get_use_ifname_num(); i++){
        srv = (struct net_tcp_server *)get_data_server(i, type);
        if(srv == NULL)
            continue;
        list_for_each_entry_safe(cl_list, list_tmp, &srv->clients, list){
            if(addr->sin_addr.s_addr == cl_list->peer_addr.sin_addr.s_addr &&
                addr->sin_port == cl_list->peer_addr.sin_port){
                cl_list->section.type[type] = false;
                cl_list->section.ch = ch;
            }
        }
    }
#else
    udp_client_delete(addr, type);
#endif
}
static inline int tcp_send_vec_data_to_client(struct net_tcp_client *client, struct iovec *iov, int iov_len)
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
   // printf_debug("send: %s:%d\n", client->get_peer_addr(client), client->get_peer_port(client));
    r = sendmsg(client->sfd.fd.fd, &msgsent, 0);
    if(r < 0){
        perror("sendmsg");
    }
    return 0;
}

static int _tcp_send_data_to_client(int fd, const char *buf, int buflen)
{
    ssize_t ret = 0, len;

    if (!buflen)
        return 0;

    while (buflen) {
        len = send(fd, buf, buflen, 0);

        if (len < 0) {
            printf_note("[fd:%d]-send len : %ld, %d[%s]\n", fd, len, errno, strerror(errno));
            if (errno == EINTR)
                continue;

            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOTCONN)
                break;

            return -1;
        }

        ret += len;
        buf += len;
        buflen -= len;
    }
    
    return ret;
}
int tcp_send_data(void *data, int len, int type)
{
    struct net_tcp_server *srv ;
    struct net_tcp_client *cl_list, *list_tmp;
    int ret = 0;
    for(int i = 0; i < get_use_ifname_num(); i++){
        srv = (struct net_tcp_server *)get_data_server(i, type);
        if(srv == NULL)
            continue;
        
        list_for_each_entry_safe(cl_list, list_tmp, &srv->clients, list){
#ifdef SUPPORT_CLIENT_DATA_DISTRIBUTE
            if (cl_list->section.type[type] == true)
#endif
                _tcp_send_data_to_client(cl_list->sfd.fd.fd, data, len);
        }
    }
    return 0;
}
int tcp_send_vec_data(struct iovec *iov, int iov_len, int type)
{
    struct net_tcp_server *srv ;
    struct net_tcp_client *cl_list, *list_tmp;
    int ret = 0;
    for(int i = 0; i < get_use_ifname_num(); i++){
        srv = (struct net_tcp_server *)get_data_server(i, type);
        if(srv == NULL)
            continue;
        
        list_for_each_entry_safe(cl_list, list_tmp, &srv->clients, list){
#ifdef SUPPORT_CLIENT_DATA_DISTRIBUTE
            if (cl_list->section.type[type] == true)
#endif
                tcp_send_vec_data_to_client(cl_list, iov, iov_len);
        }
    }
    return 0;
}

int tcp_send_vec_data_by_secid(struct iovec *iov, int iov_len, int type, int sec_id)
{
#if defined(SUPPORT_PROTOCAL_M57)
    struct net_tcp_server *srv ;
    struct net_tcp_client *cl_list, *list_tmp;
    int ret = 0;
    for(int i = 0; i < get_use_ifname_num(); i++){
        srv = (struct net_tcp_server *)get_cmd_server(i);
        if(srv == NULL)
            continue;
        
        list_for_each_entry_safe(cl_list, list_tmp, &srv->clients, list){
            if (cl_list->section.section_id == sec_id)
                tcp_send_vec_data_to_client(cl_list, iov, iov_len);
        }
    }
#endif
    return 0;
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
    cl->us->notify_read = tcp_ustream_read_data_cb;
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
    #define TCP_SEND_BUF 4*1024*1024
    struct net_tcp_server *srv;
    int sock, _err;
    
    sock = usock(USOCK_TCP | USOCK_SERVER | USOCK_IPV4ONLY, host, usock_port(port));
    if (sock < 0) {
        uh_log_err("usock");
        return NULL;
    }
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
    srv->fd.cb = tcp_data_accept_cb;
   
    uloop_fd_add(&srv->fd, ULOOP_READ);
    
    INIT_LIST_HEAD(&srv->clients);

    return srv;
    
err:
    close(sock);
    return NULL;
}

