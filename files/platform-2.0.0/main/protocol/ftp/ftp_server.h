#ifndef FTP_SERVER_H
#define FTP_SERVER_H

#include "../../../main/utils/bitops.h"

#define FTP_MAX_CLIENT_NUM 64
typedef struct ftp_server_s{
    int port;
    int (*pre_cb)(int, void *);
    ssize_t (*uplink_cb)(int, void *);
    ssize_t (*downlink_cb)(int, const void *, size_t);
    int (*post_cb)(int, void *);
    int (*usr1_handle)(int,void *);
    DECLARE_BITMAP(map, FTP_MAX_CLIENT_NUM);
}ftp_server_t;

typedef struct ftp_client_s{
    int fd;
    int retr_idx;
    int pasv_fd;
    struct sockaddr_in client_address;
    ftp_server_t *server;
}ftp_client_t;

static inline void ftp_client_init_idx(ftp_server_t *s)
{
    bitmap_zero(s->map, FTP_MAX_CLIENT_NUM);
}

static inline int ftp_client_get_idx(ftp_server_t *s)
{
    int n = 0;
    n = (int)find_first_zero_bit(s->map, FTP_MAX_CLIENT_NUM);
    if(n >= FTP_MAX_CLIENT_NUM){
        return -1;
    }
    set_bit(n, s->map);

    return n;
}

static inline void ftp_client_idx_clear(int idx, ftp_server_t *s)
{
    clear_bit(idx, s->map);
}

static inline  size_t ftp_client_idx_weight(ftp_server_t *s)
{
    return bitmap_weight(s->map, FTP_MAX_CLIENT_NUM);
}

extern int create_socket(int port);
extern void ftp_server_init(int port);
#endif // FTP_SERVER_H

