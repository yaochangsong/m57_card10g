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
#ifndef _MQ_H
#define _MQ_H

struct _mq_ops {
    mqd_t (*create)(char *, struct mq_attr*, int );
    mqd_t (*open_ro)(char *);
    mqd_t (*open_wo)(char *);
    int (*send)(mqd_t , void *, size_t);
    int (*recv)(mqd_t , void **);
    int (*unlink)(char *);
    struct mq_attr *(*getattr)(char *);
    int (*close)(mqd_t );
};

struct mq_ctx {
    mqd_t queue;
    struct _mq_ops *ops;
};


#endif


