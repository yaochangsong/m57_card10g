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
    BLK_FILE_DOWNLOAD_CMD =1,
    BLK_FILE_START_STORE_CMD,
    BLK_FILE_STOP_STORE_CMD,
    BLK_FILE_SEARCH_CMD,
    BLK_FILE_START_BACKTRACE_CMD,
    BLK_FILE_STOP_BACKTRACE_CMD,
    BLK_FILE_DELETE_CMD,
};

#define HTTP_FILE_PATH                      "file"

#define HTTP_DOWNLOAD_FILE_CMD              "download"
#define HTTP_START_STORE_FILE_CMD           "startstore"
#define HTTP_STOP_STORE_FILE_CMD            "stopstore"
#define HTTP_SEARCH_FILE_CMD                "search"
#define HTTP_START_BACKTRACE_FILE_CMD       "startbacktrace"
#define HTTP_STOP_BACKTRACE_FILE_CMD        "stopbacktrace"
#define HTTP_DELETE_FILE_CMD                "delete"


#define HTTP_QUERY_DELETE_FILE_CMD  "DeleteFile"
#define HTTP_QUERY_ADD_FILE_CMD     "AddFile"
#define HTTP_QUERY_BACKTRACE_FILE_CMD     "BackTraceFile"
#define HTTP_QUERY_SEARCH_FILE_CMD     "SearchFile"

extern void file_http_init(void);
//extern int file_http_on_request(struct uh_client *cl);
extern int file_http_read(char *filename, unsigned char *buf, int len);

#endif