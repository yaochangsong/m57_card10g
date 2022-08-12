#ifndef _NSR1800_H_H
#define _NSR1800_H_H

#define NSR1800_I2C_SLAVE_10BIT_ADDR   0x1F
#define NSR1800_I2C_SLAVE_7BIT_ADDR   0x1

/* Software Reset */
#define NSR1800CMD_RESET         0xFE
#define NSR1800_MAX_LANE_NUM     4


typedef enum _srio_srio_speed_{
    LANE_SPEED_1_25_G,
	LANE_SPEED_2_5_G,
	LANE_SPEED_5_G,	
	LANE_SPEED_3_125_G,
	LANE_SPEED_6_25_G,    
}srio_srio_speed;

/*
PLL_DIV_SEL = 0
0b00 = 1.25 Gbaud
0b01 = 2.5 Gbaud
0b1X = 5.0 Gbaud
PLL_DIV_SEL = 1
0b00 = RES
0b01 = 3.125 Gbaud
0b1X = 6.25 Gbaud

typedef enum _srio_srio_speed_{
    LANE_SPEED_1_25_G = 0x0,
	LANE_SPEED_2_5_G = 	0x01,
	LANE_SPEED_5_G = 	0x2,	
	LANE_SPEED_3_125_G=0x01,
	LANE_SPEED_6_25_G = 0x2,    
}srio_srio_speed;
*/

typedef struct _srio_port_info_{
	int port_num;  //端口号
	int pll_num;   //对应的PLL编号
	int lane_cnt;  //映射的lane数量
	int lanes[NSR1800_MAX_LANE_NUM];  //对应的LAN编号
	int speed;    //速率
}srio_port_info;

typedef struct _srio_route_rule_{
	int port_num;  //端口号
	int dest_id;   //目的设备ID
	int dest_port;  //目的端口
}srio_route_rule;

extern char *get_config_path(void);

#endif

