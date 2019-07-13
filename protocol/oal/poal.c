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


int poal_send_ok_response(struct net_tcp_client *cl, uint8_t *data, int len)
{
    char buf[]="send ok response\n";
    printf_info("send ok response\n");
    ustream_write(cl->us, buf, sizeof(buf), true);
    return 0;
}

int poal_send_error_response(struct net_tcp_client *cl, int error_code)
{
    char buf[]="send error response\n";
    printf_info("send error[%d] response\n", error_code);
    ustream_write(cl->us, buf, sizeof(buf), true);
    return 0;
}

/******************************************************************************
* FUNCTION:
*     poal_parse_request
*
* DESCRIPTION:
*      receive data handle process
*     
* PARAMETERS
*     data:  receive data pointer
*       len:  reveive data length
*       code:  return error code
* RETURNS
*     false: handle data false 
*      ture: handle data successful
******************************************************************************/

bool poal_parse_request(uint8_t *data, int len, int *code)
{
    uint8_t *payload = NULL;
    
    printf_info("Prepare to parse data\n");
    if(poal_parse_header(data, len, &payload, code) == false){
        return false;
    }
    if(payload != NULL){
        if(poal_parse_data(payload, code) == false){
            return false;
        }
    }

    if(poal_execute_method(code) == false){
        poal_free();
        return false;
    }
    poal_free();
    return true;
}


int poal_handle_request(struct net_tcp_client *cl, uint8_t *data, int len)
{
    int error_code = 0;
    int send_len = 0;
    uint8_t send_buf[MAX_SEND_DATA_LEN];

    if(poal_parse_request(data, len, &error_code)){
        send_len = poal_assamble_response_data(send_buf, error_code);
        if(send_len > MAX_SEND_DATA_LEN){
            printf_err("%d is too long\n", send_len);
        }
        poal_send_ok_response(cl, send_buf, send_len);
    }else{
        printf_info("error_code = %d\n", error_code);
        poal_send_error_response(cl, error_code);
    }
    return -1;
}


