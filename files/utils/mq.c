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
*  Rev 1.0   21 Feb. 2020   yaochangsong
*  Initial revision.
******************************************************************************/
#include <mqueue.h>
#include "config.h"
#include "mq.h"


/* example. qname :/mq123 */
static inline mqd_t _mq_create(char *qname, struct mq_attr *mattr, int oflag)
{
    struct mq_attr attr;
    int o_flags;
    if(mattr == NULL){
        attr.mq_flags = 0;
        attr.mq_maxmsg = 1;
        attr.mq_msgsize = 1024;
        attr.mq_curmsgs = 0;
    }else{
        attr.mq_flags = 0;
        attr.mq_maxmsg = mattr->mq_maxmsg;
        attr.mq_msgsize = mattr->mq_msgsize;
        attr.mq_curmsgs = 0;
    }
    if(oflag == -1){
        o_flags = O_CREAT|O_RDWR|O_EXCL|O_NONBLOCK;
    }else{
        o_flags = oflag;
    }


    mode_t mode = 0644;
    mqd_t queue = mq_open(qname, o_flags, mode, &attr);

    if (-1 == queue) {
        printf_note("mq_open error: %s\n", strerror(errno));
        return -1;
    }
    return queue;
}

static inline struct mq_attr *_mq_getattr(char *qname)
{
    static struct mq_attr attr;
    int ret;
    
    mqd_t queue = mq_open(qname, O_RDONLY);
    if (-1 == queue) {
        printf_err("mq_open error: %s", strerror(errno));
        return NULL;
    }
    ret = mq_getattr(queue, &attr);
    if (0 != ret) {
        mq_close(queue);
        printf_err("mq_getattr error: %s", strerror(errno));
        return NULL;
    } else {
        printf_note("%s: maxmsg=%ld, msgsize=%ld, curmsgs=%ld\n",
           qname, attr.mq_maxmsg, attr.mq_msgsize, attr.mq_curmsgs);
    }

    mq_close(queue);
    return &attr;
}

static inline mqd_t _mq_open_ro(char *qname)
{
    mqd_t queue;
    queue = mq_open(qname, O_RDONLY|O_NONBLOCK);
    if (-1 == queue) 
        printf_err("mq_open error: %s", strerror(errno));

    return queue;
}

static inline mqd_t _mq_open_wo(char *qname)
{
    mqd_t queue;
    queue = mq_open(qname, O_WRONLY|O_NONBLOCK);
    if (-1 == queue) 
        printf_err("mq_open error: %s", strerror(errno));

    return queue;
}


static inline int _mq_send(mqd_t queue, void *message, size_t msglen)
{
    /* Send */
    int ret = mq_send(queue, message, msglen, 0);
    if (0 != ret) {
        printf_err("mq_send error: %s\n", strerror(errno));
    }
    return ret;
}

static int _mq_unlink(char *qname)
{

    int ret = mq_unlink(qname);
    if (0 != ret) {
        printf_err("mq_unlink error: %s\n", strerror(errno));
    }
    
    return ret;
}

static int _mq_recv(mqd_t queue, void **recv_buffer)
{
    int ret;
    void *buffer;
    struct mq_attr attr;
    // retrieve the message size
    ret = mq_getattr(queue, &attr);
    if (0 != ret) {
        return -1;
    }
    buffer = malloc(attr.mq_msgsize);
    ssize_t n = mq_receive(queue, (void*)buffer, attr.mq_msgsize, NULL);
    if (n < 0) {
        free(buffer);
        return -1;
    }
    if(n > attr.mq_msgsize){
        n= attr.mq_msgsize;
    }

    *recv_buffer = buffer;
    
    return n;
}

static int _mq_close(mqd_t queue)
{
    return mq_close(queue);
}


static const struct _mq_ops mq_ops = {
    .create = _mq_create,
    .open_ro = _mq_open_ro,
    .open_wo = _mq_open_wo,
    .send = _mq_send,
    .unlink = _mq_unlink,
    .recv = _mq_recv,
    .getattr = _mq_getattr,
    .close = _mq_close,
};


struct mq_ctx * mq_create_ctx(void *mqname, void *mattr, int oflag)
{
    int ret = -ENOMEM;
    unsigned int len;
    struct mq_ctx *ctx = calloc(1,sizeof(*ctx));
    if (!ctx)
        goto err_set_errno;

    ctx->ops = &mq_ops;
    ctx->ops->unlink(mqname);
    ctx->queue = ctx->ops->create(mqname, (struct mq_attr *)mattr, oflag);

err_set_errno:
    errno = -ret;
    return ctx;

}


