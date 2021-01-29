#ifndef __SERIALCOM_H
#define __SERIALCOM_H

#define SERIAL_BUF_LEN 1024
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

typedef enum _UART_INFO_INDEX{
  UART_INFO_GPS = 0x00,
  UART_INFO_NOISER = 0x01,
  UART_INFO_COMPASS_1_6G = 0x02,
  UART_INFO_COMPASS2_30_1350M = 0x03
}UART_INFO_INDEX;

extern void uart_init(void);
extern long uart0_send_data(uint8_t *buf, uint32_t len);
extern long uart0_send_string(uint8_t *buf);
extern int uart0_read_block_timeout(uint8_t *buf, int time_sec_ms);

extern long uart_compass1_send_data(uint8_t *buf, uint32_t len);
extern int uart_compass1_read_block_timeout(uint8_t *buf, int time_sec_ms);

extern long uart_compass2_send_data(uint8_t *buf, uint32_t len);
extern int uart_compass2_read_block_timeout(uint8_t *buf, int time_sec_ms);

extern struct uart_info *get_uart_info_by_index(int index);
extern int uart_init_dev(char *name, uint32_t baudrate);


#endif

