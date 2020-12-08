#ifndef _PLATFORM_H
#define _PLATFORM_H

#ifndef MHZ
#define MHZ(x) ((long long)(x*1000000.0 + .5))
#endif
#ifndef GHZ
#define GHZ(x) ((long long)(x*1000000000.0 + .5))
#endif


#define MAX_RADIO_CHANNEL_NUM 2         /* 最大射频通道数 */
#define MAX_RF_NUM 3                    /* 最大射频数 */
#define MAX_SIGNAL_CHANNEL_NUM (17)     /* 最大解调子通道数 */
#define MAX_SIG_CHANNLE 700             /* 最大频点数 */
#define CONFIG_AUDIO_CHANNEL            16  /* 音频解调子通道 */
#define CONFIG_SIGNAL_CHECK_CHANNEL     1   /* 信号检测子通道(多频点模式下有效) */


#define PLATFORM_VERSION  "1.0.0" /* application version */

#endif
