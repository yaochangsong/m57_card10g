#ifndef __SERIALCOM_H
#define __SERIALCOM_H

typedef void (*uart_callback)(struct uloop_fd *u, unsigned int events);

struct uart_t {
    struct uloop_fd fd;
};

struct uart_info {
    char *devname;
    uint32_t baudrate;
    char *info;
    int sfd;
    struct uloop_fd *fd;
    uart_callback cb;
};


extern void uart_init(void);
extern long uart0_send_data(uint8_t *buf, uint32_t len);
extern long uart0_send_string(uint8_t *buf);
extern int uart0_read_block_timeout(uint8_t *buf, int time_sec_ms);

extern long rs485_compass1_send_data(uint8_t *buf, uint32_t len);
extern int rs485_compass1_read_block_timeout(uint8_t *buf, int time_sec_ms);

extern long rs485_compass2_send_data(uint8_t *buf, uint32_t len);
extern int rs485_compass2_read_block_timeout(uint8_t *buf, int time_sec_ms);


#endif

