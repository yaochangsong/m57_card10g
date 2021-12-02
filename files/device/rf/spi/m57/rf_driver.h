#ifndef _RF_TEST_H_
#define _RF_TEST_H_

#include "../../rf.h"

#ifndef MHZ
#define MHZ(x) ((long long)(x*1000000.0+0.5))
#endif
#ifndef GHZ
#define GHZ(x) ((long long)(x*1000000000.0+0.5))
#endif

//模块类型标识码
#define MODE_CODE_CDB           0xd5 //b11010101  超短波
#define MODE_CODE_LF            0xa2 //b10100010  L变频模块
#define MODE_CODE_IF            0x9a //b10011010  中频模块
#define MODE_CODE_CLK           0x94 //b10010100  时钟卡
#define MODE_CODE_RF            0xdd //b11011101  射频中频模块

//指令码
#define INS_CODE_FREQ           0x0  //模块调谐频率控制指令
#define INS_CODE_RF_GAIN        0x1  //模块射频衰减/增益控制指令
#define INS_CODE_IF_GAIN        0x2  //模块中频衰减/增益控制指令
#define INS_CODE_WORK_MODE      0x3  //模块工作模式控制指令
#define INS_CODE_MODULE_ADDR    0x82 //模块类型和地址查询指令
#define INS_CODE_IDENTITY       0x81 //身份查询控制指令

//EX_ICMD
#define EX_ICMD_NEED_RESP   0x00
#define EX_ICMD_NO_RESP     0xff

#define RF_PROTOCOL_VERSION_TO_STR(str, _hex) (sprintf(str, "v%d.%d.%d", (_hex>>6) &0x03, (_hex>>4) &0x03, _hex &0x0f))
#define RF_FATHER_VERSION_TO_STR(str, _hex) (sprintf(str, "v%d.%d", (_hex>>4) &0x0f, _hex &0x0f))
#define RF_FW_VERSION_TO_STR(str, _hex) (sprintf(str, "v%d.%d", (_hex>>4) &0x0f, _hex &0x0f))
#define RF_MAXCLK_TO_STR(str, _hex) (sprintf(str, "%dMhz", _hex))





struct rf_identity_info_t{
    uint8_t check_sum;
    uint8_t addr_ch;
    uint32_t serial_num: 24;
    uint32_t type_code: 8;
    uint8_t vender;
    uint8_t max_clk_mhz;
    uint8_t fw_version;
    uint8_t father_version;
    uint8_t protocol_version;
    uint8_t resv;
}__attribute__ ((packed));


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

