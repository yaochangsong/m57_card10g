#include "config.h"

struct lcd_code_convert_st convert_code_t[] ={
#ifdef LCD_TYPE == DWIN_K600_SERIAL_SCREEN
    {EX_NETWORK_CMD,  EX_NETWORK_IP,   SCREEN_COMMON_DATA1, SCREEN_IPADDR,              NULL},
    {EX_NETWORK_CMD,  EX_NETWORK_MASK, SCREEN_COMMON_DATA1, SCREEN_NETMASK_ADDR,        NULL},
    {EX_NETWORK_CMD,  EX_NETWORK_GW,   SCREEN_COMMON_DATA1, SCREEN_GATEWAY_ADDR,        NULL},
    {EX_NETWORK_CMD,  EX_NETWORK_PORT, SCREEN_COMMON_DATA1, SCREEN_PORT,                NULL},
    {EX_MID_FREQ_CMD, EX_MUTE_SW,      -1,                  SCREEN_CHANNEL_NOISE_EN,    NULL},
    {EX_MID_FREQ_CMD, EX_DEC_BW,       -1,                  SCREEN_CHANNEL_BW,          NULL},
    {EX_RF_FREQ_CMD,  EX_RF_MODE_CODE, -1,                  SCREEN_CHANNEL_MODE,        NULL},
    {EX_MID_FREQ_CMD, EX_DEC_METHOD,   -1,                  SCREEN_CHANNEL_DECODE_TYPE, NULL},
    {EX_RF_FREQ_CMD,  EX_RF_MID_FREQ , -1,                  SCREEN_CHANNEL_FRQ,         NULL},
    {EX_RF_FREQ_CMD,  EX_RF_MGC_GAIN , -1,                  SCREEN_CHANNEL_MGC,         NULL},


#endif
};

struct lcd_code_convert_st *lcd_code_convert(exec_cmd cmd, uint8_t type, uint8_t *arg)
{
    uint32_t i;
    int found = 0;
    static struct lcd_code_convert_st lcd_code;
    for(i = 0; i<ARRAY_SIZE(convert_code_t); i++){
        if(convert_code_t[i].ex_cmd == cmd && convert_code_t[i].ex_type == type){
            lcd_code.ex_cmd = convert_code_t[i].ex_cmd;
            lcd_code.ex_type = convert_code_t[i].ex_type;
            lcd_code.lcd_type = convert_code_t[i].lcd_type;
            if(convert_code_t[i].lcd_cmd == -1){
                if(arg != NULL){
                    lcd_code.lcd_cmd = *arg;
                }
                else{
                    printf_note("lcd code not set\n");
                }
            }else{
                lcd_code.lcd_cmd = convert_code_t[i].lcd_cmd;
            }
            found = 1;
            return &lcd_code;
        }
    }
    if(!found){
         printf_note("[%d %d]lcd code not found\n", cmd, type);
        return NULL;
    }
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
#ifdef LCD_TYPE == DWIN_K600_SERIAL_SCREEN
    k600_send_data_to_user(data, len);
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
int8_t lcd_printf(uint8_t cmd, uint8_t type, void *data, uint8_t *arg)
{
    struct lcd_code_convert_st *code;
    if(data == NULL){
        return -1;
    }
    code = lcd_code_convert(cmd, type, arg);
    if(code == NULL){
        return -1;
    }
#ifdef LCD_TYPE == DWIN_K600_SERIAL_SCREEN
    printf_debug("lcd_cmd:%d, lcd_type=%d\n", code->lcd_cmd , code->lcd_type);
    k600_receive_write_data_from_user(code->lcd_cmd , code->lcd_type, data);
#endif
    return 0;
}


int8_t init_lcd(void)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    uint8_t i;
    
    lcd_printf(EX_NETWORK_CMD, EX_NETWORK_IP,   &poal_config->network.ipaddress, NULL);
    lcd_printf(EX_NETWORK_CMD, EX_NETWORK_MASK, &poal_config->network.netmask, NULL);
    lcd_printf(EX_NETWORK_CMD, EX_NETWORK_GW,   &poal_config->network.gateway, NULL);
    lcd_printf(EX_NETWORK_CMD, EX_NETWORK_PORT, &poal_config->network.port, NULL);
    for(i = 1; i<=MAX_RADIO_CHANNEL_NUM; i++){
        lcd_printf(EX_MID_FREQ_CMD, EX_MUTE_SW,   &poal_config->multi_freq_point_param[i].points[0].noise_en, &i);
        lcd_printf(EX_MID_FREQ_CMD, EX_DEC_BW,   &poal_config->multi_freq_point_param[i].points[0].d_bandwith, &i);
        lcd_printf(EX_MID_FREQ_CMD, EX_DEC_METHOD,   &poal_config->multi_freq_point_param[i].points[0].d_method, &i);
        lcd_printf(EX_RF_FREQ_CMD,  EX_RF_MODE_CODE,   &poal_config->rf_para[i].rf_mode_code, &i);
        lcd_printf(EX_RF_FREQ_CMD,  EX_RF_MID_FREQ,   &poal_config->rf_para[i].mid_freq, &i);
        lcd_printf(EX_RF_FREQ_CMD,  EX_RF_MGC_GAIN,   &poal_config->rf_para[i].mgc_gain_value, &i);
    }
    return 0;
}


