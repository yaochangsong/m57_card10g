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

uint8_t *xnrp_strstr(const uint8_t *s1, const uint8_t *s2, int len)   
{  
    int n = 0;  
    if (*s2)   
    {   
        while (*s1 && len < n)   
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
        return (uint8_t *)s1;   
} 


bool xnrp_parse_header(const uint8_t *data, int len)
{
    uint8_t *val;
    struct xnrp_header *header;
    header = &xnrp_data;
    int header_len;

    header_len = sizeof(struct xnrp_header) - sizeof(header->payload);

    if(len < header_len){
        printf_err("receive data len[%d] is too short\n", len);
        return false;
    }
    val = xnrp_strstr(data, XNRP_HEADER_START, len);
    if (!val)
        return NULL;

    memcpy(header, val, sizeof(struct xnrp_header) - sizeof(header->payload));
    printf_info("find header start=%x%x%x%x\n", header->start_flag[0], header->start_flag[1],
                                                header->start_flag[2], header->start_flag[3]);
    printf_info("header.version=%x\n", header->version);
    


}

bool xnrp_parse_data(const uint8_t *data, int len)
{
    struct xnrp_header *header;
    header = &xnrp_data;

    #if 0
    if(header.payload_len != 0){
        header.payload = calloc(1, header.payload_len);
        if (!header.payload){
            printf_err("calloc failed\n");
        }
        memcpy(header.payload, data + header_len, header.payload_len);
    }
    #endif
}

bool xnrp_handle_request(uint8_t *data, int len, int *code)
{
    printf_info("str = %s[%d]\n", data, len);
    printf_info("Prepare to handle xnrp protocol data\n");
    xnrp_parse_header(data, len);
    xnrp_parse_data(data, len);
    return true;
}

int xnrp_assamble_response_data(uint8_t *buf,          int err_code)
{
    int len = 0;
    printf_info("Prepare to assamble response xnrp data\n");
    return len;
}

