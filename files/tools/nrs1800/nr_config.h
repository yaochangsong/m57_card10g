#ifndef _CONF_NRS1800_H_H
#define _CONF_NRS1800_H_H

//#define DEBUG
#ifdef DEBUG
#define printf_debug(format, ...) printf(format, ##__VA_ARGS__)
#else
#define printf_debug(format, ...)
#endif

extern void nr_config_init(void);
#endif
