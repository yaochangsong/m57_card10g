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

//BSP
#include "bsp.h"
#include "misc.h"
#include "dev.h"
#ifdef __has_include

#   if __has_include ("reg_misc.h")
#include "reg_misc.h"
#   endif
#   if __has_include ("reg_if.h")
#include "reg_if.h"
#   endif
#   if __has_include ("reg_rf.h")
#include "reg_rf.h"
#   endif
#   if __has_include ("adclock.h")
#include "adclock.h"
#   endif
#   if __has_include ("reg.h")
#include "reg.h"
#   endif

#endif

#include "../main/fs/fs.h"
#include "../main/device/gpio/gpio_raw.h"
#include "../main/device/clk_adc/clk_adc.h"
#include "../main/device/gps/gps_com.h"
#include "../main/device/lcd/lcd.h"
#include "../main/device/lcd/ss_k600.h" 
#include "../main/device/rf/rf.h"
#include "../main/device/rf/fpga/rf_driver.h"
#include "../main/device/rf/adrv/adrv.h"
#ifdef CONFIG_DEVICE_RF_ADRV
#include "../main/device/rf/adrv/adrv9009/adrv9009-iiostream.h"
#include "../main/device/rf/adrv/adrv9361/adrv9361-iiostream.h"
#endif
#include "../main/device/rf/adrv/condition/condition.h"
#include "../main/device/rs485/rs485_com.h"
#include "../main/device/uart/uart.h"

#include "../main/bsp/io.h"
#include "../main/bsp/register.h"
#include "../main/bsp/misc.h"

#include "../main/executor/spm/spm.h"
#include "../main/executor/spm/spm_fpga/spm_fpga.h"
#include "../main/executor/spm/spm_xdma/spm_xdma.h"
#include "../main/executor/spm/distributor/spm_distributor.h"
#include "../main/executor/spm/file_sink/file_sink.h"
#include "../main/executor/spm/statistics/statistics.h"
#include "../main/executor/spm/agc/agc.h"


#include "../main/executor/executor_thread.h"
#include "../main/executor/executor.h"
#include "../main/executor/spm/bottom/bottom.h"
#include "../main/executor/fft/fft.h"
#include "../main/executor/analysis/spm_analysis.h"


#include "../main/protocol/akt/akt.h"
#include "../main/protocol/resetful/request.h"
#include "../main/protocol/http/uhttpd.h"
#include "../main/protocol/http/file.h"
#include "../main/protocol/http/utils.h"
#include "../main/protocol/http/client.h"
#include "../main/protocol/ftp/ftp_server.h"
#include "../main/protocol/oal/poal.h"
