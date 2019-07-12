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
    bool verbose = false;
    bool ssl = false;
    int port = 8081;
    int opt;

    if (!verbose)
        log_init(log_debug);
    
    printf_note("VERSION:%s\n",SPCTRUM_VERSION_STRING);
    config_init();
    //executor_init();
    uloop_init();
    if(server_init() == -1){
        printf_err("server init fail!\n");
        goto done;
    }
    uloop_run();
done:
    uloop_done();
    return 0;
}


