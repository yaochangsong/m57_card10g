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

bool akt_handle_request(char *data, int len, int *code)
{
    printf_info("str = %s[%d]\n", data, len);
    printf_info("Prepare to handle akt protocol data\n");
    return true;
}

int akt_assamble_response_data(char *buf,          int err_code)
{
    int len = 0;
    printf_info("Prepare to assamble response akt data\n");
    return len;
}


