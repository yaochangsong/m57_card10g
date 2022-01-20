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
*  Rev 2.0   06 March 2021   yaochangsong
*  
******************************************************************************/

#include "config.h"
#include "protocol/resetful/request.h"
#include "protocol/akt/akt.h"

static void on_accept(struct uh_client *cl)
{
    printf_note("New connection from: %s:%d\n", cl->get_peer_addr(cl), cl->get_peer_port(cl));
}

static int on_request(struct uh_client *cl)
{
    return http_on_request(cl);
}


struct net_server *nserver = NULL;

int get_use_ifname_num(void)
{
    if(nserver == NULL)
        return -1;
    
    return nserver->number;
}
void *get_cmd_server(int index)
{
    if(nserver == NULL)
        return NULL;

    return (void*)&nserver->cmd[index];
}

static void *_get_data_server(int index)
{
    if(nserver == NULL || index >= nserver->number)
        return NULL;

    return (void*)&nserver->data[index];
}

static enum net_listen_data_type _get_net_listen_index(int type)
{
    int listen_type = 0;
    if(type == NET_DATA_TYPE_FFT)
        listen_type = NET_DATA_TYPE_FFT;
#ifdef SUPPORT_DATA_PROTOCAL_TCP
    else if(type == NET_DATA_TYPE_BIQ)
        listen_type = NET_LISTEN_TYPE_BIQ;
    else if(type == NET_DATA_TYPE_AUDIO || type == NET_DATA_TYPE_NIQ)
        listen_type = NET_LISTEN_TYPE_NIQ;
#endif
    if(listen_type >= NET_LISTEN_TYPE_MAX)
        listen_type = 0;
    return listen_type;
}

int get_cmd_server_clients(void)
{
    int clients = 0;
#if (defined SUPPORT_PROTOCAL_AKT) || (defined SUPPORT_PROTOCAL_M57)
    struct net_tcp_server *srv;
    for(int i = 0; i < get_use_ifname_num(); i++){
        if((srv = ((union _cmd_srv *)get_cmd_server(i))->tcpsvr) == NULL){
            continue;
        }
        clients += srv->nclients;
    }
#elif defined(SUPPORT_PROTOCAL_XW)
    struct uh_server *srv;
    for(int i = 0; i < get_use_ifname_num(); i++){
        if((srv = ((union _cmd_srv *)get_cmd_server(i))->uhsvr) == NULL){
            continue;
        }
        clients += srv->nclients;
    }
#endif
    return clients;
}


void *get_data_server(int index, int type)
{
    int listen_type = 0;
    union _data_srv *srv;
    srv = (union _data_srv *)_get_data_server(index);
    if(srv == NULL)
        return NULL;
    listen_type = _get_net_listen_index(type);
#if (defined SUPPORT_DATA_PROTOCAL_TCP)
    return srv->tcpsrv[listen_type];
#else
    return srv->udpsrv[listen_type];
#endif
}
void *get_http_server(int index)
{
    if(nserver == NULL)
        return NULL;

    return (void*)nserver->http_server;
}

union _cmd_srv *cmd_cmd_server_init(char *ipaddr, int port, union _cmd_srv *cmdsrv)
{
#if (defined SUPPORT_PROTOCAL_AKT) 
    struct net_tcp_server *tcpsrv = NULL;
    printf_note("akt cmd server init [ipaddr: %s, port:%d]\n", ipaddr, port);
    
    tcpsrv = tcp_server_new(ipaddr, port);
    if (!tcpsrv)
        return NULL;

    tcpsrv->on_header = akt_parse_header_v2;
    tcpsrv->on_execute = akt_execute_method;
    tcpsrv->send_error =  akt_send_resp;
    tcpsrv->on_end = akt_parse_end;
    tcpsrv->send_alert = akt_send_alert;
    cmdsrv->tcpsvr = tcpsrv;
#elif defined(SUPPORT_PROTOCAL_XW)
    struct uh_server *srv = NULL;
    printf_note("xw cmd server init [ipaddr: %s, port:%d]\n", ipaddr, port);
    
    srv = uh_server_new(ipaddr, port);
    if (!srv)
        return NULL;
    
    srv->on_accept = on_accept;
    srv->on_request = on_request;
    cmdsrv->uhsvr = srv;
#elif defined(SUPPORT_PROTOCAL_M57)
    struct net_tcp_server *tcpsrv = NULL;
    printf_note("m57 cmd server init [ipaddr: %s, port:%d]\n", ipaddr, port);

    tcpsrv = tcp_server_new(ipaddr, port);
    if (!tcpsrv)
        return NULL;

    tcpsrv->on_header = m57_parse_header;
    tcpsrv->on_execute = m57_execute;
    tcpsrv->send_error =  m57_send_resp;
    //tcpsrv->on_end = akt_parse_end;
    //tcpsrv->send_alert = akt_send_alert;
    cmdsrv->tcpsvr = tcpsrv;

#endif

    return cmdsrv;
}

union _data_srv *data_data_server_init(char *ipaddr, int port, int index, union _data_srv *datasrv)
{
#if (defined SUPPORT_PROTOCAL_AKT) 
    struct net_udp_server *udpsrv = NULL;
    printf_note("akt data udp server init [ipaddr: %s, port:%d]\n", ipaddr, port);
    
    /* udp discovery must listen on 0.0.0.0:1234 */
    port = 1234;
    udpsrv = udp_server_new("0.0.0.0", port);
    if (!udpsrv)
        return NULL;
    
    udpsrv->on_discovery = akt_parse_discovery;
    datasrv->udpsrv = udpsrv;
#elif defined(SUPPORT_PROTOCAL_XW)
#if defined(SUPPORT_DATA_PROTOCAL_TCP)
    printf_note("xw data tcp server init [ipaddr: %s, port:%d]\n", ipaddr, port);
    struct net_tcp_server *tcpsrv = NULL;
    tcpsrv = tcp_data_server_new(ipaddr, port);
    if (!tcpsrv)
        return NULL;
    datasrv->tcpsrv[index] = tcpsrv;
#else
    struct net_udp_server *udpsrv = NULL;
    printf_note("xw data udp server init [ipaddr: %s, port:%d]\n", ipaddr, port);
    udpsrv = udp_server_new(ipaddr, port);
    if (!udpsrv)
        return NULL;
    datasrv->udpsrv[index] = udpsrv;
#endif
#endif
    return datasrv;
}

int server_init(void)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    char *ifname = NULL, *str_ipaddr;
    struct in_addr ipaddr;
    int port;
    int net_num = ARRAY_SIZE(poal_config->network);
    
    nserver = calloc(1, sizeof(struct net_server) + net_num * (sizeof(union _cmd_srv) + sizeof(union _data_srv)));
    if (!nserver) {
        printf("calloc");
        return -1;
    }

    
    for(int i = 0; i < net_num; i++){
        ifname = poal_config->network[i].ifname;
        if(!ifname || get_ipaddress(ifname, &ipaddr) == -1){
            continue;
        }
        str_ipaddr = inet_ntoa(ipaddr);
        port = poal_config->network[i].port;
        if(cmd_cmd_server_init(str_ipaddr, port, &nserver->cmd[i]) == NULL){
            return -1;
        }
        for(int j = 0; j < NET_LISTEN_TYPE_MAX; j++){
            port = poal_config->network[i].data_port[j];
            if(port <= 0){
                printf_warn("%d, %d, net port: %d err!!\n", i,j,port);
                continue;
            }
            if(data_data_server_init(str_ipaddr, port, j, &nserver->data[i]) == NULL){
            return -1;
            }
        }
        nserver->number++;
    }
#ifdef  SUPPORT_PROTOCAL_HTTP
    struct uh_server *srv = NULL;
    printf_note("http server init[port:%d]\n", 8760);
    srv = uh_server_new("0.0.0.0", 8760);
    if (!srv)
        return -1;
    srv->on_accept = on_accept;
    srv->on_request = on_request;
    nserver->http_server = srv;
#endif
    
    return 0;
}

