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
*  Rev 1.0   06 July 2019   yaochangsong
*  Initial revision.
******************************************************************************/
#include "config.h"

struct xnrp_header xnrp_data;
uint8_t  *xnrp_response_payload = NULL;


uint8_t *xnrp_strstr(const uint8_t *s1, const uint8_t *s2, int len)   
{  
    int n = 0;  
    if (*s2)   
    {   
        while (*s1 && len > n)   
        {   
            for (n = 0; *(s1 + n) == *(s2 + n); n ++)  
            {   
                if (!*(s2 + n + 1))   
                    return (uint8_t *)s1;   
            }   
            s1++;   
        }  
        return NULL;   
    }   
    else   
        return s1;   
} 

int xnrp_execute_set_command(void)
{
    struct xnrp_header *header;
    int err_code;
    header = &xnrp_data;

    err_code = RET_CODE_SUCCSESS;
    
    switch (header->class_code)
    {
        case CLASS_CODE_REGISTER:
        {
            break;
        }
        case CLASS_CODE_NET:
        {
            break;
        }
        case CLASS_CODE_WORK_MODE:
        {
            break;
        }
        case CLASS_CODE_MID_FRQ:
        {
            break;
        }
        case CLASS_CODE_RF:
        {
            break;
        }
        case CLASS_CODE_CONTROL:
        {
            break;
        }
        case CLASS_CODE_STATUS:
        {
            break;
        }
        case CLASS_CODE_JOURNAL:
        {
            break;
        }
        case CLASS_CODE_FILE:
        {
            break;
        }
        case CLASS_CODE_HEARTBEAT:
        {
            err_code = RET_CODE_PARAMTER_INVALID;
            break;
        }
        default:
            printf_err("error class code[%d]\n",header->class_code);
            err_code = RET_CODE_PARAMTER_ERR;
            break;
    }

    return err_code;
}

int xnrp_execute_get_command(void)
{
    struct xnrp_header *header;
    int err_code;
    header = &xnrp_data;

    err_code = RET_CODE_SUCCSESS;
    
    switch (header->class_code)
    {
        case CLASS_CODE_REGISTER:
        {
            break;
        }
        case CLASS_CODE_NET:
        {
            break;
        }
        case CLASS_CODE_WORK_MODE:
        {
            break;
        }
        case CLASS_CODE_MID_FRQ:
        {
            break;
        }
        case CLASS_CODE_RF:
        {
            break;
        }
        case CLASS_CODE_CONTROL:
        {
            break;
        }
        case CLASS_CODE_STATUS:
        {
            break;
        }
        case CLASS_CODE_JOURNAL:
        {
            break;
        }
        case CLASS_CODE_FILE:
        {
            break;
        }
        case CLASS_CODE_HEARTBEAT:
        {
            break;
        }
        default:
            printf_err("error class code[%d]\n",header->class_code);
            err_code = RET_CODE_PARAMTER_ERR;
            break;
    }
    
    return err_code;
}


int xnrp_execute_method(void)
{
    struct xnrp_header *header;
    int err_code;
    header = &xnrp_data;

    err_code = RET_CODE_SUCCSESS;
    
    switch (header->method_code)
    {
        case METHOD_SET_COMMAND:
        {
            err_code = xnrp_execute_set_command();
            break;
        }
        case METHOD_GET_COMMAND:
        {
            err_code = xnrp_execute_get_command();
            break;
        }
        case METHOD_RESPONSE_COMMAND:
        case METHOD_REPORT_COMMAND:
        {
            err_code = RET_CODE_PARAMTER_INVALID;
            printf_err("invalid method[%d]", header->method_code);
            break;
        }
        default:
            printf_err("error method[%d]", header->method_code);
            err_code = RET_CODE_PARAMTER_ERR;
            break;
    }
    return err_code;
}


bool xnrp_parse_header(const uint8_t *data, int len, uint8_t **payload, int *err_code)
{
    uint8_t *val;
    struct xnrp_header *header;
    header = &xnrp_data;
    int header_len;

    header_len = sizeof(struct xnrp_header) - sizeof(header->payload);

    if(len < header_len){
        printf_err("receive data len[%d < %d] is too short\n", len, header_len);
        *err_code = RET_CODE_FORMAT_ERR;
        return false;
    }
    printf_debug("parse_header[%c %c %c %c][%x,%x,%x,%x]\n", data[0], data[1], data[2], data[3],data[0], data[1], data[2], data[3]);
    val = xnrp_strstr(data, XNRP_HEADER_START, len);
    if (!val){
        printf_debug("parse_header error\n");
        *err_code = RET_CODE_FORMAT_ERR;
        return false;
    }

    memcpy(header, val, sizeof(struct xnrp_header) - sizeof(header->payload));
    printf_info("find header start=%c%c%c%c\n", header->start_flag[0], header->start_flag[1],
                                                header->start_flag[2], header->start_flag[3]);
    printf_info("header.version=%x\n", header->version);
    printf_info("header.method_code=%x\n", header->method_code);
    printf_info("header.error_code=%x\n", header->error_code);
    printf_info("header.class_code=%x\n", header->class_code);
    printf_info("header.business_code=%x\n", header->business_code);
    printf_info("header.time=%s, %x\n", ctime(&header->time_stamp), header->time_stamp);
    printf_info("header.msg_id=%x\n", header->msg_id);
    printf_info("header.check_sum=%x\n", header->check_sum);
    printf_info("header.payload_len=%x\n", header->payload_len);
    
    if(header->payload_len > 0){
        *payload = data + header_len;
    }
    else{
        *payload = NULL;
    }
    
    return true;
}

bool xnrp_parse_data(const uint8_t *payload)
{
    struct xnrp_header *header;
    header = &xnrp_data;
    printf_debug("0\n");

    if(payload == NULL){
        return true;
    }
    if(header->payload_len == 0){
        return true;
    }
    printf_debug("0\n");
    header->payload = calloc(1, header->payload_len);
    if (!header->payload){
        printf_err("calloc failed\n");
        return false;
    }
    printf_debug("0\n");
    memcpy(header->payload, payload, header->payload_len);
    printf_debug("1\n");

    return true;
}

bool xnrp_handle_request(uint8_t *data, int len, int *code)
{
    uint8_t *payload = NULL;
    
    printf_info("len[%d] %d\n", len, sizeof(struct xnrp_header));
    printf_info("Prepare to handle xnrp protocol data\n");
    if(xnrp_parse_header(data, len, &payload, code) == false){
        return false;
    }
    if(payload != NULL){
        printf_debug("payload:\n");
        for(int i = 0; i< len; i++)
            printfd("%x ", payload[i]);
        printf_debug("\n");
        
        if(xnrp_parse_data(payload) == false){
            *code = RET_CODE_INTERNAL_ERR;
            return false;
        }
    }

    if(xnrp_execute_method() != RET_CODE_SUCCSESS){
        return false;
    }
    return true;
}

int xnrp_assamble_response_data(uint8_t *buf,          int err_code)
{
    int len = 0;
    printf_info("Prepare to assamble response xnrp data\n");
    return len;
}

