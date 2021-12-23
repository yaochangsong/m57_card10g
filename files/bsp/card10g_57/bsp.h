#ifndef _BSP_H
#define _BSP_H

//#define DEBUG_TEST
#define LOAD_FILE_ASYN 1
#define KEY_TOOL_ENABLE 1
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
#define MAX_CLINET_SOCKET_NUM           32   /* 客户端socket数 */
#define MAX_XDMA_NUM                    1   /* XDMA数 */
#define MAX_XDMA_RF_CARD_NUM            8   /* 射频卡数 */

#if MAX_XDMA_NUM > 1 
#define PRIO_CHANNEL_EN
#endif
#define CONFIG_NET_STATISTICS_ENABLE              1

//HASH MAP OFFSET
#define CARD_SLOT_BITS (3)
#define CARD_CHIP_BITS (2)
#define CARD_FUNC_BITS (8)
#define CARD_PRIO_BITS (1)
#define CARD_PORT_BITS (2)


#define CARD_SLOT_MASK ((1 << CARD_SLOT_BITS) - 1)
#define CARD_CHIP_MASK (((1 << CARD_CHIP_BITS) - 1) << CARD_SLOT_BITS)
#define CARD_FUNC_MASK (((1 << CARD_FUNC_BITS) - 1) << (CARD_CHIP_BITS +CARD_SLOT_BITS))
#define CARD_PRIO_MASK (((1 << CARD_PRIO_BITS) - 1) << (CARD_CHIP_BITS +CARD_SLOT_BITS+CARD_FUNC_BITS))
#define CARD_PORT_MASK (((1 << CARD_PORT_BITS) - 1) << (CARD_CHIP_BITS +CARD_SLOT_BITS+CARD_FUNC_BITS+CARD_PRIO_BITS))

#define CARD_SLOT_MODE (1 << CARD_SLOT_BITS)
#define CARD_CHIP_MODE (1 << CARD_CHIP_BITS)
#define CARD_FUNC_MODE (1 << CARD_FUNC_BITS)
#define CARD_PRIO_MODE (1 << CARD_PRIO_BITS)
#define CARD_PORT_MODE (1 << CARD_PORT_BITS)



#define CARD_SLOT_NUM(x) ((((x)>>8) & 0x0ff) % CARD_SLOT_MODE)    //05
#define CARD_CHIP_NUM(x) (((x) & 0xff) % CARD_CHIP_MODE)    //02

/* XDMA分发类型数 */
////HASH ID: funcId |chip |slot
#define MAX_XDMA_DISP_TYPE_NUM  (1 << (CARD_SLOT_BITS + CARD_CHIP_BITS + CARD_FUNC_BITS +CARD_PORT_BITS + CARD_PRIO_BITS))
#define GET_HASHMAP_ID(cid, fid, prid, port)  (CARD_SLOT_NUM(cid) + \
                                    (CARD_CHIP_NUM(cid)<<CARD_SLOT_BITS) + \
                                    (((fid) % CARD_FUNC_MODE) <<(CARD_SLOT_BITS+CARD_CHIP_BITS)) + \
                                    (((prid) % CARD_PRIO_MODE) <<(CARD_SLOT_BITS+CARD_CHIP_BITS+CARD_FUNC_BITS)) + \
                                    (((port) % CARD_PORT_MODE)<<(CARD_SLOT_BITS+CARD_CHIP_BITS+CARD_PRIO_BITS+CARD_FUNC_BITS)))

#define GET_HASHMAP_SLOT_NUM (1 << (CARD_SLOT_BITS))
#define GET_HASHMAP_CHIP_NUM (1 << (CARD_CHIP_BITS+CARD_SLOT_BITS))
#define GET_HASHMAP_FUNC_NUM (1 << (CARD_CHIP_BITS+CARD_SLOT_BITS+CARD_FUNC_BITS))
#define GET_HASHMAP_PRIO_NUM (1 << (CARD_CHIP_BITS+CARD_SLOT_BITS+CARD_FUNC_BITS+CARD_PRIO_BITS))
#define GET_HASHMAP_PORT_NUM (1 << (CARD_CHIP_BITS+CARD_SLOT_BITS+CARD_PRIO_BITS+CARD_FUNC_BITS+CARD_PORT_BITS))

#define GET_HASHMAP_SLOT_ADD_OFFSET  0
#define GET_HASHMAP_CHIP_ADD_OFFSET  GET_HASHMAP_SLOT_NUM
#define GET_HASHMAP_FUNC_ADD_OFFSET  GET_HASHMAP_CHIP_NUM
#define GET_HASHMAP_PROI_ADD_OFFSET  GET_HASHMAP_FUNC_NUM
#define GET_HASHMAP_PORT_ADD_OFFSET  GET_HASHMAP_PRIO_NUM

#define GET_SLOTID_BY_HASHID(hashid) (hashid & CARD_SLOT_MASK)
#define GET_CHIPID_BY_HASHID(hashid) ((hashid & CARD_CHIP_MASK)>>CARD_SLOT_BITS)
#define GET_FUNCID_BY_HASHID(hashid) ((hashid & CARD_FUNC_MASK)>>(CARD_CHIP_BITS +CARD_SLOT_BITS))
#define GET_PROIID_BY_HASHID(hashid) ((hashid & CARD_PRIO_MASK)>>(CARD_SLOT_BITS+CARD_CHIP_BITS+CARD_FUNC_BITS))
#define GET_PORTID_BY_HASHID(hashid) ((hashid & CARD_PORT_MASK)>>(CARD_SLOT_BITS+CARD_CHIP_BITS+CARD_PRIO_BITS+CARD_FUNC_BITS))

#define GET_SLOT_CHIP_ID_BY_HASHID(hashid) ((GET_SLOTID_BY_HASHID(hashid)<<8) | GET_CHIPID_BY_HASHID(hashid))


enum hashmap_type {
    HASHMAP_TYPE_SLOT = 0,
    HASHMAP_TYPE_CHIP,
    HASHMAP_TYPE_FUNC,
    HASHMAP_TYPE_PRIO,
    HASHMAP_TYPE_PORT,
};




#endif
