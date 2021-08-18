/******************************************************************************
*  Copyright 2019, Showay Technology Dev Co.,Ltd.
*  ---------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Showay Technology Dev Co.,Ltd. (C) 2019
******************************************************************************/
/*****************************************************************************     
*  Rev 1.0   04 June 2020   yaochangsong
*  Initial revision.
******************************************************************************/
#ifndef _RS485_COM_
#define _RS485_COM_

struct _frame_data{
    uint8_t     header;
    uint16_t    res;
    uint8_t     type;
    uint16_t    valid_len;
    uint8_t     data[0];
    uint8_t     end;
}__attribute__ ((packed));

struct _frame_read_x{
    uint8_t *frame_buffer;
    size_t offset;
};

static struct _frame_read_x frame_read;

/*电子罗盘*/
struct _elec_compass{
    uint8_t     header;
    uint8_t     data_len;
    uint8_t     addr;
    uint8_t    cmd;
    uint8_t     data[0];
    uint8_t     crc;
}__attribute__ ((packed));


#ifdef CONFIG_DEVICE_UART    
#define  rs485_send_data_by_serial(buf, len)    uart_send_data(UART_RS485_CODE, buf, len)
#define  rs485_read_block_timeout(buf, t)       uart_read_timeout(UART_RS485_CODE, buf, t)
#else    
#define  rs485_send_data_by_serial(buf, len)
#define  rs485_read_block_timeout(buf, t)
#endif

#define FRAME_HEADER    0x7e
#define FRAME_END       0x7f

#define FRAME_DATA_LEN_OFFSET   4
#define FRAME_DATA_LEN          2

#define FRAME_MAX_LEN 1024



enum{
    RS_485_LOW_NOISE_GET_CMD = 1,
    RS_485_LOW_NOISE_GET_RSP_CMD = 2,
    RS_485_LOW_NOISE_SET_CMD = 3,
    RS_485_LOW_NOISE_SET_RSP_CMD = 4,
    RS_485_MAX_CMD,
};

extern int8_t rs485_com_init(void);
extern int8_t rs485_com_get(int32_t cmd, void *pdata);
extern int8_t rs485_com_set(int32_t cmd, void *pdata, size_t len);
extern int8_t rs485_com_set_v2(int32_t cmd, void *pdata);

extern int elec_compass1_com_get_angle(float *angle);
extern int elec_compass2_com_get_angle(float *angle);
#endif


