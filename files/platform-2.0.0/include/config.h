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
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <math.h>
#include <sys/mman.h>
#include <mqueue.h>
#include <assert.h>

#include "../lib/libubox/ulog.h"
#include "../lib/libubox/uloop.h"
#include "../lib/libubox/usock.h"
#include "../lib/libubox/ustream.h"
#include "../lib/libubox/blobmsg.h"
#include "../lib/libubox/avl.h"
#include "../lib/libubox/avl-cmp.h"
#include "../lib/libubox/list.h"
#include "../lib/libubox/md5.h"

#include "../main/log/log.h"
#include "../main/conf/conf.h"
#include "../main/main.h"

#include "../main/dao/json/cJSON.h"
#include "../main/dao/oal/json/json-oal.h"

#include "../main/net/net.h"
#include "../main/net/net_tcp.h"
#include "../main/net/net_data.h"
#include "../main/net/net_udp.h"



#include "../main/utils/utils.h"
#include "../main/utils/thread.h"
#include "../main/utils/mempool.h"
#include "../main/utils/bitops.h"
#include "../main/utils/mq.h"


#ifdef CONFIG_BSP_DEMO
#include "../main/bsp/demo/bsp.h"
#include "../main/bsp/demo/dev.h"
#include "../main/bsp/demo/misc.h"
#elif defined(CONFIG_BSP_WD_XCR) 
#include "../main/bsp/wd_xcr/bsp.h"
#include "../main/bsp/wd_xcr/reg.h"
#include "../main/bsp/wd_xcr/adclock.h"
#include "../main/bsp/wd_xcr/misc.h"
#include "../main/bsp/wd_xcr/dev.h"
#include "../main/bsp/wd_xcr/reg_rf.h"
#include "../main/bsp/wd_xcr/reg_if.h"
#include "../main/bsp/wd_xcr/reg_misc.h"
#endif

#include "../main/fs/fs.h"
#include "../main/device/gpio/gpio_raw.h"
#include "../main/device/clk_adc/clk_adc.h"
#include "../main/device/gps/gps_com.h"
#include "../main/device/lcd/lcd.h"
#include "../main/device/rf/rf.h"
#include "../main/device/rf/fpga/rf_driver.h"
#include "../main/device/rs485/rs485_com.h"
#include "../main/device/uart/uart.h"

#include "../main/bsp/io.h"
#include "../main/bsp/register.h"
#include "../main/bsp/misc.h"

#include "../main/executor/spm/spm.h"
#include "../main/executor/spm/spm_fpga/spm_fpga.h"
#include "../main/executor/executor.h"
#include "../main/executor/spm/bottom/bottom.h"

#include "../main/protocol/akt/akt.h"
#include "../main/protocol/resetful/request.h"
#include "../main/protocol/http/uhttpd.h"
#include "../main/protocol/http/file.h"
#include "../main/protocol/http/utils.h"
#include "../main/protocol/http/client.h"
#include "../main/protocol/oal/poal.h"