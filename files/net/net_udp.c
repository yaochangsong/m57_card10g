#include "config.h"

struct net_udp_server *get_udp_server(void)
{
    return net_get_udp_srv_ctx();
}

static char  *udp_get_ifname(struct net_udp_client *cl)
{
    struct in_addr ipaddr, netmask, local_net, peer_net;
     
    if(get_netmask(NETWORK_EHTHERNET_POINT, &netmask) != -1){
        printf_debug("===>netmask[0x%x]\n", netmask.s_addr);
    }
    if(get_ipaddress(NETWORK_EHTHERNET_POINT, &ipaddr) != -1){
        local_net.s_addr = ipaddr.s_addr& netmask.s_addr;
        peer_net.s_addr = cl->peer_addr.sin_addr.s_addr & netmask.s_addr;
        printf_debug("===>ipaddr[0x%x,  net=0x%x]\n", ipaddr.s_addr, ipaddr.s_addr& netmask.s_addr);
        printf_debug("===>peer net=0x%x, ip=0x%x\n",   cl->peer_addr.sin_addr.s_addr & netmask.s_addr, cl->peer_addr.sin_addr.s_addr);
        if(local_net.s_addr == peer_net.s_addr){
            return NETWORK_EHTHERNET_POINT;
        }
    }
#ifdef SUPPORT_NET_WZ
    if(get_netmask(NETWORK_10G_EHTHERNET_POINT, &netmask) != -1){
        printf_debug("===>netmask[0x%x]\n", netmask.s_addr);
    }
    if(get_ipaddress(NETWORK_10G_EHTHERNET_POINT, &ipaddr) != -1){
        local_net.s_addr = ipaddr.s_addr& netmask.s_addr;
        peer_net.s_addr = cl->peer_addr.sin_addr.s_addr & netmask.s_addr;
        printf_debug("===>ipaddr[0x%x,  net=0x%x]\n", ipaddr.s_addr, ipaddr.s_addr& netmask.s_addr);
        printf_debug("===>peer net=0x%x, ip=0x%x\n",   cl->peer_addr.sin_addr.s_addr & netmask.s_addr, cl->peer_addr.sin_addr.s_addr);
        if(local_net.s_addr == peer_net.s_addr){
            return NETWORK_10G_EHTHERNET_POINT;
        }
    }
#endif
    return NULL;
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
        printf_note("close\n");
        list_del(&cl->list);
        cl->srv->nclients--;
        free(cl);
    }
}

void udp_send_data_to_client(struct net_udp_client *client, uint8_t *data, uint32_t data_len)
{    
    if(client == NULL)
        return;
    printf_debug("send: %s:%d\n", client->get_peer_addr(client), client->get_peer_port(client));
    sendto(client->srv->fd.fd, data, data_len, 0, (struct sockaddr *)&client->peer_addr, sizeof(struct sockaddr));
}

static inline int udp_send_vec_data_to_client(struct net_udp_client *client, struct iovec *iov, int iov_len)
{    
    struct msghdr msgsent;
    int r=0;

    if(client == NULL)
        return -1;

    msgsent.msg_name = &client->peer_addr;
    msgsent.msg_namelen = sizeof(client->peer_addr);
    msgsent.msg_iovlen = iov_len;
    msgsent.msg_iov = iov;
    msgsent.msg_control = NULL;
    msgsent.msg_controllen = 0;
    //printf_debug("send: %s:%d\n", client->get_peer_addr(client), client->get_peer_port(client));
    r = sendmsg(client->srv->fd.fd, &msgsent, 0);
    if(r < 0){
        perror("sendmsg");
    }
    return 0;
}

int udp_send_vec_data(struct iovec *iov, int iov_len)
{
    struct net_udp_server *srv = get_udp_server();
    struct net_udp_client *cl_list, *list_tmp;
    int ret = 0;
    list_for_each_entry_safe(cl_list, list_tmp, &srv->clients, list){
        udp_send_vec_data_to_client(cl_list, iov, iov_len);
    }
    return ret;
}

int udp_send_vec_data_to_taget_addr(struct iovec *iov, int iov_len)
{
    #define SERVER "192.168.2.11"
    #define PORT 1134 
    static struct net_udp_client client;
    static bool client_init_flag = false;
    if(client_init_flag == false){
        client.peer_addr.sin_family = AF_INET;
        client.peer_addr.sin_port = htons(PORT);
        client.srv =  get_udp_server();

        if(inet_aton(SERVER, &client.peer_addr.sin_addr) == 0){
            printf_err("error server ip\n");
            return -1;
        }
        client_init_flag = true;
    }
    udp_send_vec_data_to_client(&client, iov, iov_len);
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

    printf_note(">>>>>>>>>Add New UDP Client addr to list: %s:%d, total client: %d\n", cl->get_peer_addr(cl), cl->get_peer_port(cl), cl->srv->nclients);

}

int udp_send_data(uint8_t  *data, uint32_t data_len)
{
    struct net_udp_server *srv = get_udp_server();
    struct net_udp_client *cl_list, *list_tmp;
    int ret = 0;
    list_for_each_entry_safe(cl_list, list_tmp, &srv->clients, list){
        printf_debug("get list:%s，port=%d\n",  cl_list->get_peer_addr(cl_list), cl_list->get_peer_port(cl_list));
#if (defined SUPPORT_PROTOCAL_AKT) || (defined SUPPORT_PROTOCAL_XNRP) 
        if(tcp_find_client(&cl_list->peer_addr)){ /* client is connectting */
            printf_debug("send data to %s:%d\n",  cl_list->get_peer_addr(cl_list), cl_list->get_peer_port(cl_list));
            udp_send_data_to_client(cl_list, data, data_len);
        }
        else{/* client is unconnect */
            printf_warn("**Tcp Client is Exit!! Stop Send Data to Clinet**\n");
           // udp_free(cl_list);
            ret = -1;
        } 
#else
    udp_send_data_to_client(cl_list, data, data_len);
#endif
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

    cl = calloc(1, sizeof(struct net_udp_client));
    if (!cl) {
        printf_err("calloc\n");
        return;
    }
    memcpy(&cl->peer_addr, &addr, sizeof(addr));
    cl->get_peer_addr = udp_get_peer_addr;
    cl->get_peer_port = udp_get_peer_port;

    cl->srv = srv;
    cl->send = udp_send_data_to_client;
    cl->ifname = udp_get_ifname(cl);
    printf_note("Receive New UDP data[%d] From: %s:%d, ifname=%s\n", n, cl->get_peer_addr(cl), cl->get_peer_port(cl), cl->ifname);
    if(cl != NULL){
        //poal_udp_handle_request(cl, data, n);
        if(cl->srv->on_discovery)
            cl->srv->on_discovery(cl, data, n);
        printf_note("Deal Over Free UDP: %s:%d\n", cl->get_peer_addr(cl), cl->get_peer_port(cl));
        free(cl);
    }
}

struct net_udp_server *udp_server_new(const char *host, int port)
{
    #define UDP_SEND_BUF (8*1024*1024)
    struct net_udp_server *srv;
    int sock;
    
    printf_debug("udp server new\n");
    sock = usock(USOCK_UDP | USOCK_SERVER | USOCK_IPV4ONLY, host, usock_port(port));
    if (sock < 0) {
        uh_log_err("usock");
        return NULL;
    }
    printf_debug("sock=%d\n", sock);

    /* 优化UDP发送速度；调整发送缓冲区大小 */
    int defrcvbufsize = -1;
    int nSnd_buffer = UDP_SEND_BUF;
    socklen_t optlen = sizeof(defrcvbufsize);
    if(getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &defrcvbufsize, &optlen) < 0)
    {
        printf("getsockopt error\n");
        return NULL;
    }
    printf_info("Current udp default send buffer size is:%d\n", defrcvbufsize);

    if(setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &nSnd_buffer, optlen) < 0)
    {
        printf("set udp recive buffer size failed.\n");
        return NULL;
    }
    if(getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &defrcvbufsize, &optlen) < 0)
    {
        printf("getsockopt error\n");
        return NULL;
    }
    printf_info("Now set udp send buffer size to:%dByte\n",defrcvbufsize);
    /* end */
    
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


