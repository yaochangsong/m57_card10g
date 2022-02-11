#ifndef _HASH_TABLE_H
#define _HASH_TABLE_H

#include "uthash.h"

#define MAX_RANDOM_ENTRIES  8192

struct hash_entry {
    int key;                    /* key */
    void *data;                 /**<Payload */
    UT_hash_handle hh;         /* makes this structure hashable */
};

struct cache_hash {
    size_t max_entries; /**<Amount of entries this cache object can hold */
    pthread_rwlock_t cache_lock; /**<A lock for concurrent access */
    struct hash_entry *entries; /**<Head pointer for uthash */
    void (*free_cb) (void *element);/**<Callback function to free cache entries */
};


struct ikey_record {
    int key;
    void *value;
};


extern int hash_create(struct cache_hash **dst, const size_t capacity, void (*free_cb) (void *element));
extern int hash_delete(struct cache_hash *cache, int keep_data);
extern int hash_delete_item(struct cache_hash *cache, int key);
extern int hash_delete_key(struct cache_hash *cache, int keep_data);
extern int hash_lookup(struct cache_hash *cache, int key, void *result);
extern int hash_insert(struct cache_hash *cache, int key, void *data);
extern int hash_do_for_each(struct cache_hash *cache,int (*callback) (void *, int, int), void *args);
extern int hash_dump(struct cache_hash *hash, int (*callback) (void *, int));
#endif

