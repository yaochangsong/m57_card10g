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
#ifdef SUPPORT_SPECTRUM_FFT
#include <fftw3.h>
#endif
#if defined  SUPPORT_PROJECT_WD_XCR
#include "bsp/wd_xcr/_reg.h"
#include "bsp/wd_xcr/_reg_clk_adc.h"
#elif defined(SUPPORT_PROJECT_160M_V2) 
#include "bsp/160m_v2/_reg.h"
#include "bsp/160m_v2/_reg_clk_adc.h"
#endif
#include "bsp/io.h"
#include "lib/libubox/ulog.h"
#include "lib/libubox/uloop.h"
#include "lib/libubox/usock.h"

#include "conf/conf.h"
#include "log/log.h"
#include "protocol/http/utils.h"
#include "protocol/http/uhttpd.h"
#include "protocol/http/client.h"
#include "protocol/resetful/parse_json.h"

#include "protocol/akt/akt.h"
#include "protocol/oal/poal.h"

#include "net/net.h"
#include "net/net_tcp.h"
#include "net/net_data.h"
#include "net/net_udp.h"
#ifdef SUPPORT_DAO_XML
#include "dao/mxml-3.0/mxml.h"
#include "dao/oal/xml/dao_oal.h"
#endif
#ifdef SUPPORT_DAO_JSON
#include "dao/json/cJSON.h"
#include "dao/oal/json/json-oal.h"
#endif
#include "utils/utils.h"
#include "utils/thread.h"
#include "utils/mempool.h"

#include "executor/executor.h"


#ifdef SUPPORT_SPECTRUM_FFT
//#include "executor/fft/spectrum.h"
#include "executor/fft/fft.h"
#endif
#ifdef SUPPORT_SPECTRUM_ANALYSIS
#include "executor/analysis/spm_analysis.h"
#endif

#include "device/uart/uart.h"
#include "device/rf/rf.h"
#if defined (SUPPORT_RF_SPI)
#include "device/rf/spi/rf_spi.h"
#endif
#if defined (SUPPORT_GPS)
#include "device/gps/gps_com.h"
#endif

#if defined (SUPPORT_RF_ADRV)
#include "device/rf/adrv/adrv.h"
#endif
#if defined (SUPPORT_TEMP_HUMIDITY_SI7021)
#include "device/humidity/si7021.h" 
#endif
#if defined (SUPPORT_LCD)
#include "device/lcd/ss_k600.h"
#include "device/lcd/lcd.h"
#endif
#if defined(SUPPORT_RS485)
#include "device/rs485/rs485_com.h"
#endif

#if defined(SUPPORT_XWFS)
#include "executor/xwfs/xwfs.h"
#endif
#if defined(SUPPORT_FS)
#include "fs/fs.h"
#endif
#include "device/gpio/gpio.h"
#include "device/gpio/gpio_raw.h"
#ifdef SUPPORT_DAO_XML
#include "dao/oal/xml/dao_conf.h"
#endif
#include "version.h"


#ifndef MHZ
#define MHZ(x) ((long long)(x*1000000.0 + .5))
#endif
#ifndef GHZ
#define GHZ(x) ((long long)(x*1000000000.0 + .5))
#endif

#define MAX_RADIO_CHANNEL_NUM 1         /* 最大射频通道数 */
#define MAX_SIGNAL_CHANNEL_NUM (17)     /* 最大解调子通道数 */
#define MAX_SIG_CHANNLE 100             /* 最大频点数 */
#define CONFIG_AUDIO_CHANNEL            16  /* 音频解调子通道 */
#define CONFIG_SIGNAL_CHECK_CHANNEL     1   /* 信号检测子通道(多频点模式下有效) */


#define PLATFORM_VERSION  "1.0.0" /* application version */

