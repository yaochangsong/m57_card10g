/******************************************************************************
*  Copyright 2019, Showay Technology Dev Co.,Ltd.
*  ---------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Showay Technology Dev Co.,Ltd. (C) 2019
******************************************************************************/
/*****************************************************************************     
*  Rev 1.0   2 July. 2022   yaochangsong
*  Initial revision.
******************************************************************************/
#ifndef _LQUEUE_H_H
#define _LQUEUE_H_H

struct lqueue_ops;

#define LQUEUE_MAX_ENTRY 65535
struct tailq_data_s {
    void *args;
    TAILQ_ENTRY(tailq_data_s) next;
};

typedef struct queue_ctx_s {
    TAILQ_HEAD(ltailq_head, tailq_data_s) list_head;
    struct lqueue_ops *ops;
    pthread_mutex_t lock;
    uint32_t max_entry;
    uint32_t entries;
}queue_ctx_t;


struct lqueue_ops {
    int (*create)(void );
    int (*push)(queue_ctx_t *, void *);
    void *(*pop)(queue_ctx_t *);
    void *(*pop_head)(queue_ctx_t *);
    void (*foreach)(queue_ctx_t *, int (*func)(void *));
    uint32_t (*get_entry)(queue_ctx_t *);
    int (*clear)(queue_ctx_t *);
    bool (*is_empty)(queue_ctx_t *);
    int (*close)(void);
};

extern queue_ctx_t * queue_create_ctx(void);

#endif

