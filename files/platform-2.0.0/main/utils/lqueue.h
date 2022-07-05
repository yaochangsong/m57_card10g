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

struct tailq_data_s {
    void *args;
//    pthread_mutex_t lock;
    TAILQ_ENTRY(tailq_data_s) next;
};

typedef struct queue_ctx_s {
    TAILQ_HEAD(ltailq_head, tailq_data_s) list_head;
    struct lqueue_ops *ops;
    pthread_mutex_t lock;
}queue_ctx_t;


struct lqueue_ops {
    int (*create)(void );
    int (*push)(queue_ctx_t *, void *);
    void *(*pop)(queue_ctx_t *);
    void (*foreach)(queue_ctx_t *, int (*func)(void *));
    int (*close)(void);
};

extern queue_ctx_t * queue_create_ctx(void);

#endif

