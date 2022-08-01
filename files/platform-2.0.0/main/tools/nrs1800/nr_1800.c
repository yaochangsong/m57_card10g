#include <linux/i2c.h>  
#include <linux/i2c-dev.h> 
#include <stdlib.h>  
#include <stdio.h>  
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <getopt.h>
#include <arpa/inet.h>
#include "nr_1800.h" 
#include "nr_config.h" 
#include "../../dao/json/cJSON.h"

//#include "../../log/log.h"

extern void nr_config_init(void);

#define NSR1800_PORT_NUM 18
#define NSR1800_LANE_NUM 48

static srio_port_info ports_info[] = 
{
	{4, 4, 4, {16,17,18,19}, LANE_SPEED_5_G},
	{5, 5, 4, {20,21,22,23}, LANE_SPEED_5_G},
	{7, 7, 4, {28,29,30,31}, LANE_SPEED_5_G},
	{9, 9, 4, {36,37,38,39}, LANE_SPEED_5_G},
 	{3, 3, 4, {12,13,14,15}, LANE_SPEED_6_25_G},
	{10, 10, 4, {40,41,42,43}, LANE_SPEED_6_25_G},
};

static srio_route_rule default_rules[] =
{
	{-1,0x1A,12},
	{-1,0x1B,16},
	{-1,0x1C,13},
	{-1,0x1D,17},
	{-1,0x1E,10},
	{-1,0x40,5},
	{-1,0x48,10},
};

static int nsr1800_10bit_read_i2c_register(int file, uint16_t dev_addr, uint32_t mem_addr, uint32_t *value, int len) 
{  
    int ret = 0;
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
    messages[1].buf  = (void *)value;  

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
    messages[1].buf  = (void *)value;  

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

//in_port口收到destID的包转到out_port�?
//直接配置路由�?
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

int nsr1800_set_broadcast_router_rule(int file, uint16_t destID, uint8_t out_port)
{
	uint32_t addr = 0xE00000;
	uint32_t value;

	addr = addr + 0x4 * destID;
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
	srio_route_rule  **table;
	size_t size = 0;
	table = nr_get_route_table(&size);
	if(size == 0 || table == NULL){
		for(i = 0; i < sizeof(default_rules)/sizeof(srio_route_rule); i++)
		{
			if(default_rules[i].port_num != -1)
				nsr1800_set_router_rule(file, default_rules[i].port_num, default_rules[i].dest_id, default_rules[i].dest_port);
			else
            	nsr1800_set_broadcast_router_rule(file, default_rules[i].dest_id, default_rules[i].dest_port);
		}
	} else {
		for(int i = 0; i < size; i++){
			//printf("==>dest_id=%d, dest_port=%d, port_num=%d\n", table[i]->dest_id, table[i]->dest_port, table[i]->port_num);
			if(table[i]->port_num != -1)
				nsr1800_set_router_rule(file, table[i]->port_num, table[i]->dest_id, table[i]->dest_port);
			else
            	nsr1800_set_broadcast_router_rule(file, table[i]->dest_id, table[i]->dest_port);
		}
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

##Serdes初始�?      
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

##复位所有端�?
w, 0xF20300,0xbfffffff
*/

int nsr1800_init(int file)
{
	uint32_t addr = 0;
	uint32_t value = 0;
	int i = 0;

	printf("nsr1800 I2C init\n");
    #if 0
	file = open("/dev/i2c-0", O_RDWR);
	if(file < 0){  
		printf("####i2c device open failed####\n");  
		return (-1);  
	}
    #endif

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

	printf_debug("\n**********enable port counter**********\n");
	for(i = 0; i < NSR1800_PORT_NUM; i++)
	{
		addr = 0xF40004 + 0x100 * i;
		if(!nsr1800_7bit_read_i2c_register(file, NSR1800_I2C_SLAVE_7BIT_ADDR, addr, &value, sizeof(value)))
		{
			printf_debug("read addr:0x%x value:0x%08x\n", addr, value);
		}
		else
		{
			printf("error read addr:0x%x\n", addr);
			return -1;
		}

		value |= 0x04000000;
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
		value = 0x94422220;
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
	nsr1800_set_port_speed(file);
#if 1
    printf_debug("\n**********set Link timeout**********\n");
	addr = 0x120;
	value = 0x8e00;
	if(!nsr1800_7bit_write_i2c_register(file, NSR1800_I2C_SLAVE_7BIT_ADDR, addr, &value, sizeof(value)))
	{
		printf_debug("write addr:0x%x value:0x%x\n", addr, value);
	}
	else
	{
		printf("error write addr:0x%x value:0x%x\n", addr, value);
		return -1;
	}
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


char *config_path = NULL;
char *get_config_path(void)
{
	return config_path;
}

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [option]\n"
        "          -i nrs1800 init [default]\n"
        "          -a nrs1800 address \n"
        "          -r read nrs1800 register[ -a 0x0000 -r]\n"
        "          -w write nrs1800 register value [-a 0x0000 -w 0xff] \n"
        "          -c router config file #[default: nr_config.json]\n"
        "          -s reset nrs1800  \n", prog);
    exit(1);
}

int main(int argc, char *argv[])
{
    int cmd_opt;
    int init = 0, reset = 0, iswrite = 0, isread = 0;
    int file = -1, ret = -1, is_addr = 0;
    uint32_t addr = 0, value = 0;

    while ((cmd_opt = getopt_long(argc, argv, "c:a:rwshi", NULL, NULL)) != -1)
    {
        switch (cmd_opt)
        {
            case 'c':
                config_path = strdup(optarg);
                break;
            case 'a':
                addr = getopt_integer(optarg);
                is_addr = 1;
                break;
            case 'r':
                isread = 1;
                break;
            case 'w':
                iswrite = 1;
                break;
            case 's':
                reset = 1;
                break;
            case 'h':
                usage(argv[0]);
                exit(0);
            case 'i':
            default:
                init = 1;
                break;
        }
    }

    char *i2c_file = "/dev/i2c-0";
    file = open(i2c_file, O_RDWR);
    if(file < 0){  
        printf("####i2c device[%s] open failed####\n", i2c_file);  
        exit(1);
    }
    if(init){
        nr_config_init();
        nsr1800_init(file);
    } else if(reset){
        if(nsr1800_soft_reset(file) == -1){
            printf("error nsr1800 reset port failed\n");
            goto failed_exit;
        }
        printf("Reset nrs1800 OK\n");
    } else if(isread && is_addr != 0){
        if(!nsr1800_7bit_read_i2c_register(file, NSR1800_I2C_SLAVE_7BIT_ADDR, addr, &value, sizeof(value))){
            printf("read addr:0x%x value:0x%08x ok\n", addr, value);
        }
        else{
            printf("error read addr:0x%x\n", addr);
            goto failed_exit;
        }
    } else if(iswrite && is_addr != 0){
        if(!nsr1800_7bit_write_i2c_register(file, NSR1800_I2C_SLAVE_7BIT_ADDR, addr, &value, sizeof(value))){
            printf("write addr:0x%x value:0x%08x ok\n", addr, value);
        }
        else{
            printf("error write addr:0x%x value:0x%x\n", addr, value);
            goto failed_exit;
        }
     } else {
        printf("Param error\n");
     }
    ret = 0;
failed_exit:
    close(file);
    return ret;
}
