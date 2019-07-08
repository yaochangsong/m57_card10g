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

int poal_send_ok_response(struct net_tcp_client *cl, char *data, int len)
{
    printf_info("send ok response\n");
    return 0;
}

int poal_send_error_response(struct net_tcp_client *cl, int error_code)
{
    printf_info("send error response\n");
    return 0;
}


int poal_handle_request(struct net_tcp_client *cl, char *data, int len)
{
    int error_code = 0;
    int send_len = 0;
    char send_buf[MAX_SEND_DATA_LEN];

    if(oal_handle_request(data, len, &error_code)){
        send_len = assamble_response_data(send_buf, error_code);
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


