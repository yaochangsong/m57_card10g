#ifndef _NET_SUB_H
#define _NET_SUB_H
#include "../lib/hash/hash.h"

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
extern void *net_hash_new(void);
extern void net_hash_dump(hash_t *hash);
extern void net_hash_add(hash_t *hash, short id, int type);
extern void net_hash_del(hash_t *hash, short id, int type);
extern bool net_hash_find(hash_t *hash, short id, int type);
extern void net_hash_find_type_set(hash_t *hash, int type, int (*callback) (int ));
extern void net_hash_for_each(hash_t *hash, int (*callback) (void *, int, int), void *args);
#endif