#ifndef _BSP_H
#define _BSP_H


#define MAX_RADIO_CHANNEL_NUM 1         /* 最大射频通道数 */
#define MAX_RF_NUM 1                    /* 最大射频数 */
#define MAX_SIGNAL_CHANNEL_NUM (1)     /* 最大解调子通道数 */
#define MAX_SIG_CHANNLE 4              /* 最大频点数 */
#define CONFIG_AUDIO_CHANNEL            16  /* 音频解调子通道 */
#define CONFIG_SIGNAL_CHECK_CHANNEL     1   /* 信号检测子通道(多频点模式下有效) */
#define BROAD_CH_NUM                    1   /* 宽带通道数 */
#define SEGMENT_FREQ_NUM               (1)
#define MAX_SCAN_FREQ_HZ 			   (6000000000)
#define MAX_BIQ_NUM  0
#define CLOCK_FREQ_HZ                    (153600000)/*时钟频率 */
#define FFT_RESOLUTION_FACTOR            (1)       /* FFT 分辨率系数 */
#define SAMPLING_FFT_TIMES                1        /* 采样FFT倍数 */



#endif