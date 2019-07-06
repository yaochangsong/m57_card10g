/*
 * Copyright (C) 2017 Jianhui Zhao <jianhuizhao329@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */
 
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

#define printf_debug(fmt, ...)   ULOG_DEBUG("[%s,%4d][DEBUG]"fmt, __func__, __LINE__, ##__VA_ARGS__)
#define printf_info(fmt, ...)    ULOG_INFO("[%s,%4d][INFO]"fmt, __func__, __LINE__, ##__VA_ARGS__)
#define printf_note(fmt, ...)    ULOG_NOTE("[%s,%4d][NOTE]"fmt, __func__, __LINE__, ##__VA_ARGS__)
#define printf_warn(fmt, ...)    ULOG_WARN("[%s,%4d][WARN]"fmt, __func__, __LINE__, ##__VA_ARGS__)
#define printf_err(fmt, ...)     ULOG_ERR("[%s,%4d][ERR]"fmt, __func__, __LINE__, ##__VA_ARGS__)




void  __uh_log(const char *filename, int line, int priority, const char *fmt, ...);

#endif
