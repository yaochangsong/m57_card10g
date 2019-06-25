#ifndef _LOG_H
#define _LOG_H

#include <string.h>
#include <libubox/ulog.h>

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

/*
 * Use the syslog output log and include the name and number of rows at the call
 */
#define uh_log(priority, fmt...) __uh_log(__FILENAME__, __LINE__, priority, fmt)

#define uh_log_debug(fmt...)     uh_log(LOG_DEBUG, fmt)
#define uh_log_info(fmt...)      uh_log(LOG_INFO, fmt)
#define uh_log_err(fmt...)       uh_log(LOG_ERR, fmt)

void  __uh_log(const char *filename, int line, int priority, const char *fmt, ...);

#endif

