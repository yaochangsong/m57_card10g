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
#include "platform.h"
#ifdef SUPPORT_SPECTRUM_FFT
#include <fftw3.h>
#endif
#include "lib/libubox/ulog.h"
#include "lib/libubox/uloop.h"
#include "lib/libubox/usock.h"

#if defined  SUPPORT_PROJECT_WD_XCR
#include "bsp/wd_xcr/bsp.h"
#include "bsp/wd_xcr/_reg.h"
#include "bsp/wd_xcr/_reg_clk_adc.h"
#include "bsp/wd_xcr/_ctrl.h"
#elif defined(SUPPORT_PROJECT_160M_V2) 
#include "bsp/160m_v2/bsp.h"
#include "bsp/160m_v2/_reg.h"
#include "bsp/160m_v2/_reg_clk_adc.h"
#include "bsp/160m_v2/_ctrl.h"
#elif defined(SUPPORT_PROJECT_WD_XCR_40G) 
#include "bsp/tf713_4ch/bsp.h"
#include "bsp/40g/_reg.h"
#include "bsp/40g/_reg_clk_adc.h"
#include "bsp/40g/_ctrl.h"
#include "bsp/40g/bsp.h"
#elif defined(SUPPORT_PROJECT_TF713_4CH) 
#include "bsp/tf713_4ch/bsp.h"
#include "bsp/tf713_4ch/_reg.h"
#include "bsp/tf713_4ch/_reg_clk_adc.h"
#include "bsp/tf713_4ch/_ctrl.h"
#elif defined(SUPPORT_PROJECT_TF713_2CH) 
#include "bsp/tf713_2ch/bsp.h"
#include "bsp/tf713_2ch/_reg.h"
#include "bsp/tf713_2ch/_reg_clk_adc.h"
#include "bsp/tf713_2ch/_ctrl.h"
#elif defined(SUPPORT_PROJECT_AKT_4CH) 
#include "bsp/akt_4ch/bsp.h"
#include "bsp/akt_4ch/_reg.h"
#include "bsp/akt_4ch/_reg_clk_adc.h"
#include "bsp/akt_4ch/_ctrl.h"
#elif defined(SUPPORT_PROJECT_CARD10G_57) 
#include "bsp/card10g_57/bsp.h"
#include "bsp/card10g_57/_reg.h"
#include "bsp/card10g_57/_reg_clk_adc.h"
#include "bsp/card10g_57/_ctrl.h"

#endif

#include "conf/conf.h"
#include "log/log.h"
#include "protocol/http/utils.h"
#include "protocol/http/uhttpd.h"
#include "protocol/http/client.h"
#include "protocol/resetful/parse_json.h"

#include "protocol/akt/akt.h"
#include "protocol/m57/m57.h"
#include "protocol/oal/poal.h"

#include "net/net.h"
#include "net/net_tcp.h"
#include "net/net_data.h"
#include "net/net_udp.h"
#include "executor/spm/spm.h"
#include "executor/spm/spm_hash.h"
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
#include "utils/hash.h"
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
#include "device/clk_adc/clk_adc.h"
#if defined (SUPPORT_RF_SPI_M57)
#include "device/rf/spi/m57/rf_driver.h"
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
#if defined(SUPPORT_NRS1800)
#include "device/nrs1800/nrs1800.h"
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
#include "net/net_statistics.h"
#include "version.h"

