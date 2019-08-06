

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

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
#define SEC_H_42442_V1   21
#define SEC_H_42442_V2   22
#define SEC_42422_V1     23


typedef enum {
 U9_RFC_OUT1 = 1,
 U9_RFC_OUT2,	
 U7_RF1_11,
 U7_RF1_12,
 U7_RF1_13,
 U7_RF1_14,
 U5_RF1_1,
 U5_RF1_2,
 U5_RF1_3,
 U5_RF1_4,
 U1_RFC_IN1,
 U1_RFC_IN2,
 U6_RF2_1,
 U6_RF2_2,
 U6_RF2_3,
 U6_RF2_4,
 U8_RF2_11,
 U8_RF2_12,
 U8_RF2_13,
 U8_RF2_14,
 
 U10_0_DB,
 U10_0_5_DB,
 U10_1_DB,
 U10_2_DB,
 U10_4_DB,
 U10_8_DB,
 U10_16_DB,
 U10_31_5_DB,
 
 U2_0_DB,
 U2_0_5_DB,
 U2_1_DB,
 U2_2_DB,
 U2_4_DB,
 U2_8_DB,
 U2_16_DB,
 U2_31_5_DB,
 
}cmd_gpio;

int init_gpio(int spidev_index);
int set_gpio(int spidev_index,char val);
void init_control_gpio();
void control_rf(cmd_gpio cmd);