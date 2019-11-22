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
*  Rev 1.0   09 Nov 2019   yaochangsong
*  Initial revision.
******************************************************************************/

#ifndef _REQUSET_H
#define _REQUSET_H

#include <stdbool.h>
#include "client.h"

enum {
    BLK_FILE_DOWNLOAD_CMD =1,
    BLK_FILE_START_STORE_CMD,
    BLK_FILE_STOP_STORE_CMD,
    BLK_FILE_SEARCH_CMD,
    BLK_FILE_START_BACKTRACE_CMD,
    BLK_FILE_STOP_BACKTRACE_CMD,
    BLK_FILE_DELETE_CMD,
};

struct request_info {
    char *path;
    int cmd;
    int (*action)(struct uh_client *cl, void *arg);
};

extern void http_requset_init(void);
extern int http_request_action(struct uh_client *cl);

#endif

