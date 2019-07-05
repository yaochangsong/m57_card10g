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

#include "libubox/ulog.h"
#include "libubox/uloop.h"
#include "libubox/usock.h"
#include "log/log.h"
#include "http/utils.h"
#include "http/uhttpd.h"
#include "http/client.h"

#include "net/net_socket.h"
#include "dao/mxml-3.0/mxml.h"

/*define*/

