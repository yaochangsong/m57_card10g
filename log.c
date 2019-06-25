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

