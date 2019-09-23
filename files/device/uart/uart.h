#ifndef __SERIALCOM_H
#define __SERIALCOM_H

#define SERIAL_BUF_LEN 1024

struct uart_t {
    struct uloop_fd fd;
};

extern void uart_init(void);
extern long uart0_send_data(uint8_t *buf, uint32_t len);
extern long uart1_send_data(uint8_t *buf, uint32_t len);
#endif

