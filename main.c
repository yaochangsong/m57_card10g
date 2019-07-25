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

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [option]\n"
        "          -p port  # Default port is 8080\n"
        "          -s       # SSl on\n"
        "          -v       # verbose\n", prog);
    exit(1);
}

/******************************************************************************
* FUNCTION:
*     main
*
* DESCRIPTION:
*     
*     
*
* PARAMETERS
*     not used
*
* RETURNS
*     none
******************************************************************************/
int main(int argc, char **argv)
{
    log_init(log_debug);
    printf_note("VERSION:%s\n",SPCTRUM_VERSION_STRING);
    config_init();
    executor_init();
    uloop_init();
    uart_init();
#if (UART_LCD_SUPPORT == 1)
    init_lcd();
#endif
    uint8_t buf[]={0x5A,0xA5,0x06,0x83,0x00,0x0D,0x01,0x30,0x12};
    lcd_scanf(buf, sizeof(buf));
    spi_init();
    if(server_init() == -1){
        printf_err("server init fail!\n");
        goto done;
    }
    uloop_run();
done:
    uloop_done();
    return 0;
}


