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

    return hash_delete((struct cache_hash *)hash, 0);
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
    
    if(unlikely(hash == NULL || data == NULL))
        return -1;

    if ((entry = malloc(sizeof(*entry))) == NULL) {
        return ENOMEM;
    }
    memset(entry, 0, sizeof(*entry));

    if ((new = malloc(sizeof(*new))) == NULL) {
        free(entry);
        entry = NULL;
        return ENOMEM;
    }
    memset(new, 0, sizeof(*new));

    memcpy(&new->data[new->cnt], data, sizeof(*data));
    new->cnt = 1;

    entry->key = key;
    entry->value = new;
    
    return hash_insert((struct cache_hash *)hash, entry->key, entry);
}

int spm_hash_renew(void *hash, int key, struct iovec *data)
{
    struct ikey_record *r = NULL;
    struct _hash_data *d;
    if(unlikely(hash == NULL || data == NULL))
        return -1;

    hash_lookup((struct cache_hash *)hash, key, (void *)&r);
    if(r != NULL){
        printf_note("Find it\n");
    } else{
        printf_note("Not find key: %x[%d], insert\n", key, key);
        return  spm_hash_insert(hash, key, data);
    }
    d = (struct _hash_data *)r->value;

    printf_note("vec cnt： %d\n", d->cnt);
    memcpy(&d->data[d->cnt], data, sizeof(*data));
    d->cnt ++;
    
    return 0;
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



void spm_test_main(void)
{
    void *hash = NULL;
    int key;
    struct iovec data;
    int r = 0;

    spm_hash_create(&hash);

    key = GET_HASHMAP_ID(0x0502, 0x32, 1, 1);
    data.iov_base = (void *)0xf000;
    data.iov_len = 102;
    r= spm_hash_renew(hash, key, &data);
    printf_note("r:%d\n", r);
    key = GET_HASHMAP_ID(0x0502, 0x12, 1, 1);
    data.iov_base = (void *)0xf010;
    data.iov_len = 100;
    r = spm_hash_renew(hash, key, &data);
    printf_note("r:%d\n", r);
    
    key = GET_HASHMAP_ID(0x0502, 0x31, 1, 1);
    data.iov_base = (void *)0xf000;
    data.iov_len = 1033;
    r = spm_hash_renew(hash, key, &data);
    printf_note("r:%d\n", r);
    
    spm_hash_dump(hash);
    spm_hash_delete_item(hash, key);
    spm_hash_dump(hash);
    spm_hash_delete(hash);
    exit(0);
}


