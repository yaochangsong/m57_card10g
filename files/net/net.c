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
    return http_on_request(cl);
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

#if (defined SUPPORT_PROTOCAL_AKT) || (defined SUPPORT_PROTOCAL_XNRP) 
    struct net_tcp_server *tcpsrv = NULL;
    printf_info("tcp server init [port:%d]\n", poal_config->network.port);
    tcpsrv = tcp_server_new("0.0.0.0", poal_config->network.port);
    if (!tcpsrv)
        return -1;

    struct net_udp_server *udpsrv = NULL;
    printf_info("udp server init[port:%d]\n", poal_config->network.port);
    udpsrv = udp_server_new("0.0.0.0",  poal_config->network.port);
    if (!udpsrv)
        return -1;
#endif
#ifdef  SUPPORT_PROTOCAL_HTTP
    struct uh_server *srv = NULL;
    printf_info("http server init[port:%d]\n", 9999);
    srv = uh_server_new("0.0.0.0", 9999);
    if (!srv)
        return -1;
    srv->on_accept = on_accept;
    srv->on_request = on_request;
    
#endif
    return 0;
}


