#include "config.h"


struct lcd_code_convert_st convert_code_t[] = {
#if LCD_TYPE == DWIN_K600_SERIAL_SCREEN
//contrl para
    {EX_CTRL_CMD,   EX_CTRL_LOCAL_REMOTE ,SCREEN_COMMON_DATA1, SCREEN_CTRL_TYPE,      NULL},
//net para
    {EX_NETWORK_CMD,  EX_NETWORK_IP,   SCREEN_COMMON_DATA1, SCREEN_IPADDR,              NULL},
    {EX_NETWORK_CMD,  EX_NETWORK_MASK, SCREEN_COMMON_DATA1, SCREEN_NETMASK_ADDR,        NULL},
    {EX_NETWORK_CMD,  EX_NETWORK_GW,   SCREEN_COMMON_DATA1, SCREEN_GATEWAY_ADDR,        NULL},
    {EX_NETWORK_CMD,  EX_NETWORK_PORT, SCREEN_COMMON_DATA1, SCREEN_PORT,                NULL},
//state para
    //{EX_STATUS_CMD,   EX_CLK_STATUS ,  SCREEN_COMMON_DATA2, SCREEN_LED_CLK_STATUS,      NULL},  //clk status
    {EX_STATUS_CMD,   EX_AD_STATUS ,  SCREEN_COMMON_DATA2, SCREEN_LED_AD_STATUS,      NULL},    //ad status
    //{EX_STATUS_CMD,   EX_CPU_DATA ,  SCREEN_COMMON_DATA2, SCREEN_CPU_DATA_STATUS,      NULL},   //cpu 
    {EX_STATUS_CMD,   EX_FPGA_TEMPERATURE ,  SCREEN_COMMON_DATA2, SCREEN_FPGA_TEMP_STATUS,      NULL}, //fpga temp

    
//rf,if para
    //{EX_MID_FREQ_CMD, EX_MUTE_SW,      -1,                  SCREEN_CHANNEL_NOISE_EN,    NULL},
    {EX_MID_FREQ_CMD, EX_DEC_BW,       -1,                  SCREEN_CHANNEL_BW,          NULL},   //解调带宽
    {EX_MID_FREQ_CMD, EX_SUB_CH_DEC_BW,       -1,           SCREEN_CHANNEL_BW,          NULL},   //解调带宽
    {EX_RF_FREQ_CMD,  EX_RF_MODE_CODE, -1,                  SCREEN_CHANNEL_MODE,        NULL},   //射频模式
    {EX_MID_FREQ_CMD, EX_DEC_METHOD,   -1,                  SCREEN_CHANNEL_DECODE_TYPE, NULL},   //解调方式
    {EX_RF_FREQ_CMD,  EX_RF_MID_FREQ , -1,                  SCREEN_CHANNEL_FRQ,         NULL},   //射频输入频率
    {EX_RF_FREQ_CMD,  EX_RF_MGC_GAIN , -1,                  SCREEN_CHANNEL_MGC,         NULL},   //mgc
    {EX_RF_FREQ_CMD,  EX_RF_ATTENUATION , -1,               SCREEN_CHANNEL_RF_GAIN,     NULL},   //rf gain
    {EX_RF_FREQ_CMD,  EX_RF_GAIN_MODE , -1,                 SCREEN_CHANNEL_GAIN_MODE,   NULL},   //增益模式
    {EX_RF_FREQ_CMD,  EX_RF_MID_FREQ_FILTER , -1,           SCREEN_CHANNEL_MID_FREQ,    NULL},   //射频中频频率
    {EX_RF_FREQ_CMD,  EX_RF_MID_BW,      -1,                SCREEN_CHANNEL_RF_BW,       NULL},   //中频模拟带宽
    //{EX_MID_FREQ_CMD, EX_DEC_BW,       -1,                  SCREEN_CHANNEL_BW,          NULL},   //解调带宽
    {EX_MID_FREQ_CMD, EX_MUTE_SW,       -1,                 SCREEN_CHANNEL_NOISE_EN,    NULL},   //静躁开关
    {EX_MID_FREQ_CMD, EX_MUTE_THRE,       -1,               SCREEN_CHANNEL_NOISE_VAL,   NULL},   //静躁门限
    
    {EX_CTRL_CMD,   EX_CTRL_AUDIO_VOLUME ,-1,               SCREEN_CHANNEL_VOLUME,      NULL},
#endif
};

struct lcd_code_convert_st *lcd_code_convert(exec_cmd cmd, uint8_t type, uint8_t *arg, struct lcd_code_convert_st *lcdcode)
{
    uint32_t i;
    int found = 0;
    //static struct lcd_code_convert_st lcd_code;
    struct lcd_code_convert_st *lcd_code = lcdcode;
    for(i = 0; i<ARRAY_SIZE(convert_code_t); i++){
        if((convert_code_t[i].ex_cmd == cmd) && (convert_code_t[i].ex_type == type)){
            lcd_code->ex_cmd = convert_code_t[i].ex_cmd;
            lcd_code->ex_type = convert_code_t[i].ex_type;
            lcd_code->lcd_type = convert_code_t[i].lcd_type;
            if(convert_code_t[i].lcd_cmd == -1){
                if(arg != NULL){
                    lcd_code->lcd_cmd = *arg;
                }
                else{
                    printf_debug("lcd code not set\n");
                }
            }else{
                lcd_code->lcd_cmd = convert_code_t[i].lcd_cmd;
            }
            found = 1;
            return lcd_code;
        }
    }
    if(!found){
         printf_debug("[%d %d]lcd code not found\n", cmd, type);
        return NULL;
    }
    return NULL;
}


/******************************************************************************
* FUNCTION:
*     lcd_printf
*
* DESCRIPTION:
*     receive data from lcd input
* PARAMETERS
*     data: input data
*     len: data length
* RETURNS
*     0: ok
*    -1: false
******************************************************************************/
int8_t lcd_scanf(uint8_t *data, int32_t len)
{
#if LCD_TYPE == DWIN_K600_SERIAL_SCREEN
    k600_scanf(data, len);
#endif
    return 0;
}

/******************************************************************************
* FUNCTION:
*     lcd_printf
*
* DESCRIPTION:
*     display data value on lcd screen
* PARAMETERS
*     cmd: internal command, define on file executor.h 
*     type:internal command type, define on file executor.h 
*     data: display data
*      arg: external paramter(ex: channel...)
* RETURNS
*     0: ok
*    -1: false
******************************************************************************/
int8_t lcd_printf(uint8_t cmd, uint8_t type, void *data, uint8_t *ch)
{
    struct lcd_code_convert_st code, *pcode;
    uint8_t ch_lcd;
    if(data == NULL){
        return -1;
    }
    
    if (ch != NULL) {
        ch_lcd = *ch + 1;
        pcode = lcd_code_convert(cmd, type, &ch_lcd, &code);
    } else {
        pcode = lcd_code_convert(cmd, type, NULL, &code);
    }
    
    if(pcode == NULL){
        return -1;
    }
#if LCD_TYPE == DWIN_K600_SERIAL_SCREEN
    printf_debug("lcd_cmd:%d, lcd_type=%d[0x%x]\n", pcode->lcd_cmd , pcode->lcd_type, pcode->lcd_type);
    k600_receive_write_data_from_user(pcode->lcd_cmd , pcode->lcd_type, data);
#endif
    return 0;
}


int8_t init_lcd(void)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    uint8_t i, ch, status = 0, disk_status= 0, ad_status = 0, check_status = 0;
    uint32_t host_addr;
    uint8_t volume = 0;
    int8_t noise_en = 0, noise_thrh = -80;
    printf_note("Lcd Init...\n");

    config_read_by_cmd(EX_STATUS_CMD, EX_CLK_STATUS, 0, &status);
    printf_debug("Clk status:%d\n", status);
    lcd_printf(EX_STATUS_CMD, EX_CLK_STATUS, &status, NULL);
    
#if defined(CONFIG_FS_XW)
    config_read_by_cmd(EX_STATUS_CMD, EX_DISK_STATUS, 0, &disk_status);
    printf_debug("Disk status:%d\n", disk_status);
    check_status |= (disk_status == 0) ? 0 : 1;
#endif
    config_read_by_cmd(EX_STATUS_CMD, EX_AD_STATUS, 0, &ad_status);
    printf_debug("Ad status:%d\n", ad_status);
    check_status |= (ad_status == 0) ? 0 : 1;
    printf_debug("check status:%d\n", check_status);
    lcd_printf(EX_STATUS_CMD, EX_CHECK_STATUS, &check_status, NULL);

    int index = 0;
    for(int i = 0; i < ARRAY_SIZE(poal_config->network); i++){
        if(!strcmp(poal_config->network[i].ifname, CONFIG_NET_1G_IFNAME)){
            index = i;
            break;
        }
    }
    host_addr = ntohl(poal_config->network[index].addr.ipaddress);
    printf_info("###ip[0x%x]\n", host_addr);
    lcd_printf(EX_NETWORK_CMD, EX_NETWORK_IP,   &host_addr, NULL);
    printf_info("###netmask\n");
    host_addr = ntohl(poal_config->network[index].addr.netmask);
    lcd_printf(EX_NETWORK_CMD, EX_NETWORK_MASK, &host_addr, NULL);
    printf_info("###gw\n");
    host_addr = ntohl(poal_config->network[index].addr.gateway);
    lcd_printf(EX_NETWORK_CMD, EX_NETWORK_GW,   &host_addr, NULL);
    printf_info("###port[%d]\n", poal_config->network[index].port);
    lcd_printf(EX_NETWORK_CMD, EX_NETWORK_PORT, &poal_config->network[index].port, NULL);
    for(i = 0; i< MAX_RADIO_CHANNEL_NUM; i++){
        volume = poal_config->channel[i].multi_freq_point_param.points[0].audio_volume;
        if (volume == 0) {
            volume = 50;
        }
        io_set_audio_volume(i, volume);
        lcd_printf(EX_CTRL_CMD, EX_CTRL_AUDIO_VOLUME,   &volume, &i);

        printf_debug("[lcd init]ch%d dec bw:%u\n",i, poal_config->channel[i].multi_freq_point_param.points[0].d_bandwith);
        lcd_printf(EX_MID_FREQ_CMD, EX_DEC_BW,   &poal_config->channel[i].multi_freq_point_param.points[0].d_bandwith, &i);
        executor_set_command(EX_MID_FREQ_CMD,  EX_SUB_CH_DEC_BW, i, &poal_config->channel[i].multi_freq_point_param.points[0].d_bandwith, poal_config->channel[i].multi_freq_point_param.points[0].d_method);

        printf_debug("[lcd init]ch%d dec method:%u\n",i, poal_config->channel[i].multi_freq_point_param.points[0].d_method);
        lcd_printf(EX_MID_FREQ_CMD, EX_DEC_METHOD,   &poal_config->channel[i].multi_freq_point_param.points[0].d_method, &i);
        executor_set_command(EX_MID_FREQ_CMD, EX_DEC_METHOD, i, &poal_config->channel[i].multi_freq_point_param.points[0].d_method);

        printf_debug("[lcd init]ch%d rf freq:%"PRIu64"\n",i, poal_config->channel[i].rf_para.mid_freq);
        lcd_printf(EX_RF_FREQ_CMD,  EX_RF_MID_FREQ,   &poal_config->channel[i].rf_para.mid_freq, &i); 
        executor_set_command(EX_RF_FREQ_CMD, EX_RF_MID_FREQ, i, &poal_config->channel[i].rf_para.mid_freq);
        executor_set_command(EX_MID_FREQ_CMD, EX_SUB_CH_MID_FREQ, i, &poal_config->channel[i].rf_para.mid_freq, poal_config->channel[i].rf_para.mid_freq, i);

        printf_debug("[lcd init]ch%d rf mode:%u\n",i, poal_config->channel[i].rf_para.rf_mode_code);
        lcd_printf(EX_RF_FREQ_CMD,  EX_RF_MODE_CODE,   &poal_config->channel[i].rf_para.rf_mode_code, &i);
        executor_set_command(EX_RF_FREQ_CMD, EX_RF_MODE_CODE, i, &poal_config->channel[i].rf_para.rf_mode_code);

        printf_debug("[lcd init]ch%d mgc:%d\n",i, poal_config->channel[i].rf_para.mgc_gain_value);
        lcd_printf(EX_RF_FREQ_CMD,  EX_RF_MGC_GAIN,   &poal_config->channel[i].rf_para.mgc_gain_value, &i);
        executor_set_command(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, i, &poal_config->channel[i].rf_para.mgc_gain_value);

        printf_debug("[lcd init]ch%d rf gain:%d\n",i, poal_config->channel[i].rf_para.attenuation);
        lcd_printf(EX_RF_FREQ_CMD,  EX_RF_ATTENUATION,   &poal_config->channel[i].rf_para.attenuation, &i);
        executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, i, &poal_config->channel[i].rf_para.attenuation);

        printf_debug("[lcd init]ch%d gain mode:%d\n",i, poal_config->channel[i].rf_para.gain_ctrl_method);
        lcd_printf(EX_RF_FREQ_CMD,  EX_RF_GAIN_MODE,   &poal_config->channel[i].rf_para.gain_ctrl_method, &i);

        printf_debug("[lcd init]ch%d mid_bw:%u\n",i, poal_config->channel[i].rf_para.mid_bw);
        lcd_printf(EX_RF_FREQ_CMD,  EX_RF_MID_BW,   &poal_config->channel[i].rf_para.mid_bw, &i);
        executor_set_command(EX_RF_FREQ_CMD, EX_RF_MID_BW, i, &poal_config->channel[i].rf_para.mid_bw);

        printf_debug("[lcd init]ch%d board_mid_freq:%"PRIu64"\n",i, poal_config->channel[i].rf_para.board_mid_freq);
        lcd_printf(EX_RF_FREQ_CMD,  EX_RF_MID_FREQ_FILTER,   &poal_config->channel[i].rf_para.board_mid_freq, &i);
        executor_set_command(EX_RF_FREQ_CMD, EX_RF_MID_FREQ_FILTER, i, &poal_config->channel[i].rf_para.board_mid_freq);
        executor_set_command(EX_MID_FREQ_CMD, EX_MID_FREQ, i, &poal_config->channel[i].rf_para.board_mid_freq, poal_config->channel[i].rf_para.board_mid_freq);

        noise_en = poal_config->channel[i].multi_freq_point_param.points[0].noise_en;
        noise_thrh = poal_config->channel[i].multi_freq_point_param.points[0].noise_thrh;
        printf_debug("[lcd init]ch%d noise en:%d, level:%d\n",i, noise_en, noise_thrh);
        lcd_printf(EX_MID_FREQ_CMD,  EX_MUTE_SW,   &noise_thrh, &i);
        lcd_printf(EX_MID_FREQ_CMD,  EX_MUTE_THRE,   &noise_en, &i);
        executor_set_command(EX_MID_FREQ_CMD, EX_MUTE_THRE, CONFIG_AUDIO_CHANNEL, &noise_thrh, noise_en);
    }
    return 0;
}


