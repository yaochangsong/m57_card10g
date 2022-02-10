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
*  Rev 1.0   29 Mar 2021  yaochangsong
*  Initial revision.
******************************************************************************/
#include "config.h"
#include "net_sub.h"


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


int client_hash_create(struct net_tcp_client *cl)
{
    if(unlikely(cl == NULL))
        return -1;
    
    return hash_create((struct cache_hash **)&cl->section.hash, MAX_RANDOM_ENTRIES, free_entry);
}

int client_hash_delete(struct net_tcp_client *cl)
{
    if(unlikely(cl == NULL))
        return -1;

    return hash_delete((struct cache_hash *)cl->section.hash, 0);
}

int client_hash_delete_item(struct net_tcp_client *cl, int key)
{
    if(unlikely(cl == NULL))
        return -1;
    printf_note("key:0x%x[%d]\n", key, key);
    return hash_delete_item((struct cache_hash *)cl->section.hash, key);
}



int _client_hash_insert(struct net_tcp_client *cl, struct ikey_record *entry)
{
    if(unlikely(cl == NULL))
        return -1;

    hash_insert((struct cache_hash *)cl->section.hash, entry->key, entry);
    return 0;
}


int client_hash_insert(struct net_tcp_client *cl, int key)
{
    struct ikey_record *entry = NULL;
    struct net_sub_st *new;
    
    if(unlikely(cl == NULL))
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

    new->chip_id = GET_SLOT_CHIP_ID_BY_HASHID(key);
    new->func_id = GET_FUNCID_BY_HASHID(key);
    new->port = GET_PORTID_BY_HASHID(key);
    new->prio_id = GET_PROIID_BY_HASHID(key);

    entry->key = key;
    entry->value = new;
    _client_hash_insert(cl, entry);
    return 0;
}



static int dump_entry(void *args, int key)
{
    struct hash_entry *entry = args;
    struct ikey_record *r = entry->data;
    struct net_sub_st *d = (struct net_sub_st *)r->value;
    
    printf("key: 0x%x[%d]\n", r->key, r->key);
    printf("chipid:%x, funcid:%x,port:%x,prio:%x\n", d->chip_id, d->func_id, d->port, d->prio_id);
    return 0;
}

int client_hash_dump(struct net_tcp_client *cl)
{
    if(unlikely(cl == NULL))
        return -1;

    return hash_dump((struct cache_hash *)cl->section.hash, dump_entry);
}



void sub_test_main(void)
{
    struct net_tcp_client cl;
    int key;

    client_hash_create(&cl);
    key = GET_HASHMAP_ID(0x0502, 0x32, 1, 1);
    client_hash_insert(&cl, key);
    key = GET_HASHMAP_ID(0x0501, 0x32, 1, 1);
    client_hash_insert(&cl, key);
    key = GET_HASHMAP_ID(0x0502, 0x3, 1, 1);
    client_hash_insert(&cl, key);
    client_hash_dump(&cl);
    key = GET_HASHMAP_ID(0x0501, 0x32, 1, 1);
    client_hash_delete_item(&cl, key);
    client_hash_dump(&cl);
    client_hash_delete(&cl);
    exit(0);
}

