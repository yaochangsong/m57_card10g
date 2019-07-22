#ifndef _LCD_H_H_
#define _LCD_H_H_

enum lcd_type_e{
    DWIN_K600_SERIAL_SCREEN,
};

#define LCD_TYPE DWIN_K600_SERIAL_SCREEN

extern int8_t lcd_scanf(uint8_t *data, int32_t len);
extern int8_t lcd_printf(uint8_t cmd, uint8_t type, uint8_t *data);
#endif