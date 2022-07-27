#ifndef FTP_SERVER_H
#define FTP_SERVER_H

typedef struct ftp_server_s{
    int port;
    ssize_t (*uplink_cb)(int);
    ssize_t (*downlink_cb)(int, const void *, size_t);
}ftp_server_t;

typedef struct ftp_client_s{
    int fd;
    ftp_server_t *server;
}ftp_client_t;


extern int create_socket(int port);
extern void ftp_server_init(int port);
#endif // FTP_SERVER_H

