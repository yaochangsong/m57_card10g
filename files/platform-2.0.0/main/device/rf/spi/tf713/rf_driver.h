#ifndef _RF_TEST_H_
#define _RF_TEST_H_

#include "../../rf.h"

#define SPI_DEV_NAME "/dev/spidev2.0"

#ifndef MHZ
#define MHZ(x) ((long long)(x*1000000.0+0.5))
#endif
#ifndef GHZ
#define GHZ(x) ((long long)(x*1000000000.0+0.5))
#endif


#define RF_CMD_CODE_FREQUENCY 	0x0  //调节频率
#define RF_CMD_CODE_RF_GAIN		0x1  //射频衰减
#define RF_CMD_CODE_MF_GAIN		0x2  //中频衰减
#define RF_CMD_CODE_MODE 		0x3  //工作模式
#define RF_CMD_CODE_BW	 		0x4  //中频带宽
#define RF_CMD_CODE_CAL_SW	 	0x6  //校正-馈电开关
#define RF_CMD_CODE_WORK_PARA 	0x7  //工作参数查询
#define RF_CMD_CODE_TEMPERATURE 0x31 //温度查询
#define RF_CMD_CODE_BIT_QUERY   0x33 //BIT查询
#define RF_CMD_CODE_HANDSHAKE   0x3D //握手

/*
驻守直采项目约定第一通道的地址码为： b00001， 第二通道的地址码为： b00010，
第三通道的地址码为： b00011， 第四通道的地址码为： b00100。

双通道宽带模块项目约定可预装功分器的通道为第一通道， 另外一个为第二
通道。 第一通道地址码为： b00001， 第二通道的地址码为： b00010，

便携式测向项目约定本振、 校正功能模块的地址码为： b00001； 测向通道功
能模块的地址码为 b00010。
*/
#define RF_CHANNEL_1_ADDR 0x1  //驻守直采第一通道地址码
#define RF_CHANNEL_2_ADDR 0x2  //驻守直采第二通道地址码
#define RF_CHANNEL_3_ADDR 0x3  //驻守直采第三通道地址码
#define RF_CHANNEL_4_ADDR 0x4  //驻守直采第四通道地址码

#define RF_CHANNEL_CS 0x1   //通道选择码，约定所有直采通道的通道选择码N1N0=b01，双通道模块的的通道选择码N1N0=b01。

extern struct rf_ctx * rf_create_context(void);
#endif
