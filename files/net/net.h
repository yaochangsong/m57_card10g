#ifndef __NET_H_H
#define __NET_H_H

enum private_protocal {
    PRIVATE_PRPTOCAL_XNRP,
    PRIVATE_PRPTOCAL_AKT,
};

enum {
    BLK_FILE_READ =1,
    BLK_FILE_DELETE=2,
    BLK_FILE_ADD=3
};

extern void *net_get_tcp_srv_ctx(void);
extern void *net_get_10g_tcp_srv_ctx(void);
extern void *net_get_udp_srv_ctx(void);
extern void *net_get_uhttp_srv_ctx(void);
extern void *net_get_data_srv_ctx(void);
extern int server_init(void);
extern int server_data_init(void);
#endif

