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

#ifndef _FILE_HTTP_H
#define _FILE_HTTP_H

enum {
    BLK_FILE_READ_CMD =1,
    BLK_FILE_DELETE_CMD=2,
    BLK_FILE_ADD_CMD=3,
    BLK_FILE_BACKTRACE_CMD=4,
    BLK_FILE_SEARCH_CMD=5
};

#define HTTP_QUERY_READ_FILE_CMD    "ReadFile"
#define HTTP_QUERY_DELETE_FILE_CMD  "DeleteFile"
#define HTTP_QUERY_ADD_FILE_CMD     "AddFile"
#define HTTP_QUERY_BACKTRACE_FILE_CMD     "BackTraceFile"
#define HTTP_QUERY_SEARCH_FILE_CMD     "SearchFile"

extern void file_http_init(void);
//extern int file_http_on_request(struct uh_client *cl);
extern int file_http_read(char *filename, unsigned char *buf, int len);

#endif