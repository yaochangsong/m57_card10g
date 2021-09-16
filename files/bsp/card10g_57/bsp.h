#ifndef _BSP_H
#define _BSP_H


#define MAX_RADIO_CHANNEL_NUM 1         /* 最大射频通道数 */
#define MAX_RF_NUM 1                    /* 最大射频数 */
#define MAX_SIGNAL_CHANNEL_NUM (1)     /* 最大解调子通道数 */
#define MAX_SIG_CHANNLE 1              /* 最大频点数 */
#define CONFIG_AUDIO_CHANNEL            1  /* 音频解调子通道 */
#define CONFIG_SIGNAL_CHECK_CHANNEL     1   /* 信号检测子通道(多频点模式下有效) */
#define BROAD_CH_NUM                    1   /* 宽带通道数FFT+NIQ */
#define SEGMENT_FREQ_NUM               (1)
#define MAX_FPGA_CARD_SLOT_NUM          (16) /* FPGA槽位 */
#define MAX_FPGA_CHIPID_NUM             2   /* FPGA芯片数 */
#define MAX_CLINET_SOCKET_NUM           8   /* 客户端socket数 */
#define MAX_XDMA_NUM                    2   /* XDMA数 */

//HASH MAP OFFSET
#define CARD_SLOT_OFFSET (2)
#define CARD_CHIP_OFFSET (1)
#define CARD_FUNC_OFFSET (4)
#define CARD_PORT_OFFSET (1)


#define CARD_SLOT_NUM(x) (((x)>>8) & 0xff)    //05
#define CARD_CHIP_NUM(x) ((x)& 0xff)    //02

/* XDMA分发类型数 */
////HASH ID: funcId |chip |slot
#define MAX_XDMA_DISP_TYPE_NUM  (2 << (CARD_SLOT_OFFSET + CARD_CHIP_OFFSET + CARD_FUNC_OFFSET - 1))
#define GET_HASHMAP_ID(cid, fid)  (CARD_SLOT_NUM(cid) + (CARD_CHIP_NUM(cid)<<CARD_SLOT_OFFSET) + (fid<<(CARD_SLOT_OFFSET+CARD_CHIP_OFFSET)));



#endif
