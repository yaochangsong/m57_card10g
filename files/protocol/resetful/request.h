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
#include "protocol/http/client.h"


enum {
    DISPATCH_DOWNLOAD_CMD =1,
};


struct request_info {
    char *method;
    char *path;
    int dispatch_cmd;
    int (*action)(struct uh_client *cl, void **err_msg, void **);
};

extern void http_requset_init(void);
extern void http_request_action(struct uh_client *cl);
extern int http_err_code_check(int ret);

#endif

