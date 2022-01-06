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

#define HASH_NODE_MAX 8192

pthread_mutex_t hash_table_mutex = PTHREAD_MUTEX_INITIALIZER;

#define LOCK_HASH_TABLE() do { \
    pthread_mutex_lock(&hash_table_mutex); \
} while (0)

#define UNLOCK_HASH_TABLE() do { \
    pthread_mutex_unlock(&hash_table_mutex); \
} while (0)


static inline bool hxstr_to_int(char *str, int *ivalue, bool(*_check)(int))
{
    char *end;
    int value;
    
    if(str == NULL || ivalue == NULL)
        return false;
    
    value = (int) strtol(str, &end, 16);
    if (str == end){
        return false;
    }
    *ivalue = value;
    if(*_check == NULL){
         return true;
    }
       
    return ((*_check)(value));
}

void *net_hash_new(void)
{
    hash_t *hash = hash_new();
    return (void *)hash;
}

void net_hash_dump(hash_t *hash)
{
    int size = hash_size(hash);
    const char *keys[HASH_NODE_MAX];
    void *vals[HASH_NODE_MAX];
    int n = 0;

    LOCK_HASH_TABLE();
    hash_each(hash, {
      keys[n] = key;
      vals[n] = val;
      n++;
      if(HASH_NODE_MAX <= n){
        break;
      }
    });

    size = min(size , HASH_NODE_MAX);
    
    printf("dump:[%d]:\n", size);
    for(int i = 0; i< size; i++)
        printf("%s, %s\n", (char *)keys[i],(char *)vals[i]);
    UNLOCK_HASH_TABLE();
}


void net_hash_add(hash_t *hash, short id, int type)
{
    int h_id = id | (type << 16);
    char key[128], *keydup, val[128], *valdup;
    
    memset(key, 0, sizeof(key));
    memset(val, 0, sizeof(val));
    
    snprintf(key, sizeof(key) - 1, "%x", h_id);
    snprintf(val, sizeof(val) - 1, "%x", id);
    keydup = strdup(key);
    valdup = strdup(val);
    LOCK_HASH_TABLE();
    hash_set(hash, keydup, valdup);
    UNLOCK_HASH_TABLE();
}

void net_hash_add_ex(hash_t *hash, int id)
{
    int h_id = id;
    char key[128], *keydup, val[128], *valdup;
    
    memset(key, 0, sizeof(key));
    memset(val, 0, sizeof(val));
    
    snprintf(key, sizeof(key) - 1, "%x", h_id);
    snprintf(val, sizeof(val) - 1, "%x", id);
    keydup = strdup(key);
    valdup = strdup(val);
    LOCK_HASH_TABLE();
    hash_set(hash, keydup, valdup);
    UNLOCK_HASH_TABLE();
}


void net_hash_del_ex(hash_t *hash, int id)
{
    int h_id = id;
    const char *keys[HASH_NODE_MAX];
    void *vals[HASH_NODE_MAX];
    int n = 0;
    char tmp[128], *key_tmp = NULL, *val_tmp = NULL;

    memset(tmp, 0 ,sizeof(tmp));
    snprintf(tmp, sizeof(tmp) - 1, "%x", h_id);

    if(hash_size(hash) > HASH_NODE_MAX){
        printf_err("hash_size %d is bigger than %d\n", hash_size(hash), HASH_NODE_MAX);
        return;
    }
    LOCK_HASH_TABLE();
    hash_each(hash, {
      keys[n] = key;
      vals[n] = val;
      n++;
    });

    for(int i = 0; i < hash_size(hash); i++){
        if(!strcmp(keys[i], tmp)){
            key_tmp = keys[i];
            val_tmp = vals[i];
            break;
        }
    }
    hash_del(hash, tmp);
    _safe_free_(key_tmp);
    _safe_free_(val_tmp);
    UNLOCK_HASH_TABLE();
}


static inline int _get_type_by_key(char *key)
{
    int type = 0, ikey = 0;
    
    if(hxstr_to_int(key, &ikey, NULL) == false)
        return -1;
    
    type = (ikey >> 16)& 0x0ff;
    
    return type;
}
void net_hash_del(hash_t *hash, short id, int type)
{
    int h_id = id | (type << 16);
    const char *keys[HASH_NODE_MAX];
    void *vals[HASH_NODE_MAX];
    int n = 0;
    char tmp[128], *key_tmp = NULL, *val_tmp = NULL;

    memset(tmp, 0 ,sizeof(tmp));
    snprintf(tmp, sizeof(tmp) - 1, "%x", h_id);

    if(hash_size(hash) > HASH_NODE_MAX){
        printf_err("hash_size %d is bigger than %d\n", hash_size(hash), HASH_NODE_MAX);
        return;
    }
    LOCK_HASH_TABLE();
    hash_each(hash, {
      keys[n] = key;
      vals[n] = val;
      n++;
    });

    for(int i = 0; i < hash_size(hash); i++){
        if(!strcmp(keys[i], tmp)){
            key_tmp = keys[i];
            val_tmp = vals[i];
            break;
        }
    }
    hash_del(hash, tmp);
    _safe_free_(key_tmp);
    _safe_free_(val_tmp);
    UNLOCK_HASH_TABLE();
}


bool net_hash_find(hash_t *hash, short id, int type)
{
    void *value = NULL;
    int h_id = id | (type << 16);
    char key[128], val[128];
    snprintf(key, sizeof(key) - 1, "%x", h_id);
    snprintf(val, sizeof(val) - 1, "%x", id);

    value = hash_get(hash, key);
    if(value == NULL){
       // printf_note("not find id:%x in hash table\n", id);
        return false;
    }
    if(value && !strcmp(value, val)){
       // printf_note("YES find id:%x, %x\n", id, type);
        return true;
    }

    return false;
}


void net_hash_find_type_set(hash_t *hash, int type, int (*callback) (int ))
{
    const char *keys[HASH_NODE_MAX];
    void *vals[HASH_NODE_MAX];
    int n = 0;
    int ival = 0;

    if(hash_size(hash) > HASH_NODE_MAX){
        printf_err("hash_size %d is bigger than %d\n", hash_size(hash), HASH_NODE_MAX);
        return;
    }
    
    hash_each(hash, {
      keys[n] = key;
      vals[n] = val;
      n++;
    });
    vals[0] = vals[0];  /*  warn */
    for(int i = 0; i < hash_size(hash); i++){
        if(hxstr_to_int(keys[i], &ival, NULL) == false)
            continue;
        printf_note("ival = 0x%x, type=0x%x, %x, %x, %x\n", ival, type, ival&0x0f0000 & (type << 16), ival&0x0f0000 , (type << 16));
        if((ival&0x0f0000) & (type << 16)){
            if(callback)
                callback(ival);
        }
    }
}


void net_hash_for_each(hash_t *hash, int (*callback) (void *, int, int), void *args)
{
    const char *keys[HASH_NODE_MAX];
    void *vals[HASH_NODE_MAX];
    int n = 0;
    int ival[HASH_NODE_MAX];
    int prio = 0, hashsize = 0;

    if(hash_size(hash) > HASH_NODE_MAX){
        printf_err("hash_size %d is bigger than %d\n", hash_size(hash), HASH_NODE_MAX);
        return;
    }

    LOCK_HASH_TABLE();
    hash_each(hash, {
      keys[n] = key;
      vals[n] = val;
      n++;
    });
    keys[0] = keys[0];  /* for warning */
    hashsize = hash_size(hash);
    UNLOCK_HASH_TABLE();
    for(int i = 0; i < hashsize; i++){
        if(hxstr_to_int(vals[i], (int *)&ival[i], NULL) == false)
            continue;
       // printf("hash val: %d\n", ival[i]);
        if(callback)
            callback(args, ival[i], prio);
    }
}

