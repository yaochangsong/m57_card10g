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
#include "protocol/resetful/request.h"
#include "protocol/akt/akt.h"

static void on_accept(struct uh_client *cl)
{
    printf_info("New connection from: %s:%d\n", cl->get_peer_addr(cl), cl->get_peer_port(cl));
}

static int on_request(struct uh_client *cl)
{
    return http_on_request(cl);
}


struct net_server nserver;
struct net_ifname_t net_if[] = {
    {SRV_1G_NET, NETWORK_EHTHERNET_POINT},
#ifdef SUPPORT_NET_WZ
    {SRV_10G_NET, NETWORK_10G_EHTHERNET_POINT},
#endif
};

int get_use_ifname_num(void)
{
    return ARRAY_SIZE(net_if);
}
void *get_cmd_srv(int index)
{
    return (void*)&nserver.cmd[index];
}

void *get_data_srv(int index)
{
    return (void*)&nserver.data[index];
}

void *get_http_srv(int index)
{
    return (void*)nserver.http_server;
}

int server_init(void)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    int if_index = ARRAY_SIZE(net_if);
    struct in_addr ipaddr[if_index+1];
    char ipstr[if_index+1][32];
    int svr_index = 0;
    uint16_t port[if_index+1];
    uint16_t data_port[if_index+1];    

    for(int i = 0; i < if_index; i++){
        if(get_ipaddress(net_if[i].ifname, &ipaddr[i]) == -1){
            return -1;
        }
        strcpy(ipstr[net_if[i].index], inet_ntoa(ipaddr[i]));
        svr_index ++;
        printf_note("[%d]: ifname:%s, ip=%s\n", svr_index, net_if[i].ifname, ipstr[net_if[i].index]);
        #ifdef SUPPORT_NET_WZ
        if(i == SRV_10G_NET){
            port[i] = poal_config->network_10g.port;
            data_port[i] = poal_config->network_10g.data_port;
            printf_note("10g cmd port = %d, data port=%d\n", port[i], data_port[i]);
        }else
        #endif
        {
            port[i] = poal_config->network.port;
            data_port[i] = poal_config->network.data_port;
            printf_note("cmd port = %d, data port=%d\n", port[i], data_port[i]);
        }        
    }    
#if (defined SUPPORT_PROTOCAL_AKT) 
    struct net_tcp_server *tcpsrv = NULL;
    struct net_udp_server *udpsrv = NULL;
    for(int i = 0; i < svr_index; i++){
        printf_note("akt cmd server init [ipaddr: %s, port:%d]\n", ipstr[i], port[i]);
        tcpsrv = tcp_server_new(ipstr[i], port[i]);
        if (!tcpsrv)
            return -1;
        tcpsrv->on_header = akt_parse_header_v2;
        tcpsrv->on_execute = akt_execute_method;
        tcpsrv->send_error =  akt_send_resp;
        tcpsrv->on_end = akt_parse_end;
        tcpsrv->send_alert = akt_send_alert;
        nserver.cmd[i].tcpsvr = tcpsrv;
        
        printf_note("akt data udp server init [ipaddr: %s, port:%d]\n", ipstr[i], 1234);
        udpsrv = udp_server_new(ipstr[i],  1234);
        if (!udpsrv)
            return -1;
        nserver.data[i].udpsrv = udpsrv;
    }
    udpsrv->on_discovery = akt_parse_discovery;
#elif defined(SUPPORT_PROTOCAL_XW)
    struct uh_server *xsrv = NULL;
    struct net_udp_server *udpsrv = NULL;
    for(int i = 0; i < svr_index; i++){
        printf_note("xw cmd server init [ipaddr: %s, port:%d]\n", ipstr[i], port[i]);
        xsrv = uh_server_new(ipstr[i], port[i]);
        if (!xsrv)
            return -1;
        xsrv->on_accept = on_accept;
        xsrv->on_request = on_request;
        nserver.cmd[i].uhsvr = xsrv;

        printf_note("xw data udp server init [ipaddr: %s, port:%d]\n", ipstr[i], data_port[i]);
        udpsrv = udp_server_new(ipstr[i], data_port[i]);
        if (!udpsrv)
           return -1;
        nserver.data[i].udpsrv = udpsrv;
    }
#endif
#ifdef  SUPPORT_PROTOCAL_HTTP
    struct uh_server *srv = NULL;
    printf_note("http server init[port:%d]\n", 80);
    srv = uh_server_new("0.0.0.0", 80);
    if (!srv)
        return -1;
    srv->on_accept = on_accept;
    srv->on_request = on_request;
    nserver.http_server = srv;
#endif
    return 0;
}


