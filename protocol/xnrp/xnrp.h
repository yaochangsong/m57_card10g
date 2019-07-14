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
*  Rev 1.0   09 July 2019   yaochangsong
*  Initial revision.
******************************************************************************/

#ifndef _XNRP_H_H
#define _XNRP_H_H

#include "config.h"

#define XNRP_HEADER_START  "XNRP"
#define XNRP_HEADER_VERSION  1


typedef enum {
    RET_CODE_SUCCSESS         = 0,
    RET_CODE_FORMAT_ERR       = 1,
    RET_CODE_PARAMTER_ERR     = 2,
    RET_CODE_PARAMTER_INVALID = 3,
    RET_CODE_CHANNEL_ERR      = 4,
    RET_CODE_INVALID_MODULE   = 5,
    RET_CODE_INTERNAL_ERR     = 6,
    RET_CODE_PARAMTER_NOT_SET = 7,
    RET_CODE_PARAMTER_TOO_LONG= 8
} return_code;

/* method code */
enum {
    METHOD_SET_COMMAND      = 0x01,
    METHOD_GET_COMMAND      = 0x02,
    METHOD_RESPONSE_COMMAND = 0x04,
    METHOD_REPORT_COMMAND   = 0x08,
};

/* class code */
enum {
    CLASS_CODE_REGISTER  = 0x00,
    CLASS_CODE_NET       = 0x01,
    CLASS_CODE_WORK_MODE = 0x02,
    CLASS_CODE_MID_FRQ   = 0x04,
    CLASS_CODE_RF        = 0x08,
    CLASS_CODE_CONTROL   = 0x10,
    CLASS_CODE_STATUS    = 0x11,
    CLASS_CODE_JOURNAL   = 0x12,
    CLASS_CODE_FILE      = 0x14,
    CLASS_CODE_HEARTBEAT = 0xaa
};

/* bussiness code */
/*net*/
enum {
    B_CODE_ALL_NET  = 0x01,
};

/*work mode*/
enum {
    B_CODE_WK_MODE_MULTI_FRQ_POINT    = 0x01,
    B_CODE_WK_MODE_SUB_CH_DEC         = 0x02,
    B_CODE_WK_MODE_MULTI_FRQ_FREGMENT = 0x03,
};
    
/*middle frequency*/
enum {
    B_CODE_MID_FRQ_MUTE_SW        = 0x11,
    B_CODE_MID_FRQ_MUTE_THRE      = 0x12,
    B_CODE_MID_FRQ_DEC_METHOD     = 0x13,
    B_CODE_MID_FRQ_AU_SAMPLE_RATE = 0x14,
};




/* xnrp header info */
struct xnrp_header {
    char start_flag[4];
    uint8_t version;
    uint8_t method_code;
    uint8_t error_code;
    uint8_t class_code;
    uint8_t business_code;
    uint8_t client_id[6];
    uint8_t device_id[6];
    uint32_t time_stamp;
    uint16_t msg_id;
    uint16_t check_sum;
    uint32_t payload_len;
    uint8_t payload[MAX_RECEIVE_DATA_LEN];
}__attribute__ ((packed));

struct xnrp_net_paramter {
    uint8_t mac[6];
    struct in_addr gateway;
    struct in_addr netmask;
    struct in_addr ipaddress;
    uint16_t port;
}__attribute__ ((packed));




struct xnrp_multi_frequency_point {
}__attribute__ ((packed));

extern int xnrp_assamble_response_data(uint8_t **buf, int err_code);
extern bool xnrp_parse_header(const uint8_t *data, int len, uint8_t **payload, int *err_code);
extern bool xnrp_parse_data(const uint8_t *payload, int *code);
extern bool xnrp_execute_method(int *code);
#endif

