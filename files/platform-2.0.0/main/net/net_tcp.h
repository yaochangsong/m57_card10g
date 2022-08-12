#ifndef NET_SOCKET_H
#define NET_SOCKET_H

struct net_tcp_client;

#define MAX_RECEIVE_DATA_LEN  1024
#define NET_DATA_BUF_SIZE  512

#define MAX_SEND_DATA_LEN  1024
#define TCP_CONNECTION_TIMEOUT 60
#define TCP_MAX_KEEPALIVE_PROBES 3

enum net_tcp_state {
    NET_TCP_CLIENT_STATE_HEADER,
    NET_TCP_CLIENT_STATE_DATA,
    NET_TCP_CLIENT_STATE_DONE,
    NET_TCP_CLIENT_STATE_CLOSE
};

struct net_tcp_request {
    void *header;
    int   content_length; /*  payload len */
};

struct net_tcp_response {
    void *data;
    int   response_length; /*  payload len */
    int ch;
};

struct tcp_dispatch {
    int (*post_data)(struct net_tcp_client *cl, const char *data, int len);
    void (*post_done)(struct net_tcp_client *cl);
    void (*write_cb)(struct net_tcp_client *cl);
    void (*free)(struct net_tcp_client *cl);

    struct {
        int fd;
    } file;
    int cmd; /* add by ycs */
    int post_len;
    char *body;
};

struct tcp_section {
    bool type[32];
    int ch;
    void *thread;
};
struct net_tcp_client {
    struct list_head list;
    struct net_tcp_server *srv;
    struct ustream *us;
    struct ustream_fd sfd;
    struct uloop_timeout timeout;
    enum    net_tcp_state state;
    struct net_tcp_request request;
    struct net_tcp_response response;
    struct sockaddr_in peer_addr;
    struct sockaddr_in serv_addr;
    struct tcp_dispatch dispatch;
    struct tcp_section section;
    bool connection_close;
    int response_length;
    int tcp_keepalive_probes;
#if defined(CONFIG_FS)
    struct fs_context *fsctx;
#endif
    void (*parse_header)(struct net_tcp_client *cl, const void *data, int len);
    void (*parse_data)(struct net_tcp_client *cl, const void *data, int len);
    void (*free)(struct net_tcp_client *cl);
    void (*send_error)(struct net_tcp_client *cl, int code, const char *summary, const char *fmt, ...);
    void (*send_header)(struct net_tcp_client *cl, int code, const char *summary, int64_t length);
    void (*append_header)(struct net_tcp_client *cl, const char *name, const char *value);
    void (*header_end)(struct net_tcp_client *cl);
    void (*redirect)(struct net_tcp_client *cl, int code, const char *fmt, ...);
    void (*request_done)(struct net_tcp_client *cl);
    
    int (*send)(struct net_tcp_client *cl, const void *data, int len);
    int (*send_over)(size_t *size);
    void (*send_raw_data)(struct net_tcp_client *cl, const char *path, size_t (*callback) (void **), int (*callback_over) (size_t *));
    void (*printf)(struct net_tcp_client *cl, const char *format, ...);
    void (*vprintf)(struct net_tcp_client *cl, const char *format, va_list arg);

    const char *(*get_version)(struct net_tcp_client *cl);
    const char *(*get_peer_addr)(struct net_tcp_client *cl);
    const char *(*get_serv_addr)(struct net_tcp_client *cl);
    int (*get_peer_port)(struct net_tcp_client *cl);
    int (*get_serv_port)(struct net_tcp_client *cl);
    const char *(*get_header)(struct net_tcp_client *cl, const char *name);
    const char *(*get_body)(struct net_tcp_client *cl, int *len);
};


struct net_tcp_server {
    struct uloop_fd fd;
    struct list_head clients;
    int nclients;
    pthread_mutex_t tcp_client_lock;
    void (*free)(struct net_tcp_server *srv);
    void (*on_accept)(struct net_tcp_client *cl);
    int (*on_request)(struct net_tcp_client *cl);
    bool (*on_header)(void *cl,const char *buf, int len, int *head_len, int *code);
    bool (*on_execute)(void *cl, int *code);
    int (*on_end)(void *cl, char *buf, int len);
    void (*send)(struct net_tcp_client *cl, const void *data, int len, int code);
    void (*send_error)(void *cl, int code, void *args);
    void (*send_alert)(int code);
    size_t (*read_raw_data)(void **data);
};

struct net_tcp_server *tcp_server_new(const char *host, int port);
extern int get_ifa_name_by_ip(char *ipaddr, char *ifa_name);
extern void tcp_send_alert_to_all_client(int code);
extern bool tcp_find_client(struct sockaddr_in *addr);
extern void tcp_active_send_all_client(uint8_t *data, int len);
extern void update_tcp_keepalive(void *client);

#endif

