#ifndef _LCD_H_H_
#define _LCD_H_H_

enum lcd_type_e{
    DWIN_K600_SERIAL_SCREEN,
};

#define LCD_TYPE DWIN_K600_SERIAL_SCREEN


struct lcd_code_convert_st{
    uint8_t ex_cmd;
    uint8_t ex_type;
    int8_t lcd_cmd;
    int8_t lcd_type;
    uint8_t *arg;
};

extern int8_t init_lcd(void);
extern int8_t lcd_scanf(uint8_t *data, int32_t len);
extern int8_t lcd_printf(uint8_t cmd, uint8_t type, void *data, uint8_t *arg);
#endif