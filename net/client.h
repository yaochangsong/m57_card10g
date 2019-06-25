#ifndef _CLIENT_H_
#define _CLIENT_H_


struct uh_client {
    struct list_head list;
    struct uh_server *srv;
    struct ustream *us;
    struct ustream_fd sfd;
    struct sockaddr_in peer_addr;

}

#endif

