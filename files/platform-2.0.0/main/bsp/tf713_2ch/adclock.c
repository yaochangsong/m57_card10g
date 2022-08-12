#include "config.h"

#define BASE_ADDR           0xb0001000//0xa0001000
#define SPI_RX_FIFO_ADDR    0x20
#define SPI_TX_FIFO_ADDR    0x24
#define SPI_STATE_ADDR      0x28
#define SPI_CTRL_ADDR       0x2c
#define SPI_BAND_ADDR       0x30
#define SPI_CS_ADDR         0x34
#define RESET_ADDR          0x84
#define DC_ADDR             0x8c
#define STATE_ADDR          0xc0

//#define REG_WRITE(base, data) (*(volatile uint32_t *)(base) = data, usleep(100))
#define REG_WRITE(base, data) (*(volatile uint32_t *)(base) = data, usleep(100))
#define REG_READ(base)  (*(volatile uint32_t *)(base))

uint8_t apb_i2c_write(intptr_t base,uint8_t data)
{
	uint8_t tmp;
	REG_WRITE(base + 0x04,data);
	REG_WRITE(base + 0x0c,0x04);
	while(REG_READ(base + 0x08) & 0x02)usleep(10);
	tmp = REG_READ(base + 0x08)&0x01;
	return tmp;
}

uint8_t apb_i2c_read(intptr_t base,uint8_t ack)
{
	uint8_t tmp;
	if(ack)
		REG_WRITE(base + 0x0c,0x03 | 0x08);
	else
		REG_WRITE(base + 0x0c,0x03);
	while(REG_READ(base + 0x08) & 0x02)usleep(10);
	tmp = REG_READ(base + 0x00);
	return tmp;
}

void apb_i2c_start(intptr_t base)
{
	REG_WRITE(base + 0x0c,0x01);
	while(REG_READ(base + 0x08) & 0x02)usleep(10);
}

void apb_i2c_stop(intptr_t base)
{
	REG_WRITE(base + 0x0c,0x02);
	while(REG_READ(base + 0x08) & 0x02)usleep(10);
}

uint8_t i2c_write(intptr_t base_addr,uint8_t saddr,uint16_t addr,uint8_t alen,uint8_t data)
{
    uint8_t wbuf[4];
    uint8_t i;
    uint8_t status;
    wbuf[0] = saddr&0xfe;
    if(alen>1)
    {
    	wbuf[1] = (addr&0xff00)>>8;
    	wbuf[2] = addr&0xff;
        wbuf[3] = data;
    }
    else
    {
    	wbuf[1] = addr&0xff;
    	wbuf[2] = data;
    }
    status = 0;
    apb_i2c_start(base_addr);
    for(i=0;i<(alen+2);i++)
    {
    	status += apb_i2c_write(base_addr,wbuf[i]);
    }
    apb_i2c_stop(base_addr);
    return status;
}

uint8_t i2c_read(intptr_t base_addr,uint8_t saddr,uint16_t addr,uint8_t alen,uint8_t *data)
{
    uint8_t wbuf[3];
    uint8_t i;
    uint8_t status;
    wbuf[0] = saddr&0xfe;
    if(alen>1)
    {
    	wbuf[1] = (addr&0xff00)>>8;
    	wbuf[2] = addr&0xff;
    }
    else
    {
    	wbuf[1] = addr&0xff;
    }
    status = 0;
    apb_i2c_start(base_addr);
    for(i=0;i<(alen+1);i++)
    {
    	status +=apb_i2c_write(base_addr,wbuf[i]);
    }
    apb_i2c_start(base_addr);
    apb_i2c_write(base_addr,wbuf[0]+1);
    *data = apb_i2c_read(base_addr,1);
    apb_i2c_stop(base_addr);
    return status;
}

static void apb_spi_setcs(volatile char * base,uint8_t data)
{
    REG_WRITE(base + SPI_CS_ADDR,data);
}

static uint8_t apb_spi_write(volatile char * base,uint8_t *snd_buf,uint8_t *rcv_buf,uint8_t len)
{
    uint8_t i;
    for(i=0;i<len;i++)
    {
        REG_WRITE(base + SPI_TX_FIFO_ADDR,*snd_buf++);
    }
    for(i=0;i<len;i++)
    {
        while(REG_READ(base + SPI_STATE_ADDR) & 0x01)//empty
        {
            usleep(10);
        }
        *rcv_buf++ = REG_READ(base + SPI_RX_FIFO_ADDR);
    }
    return 0;
}

static uint8_t spi_wrapper(volatile char * base,uint16_t addr,uint8_t data,uint8_t cs)
{
    uint8_t snd_buf[3];
    uint8_t rcv_buf[3];
    uint8_t slave_cs;
    snd_buf[0] = *((uint8_t *)(&addr)+1);
    snd_buf[1] = *((uint8_t *)(&addr)+0);
    snd_buf[2] = data;
    slave_cs = cs & 0x0f;
    apb_spi_setcs(base, slave_cs);
    apb_spi_write(base, snd_buf, rcv_buf, 3);
    apb_spi_setcs(base, 0x0f);
    //xil_printf("clock data0 is %x\r\n",rcv_buf[0]);
    //xil_printf("clock data1 is %x\r\n",rcv_buf[1]);
    //xil_printf("clock data2 is %x\r\n",rcv_buf[2]);
    return rcv_buf[2];
}

#if 0
//--eeprom---------------------------------------------
void eeprom_test(void)
{
	u8 wdata,rdata;
	u16 addr;
	for(addr=0;addr<10;addr++)
    {
    	wdata = addr&0xff;
        i2c_write(BASE_ADDR,0xa4,addr,1,wdata);
    	usleep(100000);
    	i2c_read(BASE_ADDR,0xa4,addr,1,&rdata);
    	xil_printf("i = %x--%x\r\n",addr,rdata);
    }
};
#endif
//--volume set--------------------------------------------------
uint8_t i2c_buf_write(intptr_t base_addr,uint8_t alen,uint8_t *data)
{
	uint8_t i;
	uint8_t status;
	apb_i2c_start(base_addr);
    for(i=0;i<(alen);i++)
    {
    	status += apb_i2c_write(base_addr,data[i]);
    }
    apb_i2c_stop(base_addr);
    return status;
}

void volume_set(intptr_t base,uint8_t dat)//dat 0~100
{
	uint8_t buf[2];
    buf[0] = 0x9a;
    buf[1] = 0xc0 + 31*dat/100;
    i2c_buf_write(base,2,buf);
};

#define ACLK0_DIV   1
#define RCLK0_DIV   2
#define ACLK1_DIV   1
#define RCLK1_DIV   2
#define SCLK_DIVH   0x02
#define SCLK_DIVL   0x00
#define DCLK_TYPE   0x08
#define SCLK_TYPE   0xb0
#define	DDS_CS      0
#define	CLK_CS      1
#define	ADC0_CS     2
#define	ADC1_CS     3

static void hmc_7043_init(volatile char * Spi_synth,uint8_t cs)
{
    printf_note("spi_wrapper begin ......\r\n");
    spi_wrapper(Spi_synth,0x00, 0x01,cs);//reset
    usleep(50000);
    spi_wrapper(Spi_synth,0x00, 0x00,cs);//reset
    usleep(50000);
    //system default
    spi_wrapper(Spi_synth,0x98, 0x00,cs);//
    spi_wrapper(Spi_synth,0x99, 0x00,cs);//
    // spi_wrapper(Spi_synth,0x9a, 0x00,cs);//
    // spi_wrapper(Spi_synth,0x9b, 0xaa,cs);//
    // spi_wrapper(Spi_synth,0x9c, 0xaa,cs);//
    spi_wrapper(Spi_synth,0x9d, 0xaa,cs);//
    spi_wrapper(Spi_synth,0x9e, 0xaa,cs);//
    spi_wrapper(Spi_synth,0x9f, 0x4d,cs);//
    spi_wrapper(Spi_synth,0xa0, 0xdf,cs);//
    // spi_wrapper(Spi_synth,0xa1, 0x97,cs);//
    spi_wrapper(Spi_synth,0xa2, 0x03,cs);//
    spi_wrapper(Spi_synth,0xa3, 0x00,cs);//
    spi_wrapper(Spi_synth,0xa4, 0x00,cs);//
    spi_wrapper(Spi_synth,0xad, 0x00,cs);//
    // spi_wrapper(Spi_synth,0xae, 0x08,cs);//
    // spi_wrapper(Spi_synth,0xaf, 0x50,cs);//
    // spi_wrapper(Spi_synth,0xb0, 0x04,cs);//
    // spi_wrapper(Spi_synth,0xb1, 0x0d,cs);//
    // spi_wrapper(Spi_synth,0xb2, 0x00,cs);//
    // spi_wrapper(Spi_synth,0xb3, 0x00,cs);//
    spi_wrapper(Spi_synth,0xb5, 0x00,cs);//
    spi_wrapper(Spi_synth,0xb6, 0x00,cs);//
    spi_wrapper(Spi_synth,0xb7, 0x00,cs);//
    spi_wrapper(Spi_synth,0xb8, 0x00,cs);//
     //-----------------------------------
    spi_wrapper(Spi_synth,0x01, 0x40,cs);//noise pri
    spi_wrapper(Spi_synth,0x02, 0x00,cs);//
    spi_wrapper(Spi_synth,0x03, 0x14,cs);//if rfsync not use should be disable
    spi_wrapper(Spi_synth,0x04, 0x7f,cs);//0-01,2-23,6-1213
    spi_wrapper(Spi_synth,0x06, 0x00,cs);//
    spi_wrapper(Spi_synth,0x07, 0x00,cs);//
    spi_wrapper(Spi_synth,0x0a, 0x07,cs);//noise pri
    spi_wrapper(Spi_synth,0x0b, 0x07,cs);//
    spi_wrapper(Spi_synth,0x46, 0x00,cs);//
    spi_wrapper(Spi_synth,0x50, 0x0e,cs);//
    spi_wrapper(Spi_synth,0x54, 0x03,cs);//
    spi_wrapper(Spi_synth,0x5a, 0x04,cs);//0x4--8 pulse,0x7--constinue
    spi_wrapper(Spi_synth,0x5b, 0x04,cs);//
    spi_wrapper(Spi_synth,0x5c, SCLK_DIVL,cs);//sysref lsb cnt
    spi_wrapper(Spi_synth,0x5d, SCLK_DIVH,cs);//sysref msb cnt
    spi_wrapper(Spi_synth,0x64, 0x00,cs);//low freq
    spi_wrapper(Spi_synth,0x65, 0x00,cs);//
    spi_wrapper(Spi_synth,0x71, 0x16,cs);//16
    spi_wrapper(Spi_synth,0x78, 0x01,cs);//
    spi_wrapper(Spi_synth,0x79, 0x02,cs);//
    spi_wrapper(Spi_synth,0x7a, 0x03,cs);//
    spi_wrapper(Spi_synth,0x7d, 0x04,cs);//
    spi_wrapper(Spi_synth,0x91, 0x05,cs);//
     //channel 0
    spi_wrapper(Spi_synth,0x0c8, 0xd1,cs);//channel 0 config
    spi_wrapper(Spi_synth,0x0c9, ACLK0_DIV,cs);//div lsb
    spi_wrapper(Spi_synth,0x0ca, 0x00,cs);//div msb
    spi_wrapper(Spi_synth,0x0cb, 0x00,cs);//fine_delay
    spi_wrapper(Spi_synth,0x0cc, 0x00,cs);//coarse_delay
    spi_wrapper(Spi_synth,0x0cd, 0x00,cs);//mslip_lsb
    spi_wrapper(Spi_synth,0x0ce, 0x00,cs);//mslip_msb
    spi_wrapper(Spi_synth,0x0cf, 0x00,cs);//[1:0]00-divider,01-analog,10-other,11-input
    spi_wrapper(Spi_synth,0x0d0, DCLK_TYPE,cs);//[4:3]00-cml,01-lvpecl,10-lvds,11-cmos; [1:0]00-0, 01-100,11-50
     //channel 1
    spi_wrapper(Spi_synth,0x0d2, 0x5d,cs);//channel 1 config
    spi_wrapper(Spi_synth,0x0d3, SCLK_DIVL,cs);//div lsb
    spi_wrapper(Spi_synth,0x0d4, SCLK_DIVH,cs);//div msb
    spi_wrapper(Spi_synth,0x0d5, 0x00,cs);//fine_delay
    spi_wrapper(Spi_synth,0x0d6, 0x00,cs);//coarse_delay
    spi_wrapper(Spi_synth,0x0d7, 0x00,cs);//mslip_lsb
    spi_wrapper(Spi_synth,0x0d8, 0x00,cs);//mslip_msb
    spi_wrapper(Spi_synth,0x0d9, 0x00,cs);//[1:0]00-divider,01-analog,10-other,11-input
    spi_wrapper(Spi_synth,0x0da, SCLK_TYPE,cs);//[4:3]00-cml,01-lvpecl,10-lvds,11-cmos; [1:0]00-0, 01-100,11-50
     //channel 2
    spi_wrapper(Spi_synth,0x0dc, 0xd1&0xfe,cs);//channel 2 config
    spi_wrapper(Spi_synth,0x0dd, ACLK1_DIV,cs);//div lsb
    spi_wrapper(Spi_synth,0x0de, 0x00,cs);//div msb
    spi_wrapper(Spi_synth,0x0df, 0x00,cs);//fine_delay
    spi_wrapper(Spi_synth,0x0e0, 0x00,cs);//coarse_delay
    spi_wrapper(Spi_synth,0x0e1, 0x00,cs);//mslip_lsb
    spi_wrapper(Spi_synth,0x0e2, 0x00,cs);//mslip_msb
    spi_wrapper(Spi_synth,0x0e3, 0x00,cs);//[1:0]00-divider,01-analog,10-other,11-input
    spi_wrapper(Spi_synth,0x0e4, DCLK_TYPE,cs);//[4:3]00-cml,01-lvpecl,10-lvds,11-cmos; [1:0]00-0, 01-100,11-50
     //channel 3
    spi_wrapper(Spi_synth,0x0e6, 0x5d&0xfe,cs);//channel 3 config
    spi_wrapper(Spi_synth,0x0e7, SCLK_DIVL,cs);//div lsb
    spi_wrapper(Spi_synth,0x0e8, SCLK_DIVH,cs);//div msb
    spi_wrapper(Spi_synth,0x0e9, 0x00,cs);//fine_delay
    spi_wrapper(Spi_synth,0x0ea, 0x00,cs);//coarse_delay
    spi_wrapper(Spi_synth,0x0eb, 0x00,cs);//mslip_lsb
    spi_wrapper(Spi_synth,0x0ec, 0x00,cs);//mslip_msb
    spi_wrapper(Spi_synth,0x0ed, 0x00,cs);//[1:0]00-divider,01-analog,10-other,11-input
    spi_wrapper(Spi_synth,0x0ee, SCLK_TYPE,cs);//[4:3]00-cml,01-lvpecl,10-lvds,11-cmos; [1:0]00-0, 01-100,11-50
     //channel 4
    spi_wrapper(Spi_synth,0x0f0, 0xd1,cs);//channel 4 config
    spi_wrapper(Spi_synth,0x0f1, RCLK1_DIV,cs);//div lsb
    spi_wrapper(Spi_synth,0x0f2, 0x00,cs);//div msb
    spi_wrapper(Spi_synth,0x0f3, 0x00,cs);//fine_delay
    spi_wrapper(Spi_synth,0x0f4, 0x00,cs);//coarse_delay
    spi_wrapper(Spi_synth,0x0f5, 0x00,cs);//mslip_lsb
    spi_wrapper(Spi_synth,0x0f6, 0x00,cs);//mslip_msb
    spi_wrapper(Spi_synth,0x0f7, 0x00,cs);//[1:0]00-divider,01-analog,10-other,11-input
    spi_wrapper(Spi_synth,0x0f8, DCLK_TYPE,cs);//[4:3]00-cml,01-lvpecl,10-lvds,11-cmos; [1:0]00-0, 01-100,11-50
    //channel 5
    spi_wrapper(Spi_synth,0x0fa, 0x5d,cs);//channel 5 config
    spi_wrapper(Spi_synth,0x0fb, SCLK_DIVL,cs);//div lsb
    spi_wrapper(Spi_synth,0x0fc, SCLK_DIVH,cs);//div msb
    spi_wrapper(Spi_synth,0x0fd, 0x00,cs);//fine_delay
    spi_wrapper(Spi_synth,0x0fe, 0x00,cs);//coarse_delay
    spi_wrapper(Spi_synth,0x0ff, 0x00,cs);//mslip_lsb
    spi_wrapper(Spi_synth,0x100, 0x00,cs);//mslip_msb
    spi_wrapper(Spi_synth,0x101, 0x00,cs);//[1:0]00-divider,01-analog,10-other,11-input
    spi_wrapper(Spi_synth,0x102, SCLK_TYPE,cs);//[4:3]00-cml,01-lvpecl,10-lvds,11-cmos; [1:0]00-0, 01-100,11-50
    //channel 6
    spi_wrapper(Spi_synth,0x104, 0xd1&0xfe,cs);//channel 6 config
    spi_wrapper(Spi_synth,0x105, RCLK1_DIV,cs);//div lsb
    spi_wrapper(Spi_synth,0x106, 0x00,cs);//div msb
    spi_wrapper(Spi_synth,0x107, 0x00,cs);//fine_delay
    spi_wrapper(Spi_synth,0x108, 0x00,cs);//coarse_delay
    spi_wrapper(Spi_synth,0x109, 0x00,cs);//mslip_lsb
    spi_wrapper(Spi_synth,0x10a, 0x00,cs);//mslip_msb
    spi_wrapper(Spi_synth,0x10b, 0x00,cs);//[1:0]00-divider,01-analog,10-other,11-input
    spi_wrapper(Spi_synth,0x10c, DCLK_TYPE,cs);//[4:3]00-cml,01-lvpecl,10-lvds,11-cmos; [1:0]00-0, 01-100,11-50
    //channel 7
    spi_wrapper(Spi_synth,0x10e, 0x5d&0xfe,cs);//channel 7 config
    spi_wrapper(Spi_synth,0x10f, SCLK_DIVL,cs);//div lsb
    spi_wrapper(Spi_synth,0x110, SCLK_DIVH,cs);//div msb
    spi_wrapper(Spi_synth,0x111, 0x00,cs);//fine_delay
    spi_wrapper(Spi_synth,0x112, 0x00,cs);//coarse_delay
    spi_wrapper(Spi_synth,0x113, 0x00,cs);//mslip_lsb
    spi_wrapper(Spi_synth,0x114, 0x00,cs);//mslip_msb
    spi_wrapper(Spi_synth,0x115, 0x00,cs);//[1:0]00-divider,01-analog,10-other,11-input
    spi_wrapper(Spi_synth,0x116, SCLK_TYPE,cs);//[4:3]00-cml,01-lvpecl,10-lvds,11-cmos; [1:0]00-0, 01-100,11-50
    //channel 8 none
    spi_wrapper(Spi_synth,0x118, 0xd1&0xfe,cs);//channel 8 config
    spi_wrapper(Spi_synth,0x119, RCLK0_DIV,cs);//div lsb
    spi_wrapper(Spi_synth,0x11a, 0x00,cs);//div msb
    spi_wrapper(Spi_synth,0x11b, 0x00,cs);//fine_delay
    spi_wrapper(Spi_synth,0x11c, 0x00,cs);//coarse_delay
    spi_wrapper(Spi_synth,0x11d, 0x00,cs);//mslip_lsb
    spi_wrapper(Spi_synth,0x11e, 0x00,cs);//mslip_msb
    spi_wrapper(Spi_synth,0x11f, 0x00,cs);//[1:0]00-divider,01-analog,10-other,11-input
    spi_wrapper(Spi_synth,0x120, DCLK_TYPE,cs);//[4:3]00-cml,01-lvpecl,10-lvds,11-cmos; [1:0]00-0, 01-100,11-50
    //channel 9
    spi_wrapper(Spi_synth,0x122, 0x5d&0xfe,cs);//channel 9 config
    spi_wrapper(Spi_synth,0x123, SCLK_DIVL,cs);//div lsb
    spi_wrapper(Spi_synth,0x124, SCLK_DIVH,cs);//div msb
    spi_wrapper(Spi_synth,0x125, 0x00,cs);//fine_delay
    spi_wrapper(Spi_synth,0x126, 0x00,cs);//coarse_delay
    spi_wrapper(Spi_synth,0x127, 0x00,cs);//mslip_lsb
    spi_wrapper(Spi_synth,0x128, 0x00,cs);//mslip_msb
    spi_wrapper(Spi_synth,0x129, 0x00,cs);//[1:0]00-divider,01-analog,10-other,11-input
    spi_wrapper(Spi_synth,0x12a, SCLK_TYPE,cs);////[4:3]00-cml,01-lvpecl,10-lvds,11-cmos; [1:0]00-0, 01-100,11-50
    //channel 10
    spi_wrapper(Spi_synth,0x12c, 0xd1,cs);//channel 10 config
    spi_wrapper(Spi_synth,0x12d, RCLK0_DIV,cs);//div lsb
    spi_wrapper(Spi_synth,0x12e, 0x00,cs);//div msb
    spi_wrapper(Spi_synth,0x12f, 0x00,cs);//fine_delay
    spi_wrapper(Spi_synth,0x130, 0x00,cs);//coarse_delay
    spi_wrapper(Spi_synth,0x131, 0x00,cs);//mslip_lsb
    spi_wrapper(Spi_synth,0x132, 0x00,cs);//mslip_msb
    spi_wrapper(Spi_synth,0x133, 0x00,cs);//[1:0]00-divider,01-analog,10-other,11-input
    spi_wrapper(Spi_synth,0x134, DCLK_TYPE,cs);////[4:3]00-cml,01-lvpecl,10-lvds,11-cmos; [1:0]00-0, 01-100,11-50
    //channel 11
    spi_wrapper(Spi_synth,0x136, 0x5d&0xfe,cs);//channel 11 config
    spi_wrapper(Spi_synth,0x137, SCLK_DIVL,cs);//div lsb
    spi_wrapper(Spi_synth,0x138, SCLK_DIVH,cs);//div msb
    spi_wrapper(Spi_synth,0x139, 0x00,cs);//fine_delay
    spi_wrapper(Spi_synth,0x13a, 0x00,cs);//coarse_delay
    spi_wrapper(Spi_synth,0x13b, 0x00,cs);//mslip_lsb
    spi_wrapper(Spi_synth,0x13c, 0x00,cs);//mslip_msb
    spi_wrapper(Spi_synth,0x13d, 0x00,cs);//[1:0]00-divider,01-analog,10-other,11-input
    spi_wrapper(Spi_synth,0x13e, SCLK_TYPE,cs);////[4:3]00-cml,01-lvpecl,10-lvds,11-cmos; [1:0]00-0, 01-100,11-50
    //channel 12
    spi_wrapper(Spi_synth,0x140, 0xd1&0xfe,cs);//channel 12 config
    spi_wrapper(Spi_synth,0x141, RCLK1_DIV,cs);//div lsb
    spi_wrapper(Spi_synth,0x142, 0x00,cs);//div msb
    spi_wrapper(Spi_synth,0x143, 0x00,cs);//fine_delay
    spi_wrapper(Spi_synth,0x144, 0x00,cs);//coarse_delay
    spi_wrapper(Spi_synth,0x145, 0x00,cs);//mslip_lsb
    spi_wrapper(Spi_synth,0x146, 0x00,cs);//mslip_msb
    spi_wrapper(Spi_synth,0x147, 0x00,cs);//[1:0]00-divider,01-analog,10-other,11-input
    spi_wrapper(Spi_synth,0x148, DCLK_TYPE,cs);////[4:3]00-cml,01-lvpecl,10-lvds,11-cmos; [1:0]00-0, 01-100,11-50
    //channel 13
    spi_wrapper(Spi_synth,0x14a, 0x5d&0xfe,cs);//channel 13 config
    spi_wrapper(Spi_synth,0x14b, SCLK_DIVL,cs);//div lsb
    spi_wrapper(Spi_synth,0x14c, SCLK_DIVH,cs);//div msb
    spi_wrapper(Spi_synth,0x14d, 0x00,cs);//fine_delay
    spi_wrapper(Spi_synth,0x14e, 0x00,cs);//coarse_delay
    spi_wrapper(Spi_synth,0x14f, 0x00,cs);//mslip_lsb
    spi_wrapper(Spi_synth,0x150, 0x00,cs);//mslip_msb
    spi_wrapper(Spi_synth,0x151, 0x00,cs);//[1:0]00-divider,01-analog,10-other,11-input
    spi_wrapper(Spi_synth,0x152, SCLK_TYPE,cs);////[4:3]00-cml,01-lvpecl,10-lvds,11-cmos; [1:0]00-0, 01-100,11-50
    usleep(100);
    spi_wrapper(Spi_synth,0x001, 0x42,cs);//restart dividers/FSMs
    spi_wrapper(Spi_synth,0x001, 0x40,cs);//restart
    usleep(100);
    spi_wrapper(Spi_synth,0x001, 0xc0,cs);//sync internal div
    spi_wrapper(Spi_synth,0x001, 0x40,cs);//sync internal div
    usleep(100);
};


static void hmc7043_generate_sync(char *Spi_synth,uint8_t cs)
{
    spi_wrapper(Spi_synth,0x01, 0x44,cs);//request pluse gen
    spi_wrapper(Spi_synth,0x01, 0x40,cs);
}

uint8_t spi_24bit_wrapper(volatile char * base,uint8_t addr,uint32_t data,uint8_t cs)
{
    uint8_t snd_buf[5];
    uint8_t rcv_buf[5];
    uint8_t slave_cs;
    snd_buf[0] = addr;
    snd_buf[1] = *((uint8_t *)(&data)+1);
    snd_buf[2] = *((uint8_t *)(&data)+0);
    slave_cs = cs & 0x0f;
    apb_spi_setcs(base, slave_cs);
    apb_spi_write(base, snd_buf, rcv_buf, 3);
    apb_spi_setcs(base, 0x07);
    return rcv_buf[2];
}

void lmx2582_4096MHz_init(volatile char * Spi_synth,uint8_t cs)
{
	spi_24bit_wrapper(Spi_synth,0x00,0x0002,cs);//reset
    usleep(100);
    spi_24bit_wrapper(Spi_synth,0x40,0x0077,cs);
    spi_24bit_wrapper(Spi_synth,0x3E,0x0000,cs);
    spi_24bit_wrapper(Spi_synth,0x3D,0x0001,cs);
    spi_24bit_wrapper(Spi_synth,0x3B,0x0000,cs);
    spi_24bit_wrapper(Spi_synth,0x30,0x03FC,cs);
    spi_24bit_wrapper(Spi_synth,0x2F,0x00CF,cs);
    spi_24bit_wrapper(Spi_synth,0x2E,0x3FA3,cs);
    spi_24bit_wrapper(Spi_synth,0x2D,0x00c8,cs);
    spi_24bit_wrapper(Spi_synth,0x2C,0x0000,cs);
    spi_24bit_wrapper(Spi_synth,0x2B,0x0000,cs);
    spi_24bit_wrapper(Spi_synth,0x2A,0x0000,cs);
    spi_24bit_wrapper(Spi_synth,0x29,0x00FA,cs);
    spi_24bit_wrapper(Spi_synth,0x28,0x0000,cs);
    spi_24bit_wrapper(Spi_synth,0x27,0x8204,cs);
    spi_24bit_wrapper(Spi_synth,0x26,0x0198,cs);
    spi_24bit_wrapper(Spi_synth,0x25,0x4000,cs);
    spi_24bit_wrapper(Spi_synth,0x24,0x0C10,cs);
    spi_24bit_wrapper(Spi_synth,0x23,0x001B,cs);
    spi_24bit_wrapper(Spi_synth,0x22,0xC3EA,cs);
    spi_24bit_wrapper(Spi_synth,0x21,0x2A0A,cs);
    spi_24bit_wrapper(Spi_synth,0x20,0x210A,cs);
    spi_24bit_wrapper(Spi_synth,0x1F,0x0601,cs);
    spi_24bit_wrapper(Spi_synth,0x1E,0x0034,cs);
    spi_24bit_wrapper(Spi_synth,0x1D,0x0084,cs);
    spi_24bit_wrapper(Spi_synth,0x1C,0x2924,cs);
    spi_24bit_wrapper(Spi_synth,0x19,0x0000,cs);
    spi_24bit_wrapper(Spi_synth,0x18,0x0509,cs);
    spi_24bit_wrapper(Spi_synth,0x17,0x8842,cs);
    spi_24bit_wrapper(Spi_synth,0x16,0x2300,cs);
    spi_24bit_wrapper(Spi_synth,0x14,0x012C,cs);
    spi_24bit_wrapper(Spi_synth,0x13,0x0965,cs);
    spi_24bit_wrapper(Spi_synth,0x0E,0x018C,cs);
    spi_24bit_wrapper(Spi_synth,0x0D,0x4000,cs);
    spi_24bit_wrapper(Spi_synth,0x0C,0x7001,cs);
    spi_24bit_wrapper(Spi_synth,0x0B,0x0018,cs);
    spi_24bit_wrapper(Spi_synth,0x0A,0x10D8,cs);
    spi_24bit_wrapper(Spi_synth,0x09,0x0302,cs);
    spi_24bit_wrapper(Spi_synth,0x08,0x1084,cs);
    spi_24bit_wrapper(Spi_synth,0x07,0x28B2,cs);
    spi_24bit_wrapper(Spi_synth,0x04,0x1943,cs);
    spi_24bit_wrapper(Spi_synth,0x02,0x0500,cs);
    spi_24bit_wrapper(Spi_synth,0x01,0x0808,cs);
    spi_24bit_wrapper(Spi_synth,0x00,0x221c,cs);//vco cal
    usleep(100);
};
void ad_9680_ddc_10g_x2_init(volatile char *Spi_synth,uint8_t cs)
{
    printf_note("%s......\r\n",__FUNCTION__);
    spi_wrapper(Spi_synth, 0x000,0x81,cs);
    usleep(10000);
    spi_wrapper(Spi_synth, 0x000,0x00,cs);
    spi_wrapper(Spi_synth, 0x018,0x40,cs);
    spi_wrapper(Spi_synth, 0x019,0x50,cs);
    spi_wrapper(Spi_synth, 0x01a,0x09,cs);
    spi_wrapper(Spi_synth, 0x11a,0x00,cs);
    spi_wrapper(Spi_synth, 0x935,0x04,cs);
    spi_wrapper(Spi_synth, 0x025,0x0a,cs);
    spi_wrapper(Spi_synth, 0x030,0x18,cs);
    spi_wrapper(Spi_synth, 0x016,0x0e,cs);//400 temination
    spi_wrapper(Spi_synth, 0x934,0x1f,cs);
    spi_wrapper(Spi_synth, 0x228,0x00,cs);//offset
    spi_wrapper(Spi_synth, 0x008,0x03,cs);
    spi_wrapper(Spi_synth, 0x03f,0x80,cs);
    spi_wrapper(Spi_synth, 0x10b,0x00,cs);
    spi_wrapper(Spi_synth, 0x10c,0x01,cs);
    spi_wrapper(Spi_synth, 0x040,0xbf,cs);
    spi_wrapper(Spi_synth, 0x571,0x15,cs);
    spi_wrapper(Spi_synth, 0x200,0x02,cs);//ddc0 & ddc1
    spi_wrapper(Spi_synth, 0x201,0x01,cs);//dec 2
//----ddc0 mode
    spi_wrapper(Spi_synth, 0x310,0x43,cs);//test mode and dec 2 == 0x33
    spi_wrapper(Spi_synth, 0x311,0x00,cs);//ddc0 a
    spi_wrapper(Spi_synth, 0x314,0x00,cs);//nco0 f/fs*4096
    spi_wrapper(Spi_synth, 0x315,0x0c,cs);//nco0 f/fs*4096
    spi_wrapper(Spi_synth, 0x320,0x00,cs);//nco0 phase
    spi_wrapper(Spi_synth, 0x321,0x00,cs);
//----ddc1 mode
    spi_wrapper(Spi_synth, 0x330,0x43,cs);
    spi_wrapper(Spi_synth, 0x331,0x05,cs);//ddc1 b
    spi_wrapper(Spi_synth, 0x334,0x00,cs);//nco1 f/fs*4096
    spi_wrapper(Spi_synth, 0x335,0x0c,cs);//nco1 f/fs*4096
    spi_wrapper(Spi_synth, 0x340,0x00,cs);//nco1 phase
    spi_wrapper(Spi_synth, 0x341,0x00,cs);

    spi_wrapper(Spi_synth, 0x570,0x52,cs);//5Gbps
    spi_wrapper(Spi_synth, 0x56e,0x00,cs);//serial line rate great than 6.25Gbps
    //spi_wrapper(Spi_synth, 0x550,0x0f,cs);
    spi_wrapper(Spi_synth, 0x58b,0x03,cs);
    spi_wrapper(Spi_synth, 0x120,0x02,cs);//constinuous
    spi_wrapper(Spi_synth, 0x121,0x00,cs);
    spi_wrapper(Spi_synth, 0x300,0x01,cs);
    spi_wrapper(Spi_synth, 0x001,0x02,cs);
    spi_wrapper(Spi_synth, 0x571,0x14,cs);
}

//--reset--------------------------------------------------
static void dds_reset_set(volatile char * base,uint8_t dat)
{
    if(dat>0)
      REG_WRITE(base + RESET_ADDR,0x01);
    else
      REG_WRITE(base + RESET_ADDR,0x00);
}
static void clock_reset_set(volatile char * base,uint8_t dat)
{
    if(dat>0)
      REG_WRITE(base + RESET_ADDR,0x03);
    else
      REG_WRITE(base + RESET_ADDR,0x01);
}
static void adc_reset_set(volatile char * base,uint8_t dat)
{
    if(dat>0)
      REG_WRITE(base + RESET_ADDR,0x07);
    else
      REG_WRITE(base + RESET_ADDR,0x03);
}
static void logic_reset_set(volatile char * base,uint8_t dat)
{
    if(dat>0)
      REG_WRITE(base + RESET_ADDR,0x0f);
    else
      REG_WRITE(base + RESET_ADDR,0x07);
}


static uint32_t logic_state(volatile char * base)
{
    uint32_t state;
    state = REG_READ(base + STATE_ADDR);
    return	state;
}

uint32_t get_dc_offset(intptr_t base)
{
    return REG_READ(base + 0x8c);
}

void set_dc_offset(intptr_t base,uint32_t dat)
{
    REG_WRITE(base + 0x8c,dat);
}




static void *_memmap(int fd_dev, off_t phr_addr, int length)
{
    void *base_addr = NULL;

    base_addr = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED, fd_dev, phr_addr);
 
    if (base_addr == NULL)
    {
        printf("mmap failed, NULL pointer!\n");
        return NULL;
    }

    return base_addr;
}

struct clock_adc_t ca_ctx;

#define  _MEM_64K (0x10000)

static int clock_adc_fpga_init(void)
{
    #define _MEM_REG_DEV "/dev/mem"
    printf_note("clock adc fpga init...\n");
    ca_ctx.fd_mem_dev = open(_MEM_REG_DEV, O_RDWR|O_SYNC);
    if (ca_ctx.fd_mem_dev == -1){
        printf_warn("%s, open error", _MEM_REG_DEV);
        return (-1);
    }

    ca_ctx.vir_addr = _memmap(ca_ctx.fd_mem_dev, BASE_ADDR, _MEM_64K);
    if(ca_ctx.vir_addr == NULL){
        return -1;
    }

    dds_reset_set(ca_ctx.vir_addr,1);
    usleep(10000);
    lmx2582_4096MHz_init(ca_ctx.vir_addr,0);
    usleep(10000);
    clock_reset_set(ca_ctx.vir_addr,1);
    usleep(10000);
    hmc_7043_init(ca_ctx.vir_addr,CLK_CS);
    usleep(10000);
    adc_reset_set(ca_ctx.vir_addr,1);
    usleep(10000);
    ad_9680_ddc_10g_x2_init(ca_ctx.vir_addr,ADC0_CS);
    usleep(10000);
    logic_reset_set(ca_ctx.vir_addr,1);
    usleep(10000);
    hmc7043_generate_sync(ca_ctx.vir_addr,CLK_CS);
    usleep(10000);
    return 0;
}

static int clock_adc_fpga_close(void)
{
    int ret = 0;

    ret = munmap(ca_ctx.vir_addr, _MEM_64K); 
    if (ret){
        perror("munmap");
        return -1;
    }
    close(ca_ctx.fd_mem_dev);
    printf_note("close fpga clock & adc\n");
    return 0;
}

static const struct clock_adc_ops ca_ctx_ops = {
    .init = clock_adc_fpga_init,
    .close = clock_adc_fpga_close,
};

struct clock_adc_ctx * clock_adc_fpga_cxt(void)
{
    int ret = -ENOMEM;
    struct clock_adc_ctx *ctx = calloc(1, sizeof(*ctx));
    if (!ctx)
        goto err_set_errno;
    
    ctx->ops = &ca_ctx_ops;
err_set_errno:
    errno = -ret;
    return ctx;

}

