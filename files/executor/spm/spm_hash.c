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
#include "config.h"
#include "spm_hash.h"

struct _hash_data {
    struct iovec data[MAX_SPM_HASH_DATA];
    int cnt;
    int max_cnt;
};

static void free_entry(void *entry)
{
    struct ikey_record *record = entry;
    if (!record)
        return;
    
    if (record->value)
        free(record->value);
    free(record);
    record = NULL;
}


int spm_hash_create(void **hash)
{
    if(unlikely(hash == NULL))
        return -1;
    
    return hash_create((struct cache_hash **)hash, MAX_SPM_RANDOM_ENTRIES, free_entry);
}

int spm_hash_delete(void *hash)
{
    if(unlikely(hash == NULL))
        return -1;

    return hash_delete_key((struct cache_hash *)hash, 0);
}

int spm_hash_delete_item(void *hash, int key)
{
    if(unlikely(hash == NULL))
        return -1;
    
    printf_note("key:0x%x[%d]\n", key, key);
    return hash_delete_item((struct cache_hash *)hash, key);
}



int spm_hash_insert(void *hash, int key, struct iovec *data)
{
    struct ikey_record *entry = NULL;
    struct _hash_data *new;
    int rv = 0;
    
    if(unlikely(hash == NULL || data == NULL))
        return -1;

    if ((entry = malloc(sizeof(*entry))) == NULL) {
        return -ENOMEM;
    }
    memset(entry, 0, sizeof(*entry));

    if ((new = malloc(sizeof(*new))) == NULL) {
        free(entry);
        entry = NULL;
        return -ENOMEM;
    }
    memset(new, 0, sizeof(*new));

    memcpy(&new->data[new->cnt], data, sizeof(*data));
    new->cnt = 1;
    new->max_cnt = MAX_SPM_HASH_DATA;

    entry->key = key;
    entry->value = new;
    rv = hash_insert((struct cache_hash *)hash, entry->key, entry);
    if(rv != 0){
        free(entry);
        entry = NULL;
        free(new);
        new = NULL;
    }
    
    return rv;
}

int spm_hash_renew(void *hash, int key, struct iovec *data)
{
    struct ikey_record *r = NULL;
    struct _hash_data *d;
    if(unlikely(hash == NULL || data == NULL)){
        printf_err("null\n");
        return -1;
    }

    hash_lookup((struct cache_hash *)hash, key, (void *)&r);
    if(r != NULL){
        printf_note("Find it\n");
    } else{
        printf_note("Not find key: %x[%d], insert\n", key, key);
        if(spm_hash_insert(hash, key, data) == 0)   //insert OK
            return 1;
        else
            return -1;
    }
    d = (struct _hash_data *)r->value;

    if(d->cnt >= d->max_cnt){
        printf_warn("vec count is full:%d, max:%d\n", d->cnt, d->max_cnt);
        return -1;
    }
        
    memcpy(&d->data[d->cnt], data, sizeof(*data));
    d->cnt ++;
    
    return d->cnt;
}

ssize_t spm_hash_do_for_each_vec(void *hash, int key, ssize_t (*callback) (void *, void *), void *args)
{
    struct ikey_record *r = NULL;
    struct _hash_data *d;
    ssize_t rv = 0;
    
    if(unlikely(hash == NULL)){
        return -1;
    }
    hash_lookup((struct cache_hash *)hash, key, (void *)&r);
    if(r != NULL){
        printf_note("Find it\n");
    } else{
        printf_note("Not find key: %x[%d]\n", key, key);
        return -1;
    }
    
    d = (struct _hash_data *)r->value;
    for(int i = 0; i < d->cnt; i++){
        if(callback)
            rv += callback(args, &d->data[i]);
    }
    return rv;
}

void spm_hash_clear_vec_data(void *hash)
{
    if(unlikely(hash == NULL)){
        return;
    }

    struct cache_hash *_hash = (struct cache_hash *)hash;
    struct ikey_record *r = NULL;
    struct _hash_data *d;
    struct hash_entry *entry = _hash->entries;
    r = entry->data;
    d = r->value;
    printf_note("key: %d, vec cnt: %d\n", entry->key, d->cnt);
    for(int i = 0; i < d->cnt; i++){
        memset(&d->data[i], 0, sizeof(struct iovec));
    }
    d->cnt = 0;
}

static int dump_entry(void *args, int key)
{
    struct hash_entry *entry = args;
    struct ikey_record *r = entry->data;
    struct _hash_data *d = (struct _hash_data *)r->value;
    
    printf("key: 0x%x[%d]\n", r->key, r->key);
    for(int i = 0; i < d->cnt; i++)
        printf("iov_base:%p, iov_len=%ld\n", d->data[i].iov_base, d->data[i].iov_len);
    return 0;
}

int spm_hash_dump(void *hash)
{
    if(unlikely(hash == NULL))
        return -1;

    return hash_dump((struct cache_hash *)hash, dump_entry);
}


volatile bool notify_over =false;
static void *_producer(void *arg)
{
    struct cache_hash *hash = arg;
    struct iovec data;
    int key;
    intptr_t base = 0x1000;
    size_t len = 0;


    for(int i = 0; i < 1; i++){
        for(len = 0; len < 1; len++){
            key = GET_HASHMAP_ID(0x0502, 0x32, 1, 1);
            data.iov_base = (void *)base ++;
            data.iov_len = len;
            spm_hash_renew(hash, key, &data);
            printf_note("------spm hash dump-----------\n");
            spm_hash_dump(hash);
            usleep(1000);
        }
        notify_over = true;
        usleep(10000);
    }

    pthread_exit(NULL);
}

static ssize_t _do_consumer(void *ptr, void *args)
{
    struct iovec *vec = (struct iovec *)args;
    printf_note("<<>>iov_base=%p, iov_len=%zu\n", vec->iov_base, vec->iov_len);
    return vec->iov_len;
}

static int  do_consumer(void *args, int hid, int prio)
{
    ssize_t r = 0;
    r = spm_hash_do_for_each_vec(args, hid, _do_consumer, NULL);
    printf_note("hid[%d] consume %d\n", hid, r);
    return r;
}


static void *_consumer(void *arg)
{
    struct net_tcp_client cl;
    int key;
    
    client_hash_create(&cl);
    key = GET_HASHMAP_ID(0x0502, 0x32, 1, 1);
    client_hash_insert(&cl, key);
    printf_note("------client hash dump-----------%p\n", cl.section.hash);
    client_hash_dump(&cl);
    while(1){
        if(notify_over){
            printf_note("Start consume\n");
            notify_over = false;
            hash_do_for_each(cl.section.hash, do_consumer, arg);
        }
        usleep(1000);
    }

    pthread_exit(NULL);
}


void spm_thread_test(void)
{
    int rv;
    void *hash = NULL;
    pthread_t workers[2];
    
    spm_hash_create(&hash);
    (void)pthread_create(&workers[0], NULL, _producer, (void *)hash);
    (void)pthread_create(&workers[1], NULL, _consumer, (void *)hash);
    pthread_join(workers[0], NULL);
    pthread_join(workers[1], NULL);
}


void spm_test_main(void)
{
    void *hash = NULL;
    int key;
    struct iovec data;
    int r = 0;
    spm_thread_test();
    return;

    spm_hash_create(&hash);

    key = GET_HASHMAP_ID(0x0502, 0x32, 1, 1);
    data.iov_base = (void *)0xf000;
    data.iov_len = 102;
    r= spm_hash_renew(hash, key, &data);
    printf_note("r:%d\n", r);
    key = GET_HASHMAP_ID(0x0502, 0x32, 1, 1);
    data.iov_base = (void *)0xf010;
    data.iov_len = 100;
    r = spm_hash_renew(hash, key, &data);
    printf_note("r:%d\n", r);
    
    key = GET_HASHMAP_ID(0x0502, 0x32, 1, 1);
    data.iov_base = (void *)0xf000;
    data.iov_len = 1033;
    r = spm_hash_renew(hash, key, &data);
    printf_note("r:%d\n", r);
    
    spm_hash_dump(hash);
    spm_hash_delete_item(hash, key);
    spm_hash_dump(hash);
    spm_hash_delete(hash);
    
}


