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
extern char *get_version_string(void);


static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [option]\n"
        "          -d debug_level  # [3(LOG_ERR),4(LOG_WARNING),5(LOG_NOTICE),6(LOG_INFO),7(LOG_DEBUG),-1(OFF)]\n"
        "          -m power level  # [fft power level threshold, 1,2,3...n]\n"
        "          -c spectrum continuous mode # [true or false,Default false]\n"
        "          -f format disk  # [Warn:format once when start,Default false]\n"
        "          -t ADI Tool     # [(ADI IIO)specturm tool on; true or false,Default false]\n", prog);
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
bool disk_format = false;

bool is_disk_format(void)
{
    return disk_format;
}

bool spectrum_aditool_debug = false;

bool is_spectrum_aditool_debug(void)
{
    return spectrum_aditool_debug;
}

uint32_t power_level_threshold = 0;

uint32_t get_power_level_threshold(void)
{
    return power_level_threshold;
}

bool spectrum_continuous_mode = false;

bool is_spectrum_continuous_mode(void)
{
    return spectrum_continuous_mode;
}

int main(int argc, char **argv)
{
    int debug_level = -1;
    int opt;
    while ((opt = getopt(argc, argv, "d:tm:cf")) != -1) {
        switch (opt)
        {
        case 'd':
            printf("debug=%s\n", optarg);
            debug_level = atoi(optarg);
            if((debug_level > log_debug) ||
              ((debug_level != log_off) && (debug_level < log_err))){
                printf("NOT SUPPORT DEBUG LEVEL:%d\n", debug_level);
                usage(argv[0]);
                exit(-1);
            }
            break;
        case 't':
            spectrum_aditool_debug = true;
            printf("spectrum_aditool_debug:%d\n", spectrum_aditool_debug);
            break;
        case 'm':
            printf("power level=%s\n", optarg);
            power_level_threshold = atoi(optarg);
            break;
        case 'c':
            printf("spectrum continuous_mode true\n");
            spectrum_continuous_mode = true;
            break;
        case 'f':
            printf("format disk once; when start\n");
            disk_format = true;
            break;
        default: /* '?' */
            usage(argv[0]);
        }
    }
    log_init(debug_level);
    printf("Platform Start...\n");
    printf("VERSION:%s\n",get_version_string());
    config_init();
    uloop_init();
    gpio_raw_init();
#ifdef SUPPORT_PROJECT_SSA
    gpio_init_control();
#endif
#ifdef SUPPORT_AUDIO
    audio_init();
#endif
#ifdef SUPPORT_AMBIENT_TEMP_HUMIDITY
    temp_humidity_init();
#endif 
    executor_init();    /* DMA频谱初始化需要在时钟初始化之前 */
 if(spectrum_aditool_debug == false){
#ifdef SUPPORT_RF
    rf_init();  /* RF初始化， */
#endif
    clock_adc_init();   /* 时钟/AD初始化 */
#ifdef SUPPORT_SPECTRUM_FFT
    spectrum_init();
#endif
    }
#ifdef SUPPORT_UART
    uart_init();   /* PL UART依赖时钟 */
#endif
#ifdef SUPPORT_LCD
    init_lcd(); /* 串口屏依赖串口 */
#endif
#if defined(SUPPORT_XWFS)
    xwfs_init();
    http_requset_init();
#endif
    if(server_init() == -1){
        printf_err("server init fail!\n");
        goto done;
    }
    uloop_run();
done:
    executor_close();
    uloop_done();
    return 0;
}


