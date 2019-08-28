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
#include <sys/ioctl.h>
#include <semaphore.h>
#include <net/if.h>
#include <sys/stat.h>
#include <termios.h>
#include <errno.h>
#include <linux/spi/spidev.h>
#include <time.h>
#include <fftw3.h>


#include "libubox/ulog.h"
#include "libubox/uloop.h"
#include "libubox/usock.h"

#include "conf/conf.h"
#include "log/log.h"
#include "protocol/http/utils.h"
#include "protocol/http/uhttpd.h"
#include "protocol/http/client.h"
#include "protocol/akt/akt.h"
#include "protocol/xnrp/xnrp.h"
#include "protocol/xnrp/xnrp-xml.h"

#include "protocol/oal/poal.h"

#include "net/net_socket.h"
#include "net/net_udp.h"

#include "dao/mxml-3.0/mxml.h"
#include "dao/oal/dao_oal.h"
#include "utils/utils.h"

#include "executor/executor.h"
#include "executor/io.h"
#include "executor/spectrum.h"
#include "executor/fft/fft.h"


#include "device/uart/uart.h"
#include "device/rf/rf.h"
#include "device/rf/adrv9009-iiostream.h"


#include "device/lcd/ss_k600.h"
#include "device/lcd/lcd.h"
#include "device/gpio/gpio.h"
#include "dao/oal/dao_conf.h"





/*define*/
/*Support PlatForm Arch: 
PLAT_FORM_ARCH_X86, (For debug)
PLAT_FORM_ARCH_ARM
*/
#define PLAT_FORM_ARCH_ARM   
#define SPCTRUM_VERSION_STRING "1.0.0-"__DATE__"."__TIME__ /* application version */

/*Protocal Support*/
#define PROTOCAL_HTTP  1
#define PROTOCAL_XNRP  0
#define PROTOCAL_ATE   1

/*dao support*/
#define DAO_JSON  0
#define DAO_XML   1

/* local or remote control support, if not set, default use remote control */
#define CONTROL_MODE_SUPPORT 0

/* uart support */
#define UART_SUPPORT 0
/* uart lcd support */
#define UART_LCD_SUPPORT 0

#define KERNEL_IOCTL_EN 0

#ifdef PLAT_FORM_ARCH_ARM
#define  RF_ADRV9009_IIO   1
#else 
#define  RF_ADRV9009_IIO   0
#endif








