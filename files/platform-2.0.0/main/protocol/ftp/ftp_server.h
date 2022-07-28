#ifndef FTP_SERVER_H
#define FTP_SERVER_H

typedef struct ftp_server_s{
    int port;
    int (*pre_cb)(int, void *);
    ssize_t (*uplink_cb)(int, void *);
    ssize_t (*downlink_cb)(int, const void *, size_t);
    int (*post_cb)(int, void *);
}ftp_server_t;

typedef struct ftp_client_s{
    int fd;
    struct sockaddr_in client_address;
    ftp_server_t *server;
}ftp_client_t;


extern int create_socket(int port);
extern void ftp_server_init(int port);
#endif // FTP_SERVER_H

