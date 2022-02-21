#ifndef _NET_SUB_H
#define _NET_SUB_H


#define MAX_RANDOM_ENTRIES  8192

enum route_table_type {
        RT_CHIPID = 0x1,
        RT_FUNCID = 0x2,
        RT_PRIOID = 0x4,
        RT_PORTID = 0x8,
};

struct net_sub_st{
    uint16_t chip_id;
    uint16_t func_id;
    uint16_t prio_id;
    uint16_t port;
    uint16_t rev;
};


void sub_test_main(void);


extern int client_hash_create(struct net_tcp_client *cl);
extern int client_hash_delete(struct net_tcp_client *cl);
extern int client_hash_delete_item(struct net_tcp_client *cl, int key);
extern int client_hash_insert(struct net_tcp_client *cl, int key);
extern int client_hash_dump(struct net_tcp_client *cl);
extern ssize_t client_hash_do_for_each_key(struct net_tcp_client *cl, int key, void *data, void *vec, 
                                                ssize_t (*callback) (void *, void *, void *), void *args);

#endif
