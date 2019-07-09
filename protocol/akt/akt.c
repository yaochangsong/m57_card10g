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

PDU_CFG_RSP_HEADER_ST akt_header;

static int akt_execute_method(void)
{
    return 0;
}

static bool akt_parse_header(const uint8_t *data, int len, uint8_t **payload, int *err_code)
{
    uint8_t *val;
    PDU_CFG_REQ_HEADER_ST *header, *pdata;
    header = &akt_header;
    pdata = data;
    int header_len;

    printf_debug("parse_header[%x,%x]\n", data[0], data[1]);
    if(pdata->start_flag != AKT_START_FLAG){
        printf_debug("parse_header error\n");
        *err_code = RET_CODE_FORMAT_ERR;
        return false;
    }
    memcpy(header, pdata, sizeof(PDU_CFG_REQ_HEADER_ST));
    
    return true;
}


static bool akt_parse_data(const uint8_t *payload)
{
    return true;
}


bool akt_handle_request(char *data, int len, int *code)
{
    uint8_t *payload = NULL;
    
    printf_info("len[%d] %d\n", len, sizeof(struct xnrp_header));
    printf_info("Prepare to handle akt protocol data\n");
    if(akt_parse_header(data, len, &payload, code) == false){
        return false;
    }
    if(payload != NULL){
        if(akt_parse_data(payload) == false){
            *code = RET_CODE_INTERNAL_ERR;
            return false;
        }
    }

    if(akt_execute_method() != RET_CODE_SUCCSESS){
        return false;
    }
    return true;
}

int akt_assamble_response_data(char *buf,          int err_code)
{
    int len = 0;
    printf_info("Prepare to assamble response akt data\n");
    return len;
}


