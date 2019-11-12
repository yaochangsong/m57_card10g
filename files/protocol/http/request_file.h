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
#ifndef _REQUEST_FILE_H_
#define _REQUEST_FILE_H_

#define REQUEST_FILESIZE_BUFFER_LEN  512

extern int file_download(struct uh_client *cl, void *arg);
extern int file_startstore(struct uh_client *cl, void *arg);
extern int file_stopstore(struct uh_client *cl, void *arg);
extern int file_search(struct uh_client *cl, void *arg);
extern int file_start_backtrace(struct uh_client *cl, void *arg);
extern int file_stop_backtrace(struct uh_client *cl, void *arg);
extern int file_delete(struct uh_client *cl, void *arg);
extern void file_handle_init(void);


#endif
