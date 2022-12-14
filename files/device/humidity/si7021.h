#ifndef _SI7021_H_H
#define _SI7021_H_H

#define SI7021_I2C_SLAVE_ADDR   0x40

/* Measure Relative Humidity, Hold Master Mode */
#define SI7021CMD_RH_HOLD       0xE5
/* Measure Temperature, Hold Master Mode */
#define SI7021CMD_TEMP_HOLD     0xE3
/* Software Reset */
#define SI7021CMD_RESET         0xFE

#define NONE_EXISTENT_TEMP         (-1000)
#define NONE_EXISTENT_HUMIDITY         (0)

#ifdef SUPPORT_TEMP_HUMIDITY_SI7021
#define temp_humidity_init() si7021_init()
#else 
#define temp_humidity_init() 0
#endif

extern void si7021_init(void);
extern float si7021_read_humidity(void);
extern float si7021_read_temperature(void);

#endif

