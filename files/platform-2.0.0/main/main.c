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
extern void fpga_io_init(void);


static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [option]\n"
        "          -d debug_level  # [3(LOG_ERR),4(LOG_WARNING),5(LOG_NOTICE),6(LOG_INFO),7(LOG_DEBUG),-1(OFF)]\n"
        "          -m power level  # [fft power level threshold, 1,2,3...n]\n"
        "          -c config.json path #[default path: /etc/config.json]\n"
        "          -f format disk  # [Warn:format once when start,Default false]\n"
        "          -b bottom calibration  # [bottom calibration,Default false]\n"
        "          -o output stream sink filename, default fft data source\n"
        "          -t time ms of output stream sink filename\n"
        "          -a ADI Tool     # [(ADI IIO)specturm tool on; true or false,Default false]\n", prog);
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


int32_t signal_snr_offset = 25;

int32_t get_signal_snr(void)
{
    return signal_snr_offset;
}

char *config_path = NULL;
char *get_config_path(void)
{
    return config_path;
}

bool avg_bottom_noise_cali_en = false;

bool bottom_noise_cali_en(void)
{
    return avg_bottom_noise_cali_en;
}

char *sink_file_path_name = NULL;
char *get_sink_file_path_name(void)
{
    return sink_file_path_name;
}

int32_t sink_file_time_ms = 0;

int32_t get_sink_file_time_ms(void)
{
    return sink_file_time_ms;
}


static void pl_handle_sig(int sig)
{
    printf("ctrl+c or killed; ready to exit!!\n");
    usleep(1000);
    executor_close();
    uloop_done();
    exit(0);
}


void device_init(void)
{
#ifdef CONFIG_DEVICE_GPIO
    gpio_raw_init();
#endif
#ifdef CONFIG_DEVICE_AUDIO
    audio_init();
#endif
#ifdef CONFIG_DEVICE_HUMIDITY
    temp_humidity_init();
#endif 
#ifdef  CONFIG_DEVICE_ADC_CLOCK
    clock_adc_init();
#endif
#ifdef CONFIG_DEVICE_UART
    /*NOTE: PL uart depends on clock */
    uart_init();
#endif
 if(spectrum_aditool_debug == false){
#ifdef CONFIG_DEVICE_RF
    rf_init(); 
#endif
    }
#ifdef CONFIG_DEVICE_LCD
    init_lcd();
#endif
}

void register_init(void)
{
#ifdef CONFIG_FPGA_REGISTER
    fpga_io_init();
#endif
}

int main(int argc, char **argv)
{
    int debug_level = -1;
    int opt;
    while ((opt = getopt(argc, argv, "d:am:c:fbo:t:")) != -1) {
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
        case 'a':
            spectrum_aditool_debug = true;
            printf("spectrum_aditool_debug:%d\n", spectrum_aditool_debug);
            break;
        case 'm':
            printf("power level=%s\n", optarg);
            power_level_threshold = atoi(optarg);
            break;
        case 'c':
            printf("config path: %s\n", optarg);
            config_path = strdup(optarg);
            break;
        case 'f':
            printf("format disk once; when start\n");
            disk_format = true;
            break;
        case 'b':
            avg_bottom_noise_cali_en = true;
            printf("avg_bottom_noise_cali_en: true\n");
            break;
        case 'o':
            sink_file_path_name = strdup(optarg);
            printf("sink file: %s\n", sink_file_path_name);
            break;
        case 't':
            sink_file_time_ms = atoi(optarg);
            printf("sink file time: %dms\n", sink_file_time_ms);
            break;
        default: /* '?' */
            usage(argv[0]);
        }
    }
    log_init(debug_level);
    printf("Platform Start...\n");
#if (defined CONFIG_PROTOCOL_AKT)
    printf("ACT Protocal\n");
#elif defined(CONFIG_PROTOCOL_XW)
    printf("XW Protocal\n");
#endif
    printf("VERSION:%s\n",get_version_string());
    // Listen to ctrl+c and ASSERT
    signal(SIGINT, pl_handle_sig);
    signal(SIGKILL, pl_handle_sig);

    config_init();
    uloop_init();
    register_init();
    device_init();
    executor_init();
#if defined(CONFIG_FS)
    fs_init();
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
