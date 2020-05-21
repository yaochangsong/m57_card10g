#ifndef __SERIALCOM_H
#define __SERIALCOM_H

#define SERIAL_BUF_LEN 1024
#define UART_HANDER_TIMROUT 1000   //ms
typedef void (*uart_callback)(struct uloop_fd *u, unsigned int events);

struct uart_t {
    struct uloop_fd fd;
};

struct uart_info {
    int index;
    char *devname;
    uint32_t baudrate;
    char *info;
    struct uloop_fd *fd;
    uart_callback cb;
    uloop_timeout_handler timeout_hander;
    struct uloop_timeout *timeout;
};


extern void uart_init(void);
extern long uart0_send_data(uint8_t *buf, uint32_t len);
extern long uart1_send_data(uint8_t *buf, uint32_t len);
extern long uart0_send_string(uint8_t *buf);
extern long uart1_send_string(uint8_t *buf);

#endif

