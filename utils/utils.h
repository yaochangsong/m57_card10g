#ifndef _UTILS_H_H_
#define _UTILS_H_H_


/*向上取整*/
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d)) 
#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))


extern char *safe_strdup(const char *s);
extern uint16_t crc16_caculate(uint8_t *pchMsg, uint16_t wDataLen);
extern int get_mac(char * mac, int len_limit);
#endif

