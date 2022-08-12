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


#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

#include "log.h"

void __uh_log(const char *filename, int line, int priority, const char *fmt, ...)
{
    va_list ap;
    static char buf[128];

    snprintf(buf, sizeof(buf), "(%s:%d) ", filename, line);
    
    va_start(ap, fmt);
    vsnprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), fmt, ap);
    va_end(ap);

    if (priority == LOG_ERR && errno > 0) {
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), ":%s", strerror(errno));
        errno = 0;
    }

    ulog(priority, "%s\n", buf);
}

void log_init(int channels,int threshold)
{
    //ulog_open(ULOG_SYSLOG, LOG_DAEMON, NULL);
    //ulog_open(ULOG_STDIO, LOG_USER, NULL);
    ulog_open(channels, LOG_LOCAL0, NULL);
    ulog_threshold(threshold);
}

