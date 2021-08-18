#ifndef __SERIALCOM_H
#define __SERIALCOM_H

#define SERIAL_BUF_LEN 1024

struct uart_info_t {
    int funcode;
    char *devname;
    uint32_t baudrate;
    char *info;
    bool is_cb;
    int sfd;
    struct uloop_fd fd;
};

extern void uart_init(void);
extern int uart_init_dev(char *name, uint32_t baudrate);
extern struct uart_info_t *uart_get_dev_by_code(int funcode);
extern ssize_t uart_send_data(int funcode, uint8_t *buf, uint32_t len);
extern ssize_t uart_read_timeout(int funcode, uint8_t *buf, int time_sec_ms);

#endif

