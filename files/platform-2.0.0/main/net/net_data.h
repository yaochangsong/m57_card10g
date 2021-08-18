#ifndef _NET_DATA_H
#define _NET_DATA_H

extern struct net_tcp_server *tcp_data_server_new(const char *host, int port);
extern int tcp_send_vec_data(struct iovec *iov, int iov_len, int type);
extern void *get_data_server(int index, int type);
extern int tcp_send_data(void *data, int len, int type);
extern void net_data_add_client(struct sockaddr_in *addr, int ch, int type);
extern void net_data_remove_client(struct sockaddr_in *addr, int ch, int type);

#endif
