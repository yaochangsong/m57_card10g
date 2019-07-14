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
struct xnrp_header xnrp_response_data;

//uint8_t  xnrp_response_payload_data[MAX_SEND_DATA_LEN];


static uint8_t *xnrp_strstr(const uint8_t *s1, const uint8_t *s2, int len)   
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

static int xnrp_execute_set_command(void)
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
            switch (header->business_code)
            {
                case B_CODE_WK_MODE_MULTI_FRQ_POINT:
                {
                    struct xnrp_multi_frequency_point frqp;
                    config_parse_data(header->payload, header->payload_len, &frqp);
                    config_save_batch(header->class_code, header->business_code,&frqp);
                    executor_set_command(header->class_code, header->business_code,&frqp);
                    break;
                }
                case B_CODE_WK_MODE_SUB_CH_DEC:
                    break;
                case B_CODE_WK_MODE_MULTI_FRQ_FREGMENT:
                    break;
                default:
                    printf_err("invalid bussiness code[%d]", header->business_code);
            }
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

static int xnrp_execute_get_command(void)
{
    struct xnrp_header *header;
    struct xnrp_header *resp_header;
    int err_code;
    header = &xnrp_data;
    resp_header = &xnrp_response_data;

    err_code = RET_CODE_SUCCSESS;
    
    switch (header->class_code)
    {
        case CLASS_CODE_REGISTER:
        {
            printf_debug("register\n");
        }
        case CLASS_CODE_NET:
        {
            struct xnrp_net_paramter value;
            value.ipaddress.s_addr = htonl(1255577);
            memcpy(resp_header->payload, &value, sizeof(struct xnrp_net_paramter));
            resp_header->payload_len = sizeof(struct xnrp_net_paramter);
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


bool xnrp_execute_method(int *code)
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
    *code = err_code;
     printf_debug("error code[%d]\n", *code);
    if(err_code == RET_CODE_SUCCSESS)
        return true;
    else
        return false;

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
    val = xnrp_strstr(data, (uint8_t *)XNRP_HEADER_START, len);
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

    if(header_len + header->payload_len > len){
        *err_code = RET_CODE_FORMAT_ERR;
        printf_err("invalid payload len=%d\n", header->payload_len);
        return false;
    }
    if(header->payload_len > 0){
        *payload = data + header_len;
    }
    else{
        *payload = NULL;
    }
    
    return true;
}

bool xnrp_parse_data(const uint8_t *payload, int *code)
{
    uint8_t i;
    struct xnrp_header *header;
    header = &xnrp_data;
    
    if(payload == NULL){
        return true;
    }
    if(header->payload_len == 0){
        return true;
    }

    printf_debug("payload[%d]:\n", header->payload_len);
    for(i = 0; i< header->payload_len; i++)
        printfd("%x ", payload[i]);
    printfd("\n");

    if(header->payload_len > MAX_SEND_DATA_LEN){
        *code = RET_CODE_PARAMTER_TOO_LONG;
        return false;
    }
/*    
    header->payload = calloc(1, header->payload_len);
    if (!header->payload){
        *code = RET_CODE_INTERNAL_ERR;
        printf_err("calloc failed\n");
        return false;
    }
*/    
    memcpy(header->payload, payload, header->payload_len);

    return true;
}

int xnrp_assamble_response_data(uint8_t **buf,          int err_code)
{
    struct xnrp_header *header, *response_header;
    header = &xnrp_data;
    response_header = &xnrp_response_data;
    int len = 0;
    static uint16_t msg_id_counter = 0;
    
    printf_info("Prepare to assamble response xnrp data\n");

    *buf = response_header;
    memcpy(response_header->start_flag, XNRP_HEADER_START, sizeof(response_header->start_flag)); 
    response_header->version = XNRP_HEADER_VERSION;
    response_header->method_code = METHOD_RESPONSE_COMMAND;
    response_header->error_code = err_code;
    response_header->class_code = header->class_code;
    response_header->business_code = header->business_code;
    memcpy(response_header->client_id, header->client_id, sizeof(response_header->client_id));
    memcpy(response_header->device_id, header->device_id, sizeof(response_header->device_id));
    response_header->time_stamp = time(NULL);
    response_header->msg_id = msg_id_counter++;
    response_header->check_sum = 0;

    len = sizeof(struct xnrp_header) - sizeof(response_header->payload) + response_header->payload_len;
   
    return len;
}


