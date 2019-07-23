#include "config.h"

struct lcd_code_convert_st convert_code_t[] ={
#ifdef LCD_TYPE == DWIN_K600_SERIAL_SCREEN
    {EX_NETWORK_CMD,  EX_NETWORK_IP,   SCREEN_COMMON_DATA1, SCREEN_IPADDR,              NULL},
    {EX_NETWORK_CMD,  EX_NETWORK_MASK, SCREEN_COMMON_DATA1, SCREEN_NETMASK_ADDR,        NULL},
    {EX_NETWORK_CMD,  EX_NETWORK_GW,   SCREEN_COMMON_DATA1, SCREEN_GATEWAY_ADDR,        NULL},
    {EX_NETWORK_CMD,  EX_NETWORK_PORT, SCREEN_COMMON_DATA1, SCREEN_PORT,                NULL},
    {EX_MID_FREQ_CMD, EX_MUTE_SW,      -1,                  SCREEN_CHANNEL_NOISE_EN,    NULL},
#endif
};

struct lcd_code_convert_st *lcd_code_convert(exec_cmd cmd, uint8_t type, uint8_t *arg)
{
    uint32_t i;
    int found = 0;

    for(i = 0; i<ARRAY_SIZE(convert_code_t); i++){
        if(convert_code_t[i].ex_cmd == cmd && convert_code_t[i].ex_type == type){
            convert_code_t[i].arg = arg;
            if(convert_code_t[i].lcd_cmd == -1){
                if(arg != NULL)
                    convert_code_t[i].lcd_cmd = *arg;
            }
            found = 1;
            return &convert_code_t[i];
        }
    }
    if(!found){
        printf_note("lcd code not found\n");
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
    printf_note("lcd_cmd:%d, lcd_type=%d\n", code->lcd_cmd , code->lcd_type);
    k600_receive_write_data_from_user(code->lcd_cmd , code->lcd_type, data);
#endif
    return 0;
}





