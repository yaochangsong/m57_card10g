#ifndef _DWIN_SS_K600_H
#define _DWIN_SS_K600_H

extern int8_t k600_receive_from_user_data(uint8_t cmd, uint8_t type, uint8_t *data);
extern int8_t k600_send_to_user_data(uint8_t *data, int32_t len);
#endif