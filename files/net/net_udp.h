#ifndef _NET_UDP_H_H
#define _NET_UDP_H_H

struct net_udp_client {
    struct list_head list;
    struct net_udp_server *srv;

    struct sockaddr_in peer_addr;
    struct sockaddr_in discover_peer_addr;  /* add by ycs for act */
    bool connection_close;
    int response_length;
    int tag;
    int ch; /* channel */
    char *ifname;
    void (*free)(struct net_udp_client *cl);
    void (*send_error)(struct net_udp_client *cl, int code, const char *summary, const char *fmt, ...);
    void (*send)(struct net_udp_client *cl, const void *data, int len);
    const char *(*get_version)(struct net_udp_client *cl);
    const char *(*get_peer_addr)(struct net_udp_client *cl);
    int (*get_peer_port)(struct net_udp_client *cl);
};


struct net_udp_server {
    struct uloop_fd fd;
    struct list_head clients;
    int nclients;
    void (*free)(struct net_udp_server *srv);
    void (*on_accept)(struct net_udp_client *cl);
    int (*on_request)(struct net_udp_client *cl);
};

extern struct net_udp_server *udp_server_new(const char *host, int port);
extern int udp_send_data(uint8_t  *data, uint32_t data_len);
extern int udp_send_data_to_client(struct net_udp_client *client, uint8_t *data, uint32_t data_len);
extern void udp_add_client_to_list(struct sockaddr_in *addr, int ch);
extern struct net_udp_server *get_udp_server(void);
extern int udp_send_vec_data(struct iovec *iov, int iov_len);
extern int udp_send_vec_data_to_taget_addr(struct iovec *iov, int iov_len);


#endif

