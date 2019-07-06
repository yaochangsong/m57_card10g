#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <poll.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <poll.h>
#include <arpa/inet.h>
#include <pthread.h>


#include "libubox/ulog.h"
#include "libubox/uloop.h"
#include "libubox/usock.h"

#include "conf/conf.h"
#include "log/log.h"
#include "protocol/http/utils.h"
#include "protocol/http/uhttpd.h"
#include "protocol/http/client.h"

#include "net/net_socket.h"
#include "dao/mxml-3.0/mxml.h"
#include "dao/oal/dao_oal.h"

/*define*/
#define PLAT_FORM_ARCH_X86   //PLAT_FORM_ARCH_ARM
#define SPCTRUM_VERSION_STRING "1.0.0-"__DATE__"."__TIME__




