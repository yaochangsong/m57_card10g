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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <poll.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <poll.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <semaphore.h>
#include <net/if.h>
#include <sys/stat.h>
#include <termios.h>
#include <errno.h>
#include <linux/spi/spidev.h>
#include <time.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <sys/mman.h>
#include <mqueue.h>
#include <assert.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/limits.h>
#include <stddef.h>
#include <stdbool.h>
#include "queue.h"
#include "lqueue.h"

#ifndef container_of
#define container_of(ptr, type, member)                                 \
         ({                                                              \
                 const __typeof__(((type *) NULL)->member) *__mptr = (ptr);      \
                 (type *) ((char *) __mptr - offsetof(type, member));    \
         })
 #endif 


int _queue_create(void)
{
    return 0;
}

int _queue_push(queue_ctx_t *q, void *args)
{
    struct tailq_data_s *data = (struct tailq_data_s *)calloc(1, sizeof(*data));
    data->args = args;
    TAILQ_INSERT_HEAD(&q->list_head, data, next);
    return 0;
}

struct lqueue_ops;


void *_queue_pop(queue_ctx_t *q)
{
    struct tailq_data_s *data = NULL;
    void *args = NULL;
    if (!TAILQ_EMPTY(&q->list_head)) {
        data = TAILQ_LAST(&q->list_head, ltailq_head);
        args = data->args;
        TAILQ_REMOVE(&q->list_head, data, next);
        free(data);
        data = NULL;
    }
    return args;
}

void _queue_foreach(queue_ctx_t *q, int (*func)(void *))
{
    struct tailq_data_s *data = NULL;
    TAILQ_FOREACH(data, &q->list_head, next) {
        func(data->args);
    }
}



int _queue_close(void)
{
    return 0;
}



static  struct lqueue_ops queue_ops = {
    .create = _queue_create,
    .push = _queue_push,
    .pop = _queue_pop,
    .foreach = _queue_foreach,
    .close = _queue_close,
};


queue_ctx_t * queue_create_ctx(void)
{
    int ret = -ENOMEM;
    unsigned int len;
    queue_ctx_t *ctx = calloc(1,sizeof(*ctx));
    if (!ctx)
        goto err_set_errno;
    
    TAILQ_INIT(&ctx->list_head);

    ctx->ops = &queue_ops;
    ctx->ops->create();

err_set_errno:
    errno = -ret;
    return ctx;

}



