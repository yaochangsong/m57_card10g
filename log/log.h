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
*  Rev 1.0   06 July 2019   yaochangsong
*  Initial revision.
******************************************************************************/

 
#ifndef _LOG_H
#define _LOG_H

#include <string.h>
#include "libubox/ulog.h"

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

/*
 * Use the syslog output log and include the name and number of rows at the call
 */
#define uh_log(priority, fmt...) __uh_log(__FILENAME__, __LINE__, priority, fmt)

#define uh_log_debug(fmt...)     uh_log(LOG_DEBUG, fmt)
#define uh_log_info(fmt...)      uh_log(LOG_INFO, fmt)
#define uh_log_err(fmt...)       uh_log(LOG_ERR, fmt)

#define log_err     LOG_ERR		/* 3 error conditions */
#define	log_warn    LOG_WARNING /* 4 warning conditions */
#define	log_notice  LOG_NOTICE	/* 5 normal but significant condition */
#define	log_info    LOG_INFO	/* 6 informational */
#define	log_debug   LOG_DEBUG	/* 7 debug-level messages */
#define	log_off     -1


#define printfd ULOG_DEBUG
#define printfe ULOG_ERR


#define printf_debug(fmt, ...)   ULOG_DEBUG("[DEBUG][%s,%4d]"fmt, __func__, __LINE__, ##__VA_ARGS__)
#define printf_info(fmt, ...)    ULOG_INFO("[INFO][%s,%4d]"fmt, __func__, __LINE__, ##__VA_ARGS__)
#define printf_note(fmt, ...)    ULOG_NOTE("[NOTE][%s,%4d]"fmt, __func__, __LINE__, ##__VA_ARGS__)
#define printf_warn(fmt, ...)    ULOG_WARN("[WARN][%s,%4d]"fmt, __func__, __LINE__, ##__VA_ARGS__)
#define printf_err(fmt, ...)     ULOG_ERR("[ERROR][%s,%4d]"fmt, __func__, __LINE__, ##__VA_ARGS__)




void  __uh_log(const char *filename, int line, int priority, const char *fmt, ...);

#endif
