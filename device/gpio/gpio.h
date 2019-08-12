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

#define  BAND_WITH_100M (100000000ULL)
#define  BAND_WITH_200M (200000000ULL)

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