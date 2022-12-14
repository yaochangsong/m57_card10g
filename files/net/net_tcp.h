#ifndef NET_SOCKET_H
#define NET_SOCKET_H

struct net_tcp_client;

#define MAX_RECEIVE_DATA_LEN  4096
#define NET_DATA_BUF_SIZE  512

#define MAX_SEND_DATA_LEN  1024
#define TCP_CONNECTION_TIMEOUT 2//60
#define TCP_MAX_KEEPALIVE_PROBES 60

enum net_tcp_state {
    NET_TCP_CLIENT_STATE_HEADER,
    NET_TCP_CLIENT_STATE_DATA,
    NET_TCP_CLIENT_STATE_DONE,
    NET_TCP_CLIENT_STATE_CLOSE
};

enum net_tcp_data_state {
    NET_TCP_DATA_NORMAL,
    NET_TCP_DATA_ERR,
    NET_TCP_DATA_WAIT,
    NET_TCP_DATA_NO_DEAL,
};


struct net_tcp_request {
    void *header;
    int   prio;
    int   data_type;
    int   content_length; /*  payload len */
    int   remain_length; 
    int   data_state;
};

struct net_tcp_response {
    void *data;
    int   response_length; /*  payload len */
    int   prio;
    int ch;
    bool need_resp;
};

struct tcp_dispatch {
    int (*post_data)(struct net_tcp_client *cl, const char *data, int len);
    void (*post_done)(struct net_tcp_client *cl);
    void (*write_cb)(struct net_tcp_client *cl);
    void (*free)(struct net_tcp_client *cl);

    struct {
        int fd;
        uint32_t len;
    } file;
    int cmd; /* add by ycs */
    int post_len;
    char *body;
};

struct tcp_section {
    struct {
        char path[256];
        int fd;
        uint32_t len;
        int len_offset;
        int sn;
    } file;
    void *hash;
    bool  is_run_loadfile;
    bool  is_loadfile_ok;
    int chip_id;
    uint16_t beatheat;
    int prio;
	bool type[32];
    int section_id;
    struct net_thread_context *thread;
    struct asyn_load_ctx_t *load_bit_thread;
    volatile bool is_exitting;
    pthread_mutex_t free_lock;
    bool is_keytool_cmd;
    volatile bool is_unloading;
    volatile bool thread_exited;
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
    bool (*on_header)(void *cl, char *buf, int len, int *head_len, int *code);
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
extern int tcp_send_vec_data_uplink(struct iovec *iov, int iov_len, void *args);
extern int tcp_send_data_uplink(char  *data, int len, void *args);
extern int tcp_client_do_for_each(int (*f)(struct net_tcp_client *, void *), void **cl, int prio, void *args);
extern int send_vec_data_to_client(struct net_tcp_client *client, struct iovec *iov, int iov_len);
extern int tcp_send_data_to_client(int fd, const char *buf, int buflen);
extern int tcp_send_data_to_client_ex(struct net_tcp_client *client, const char *buf, int buflen);
extern struct net_tcp_client *tcp_find_prio_client(void *client, int prio);

#endif

