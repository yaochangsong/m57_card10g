/******************************************************************************
*  Copyright 2021, Showay Technology Dev Co.,Ltd.
*  ---------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Showay Technology Dev Co.,Ltd. (C) 2019
******************************************************************************/
/*****************************************************************************     
*  Rev 1.0   10 Feb 2022  yaochangsong
*  Initial revision.
******************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include "hash.h"
#include "uthash.h"


/** Creates a new cache object

    @param dst
    Where the newly allocated cache object will be stored in

    @param capacity
    The maximum number of elements this cache object can hold

    @return EINVAL if dst is NULL, ENOMEM if malloc fails, 0 otherwise
*/
int hash_create(struct cache_hash **dst, const size_t capacity, 
                void (*free_cb) (void *element))
{
    struct cache_hash *new = NULL;
    int rv = 0;

    if (!dst)
        return -EINVAL;

    if ((new = malloc(sizeof(*new))) == NULL)
        return -ENOMEM;

    memset(new, 0, sizeof(*new));

    if ((rv = pthread_rwlock_init(&(new->cache_lock), NULL)) != 0)
        goto err_out;

    new->max_entries = capacity;
    new->entries = NULL;
    new->free_cb = free_cb;
    *dst = new;
    return 0;

err_out:
    if (new)
        free(new);
    return rv;
}

/** Frees an allocated cache object

    @param cache
    The cache object to free

    @param keep_data
    Whether to free contained data or just delete references to it

    @return EINVAL if cache is NULL, 0 otherwise
*/
int hash_delete(struct cache_hash *cache, int keep_data)
{
    struct hash_entry *entry, *tmp;
    int rv = 0;

    if (!cache)
        return -EINVAL;

    rv = pthread_rwlock_wrlock(&(cache->cache_lock));
    if (rv)
        return rv;

    if (keep_data) {
        HASH_CLEAR(hh, cache->entries);
    } else {
        HASH_ITER(hh, cache->entries, entry, tmp) {
            HASH_DEL(cache->entries, entry);
            if (cache->free_cb)
                cache->free_cb(entry->data);
            free(entry);
        }
    }
    (void)pthread_rwlock_unlock(&(cache->cache_lock));
    (void)pthread_rwlock_destroy(&(cache->cache_lock));
    free(cache);
    cache = NULL;
    return 0;
}

int hash_delete_key(struct cache_hash *cache, int keep_data)
{
    struct hash_entry *entry, *tmp;
    int rv = 0;

    if (!cache)
        return -EINVAL;

    rv = pthread_rwlock_wrlock(&(cache->cache_lock));
    if (rv)
        return rv;

    if (keep_data) {
        HASH_CLEAR(hh, cache->entries);
    } else {
        HASH_ITER(hh, cache->entries, entry, tmp) {
            HASH_DEL(cache->entries, entry);
            if (cache->free_cb)
                cache->free_cb(entry->data);
            free(entry);
        }
    }
    (void)pthread_rwlock_unlock(&(cache->cache_lock));

    return 0;
}


int hash_delete_item(struct cache_hash *cache, int key)
{
    struct hash_entry *entry, *tmp;
    int rv = 0;

    if (!cache)
        return -EINVAL;

    rv = pthread_rwlock_wrlock(&(cache->cache_lock));
    if (rv)
        return rv;

    HASH_ITER(hh, cache->entries, entry, tmp) {
        if(key == entry->key){
            if (cache->free_cb)
                cache->free_cb(entry->data);
            HASH_DEL(cache->entries, entry);
            free(entry);
        }
    }
    (void)pthread_rwlock_unlock(&(cache->cache_lock));
    return 0;
}

/** Checks if a given key is in the hash table

    @param cache
    The cache object

    @param key
    The key to look-up

    @param result
    Where to store the result if key is found.

    A warning: Even though result is just a pointer,
    you have to call this function with a **ptr,
    otherwise this will blow up in your face.

    @return EINVAL if cache is NULL, 0 otherwise
*/
int hash_lookup(struct cache_hash *cache, int key, void *result)
{
    int rv = 0;
    struct hash_entry *tmp = NULL;
    void **dirty_hack = result;

    if (!cache || !key)
        return -EINVAL;

    rv = pthread_rwlock_wrlock(&(cache->cache_lock));
    if (rv)
        return rv;

    HASH_FIND_INT(cache->entries, &key, tmp);  /* s: output pointer */
    if (tmp) {
        *dirty_hack = tmp->data;
    } else {
        *dirty_hack = result = NULL;
    }
    rv = pthread_rwlock_unlock(&(cache->cache_lock));
    return rv;
}

/** Inserts a given <key, value> pair into the cache

    @param cache
    The cache object

    @param key
    The key that identifies <value>

    @param data
    Data associated with <key>

    @return EINVAL if cache is NULL, ENOMEM if malloc fails, 0 otherwise
*/
int hash_insert(struct cache_hash *cache, int key, void *data)
{
    struct hash_entry *entry = NULL;
    struct hash_entry *tmp_entry = NULL;
    int rv = 0;

    if (!cache || !data)
        return -EINVAL;

    if ((rv = pthread_rwlock_wrlock(&(cache->cache_lock))) != 0)
        goto err_out;

    if (HASH_COUNT(cache->entries) >= cache->max_entries){
        printf("The hash table size [%u] is full\n", HASH_COUNT(cache->entries));
        rv = -1;
        goto err_out;
    }
    
    HASH_FIND_INT(cache->entries, &key, entry);  /* id already in the hash? */
    if (entry == NULL) {
        if ((entry = malloc(sizeof(*entry))) == NULL)
            goto err_out;
        memset(entry, 0, sizeof(*entry));
        entry->key = key;
        entry->data = data;
        HASH_ADD_INT(cache->entries, key, entry);  /* id is the key field */
    }else{
        printf("key:%d has already in hash table\n", key);
        rv = -1;
    }

err_out:
    pthread_rwlock_unlock(&(cache->cache_lock));
    return rv;
}

int hash_do_for_each(struct cache_hash *cache,int (*callback) (void *, int, int), void *args)
{
    struct hash_entry *entry;
    struct hash_entry *tmp;
    int rv = 0;

    if (!cache)
        return -EINVAL;

    rv = pthread_rwlock_wrlock(&(cache->cache_lock));
    if (rv)
        return rv;
    
    HASH_ITER(hh, cache->entries, entry, tmp) {
        if(callback)
            callback(args, entry->key, 0);
    }
    
    rv = pthread_rwlock_unlock(&(cache->cache_lock));
    return rv;

}

int hash_dump(struct cache_hash *hash, int (*callback) (void *, int))
{
    struct hash_entry *entry;
    struct hash_entry *tmp;
    int rv = 0;

    if (!hash)
        return -EINVAL;

    rv = pthread_rwlock_wrlock(&(hash->cache_lock));
    if (rv)
        return rv;

    printf("-------HASH Dump Start----\n");
    HASH_ITER(hh, hash->entries, entry, tmp) {
        if(callback)
            callback(entry, entry->key);
    }
    printf("-------HASH Dump End-----\n");
    
    rv = pthread_rwlock_unlock(&(hash->cache_lock));
    return rv;

}

