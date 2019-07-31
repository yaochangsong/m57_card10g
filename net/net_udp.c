#include "config.h"

static void udp_ustream_write_cb(struct ustream *s, int bytes)
{
     printf_debug("tcp_ustream_write_cb[%d]bytes\n",bytes);
}

static void udp_free(struct net_udp_client *cl)
{
    printf_debug("udp_free\n");
    if (cl) {
       // close(cl->sfd.fd.fd);
        printf_debug("close\n");
        list_del(&cl->list);
        cl->srv->nclients--;
        free(cl);
    }
}


static inline const char *udp_get_peer_addr(struct net_udp_client *cl)
{
    return inet_ntoa(cl->peer_addr.sin_addr);
}

static inline int udp_get_peer_port(struct net_udp_client *cl)
{
    return ntohs(cl->peer_addr.sin_port);
}

static void udp_read_cb(struct uloop_fd *fd, unsigned int events)
{
    struct net_udp_server *srv = container_of(fd, struct net_udp_server, fd);
    struct net_udp_client *cl = NULL;
    unsigned int sl, n;
    
    struct sockaddr_in addr;
    uint8_t data[MAX_RECEIVE_DATA_LEN];

    sl = sizeof(addr);
    
    n = recvfrom(srv->fd.fd, data, MAX_RECEIVE_DATA_LEN, 0, (struct sockaddr *)&addr, &sl);
    if(n < 0){
        perror("recvfrom");
        return;
    }

    struct net_udp_client *cl_list, *list_tmp;
    list_for_each_entry_safe(cl_list, list_tmp, &srv->clients, list){
        if(memcmp(&cl_list->peer_addr.sin_addr, &addr.sin_addr, sizeof(addr.sin_addr)) == 0){
            printf_debug("Find ipaddree on list:%s， tag=%d\n",  cl_list->get_peer_addr(cl_list), cl_list->tag);
            cl = cl_list;
            goto udp_handle;
        }
    }
    if(srv->nclients > MAX_CLINET_NUM){
        printf_warn("server client is full\n");
        struct net_udp_client cl_tmp;
        cl = &cl_tmp;
    }else{
        cl = calloc(1, sizeof(struct net_udp_client));
        if (!cl) {
            printf_err("calloc\n");
            return;
        }
        memcpy(&cl->peer_addr, &addr, sizeof(addr));
        cl->get_peer_addr = udp_get_peer_addr;
        cl->get_peer_port = udp_get_peer_port;

        list_add(&cl->list, &srv->clients);
        cl->srv = srv;
        cl->srv->nclients++;
        printf_info("Receive New UDP data From: %s:%d\n", cl->get_peer_addr(cl), cl->get_peer_port(cl));
    }
udp_handle:
    if(cl != NULL)
        poal_udp_handle_request(cl, data, n);
}

struct net_udp_server *udp_server_new(const char *host, int port)
{
    struct net_udp_server *srv;
    int sock;
    
    printf_debug("udp server new\n");
    sock = usock(USOCK_UDP | USOCK_SERVER | USOCK_IPV4ONLY, host, usock_port(port));
    if (sock < 0) {
        uh_log_err("usock");
        return NULL;
    }
    printf_debug("sock=%d\n", sock);
    srv = calloc(1, sizeof(struct net_udp_server));
    if (!srv) {
        uh_log_err("calloc");
        goto err;
    }
    
    srv->fd.fd = sock;
    srv->fd.cb = udp_read_cb;
   
    uloop_fd_add(&srv->fd, ULOOP_READ);
    
    INIT_LIST_HEAD(&srv->clients);

    return srv;
    
err:
    close(sock);
    return NULL;
}

