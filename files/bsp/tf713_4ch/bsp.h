#ifndef _BSP_H
#define _BSP_H


#define MAX_RADIO_CHANNEL_NUM 4         /* 最大射频通道数 */
#define MAX_RF_NUM 4                    /* 最大射频数 */
#define MAX_SIGNAL_CHANNEL_NUM (24)     /* 最大解调子通道数 */
#define MAX_SIG_CHANNLE 40              /* 最大频点数 */
#define CONFIG_AUDIO_CHANNEL            16  /* 音频解调子通道 */
#define CONFIG_SIGNAL_CHECK_CHANNEL     1   /* 信号检测子通道(多频点模式下有效) */
#define BROAD_CH_NUM                    6   /* 宽带通道数FFT+BIQ+4路ADC read */
#define SEGMENT_FREQ_NUM               (3)
#endif
