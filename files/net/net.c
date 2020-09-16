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

static struct net_tcp_server *ptcp_data_srv = NULL;
static struct net_tcp_server *ptcp_srv_1gnet = NULL;
static struct net_tcp_server *ptcp_srv_10gnet = NULL;
static struct uh_server *puhttp_srv = NULL;
static struct net_udp_server *pudp_srv = NULL;

void *net_get_tcp_srv_ctx(void)
{
    return (void*)ptcp_srv_1gnet;
}
void *net_get_10g_tcp_srv_ctx(void)
{
    return (void*)ptcp_srv_10gnet;
}
void *net_get_data_srv_ctx(void)
{
    return (void*)ptcp_data_srv;
}

void *net_get_udp_srv_ctx(void)
{
    return (void*)pudp_srv;
}

void *net_get_uhttp_srv_ctx(void)
{
    return (void*)puhttp_srv;
}


static void on_accept(struct uh_client *cl)
{
    printf_info("New connection from: %s:%d\n", cl->get_peer_addr(cl), cl->get_peer_port(cl));
}

#if 0
static int on_request(struct uh_client *cl)
{
    const char *path = cl->get_path(cl);
    int body_len = 0;

    if (strcmp(path, "/hello"))
        return UH_REQUEST_CONTINUE;

    cl->send_header(cl, 200, "OK", -1);
    cl->append_header(cl, "Myheader", "Hello");
    cl->header_end(cl);

    cl->chunk_printf(cl, "<h1>Hello Libuhttpd %s</h1>", UHTTPD_VERSION_STRING);
    cl->chunk_printf(cl, "<h1>REMOTE_ADDR: %s</h1>", cl->get_peer_addr(cl));
    cl->chunk_printf(cl, "<h1>URL: %s</h1>", cl->get_url(cl));
    cl->chunk_printf(cl, "<h1>PATH: %s</h1>", cl->get_path(cl));
    cl->chunk_printf(cl, "<h1>QUERY: %s</h1>", cl->get_query(cl));
    cl->chunk_printf(cl, "<h1>VAR name: %s</h1>", cl->get_var(cl, "name"));
    cl->chunk_printf(cl, "<h1>BODY:%s</h1>", cl->get_body(cl, &body_len));
    cl->request_done(cl);

    return UH_REQUEST_DONE;
}
#endif

static int on_request(struct uh_client *cl)
{
 #if defined(SUPPORT_PROTOCAL_RESETFUL)
    return http_on_request(cl);
 #endif
}



/******************************************************************************
* FUNCTION:
*     server_init
*
* DESCRIPTION:
*     
*     
*
* PARAMETERS
*     not used
*
* RETURNS
*     none
******************************************************************************/

int server_init(void)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);

#if (defined SUPPORT_PROTOCAL_AKT) 
    struct net_tcp_server *tcpsrv = NULL;
    printf_note("tcp server init [port:%d]\n", poal_config->network.port);
    tcpsrv = tcp_server_new("0.0.0.0", poal_config->network.port);
    if (!tcpsrv)
        return -1;
    ptcp_srv_1gnet = tcpsrv;
    #ifdef SUPPORT_NET_WZ
    if(poal_config->network.port != poal_config->network_10g.port){
        struct net_tcp_server *tcpsrv_10g = NULL;
        printf_note("10g tcp server init [port:%d]\n", poal_config->network_10g.port);
        tcpsrv = tcp_server_new("0.0.0.0", poal_config->network_10g.port);
        if (!tcpsrv)
            return -1;
        ptcp_srv_10gnet = tcpsrv;
    }
    #endif
    tcpsrv->on_header = akt_parse_header_v2;
    tcpsrv->on_execute = akt_execute_method;
    tcpsrv->send_error =  akt_send_resp;
    tcpsrv->on_end = akt_parse_end;
   // tcpsrv->read_raw_data = ;
   //read_raw_data_cancel
    //tcpsrv->send =  akt_send_rsp;

    struct net_udp_server *udpsrv = NULL;
    printf_note("udp server init[port:%d]\n", 1234);
    udpsrv = udp_server_new("0.0.0.0",  1234);
    if (!udpsrv)
        return -1;
    pudp_srv = udpsrv;
    udpsrv->on_discovery = akt_parse_discovery;
#endif

    struct net_tcp_server *tcpdatasrv = NULL;
    printf_note("tcp server init [port:%d]\n", 6080);
    tcpdatasrv = tcp_data_server_new("0.0.0.0", 6080);
    if (!tcpdatasrv)
        return -1;
    ptcp_data_srv = tcpdatasrv;


#ifdef  SUPPORT_PROTOCAL_HTTP
    struct uh_server *srv = NULL;
    printf_note("http server init[port:%d]\n", 80);
    srv = uh_server_new("0.0.0.0", 80);
    if (!srv)
        return -1;
    srv->on_accept = on_accept;
    srv->on_request = on_request;
    puhttp_srv = srv;
#endif
    return 0;
}


