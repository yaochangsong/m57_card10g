#ifndef _DWIN_SS_K600_H
#define _DWIN_SS_K600_H


/* The secreen address Type */



#define SERIAL_SCREEN_STAT_FLAG  0x5aa5
#define SERIAL_SCREEN_HEAD_LEN   3

#define GET_HIGH_8_BIT_16BIT(x) ((x & 0xffff) >> 8)
#define GET_LOW_8_BIT_16BIT(x)  (x & 0x0ff)
#define COMPOSE_16BYTE_BY_8BIT(high, low)   ((high << 8) | (low))
#define COMPOSE_32BYTE_BY_16BIT(high, low)   ((high << 16) | (low))
#define INT_TO_CHAR(i, c1,c2,c3,c4)         (c1 = (i >> 24)&0xff, c2 = (i >> 16)&0xff, c3 = (i >> 8)&0xff,c4 = i&0xff)
#define FLOAT_SET_3_PRESION(f) (( (float)( (int)( (f+0.0005)*1000 ) ) )/1000);
#ifdef SUPPORT_UART
#define  send_data_by_serial(buf, len)    uart1_send_data(buf, len)#endif

#define SCREEN_BUTTON_PRESS_DOWN 1

//#define  LOCAL_CONTROL_MODE()     (serial_screen_get_control_mode() == SCREEN_CTRL_LOCAL)
//#define  REMOTE_CONTROL_MODE()  (serial_screen_get_control_mode() == SCREEN_CTRL_REMOTE)
#include "config.h"

typedef enum _SCREEN_ADDR_RW_TYPE {
    WRITE_SCREEN_ADDR = 0x82,
    READ_SCREEN_ADDR = 0x83,
}SCREEN_ADDR_RW_TYPE;

typedef enum _SCREEN_ADDR_TYPE {
    SCREEN_COMMON_DATA1 = 0x00,
    SCREEN_CHANNEL1_DATA = 0x01,
    SCREEN_CHANNEL2_DATA = 0x02,
    SCREEN_CHANNEL3_DATA = 0x03,
    SCREEN_CHANNEL4_DATA = 0x04,
    SCREEN_CHANNEL5_DATA = 0x05,
    SCREEN_CHANNEL6_DATA = 0x06,
    SCREEN_CHANNEL7_DATA = 0x07,
    SCREEN_CHANNEL8_DATA = 0x08,
    SCREEN_COMMON_DATA2  = 0x10,
}SCREEN_ADDR_TYPE;

typedef enum _SCREEN_CHANNEL_ADDR_TYPE {
    SCREEN_CHANNEL_BW      = 0x00,
     SCREEN_CHANNEL_RF_BW      = 0x01,    //SCREEN_CHANNEL_NOISE_EN = 0x01,
    SCREEN_CHANNEL_MODE    = 0x02,
    SCREEN_CHANNEL_DECODE_TYPE = 0x03,
    
    SCREEN_CHANNEL_FRQ     = 0x04,
    SCREEN_CHANNEL_FRQ1    = 0x04,
    SCREEN_CHANNEL_FRQ2    = 0x05,
    SCREEN_CHANNEL_VOLUME  = 0x10,
    SCREEN_CHANNEL_MGC     = 0x11,
    SCREEN_CHANNEL_AU_CTRL = 0x12,
    SCREEN_CHANNEL_AU_BUTTON_CTRL  = 0x13,
}SCREEN_CHANNEL_ADDR_TYPE;


typedef enum _SCREEN_COMMON_DATA1_SUB_ADDR_TYPE {	
	SCREEN_CTRL_TYPE  = 0x00,
        
    SCREEN_IPADDR     = 0x01,
	SCREEN_IPADDR1    = 0x01,
	SCREEN_IPADDR2    = 0x02,
	SCREEN_IPADDR3    = 0x03,
	SCREEN_IPADDR4    = 0x04,
	
	SCREEN_NETMASK_ADDR     = 0x05,
	SCREEN_NETMASK_ADDR1    = 0x05,
	SCREEN_NETMASK_ADDR2    = 0x06,
	SCREEN_NETMASK_ADDR3    = 0x07,
	SCREEN_NETMASK_ADDR4    = 0x08,
	
	SCREEN_GATEWAY_ADDR     = 0x09,
	SCREEN_GATEWAY_ADDR1    = 0x09,
	SCREEN_GATEWAY_ADDR2    = 0x0a,
	SCREEN_GATEWAY_ADDR3    = 0x0b,
	SCREEN_GATEWAY_ADDR4    = 0x0c,
	
	SCREEN_PORT             = 0x0d,
}SCREEN_COMMON_DATA1_SUB_ADDR_TYPE;
	

typedef enum _SCREEN_COMMON_DATA2_SUB_ADDR_TYPE {	
	SCREEN_LED1_STATUS    = 0x00,
	SCREEN_LED2_STATUS    = 0x01,
	SCREEN_LED3_STATUS    = 0x02,
	SCREEN_LED4_STATUS    = 0x03,
	SCREEN_LED5_STATUS    = 0x04,
	SCREEN_LED6_STATUS    = 0x05,
	SCREEN_LED7_STATUS    = 0x06,
	SCREEN_LED8_STATUS    = 0x07,
	SCREEN_LED_CLK_STATUS    = 0x08,
	SCREEN_LED_AD_STATUS    = 0x09,
	SCREEN_CPU_STATUS         = 0x0a,
	SCREEN_FPGA_STATUS        = 0x0b,
	SCREEN_CPU_DATA_STATUS    = 0x0c,
	SCREEN_FPGA_DATA_STATUS   = 0x0d,
}SCREEN_COMMON_DATA2_SUB_ADDR_TYPE;


typedef enum _SCREEN_BW_PARA {	
	SCREEN_BW_0    = 0,
	SCREEN_BW_1    = 1,
	SCREEN_BW_2    = 2,
	SCREEN_BW_3      = 3,
	SCREEN_BW_4      = 4,
	SCREEN_BW_5      = 5,
	SCREEN_BW_6     = 6,
	SCREEN_BW_7     = 7,
	SCREEN_BW_8     = 8,
	SCREEN_BW_9     = 9,
	SCREEN_BW_10     = 10,
	SCREEN_BW_11     = 11,
	SCREEN_BW_12     = 12,
	SCREEN_BW_13     = 13,
}SCREEN_BW_PARA;

typedef enum _SCREEN_NET_CTRL_MODE_PARA {	
	SCREEN_CTRL_LOCAL   = 0,
	SCREEN_CTRL_REMOTE  = 1,
}SCREEN_NET_CTRL_MODE_PARA;


typedef enum _SCREEN_NOISE_PARA {	
	SCREEN_NOISE_OFF   = 0,
	SCREEN_NOISE_ON    = 1,
}SCREEN_NOISE_PARA;


typedef enum _SCREEN_MODE_PARA {	
	SCREEN_MODE_LOWDIS    = 0,
	SCREEN_MODE_NORMAL    = 1,
	SCREEN_MODE_RECEIVE   = 2,
}SCREEN_MODE_PARA;

typedef enum _SCREEN_DECODE_PARA {	
	SCREEN_DECODE_WFM   = 0,
	SCREEN_DECODE_FM    = 1,
	SCREEN_DECODE_AM    = 2,
    SCREEN_DECODE_CW    = 3,
	SCREEN_DECODE_LSB   = 4,
	SCREEN_DECODE_USB   = 5,
}SCREEN_DECODE_PARA;


typedef struct  _SCREEN_BW_PARA_ST{
    uint8_t type;
    uint32_t idata;//float fdata;  //Hz
}__attribute__ ((packed)) SCREEN_BW_PARA_ST; 


typedef struct  _SCREEN_READ_CONTROL_ST{
    uint16_t start_flag;
    uint8_t len;
    uint8_t type;  /* 0x82: write; 0x83: read */
    uint16_t addr;  
    uint8_t datanum;
    uint16_t data[4];
}__attribute__ ((packed)) SCREEN_READ_CONTROL_ST;      

typedef union  _SCREEN_SEND_V_DATA{
    uint8_t datanum;
    uint16_t data[4];
}__attribute__ ((packed)) SCREEN_SEND_V_DATA;


typedef struct  _SCREEN_SEND_CONTROL_ST{
    uint16_t start_flag;
    uint8_t len;
    uint8_t type;  /* 0x82: write; 0x83: read */
    uint16_t addr;  
    SCREEN_SEND_V_DATA  payload;
}__attribute__ ((packed)) SCREEN_SEND_CONTROL_ST;     

typedef struct  _SCREEN_CH_CTRL_DATA_ST{
    uint8_t audio_enable[MAX_RADIO_CHANNEL_NUM];
    uint8_t audio_volume[MAX_RADIO_CHANNEL_NUM];
//    FIXED_FREQ_ANYS_D_PARAM_ST decode[MAX_CHANNEL_NUM];
//    DEVICE_RF_INFO_ST rf[MAX_CHANNEL_NUM];
}__attribute__ ((packed)) SCREEN_CH_CTRL_DATA_ST;     

typedef struct  _SCREEN_NET_CTRL_DATA_ST{
    volatile uint8_t ctrl_mode;
    uint32_t ipaddress;
    uint32_t netmask;
    uint32_t gateway;
    uint16_t port;
}__attribute__ ((packed)) SCREEN_NET_CTRL_DATA_ST; 


extern int8_t k600_receive_write_data_from_user(uint8_t data_type, uint8_t data_cmd, void *pdata);
extern int8_t k600_send_data_to_user(uint8_t *pdata, int32_t total_len);
#endif
