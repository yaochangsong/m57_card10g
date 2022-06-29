#ifndef _RF_TEST_H_
#define _RF_TEST_H_

#include "../../rf.h"

#ifndef MHZ
#define MHZ(x) ((long long)(x*1000000.0+0.5))
#endif
#ifndef GHZ
#define GHZ(x) ((long long)(x*1000000000.0+0.5))
#endif


#define RF_CMD_CODE_FREQUENCY 	0x01  //调节频率
#define RF_CMD_CODE_RF_GAIN		0x02  //射频衰减
#define RF_CMD_CODE_MF_GAIN		0x06  //中频衰减
#define RF_CMD_CODE_MODE 		0x04  //工作模式
#define RF_CMD_CODE_BW	 		0x03  //中频带宽
#define RF_CMD_CODE_MID_FREQ	0x07  //中频频率
#define RF_CMD_CODE_TEMPERATURE 0x41   //温度查询

#define RF_CMD_CODE_BIT_QUERY   0xff   //BIT查询
#define RF_CMD_CODE_HANDSHAKE   0xff   //握手
#define RF_CMD_CODE_CAL_SW	 	0xff  //校正-馈电开关
#define RF_CMD_CODE_WORK_PARA 	0xff  //工作参数查询

//与设置对应的查询命令
#define RF_QUERY_CMD_CODE_FREQUENCY 	0x10  //调节频率
#define RF_QUERY_CMD_CODE_RF_GAIN		0x11  //射频衰减
#define RF_QUERY_CMD_CODE_MF_GAIN		0x15  //中频衰减
#define RF_QUERY_CMD_CODE_MODE 		    0x13  //工作模式
#define RF_QUERY_CMD_CODE_BW	 		0x12  //中频带宽
#define RF_QUERY_CMD_CODE_MID_FREQ	    0x16  //中频频率

extern struct rf_ctx * rf_create_context(void);
#endif
