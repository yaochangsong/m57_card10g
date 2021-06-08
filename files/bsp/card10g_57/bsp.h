#ifndef _BSP_H
#define _BSP_H


#define MAX_RADIO_CHANNEL_NUM 1         /* 最大射频通道数 */
#define MAX_RF_NUM 1                    /* 最大射频数 */
#define MAX_SIGNAL_CHANNEL_NUM (1)     /* 最大解调子通道数 */
#define MAX_SIG_CHANNLE 1              /* 最大频点数 */
#define CONFIG_AUDIO_CHANNEL            1  /* 音频解调子通道 */
#define CONFIG_SIGNAL_CHECK_CHANNEL     1   /* 信号检测子通道(多频点模式下有效) */
#define BROAD_CH_NUM                    1   /* 宽带通道数FFT+NIQ */
#define SEGMENT_FREQ_NUM               (0)
#define MAX_FPGA_CARD_SLOT_NUM          (4) /* FPGA槽位 */
#define MAX_FPGA_CHIPID_NUM             2   /* FPGA芯片数 */


#endif
