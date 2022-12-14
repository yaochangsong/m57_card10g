#include <linux/i2c.h>  
#include <linux/i2c-dev.h> 
#include <stdlib.h>  
#include <stdio.h>  
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <getopt.h>
#include <arpa/inet.h>
#include "nrs1800.h" 
#include "../../log/log.h"

static int i2c_fd = 0;
#define NSR1800_PORT_NUM 18
#define NSR1800_LANE_NUM 48

static srio_port_info ports_info[] = 
{
	{4, 4, 4, {16,17,18,19}, LANE_SPEED_5_G},
	{5, 5, 4, {20,21,22,23}, LANE_SPEED_5_G},
	{7, 7, 4, {28,29,30,31}, LANE_SPEED_5_G},
	{9, 9, 4, {36,37,38,39}, LANE_SPEED_5_G},
};

static srio_route_rule default_rules[] =
{
	{4,0x48,10},
	{5,0x48,10},
	{7,0x48,10},
	{9,0x48,10},
	{12,0x1e,10},
	{13,0x1e,10},
	{16,0x1e,10},
	{17,0x1e,10},
	{10,0x1a,12},
	{10,0x1c,13},
	{10,0x1b,16},
	{10,0x1d,17},
	{10,0x40,5},
};

static int nsr1800_10bit_read_i2c_register(int file, uint16_t dev_addr, uint32_t mem_addr, uint32_t *value, int len) 
{  
    int ret = 0;
    uint16_t read_num = 0;
    unsigned char outbuf[4] = {0};  

    struct i2c_rdwr_ioctl_data packets;  
    struct i2c_msg messages[2];  

	mem_addr = mem_addr >> 2;  //寄存器地址必须4字节对齐

    outbuf[0] = (mem_addr >> 24) & 0xFF;
	outbuf[0] |= 0x80;
    outbuf[1] = (mem_addr >> 16) & 0xFF;
    outbuf[2] = (mem_addr >> 8) & 0xFF;
    outbuf[3] = mem_addr & 0xFF;

    messages[0].addr  = dev_addr;  
	messages[0].flags = I2C_M_TEN;
    messages[0].len   = sizeof(outbuf);    
    messages[0].buf   = outbuf;  

	messages[1].addr  = dev_addr;  
	messages[1].flags = I2C_M_RD | I2C_M_TEN;  
    messages[1].len   = len;    
    messages[1].buf  = value;  

    packets.msgs  = messages;  
    packets.nmsgs = 2;  
    if(ioctl(file, I2C_RDWR, &packets) < 0) {  
        perror("Unable to send data\n");  
        ret = -1;
    }

    return ret;  
}

static int nsr1800_10bit_write_i2c_register(int file, uint16_t dev_addr, uint32_t mem_addr, uint32_t *value, int len) 
{  
    int ret = 0;
    unsigned char *outbuf = (unsigned char *)malloc(sizeof(unsigned char)*(len+4));  
    if(outbuf==NULL)  
    {  
        perror("MALLOC");  
        printf("malloc failed for i2c send buf\n");
        return -1;  
    }  
    struct i2c_rdwr_ioctl_data packets;  
    struct i2c_msg messages[1];  

	mem_addr = mem_addr >> 2;  //寄存器地址必须4字节对齐

    outbuf[0] = (mem_addr >> 24) & 0xFF;
    outbuf[1] = (mem_addr >> 16) & 0xFF;
    outbuf[2] = (mem_addr >> 8) & 0xFF;
    outbuf[3] = mem_addr & 0xFF;

    messages[0].addr  = dev_addr;  
	messages[0].flags = I2C_M_TEN;
    messages[0].len   = sizeof(outbuf);    
    messages[0].buf   = outbuf;  

    memcpy(outbuf+4, value, len);  

    packets.msgs  = messages;  
    packets.nmsgs = 1;  
    if(ioctl(file, I2C_RDWR, &packets) < 0) {  
        perror("Unable to send data\n");  
        ret = -1;
    }  
    if(outbuf){
        free(outbuf);
    }
    return ret;  
}

static int nsr1800_7bit_read_i2c_register(int file, uint8_t dev_addr, uint32_t mem_addr, uint32_t *value, int len) 
{  
    int ret = 0;
    uint16_t read_num = 0;
    unsigned char outbuf[4] = {0};  

    struct i2c_rdwr_ioctl_data packets;  
    struct i2c_msg messages[2];  

	mem_addr = mem_addr >> 2;  //寄存器地址必须4字节对齐

    outbuf[0] = (mem_addr >> 24) & 0xFF;
	outbuf[0] |= 0x80;
    outbuf[1] = (mem_addr >> 16) & 0xFF;
    outbuf[2] = (mem_addr >> 8) & 0xFF;
    outbuf[3] = mem_addr & 0xFF;

    messages[0].addr  = dev_addr;  
	messages[0].flags = 0;
    messages[0].len   = sizeof(outbuf);    
    messages[0].buf   = outbuf;  

	messages[1].addr  = dev_addr;  
	messages[1].flags = I2C_M_RD;  
    messages[1].len   = len;    
    messages[1].buf  = value;  

    packets.msgs  = messages;  
    packets.nmsgs = 2;  
    if(ioctl(file, I2C_RDWR, &packets) < 0) {  
        perror("Unable to send data\n");  
        ret = -1;
    }
    
	*value = htonl(*value);

    return ret;  
}

static int nsr1800_7bit_write_i2c_register(int file, uint8_t dev_addr, uint32_t mem_addr, uint32_t *value, int len) 
{  
    int ret = 0;
    uint32_t w_value = 0;
    
    unsigned char *outbuf = (unsigned char *)malloc(sizeof(unsigned char)*(len+4));  
    if(outbuf==NULL)  
    {  
        perror("MALLOC");  
        printf("malloc failed for i2c send buf\n");
        return -1;  
    }  
    struct i2c_rdwr_ioctl_data packets;  
    struct i2c_msg messages[1];  

	mem_addr = mem_addr >> 2;  //寄存器地址必须4字节对齐

    outbuf[0] = (mem_addr >> 24) & 0xFF;
    outbuf[1] = (mem_addr >> 16) & 0xFF;
    outbuf[2] = (mem_addr >> 8) & 0xFF;
    outbuf[3] = mem_addr & 0xFF;

    messages[0].addr  = dev_addr;  
	messages[0].flags = 0;
    messages[0].len   = sizeof(outbuf);    
    messages[0].buf   = outbuf;  
	w_value = htonl(*value);
	//w_value = (*value);
    memcpy(outbuf+4, &w_value, len);  

    packets.msgs  = messages;  
    packets.nmsgs = 1;  
    if(ioctl(file, I2C_RDWR, &packets) < 0) {  
        perror("Unable to send data\n");  
        ret = -1;
    }  
    if(outbuf){
        free(outbuf);
    }
    
    return ret;  
}

//in_port口收到destID的包转到out_port???
//直接配置路由???
//addr = 0xE10000 + (0x1000 * port_num) + (0x4 * DestID)  
//value = dest port
int nsr1800_set_router_rule(int file, uint8_t in_port, uint16_t destID, uint8_t out_port)
{
	uint32_t addr = 0xE10000;
	uint32_t value;

	addr = addr + 0x1000 * in_port + 0x4 * destID;
	value = out_port;
	if(!nsr1800_7bit_write_i2c_register(file, NSR1800_I2C_SLAVE_7BIT_ADDR, addr, &value, sizeof(value)))
	{
		printf_debug("write addr:0x%x value:0x%x\n", addr, value);
	}
	else
	{
		printf("error write addr:0x%x value:0x%x\n", addr, value);
		return -1;
	}

	return 0;
}

int nsr1800_set_router_rules(int file)
{
	int i = 0;
#if 0
	nsr1800_set_router_rule(file, 3, 0x48, 10);
	nsr1800_set_router_rule(file, 10, 0x40, 3);

#if 1
	nsr1800_set_router_rule(file, 5, 0x48, 10);
	//nsr1800_set_router_rule(file, 5, 0x48, 3);
	//nsr1800_set_router_rule(file, 4, 2, 3);

	//nsr1800_set_router_rule(file, 3, 1, 5);
	//nsr1800_set_router_rule(file, 9, 1, 4);	
	//nsr1800_set_router_rule(file, 7, 1, 10);
#else
	nsr1800_set_router_rule(file, 10, 2, 16);
	nsr1800_set_router_rule(file, 12, 2, 17);
	nsr1800_set_router_rule(file, 13, 2, 3);

	nsr1800_set_router_rule(file, 3, 1, 12);
	nsr1800_set_router_rule(file, 16, 1, 13); 
	nsr1800_set_router_rule(file, 17, 1, 10);
#endif
#endif

	for(i = 0; i < sizeof(default_rules)/sizeof(srio_route_rule); i++)
	{
		nsr1800_set_router_rule(file, default_rules[i].port_num, default_rules[i].dest_id, default_rules[i].dest_port);
	}

	return 0;
}

int nsr1800_set_port_speed(int file)
{
	int i, j;
	uint32_t addr = 0xE10000;
	uint32_t value;
	
	for(i = 0; i < sizeof(ports_info) / sizeof(srio_port_info); i++)
	{
		printf_debug("port_num%d\n", ports_info[i].port_num);
		addr=0xFF0000 + 0x10 * ports_info[i].pll_num;
		if(ports_info[i].speed == LANE_SPEED_3_125_G || ports_info[i].speed == LANE_SPEED_6_25_G)
			value=0x1;
		else
			value=0x0;
		if(!nsr1800_7bit_write_i2c_register(file, NSR1800_I2C_SLAVE_7BIT_ADDR, addr, &value, sizeof(value)))
		{
			printf_debug("write addr:0x%x value:0x%x\n", addr, value);
		}
		else
		{
			printf("error write addr:0x%x value:0x%x\n", addr, value);
			return -1;
		}

		for(j = 0; j < ports_info[i].lane_cnt; j++)
		{
			addr=0xFF8000 + 0x100 * ports_info[i].lanes[j];
			switch (ports_info[i].speed)
			{
				case LANE_SPEED_1_25_G:
				{
					value=0x0; 
					break;
				}
				case LANE_SPEED_2_5_G:
				case LANE_SPEED_3_125_G:
				{
					value=0xa; 
					break;
				}
				case LANE_SPEED_5_G:
				case LANE_SPEED_6_25_G:
				{
					value=0x14; 
					break;
				}
				default:
				printf("error invalid speed:%d\n", ports_info[i].speed);
				return -1;
				break;
			}
			
			if(!nsr1800_7bit_write_i2c_register(file, NSR1800_I2C_SLAVE_7BIT_ADDR, addr, &value, sizeof(value)))
			{
				printf_debug("write addr:0x%x value:0x%x\n", addr, value);
			}
			else
			{
				printf("error write addr:0x%x value:0x%x\n", addr, value);
				return -1;
			}			
		}
	}
	return 0;
}

static uint32_t getopt_integer(char *optarg)
{
	int rc;
	uint32_t value;
	rc = sscanf(optarg, "0x%x", &value);
	if (rc <= 0)
	rc = sscanf(optarg, "%ul", &value);
	//printf("sscanf() = %d, value = 0x%08x\n", rc, (unsigned int)value);
	return value;
}

/*
##关闭Idle2模式
w,0x158 + 0x20[18],  0x88000000  

##Serdes初始???      
w, 0xFF1000 + 0x10[12],0x1e1557a8
w, 0xFF1008 + 0x10[12],0x6a1b6400
w, 0xFF8028 + 0x100[48],0x00007800
w, 0xFF802C + 0x100[48],0x0004071f
w, 0xFF8034 + 0x100[48],0x9c422220
w, 0xFF8030 + 0x100[48],0x45400090

##其他配置
##       Route

##       Link timeout

##       Input Port Enable

##       Flow Control

##复位所有端???
w, 0xF20300,0xbfffffff
*/
int nr1800_reset_port_by_slot(int slot)
{
    struct _nr_1800_mapport_s{
        int slot;
        int port;
    }mapport[]={
        {5, 5},
        {4, 9},
        {3, 4},
        {2, 7}
    };
    uint32_t value = 0;
    uint32_t addr = 0xf20300;
//addr 0xf20300 -d 0x8000xxxx
    int port = -1;
    for(int i = 0; i < (sizeof(mapport)/sizeof(mapport[0])); i++){
        if(slot == mapport[i].slot){
            port = mapport[i].port;
            break;
        }
    }
    if(port == -1)
        return -1;
    value = 0x80000000|(1<<port);
    //printf_note("addr:0x%x, Write value:0x%x\n", addr, value);
    
    if(i2c_fd <= 0){
        i2c_fd = open("/dev/i2c-1", O_RDWR);
        if(i2c_fd < 0){  
            printf("####i2c device open failed####\n");  
            return (-1);  
        }
        if(ioctl(i2c_fd, I2C_RETRIES, 5) < 0) {  
            perror("Unable to set I2C_RETRIES\n");  
        }  

        if(ioctl(i2c_fd, I2C_TIMEOUT, 10) < 0) {  
            perror("Unable to set I2C_TIMEOUT\n");  
        }  
    }

    if(!nsr1800_7bit_write_i2c_register(i2c_fd, NSR1800_I2C_SLAVE_7BIT_ADDR, addr, &value, sizeof(value))){
        printf("write addr:0x%x value:0x%08x\n", addr, value);
    }
    else
        printf("error write addr:0x%x value:0x%x\n", addr, value);
    return 0;
}

int nsr1800_init(void)
{
	uint32_t addr = 0;
	uint32_t value = 0;
	int i = 0, j = 0;
	int file;

	printf("/usr/bin/sroute -c /etc/nr_config.json &");
	system("/usr/bin/sroute -c /etc/nr_config.json &");
	return 0;
#if 0
	printf_note("nsr1800 I2C init\n");

	file = open("/dev/i2c-1", O_RDWR);
	if(file < 0){  
		printf("####i2c device open failed####\n");  
		return (-1);  
	}

	printf_debug("**********disable Idle2 mode**********\n");
	//关闭Idle2模式	
	for(i = 0; i < NSR1800_PORT_NUM; i++)
	{
		addr = 0x158 + 0x20 * i;
		value = 0x88000000;
		if(!nsr1800_7bit_write_i2c_register(file, NSR1800_I2C_SLAVE_7BIT_ADDR, addr, &value, sizeof(value)))
		{
			printf_debug("write addr:0x%x value:0x%x\n", addr, value);
		}
		else
		{
			printf("error write addr:0x%x value:0x%x\n", addr, value);
			return -1;
		}
	}

	printf_debug("\n**********enable port input and output**********\n");
	for(i = 0; i < NSR1800_PORT_NUM; i++)
	{
		addr = 0x15c + 0x20 * i;
		if(!nsr1800_7bit_read_i2c_register(file, NSR1800_I2C_SLAVE_7BIT_ADDR, addr, &value, sizeof(value)))
		{
			printf_debug("read addr:0x%x value:0x%08x\n", addr, value);
		}
		else
		{
			printf("error read addr:0x%x\n", addr);
			return -1;
		}

		value |= 0x00600000;
		if(!nsr1800_7bit_write_i2c_register(file, NSR1800_I2C_SLAVE_7BIT_ADDR, addr, &value, sizeof(value)))
		{
			printf_debug("write addr:0x%x value:0x%x\n", addr, value);
		}
		else
		{
			printf("error write addr:0x%x value:0x%x\n", addr, value);
			return -1;
		}
	}

	printf_debug("\n**********init all lanes**********\n");
	//Serdes初始化
	for(i = 0; i < 12; i++)
	{
		addr = 0xFF1000 + 0x10 * i;
		value = 0x1e1557a8;
		if(!nsr1800_7bit_write_i2c_register(file, NSR1800_I2C_SLAVE_7BIT_ADDR, addr, &value, sizeof(value)))
		{
			printf_debug("write addr:0x%x value:0x%x\n", addr, value);
		}
		else
		{
			printf("error write addr:0x%x value:0x%x\n", addr, value);
			return -1;
		}
	}	

	for(i = 0; i < 12; i++)
	{
		addr = 0xFF1008 + 0x10 * i;
		value = 0x6a1b6400;
		if(!nsr1800_7bit_write_i2c_register(file, NSR1800_I2C_SLAVE_7BIT_ADDR, addr, &value, sizeof(value)))
		{
			printf_debug("write addr:0x%x value:0x%x\n", addr, value);
		}
		else
		{
			printf("error write addr:0x%x value:0x%x\n", addr, value);
			return -1;
		}
	}		

	for(i = 0; i < NSR1800_LANE_NUM; i++)
	{
		addr = 0xFF8028 + 0x100 * i;
		value = 0x00007800;
		if(!nsr1800_7bit_write_i2c_register(file, NSR1800_I2C_SLAVE_7BIT_ADDR, addr, &value, sizeof(value)))
		{
			printf_debug("write addr:0x%x value:0x%x\n", addr, value);
		}
		else
		{
			printf("error write addr:0x%x value:0x%x\n", addr, value);
			return -1;
		}
	}		

	for(i = 0; i < NSR1800_LANE_NUM; i++)
	{
		addr = 0xFF802C + 0x100 * i;
		value = 0x0004071f;
		if(!nsr1800_7bit_write_i2c_register(file, NSR1800_I2C_SLAVE_7BIT_ADDR, addr, &value, sizeof(value)))
		{
			printf_debug("write addr:0x%x value:0x%x\n", addr, value);
		}
		else
		{
			printf("error write addr:0x%x value:0x%x\n", addr, value);
			return -1;
		}
	}		

	for(i = 0; i < NSR1800_LANE_NUM; i++)
	{
		addr = 0xFF8034 + 0x100 * i;
		value = 0x9c422220;
		if(!nsr1800_7bit_write_i2c_register(file, NSR1800_I2C_SLAVE_7BIT_ADDR, addr, &value, sizeof(value)))
		{
			printf_debug("write addr:0x%x value:0x%x\n", addr, value);
		}
		else
		{
			printf("error write addr:0x%x value:0x%x\n", addr, value);
			return -1;
		}
	}		
	
	for(i = 0; i < NSR1800_LANE_NUM; i++)
	{
		addr = 0xFF8030 + 0x100 * i;
		value = 0x45400090;
		if(!nsr1800_7bit_write_i2c_register(file, NSR1800_I2C_SLAVE_7BIT_ADDR, addr, &value, sizeof(value)))
		{
			printf_debug("write addr:0x%x value:0x%x\n", addr, value);
		}
		else
		{
			printf("error write addr:0x%x value:0x%x\n", addr, value);
			return -1;
		}
	}		

	printf_debug("\n**********set route rule**********\n");
	nsr1800_set_router_rules(file);

	printf_debug("\n**********set port speed**********\n");
#if 0	
	addr=0xFF0050;
	value=0x1;
	nsr1800_7bit_write_i2c_register(file, NSR1800_I2C_SLAVE_7BIT_ADDR, addr, &value, sizeof(value));
	addr=0xFF9400;
	value=0xa;
	nsr1800_7bit_write_i2c_register(file, NSR1800_I2C_SLAVE_7BIT_ADDR, addr, &value, sizeof(value));
	addr=0xFF9500;
	value=0xa;
	nsr1800_7bit_write_i2c_register(file, NSR1800_I2C_SLAVE_7BIT_ADDR, addr, &value, sizeof(value));
	addr=0xFF9600;
	value=0xa;
	nsr1800_7bit_write_i2c_register(file, NSR1800_I2C_SLAVE_7BIT_ADDR, addr, &value, sizeof(value));
	addr=0xFF9700;
	value=0xa;
	nsr1800_7bit_write_i2c_register(file, NSR1800_I2C_SLAVE_7BIT_ADDR, addr, &value, sizeof(value));
#else
#if 1
	nsr1800_set_port_speed(file);
#else
/*
	该字段和 PLL{0..11} Control 1 Register.PLL_DIV_SEL 一起决定
	了通道的接收速率。
	PLL_DIV_SEL = 0
	0b00 = 1.25 Gbaud
	0b01 = 2.5 Gbaud
	0b1X = 5.0 Gbaud
	PLL_DIV_SEL = 1
	0b00 = RES
	0b01 = 3.125 Gbaud
	0b1X = 6.25 Gbaud
*/
	for(i = 0; i < sizeof(ports_info) / sizeof(srio_port_info); i++)
	{
		printf("port_num%d\n", ports_info[i].port_num);
		addr=0xFF0000 + 0x10 * ports_info[i].pll_num;
		if(ports_info[i].speed == LANE_SPEED_3_125_G || ports_info[i].speed == LANE_SPEED_6_25_G)
			value=0x1;
		else
			value=0x0;
		if(!nsr1800_7bit_write_i2c_register(file, NSR1800_I2C_SLAVE_7BIT_ADDR, addr, &value, sizeof(value)))
		{
			printf("write addr:0x%x value:0x%x\n", addr, value);
		}
		else
		{
			printf("error write addr:0x%x value:0x%x\n", addr, value);
			return -1;
		}

		for(j = 0; j < ports_info[i].lane_cnt; j++)
		{
			addr=0xFF8000 + 0x100 * ports_info[i].lanes[j];
			switch (ports_info[i].speed)
			{
				case LANE_SPEED_1_25_G:
				{
					value=0x0; 
					break;
				}
				case LANE_SPEED_2_5_G:
				case LANE_SPEED_3_125_G:
				{
					value=0xa; 
					break;
				}
				case LANE_SPEED_5_G:
				case LANE_SPEED_6_25_G:
				{
					value=0x14; 
					break;
				}
				default:
				printf("error invalid speed:%d\n", ports_info[i].speed);
				return -1;
				break;
			}
			
			if(!nsr1800_7bit_write_i2c_register(file, NSR1800_I2C_SLAVE_7BIT_ADDR, addr, &value, sizeof(value)))
			{
				printf("write addr:0x%x value:0x%x\n", addr, value);
			}
			else
			{
				printf("error write addr:0x%x value:0x%x\n", addr, value);
				return -1;
			}			
		}
	}
#endif
#endif	
	printf_debug("\n**********reset all port**********\n");
	//复位所有端口
	addr = 0xF20300;
	value = 0xbfffffff;
	if(!nsr1800_7bit_write_i2c_register(file, NSR1800_I2C_SLAVE_7BIT_ADDR, addr, &value, sizeof(value)))
	{
		printf_debug("write addr:0x%x value:0x%x\n", addr, value);
	}
	else
	{
		printf("error write addr:0x%x value:0x%x\n", addr, value);
		return -1;
	}
	close(file);
#endif
	return 0;
}

int nsr1800_soft_reset(int file)
{
	uint32_t addr = 0;
	uint32_t value = 0;

	printf_debug("\n**********nsr1800 soft reset**********\n");
	
	addr = 0xF20040;
	value = 0x00030097;
	if(!nsr1800_7bit_write_i2c_register(file, NSR1800_I2C_SLAVE_7BIT_ADDR, addr, &value, sizeof(value)))
	{
		printf_debug("write addr:0x%x value:0x%x\n", addr, value);
	}
	else
	{
		printf("error write addr:0x%x value:0x%x\n", addr, value);
		return -1;
	}

	return 0;
}

#if 0
static int main_test(int argc, char *argv[])
{
	int cmd_opt;
	int i2c_fd = 0;
	uint32_t addr = 0;
	uint32_t value = 0;
	int isWrite = 0;
	int retry = 3;
	int timeout = 100;
	int init = 0;
	int reset = 0;
	
    printf("nsr1800 I2C init\n");  
    i2c_fd = open("/dev/i2c-1", O_RDWR);
    if(i2c_fd < 0){  
        printf("####i2c device open failed####\n");  
        return (-1);  
    }

	while ((cmd_opt = getopt_long(argc, argv, "a:d:ir", NULL, NULL)) != -1)
	{
		switch (cmd_opt)
		{
		  case 'i':
		    init = 1;
		    break;		
		  case 'r':
		    reset = 1;
		    break;	
		  case 'a':
		    addr = getopt_integer(optarg);
		    break;
		  case 'd':
		  	isWrite = 1;
		    value = getopt_integer(optarg);
		    break;
		  default:
		    exit(0);
		    break;
		}
	}
#if 1
    if(ioctl(i2c_fd, I2C_RETRIES, 5) < 0) {  
        perror("Unable to set I2C_RETRIES\n");  
    }  

    if(ioctl(i2c_fd, I2C_TIMEOUT, 10) < 0) {  
        perror("Unable to set I2C_TIMEOUT\n");  
    }  
#endif

	if(init)
	{
		if(nsr1800_init(i2c_fd))
		{
    		printf("error nsr1800 init failed\n");
		}
		close(i2c_fd);
    	return 0;
	}

	if(reset)
	{
		if(nsr1800_soft_reset(i2c_fd))
		{
    		printf("error nsr1800 reset port failed\n");
		}
		close(i2c_fd);
    	return 0;
	}
	
	if(isWrite)
	{
    	if(!nsr1800_7bit_write_i2c_register(i2c_fd, NSR1800_I2C_SLAVE_7BIT_ADDR, addr, &value, sizeof(value)))
    	{
			printf("write addr:0x%x value:0x%08x\n", addr, value);
    	}
    	else
    		printf("error write addr:0x%x value:0x%x\n", addr, value);
    }
    else
    {
		if(!nsr1800_7bit_read_i2c_register(i2c_fd, NSR1800_I2C_SLAVE_7BIT_ADDR, addr, &value, sizeof(value)))
		{
			printf("read addr:0x%x value:0x%08x\n", addr, value);
		}
		else
			printf("error read addr:0x%x\n", addr);
	}
	
    close(i2c_fd);
    return 0;
}
#endif

