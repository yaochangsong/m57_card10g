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
        "          -d debug_level  # [3(LOG_ERR),4(LOG_WARNING),5(LOG_NOTICE),6(LOG_INFO),7(LOG_DEBUG),-1(OFF)]\n", prog);
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
    int debug_level = -1;
    int opt;
    while ((opt = getopt(argc, argv, "d:")) != -1) {
        switch (opt)
        {
        case 'd':
            printf("optarg=%s\n", optarg);
            debug_level = atoi(optarg);
            if((debug_level > log_debug) ||
              ((debug_level != log_off) && (debug_level < log_err))){
                printf("NOT SUPPORT DEBUG LEVEL:%d\n", debug_level);
                usage(argv[0]);
                exit(-1);
            }
            break;
        default: /* '?' */
            usage(argv[0]);
        }
    }
    log_init(debug_level);
    printf_note("VERSION:%s\n",SPCTRUM_VERSION_STRING);
    config_init();
    uloop_init();
#if (UART_SUPPORT == 1)
    uart_init();
#endif
    gpio_init_control();
#if (UART_LCD_SUPPORT == 1)
    init_lcd();
#endif
    rf_init();
    executor_init();
    spectrum_init();
    if(server_init() == -1){
        printf_err("server init fail!\n");
        goto done;
    }
    uloop_run();
done:
    uloop_done();
    return 0;
}


