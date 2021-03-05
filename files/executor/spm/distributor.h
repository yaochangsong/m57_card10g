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
*  Rev 1.0   1 May. 2021   yaochangsong
*  Initial revision.
******************************************************************************/
#ifndef _DISTRIBUTOR_H
#define _DISTRIBUTOR_H

#define DISTRIBUTOR_NUM MAX_RADIO_CHANNEL_NUM

/* 通道链表节点 */
struct dtr_data_node{
    struct list_head list;  /* 节点链表头指针 */
    void *data;             /* 节点数据 */
    size_t len;             /* 节点数据长度 */
};
/* 数据分发器结构 */
struct distributor{
    struct list_head lists;  /* 通道链表 */
    void *hash;
};

enum dtr_stream_type {
    DTR_STREAM_TYPE_AUDIO = 0,
    DTR_STREAM_TYPE_NIQ,
    DTR_STREAM_TYPE_BIQ,
    DTR_STREAM_TYPE_FFT,
    DTR_STREAM_TYPE_MAX,
};

struct hash_src_table{
    #define _HASH_TYPE_NUM 4
    uint32_t typeid[_HASH_TYPE_NUM];
};

struct hash_type_table{
    int index;
    uint32_t typeid[_HASH_TYPE_NUM];
    void *hash;
    char *name;
};

extern int distributor_main(void);
extern void **distributor_init(void);
extern int distributor_iq(void *iq_data, size_t len, void *args);
extern int distributor_iq_send(void *args, ssize_t len);


#endif
