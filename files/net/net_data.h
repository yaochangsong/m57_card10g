#ifndef _NET_DATA_H
#define _NET_DATA_H

extern struct net_tcp_server *tcp_data_server_new(const char *host, int port);
extern struct net_tcp_client *tcp_get_datasrv_client(char *ipaddr);

#endif
