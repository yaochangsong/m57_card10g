#include "config.h"

struct lcd_code_convert_st convert_code_t[] ={
#if LCD_TYPE == DWIN_K600_SERIAL_SCREEN
    {EX_NETWORK_CMD,  EX_NETWORK_IP,   SCREEN_COMMON_DATA1, SCREEN_IPADDR,              NULL},
    {EX_NETWORK_CMD,  EX_NETWORK_MASK, SCREEN_COMMON_DATA1, SCREEN_NETMASK_ADDR,        NULL},
    {EX_NETWORK_CMD,  EX_NETWORK_GW,   SCREEN_COMMON_DATA1, SCREEN_GATEWAY_ADDR,        NULL},
    {EX_NETWORK_CMD,  EX_NETWORK_PORT, SCREEN_COMMON_DATA1, SCREEN_PORT,                NULL},
    //{EX_MID_FREQ_CMD, EX_MUTE_SW,      -1,                  SCREEN_CHANNEL_NOISE_EN,    NULL},
    {EX_RF_FREQ_CMD, EX_RF_MID_BW,      -1,                  SCREEN_CHANNEL_RF_BW,    NULL},
    {EX_MID_FREQ_CMD, EX_DEC_BW,       -1,                  SCREEN_CHANNEL_BW,          NULL},
    {EX_RF_FREQ_CMD,  EX_RF_MODE_CODE, -1,                  SCREEN_CHANNEL_MODE,        NULL},
    {EX_MID_FREQ_CMD, EX_DEC_METHOD,   -1,                  SCREEN_CHANNEL_DECODE_TYPE, NULL},
    {EX_RF_FREQ_CMD,  EX_RF_MID_FREQ , -1,                  SCREEN_CHANNEL_FRQ,         NULL},
    {EX_RF_FREQ_CMD,  EX_RF_MGC_GAIN , -1,                  SCREEN_CHANNEL_MGC,         NULL},
    {EX_STATUS_CMD,   EX_CLK_STATUS ,  SCREEN_COMMON_DATA2, SCREEN_LED_CLK_STATUS,      NULL},
    {EX_STATUS_CMD,   EX_CHECK_STATUS ,SCREEN_COMMON_DATA2, SCREEN_LED1_STATUS,      NULL},
    
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
    if(data == NULL){
        return -1;
    }
    pcode = lcd_code_convert(cmd, type, ch, &code);
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
    printf_note("Lcd Init...\n");

    config_read_by_cmd(EX_STATUS_CMD, EX_CLK_STATUS, 0, &status);
    printf_debug("Clk status:%d\n", status);
    lcd_printf(EX_STATUS_CMD, EX_CLK_STATUS, &status, NULL);
    
#if defined(SUPPORT_XWFS)
    config_read_by_cmd(EX_STATUS_CMD, EX_DISK_STATUS, 0, &disk_status);
    printf_debug("Disk status:%d\n", disk_status);
    check_status |= (disk_status == 0) ? 0 : 1;
#endif
    config_read_by_cmd(EX_STATUS_CMD, EX_AD_STATUS, 0, &ad_status);
    printf_debug("Ad status:%d\n", ad_status);
    check_status |= (ad_status == 0) ? 0 : 1;
    printf_debug("check status:%d\n", check_status);
    lcd_printf(EX_STATUS_CMD, EX_CHECK_STATUS, &check_status, NULL);

    host_addr = ntohl(poal_config->network.ipaddress);
    printf_info("###ip[0x%x]\n", host_addr);
    lcd_printf(EX_NETWORK_CMD, EX_NETWORK_IP,   &host_addr, NULL);
    printf_info("###netmask\n");
    host_addr = ntohl(poal_config->network.netmask);
    lcd_printf(EX_NETWORK_CMD, EX_NETWORK_MASK, &host_addr, NULL);
    printf_info("###gw\n");
    host_addr = ntohl(poal_config->network.gateway);
    lcd_printf(EX_NETWORK_CMD, EX_NETWORK_GW,   &host_addr, NULL);
    printf_info("###port[%d]\n", poal_config->network.port);
    lcd_printf(EX_NETWORK_CMD, EX_NETWORK_PORT, &poal_config->network.port, NULL);
    for(i = 0; i< MAX_RADIO_CHANNEL_NUM; i++){
        ch = i +1;
        printf_info("###noise_en=%d\n", poal_config->channel[i].multi_freq_point_param.points[0].noise_en);
        lcd_printf(EX_RF_FREQ_CMD, EX_RF_MID_BW,   &poal_config->channel[i].rf_para.mid_bw, &ch);
        printf_info("###dec_bandwith=%d\n", poal_config->channel[i].multi_freq_point_param.points[0].d_bandwith);
        lcd_printf(EX_MID_FREQ_CMD, EX_DEC_BW,   &poal_config->channel[i].multi_freq_point_param.points[0].d_bandwith, &ch);
        printf_info("###dec_method=%d\n", poal_config->channel[i].multi_freq_point_param.points[0].d_method);
        lcd_printf(EX_MID_FREQ_CMD, EX_DEC_METHOD,   &poal_config->channel[i].multi_freq_point_param.points[0].d_method, &ch);
        printf_info("###rf_mode_code=%d\n", poal_config->channel[i].rf_para.rf_mode_code);
        lcd_printf(EX_RF_FREQ_CMD,  EX_RF_MODE_CODE,   &poal_config->channel[i].rf_para.rf_mode_code, &ch);
        printf_info("###mid_freq=%llu\n", poal_config->channel[i].rf_para.mid_freq);
        lcd_printf(EX_RF_FREQ_CMD,  EX_RF_MID_FREQ,   &poal_config->channel[i].rf_para.mid_freq, &ch); 
        printf_info("###mgc_gain_value=%d\n", poal_config->channel[i].rf_para.mgc_gain_value);
        lcd_printf(EX_RF_FREQ_CMD,  EX_RF_MGC_GAIN,   &poal_config->channel[i].rf_para.mgc_gain_value, &ch);
        printf_info("###mid_bw=%d\n", poal_config->channel[i].rf_para.mid_bw);
        lcd_printf(EX_RF_FREQ_CMD,  SCREEN_CHANNEL_RF_BW,   &poal_config->channel[i].rf_para.mid_bw, &ch);
    }
    return 0;
}


