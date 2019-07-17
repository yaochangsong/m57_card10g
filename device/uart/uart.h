#ifndef __SERIALCOM_H
#define __SERIALCOM_H

int uart_init(char *name);
char *uart_write_read(int fd ,char *cmd,unsigned int cmd_len ,int *recv_len);

#endif

