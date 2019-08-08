#ifndef __GPIO_H
#define __GPIO_H

#include "config.h"

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

typedef enum {
     HPF1 = 1,       //50-75M
     HPF2,	
     HPF3,
     HPF4,
     HPF5,
     HPF6,
     HPF7,
     HPF8
}Channel_Rf;

typedef enum {
     U10_0_DB,
     U10_0_5_DB,
     U10_1_DB,
     U10_2_DB,
     U10_4_DB,
     U10_8_DB,
     U10_16_DB,
     U10_31_5_DB,

}Attenuation1_Rf;

typedef enum {
     U2_0_DB,
     U2_0_5_DB,
     U2_1_DB,
     U2_2_DB,
     U2_4_DB,
     U2_8_DB,
     U2_16_DB,
     U2_31_5_DB
}Attenuation2_Rf;

int gpio_init(int spidev_index);
int gpio_set(int spidev_index,char val);
void gpio_init_control();
void gpio_control_rf(Channel_Rf channel_val,Attenuation1_Rf attenuation1_val,Attenuation2_Rf attenuation2_val);
#endif