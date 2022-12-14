#ifndef __GPIO_H
#define __GPIO_H

//#include "config.h"

#define GPIO_BASE_OFFSET 960

#define PRI_624A_D0      0
#define PRI_624A_D1      1
#define PRI_624A_D2      2 
#define PRI_624A_D3      3
#define PRI_624A_D4      4
#define PRI_624A_D5      5
#define SEC_624A_D0      6
#define SEC_624A_D1      7
#define SEC_624A_D2      8
#define SEC_624A_D3      9
#define SEC_624A_D4      10
#define SEC_624A_D5      11
#define PRI_L_42442_V1   12
#define PRI_L_42442_V2   13
#define PRI_H_42442_V1   14
#define PRI_H_42442_V2   15
#define SEC_L_42442_V1   16
#define SEC_L_42442_V2   17
#define SEC_42422_LS     18
#define PRI_42422_LS     19
#define PRI_42422_V1     20
#define SEC_H_42442_V2   21
#define SEC_H_42442_V1   22
#define SEC_42422_V1     23


#ifdef SUPPORT_PROJECT_SSA_MONITOR
#define SW_TO_2_4_G()          gpio_set(SEC_L_42442_V1, 1)

#define SW_TO_7_75_G()          do{ \
                                        gpio_set(SEC_L_42442_V1, 0); \
                                        gpio_set(PRI_L_42442_V1, 1); \
                                    }while(0)

#define SW_TO_75_9_G()          do{ \
                                        gpio_set(SEC_L_42442_V1, 0); \
                                        gpio_set(PRI_L_42442_V1, 0); \
                                    }while(0)

#define LVTTL_FREQ_START1   (2000000000ULL)
#define LVTTL_FREQ_end1     (4000000000ULL)

//(3.9-4.4G)
#define LVTTL_FREQ_START2   (7000000000ULL)
#define LVTTL_FREQ_end2     (7500000000ULL)
#define LVTTL_FREQ_BZ2      (11400000000ULL)   //本振输出11.4G

//(2.1-3.6G)
#define LVTTL_FREQ_START3   (7500000000ULL)
#define LVTTL_FREQ_end3     (9000000000ULL)
#define LVTTL_FREQ_BZ3      (11100000000ULL)   //本振输出11.1G

static inline uint64_t lvttv_freq_convert(uint64_t m_freq)
{
    uint64_t tmp_freq;
    if (m_freq >= LVTTL_FREQ_START2 && m_freq < LVTTL_FREQ_end2) {
        tmp_freq = LVTTL_FREQ_BZ2 - m_freq;
    } else if (m_freq >= LVTTL_FREQ_START3 && m_freq <= LVTTL_FREQ_end3) {
        tmp_freq = LVTTL_FREQ_BZ3 - m_freq;
    } else {
        tmp_freq = m_freq;
    }
    return tmp_freq;
}

#endif

                                    
#define  BAND_WITH_100M (100000000ULL)
#define  BAND_WITH_200M (200000000ULL)

#if 0
#define  RF_START_0M (000000000ULL)
//#define  RF_START_0M (000000000ULL)
#define  RF_START_145M (145000000ULL)
//#define  RF_START_145M (145000000ULL)
#define  RF_START_650M (650000000ULL)
#define  RF_START_1150M (1150000000ULL)
#define  RF_START_2100M (2100000000ULL)
#define  RF_START_2700M (2700000000ULL)


#define  RF_END_80M (80000000ULL)
#define  RF_END_160M (160000000ULL)
#define  RF_END_320M (320000000ULL)
#define  RF_END_630M (630000000ULL)  //#define  RF_END_630M (800000000ULL)
#define  RF_END_1325M (1325000000ULL)
#define  RF_END_2750M (2750000000ULL)
#define  RF_END_3800M (3800000000ULL)
#define  RF_END_6000M (6000000000ULL)
#else
#define  RF_START_1 (50000000ULL)
#define  RF_START_2 (120000000ULL)
#define  RF_START_3 (210000000ULL)
#define  RF_START_4 (420000000ULL)
#define  RF_START_5 (680000000ULL)
#define  RF_START_6 (1240000000ULL)
#define  RF_START_7 (2180000000ULL)
#define  RF_START_8 (3340000000ULL)
#define  RF_END_1 (120000000ULL)
#define  RF_END_2 (210000000ULL)
#define  RF_END_3 (420000000ULL)
#define  RF_END_4 (680000000ULL)  
#define  RF_END_5 (1240000000ULL)
#define  RF_END_6 (2180000000ULL)
#define  RF_END_7 (3340000000ULL)
#define  RF_END_8 (5880000000ULL)
#endif

typedef struct  {
     uint8_t  index_rf;
     uint64_t s_freq_rf;
     uint64_t e_freq_rf;
     uint32_t bw_rf;
}__attribute__ ((packed)) RF_CHANNEL_SN;   


typedef enum {
     HPF1 = 1,       //50-75M
     HPF2,	
     HPF3,
     HPF4,
     HPF5,
     HPF6,
     HPF7,
     HPF8
}rf_channel;    //射频通道

typedef enum {
     U10_0_DB = 0,
     U10_0_5_DB,
     U10_1_DB,
     U10_2_DB,
     U10_4_DB,
     U10_8_DB,
     U10_16_DB,
     U10_31_5_DB
}rf_pre_reduce;      //前级衰减

typedef enum {
     U2_0_DB = 0,
     U2_0_5_DB,
     U2_1_DB,
     U2_2_DB,
     U2_4_DB,
     U2_8_DB,
     U2_16_DB,
     U2_31_5_DB
}rf_pos_reduce;      //后级衰减

int  gpio_init(int spidev_index);
int  gpio_set(int spidev_index,char val);
void gpio_init_control();

void gpio_control_rf(rf_channel channel_val);
void gpio_attenuation_rf(rf_pre_reduce pre_reduce_val,rf_pos_reduce pos_reduce_val);
void gpio_select_rf_channel(uint64_t mid_freq);                                       //射频通道选择
int  gpio_select_rf_attenuation(float attenuation_val);
#endif