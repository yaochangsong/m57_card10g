#include "config.h"

struct net_udp_server *g_udp_srv;

struct net_udp_server *get_udp_server(void)
{
    return g_udp_srv;
}

static inline const char *udp_get_peer_addr(struct net_udp_client *cl)
{
    return inet_ntoa(cl->peer_addr.sin_addr);
}

static inline int udp_get_peer_port(struct net_udp_client *cl)
{
    return ntohs(cl->peer_addr.sin_port);
}

void udp_free(struct net_udp_client *cl)
{
    printf_note("udp_free\n");
    if (cl) {
        printf_debug("close\n");
        list_del(&cl->list);
        cl->srv->nclients--;
        free(cl);
    }
}

int udp_send_data_to_client(struct net_udp_client *client, uint8_t *data, uint32_t data_len)
{    
    if(client == NULL)
        return -1;
    printf_debug("send: %s:%d\n", client->get_peer_addr(client), client->get_peer_port(client));
    sendto(client->srv->fd.fd, data, data_len, 0, (struct sockaddr *)&client->peer_addr, sizeof(struct sockaddr));
    return 0;
}

void udp_client_dump(void)
{
    struct net_udp_client *cl_list, *list_tmp;
    struct net_udp_server *srv = get_udp_server();

    list_for_each_entry_safe(cl_list, list_tmp, &srv->clients, list){
           printf("udp client: addr=%s, port=%d\n", cl_list->get_peer_addr(cl_list), cl_list->get_peer_port(cl_list));
       }
}

void udp_add_client_to_list(struct sockaddr_in *addr, int ch)
{
    struct net_udp_client *cl = NULL;
    struct net_udp_client *cl_list, *list_tmp;
    struct net_udp_server *srv = get_udp_server();

    list_for_each_entry_safe(cl_list, list_tmp, &srv->clients, list){
        if(memcmp(&cl_list->peer_addr.sin_addr, &addr->sin_addr, sizeof(addr->sin_addr)) == 0){
            printf_warn("Find ipaddress on list:%s:%d，delete it\n",  cl_list->get_peer_addr(cl_list), cl_list->get_peer_port(cl_list));
            udp_free(cl_list);
        }
    }
/*
    list_for_each_entry_safe(cl_list, list_tmp, &srv->clients, list){
        if(memcmp(&cl_list->peer_addr.sin_addr, &addr->sin_addr, sizeof(addr->sin_addr)) == 0 && 
            cl_list->get_peer_port(cl_list) == addr->sin_port){
            printf_warn("Find ipaddress on list:%s:%d，not need to add\n",  cl_list->get_peer_addr(cl_list), cl_list->get_peer_port(cl_list));
            return;
        }
    }
*/
    cl = calloc(1, sizeof(struct net_udp_client));
    if (!cl) {
        printf_err("calloc\n");
        return;
    }

    memcpy(&cl->peer_addr, addr, sizeof(struct sockaddr_in));
    cl->get_peer_addr = udp_get_peer_addr;
    cl->get_peer_port = udp_get_peer_port;
    cl->peer_addr.sin_family = AF_INET;
    cl->ch = ch;
    list_add(&cl->list, &srv->clients);
    cl->srv = srv;
    cl->srv->nclients++;

    printf_note("Add New UDP Client addr to list: %s:%d, total client: %d\n", cl->get_peer_addr(cl), cl->get_peer_port(cl), cl->srv->nclients);

}

int udp_send_data(uint8_t  *data, uint32_t data_len)
{
    struct net_udp_server *srv = get_udp_server();
    struct net_udp_client *cl_list, *list_tmp;
    int ret = 0;
    list_for_each_entry_safe(cl_list, list_tmp, &srv->clients, list){
        printf_debug("get list:%s，port=%d\n",  cl_list->get_peer_addr(cl_list), cl_list->get_peer_port(cl_list));
        if(tcp_find_client(&cl_list->peer_addr)){ /* client is connectting */
            printf_debug("send data to %s:%d\n",  cl_list->get_peer_addr(cl_list), cl_list->get_peer_port(cl_list));
            udp_send_data_to_client(cl_list, data, data_len);
        }
        else{/* client is unconnect */
            printf_warn("**Tcp Client is Exit!! Stop Send Data And free udp Clinet**\n");
            udp_free(cl_list);
            ret = -1;
        } 
    }
    return ret;
}


static void udp_ustream_write_cb(struct ustream *s, int bytes)
{
     printf_debug("tcp_ustream_write_cb[%d]bytes\n",bytes);
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

#if 0
    struct net_udp_client *cl_list, *list_tmp;
    list_for_each_entry_safe(cl_list, list_tmp, &srv->clients, list){
        if(memcmp(&cl_list->peer_addr.sin_addr, &addr.sin_addr, sizeof(addr.sin_addr)) == 0 &&
            cl_list->peer_addr.sin_port == addr.sin_port){
            printf_note("Find UDP ipaddree on list:%s:%d tag=%d\n",  cl_list->get_peer_addr(cl_list), cl_list->peer_addr.sin_port, cl_list->tag);
            cl = cl_list;
            goto udp_handle;
        }
    }
#endif
    cl = calloc(1, sizeof(struct net_udp_client));
    if (!cl) {
        printf_err("calloc\n");
        return;
    }
    memcpy(&cl->peer_addr, &addr, sizeof(addr));
    cl->get_peer_addr = udp_get_peer_addr;
    cl->get_peer_port = udp_get_peer_port;

    //list_add(&cl->list, &srv->clients);
    cl->srv = srv;
    //cl->srv->nclients++;
    printf_note("Receive New UDP data From: %s:%d\n", cl->get_peer_addr(cl), cl->get_peer_port(cl));
    if(cl != NULL){
        poal_udp_handle_request(cl, data, n);
        printf_note("Deal Over Free UDP: %s:%d\n", cl->get_peer_addr(cl), cl->get_peer_port(cl));
        free(cl);
    }
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
    g_udp_srv = srv;
    uloop_fd_add(&srv->fd, ULOOP_READ);
    
    INIT_LIST_HEAD(&srv->clients);

    return srv;
    
err:
    close(sock);
    return NULL;
}


