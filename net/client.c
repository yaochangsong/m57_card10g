#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/limits.h>
#include "log.h"


void uh_accept_client(struct uh_server *srv, bool ssl)
{
    struct uh_client *cl;
    unsigned int sl;
    int sfd;
    struct sockaddr_in addr;

    sl = sizeof(addr);
    sfd = accept(srv->fd.fd, (struct sockaddr *)&addr, &sl);
    if (sfd < 0) {
        uh_log_err("accept");
        return;
    }

    cl = calloc(1, sizeof(struct uh_client));
    if (!cl) {
        uh_log_err("calloc");
        goto err;
    }

    memcpy(&cl->peer_addr, &addr, sizeof(addr));
    
    cl->us = &cl->sfd.stream;
    if (ssl) {
        //uh_ssl_client_attach(cl);
    } else {
        cl->us->notify_read = client_ustream_read_cb;
        cl->us->notify_write = client_ustream_write_cb;
        cl->us->notify_state = client_notify_state;
    }

    cl->us->string_data = true;
    ustream_fd_init(&cl->sfd, sfd);

    cl->timeout.cb = keepalive_cb;
    uloop_timeout_set(&cl->timeout, UHTTPD_CONNECTION_TIMEOUT * 1000);

    list_add(&cl->list, &srv->clients);
    kvlist_init(&cl->request.url, hdr_get_len);
    kvlist_init(&cl->request.var, hdr_get_len);
    kvlist_init(&cl->request.header, hdr_get_len);
    
    cl->srv = srv;
    cl->srv->nclients++;

    cl->free = client_free;
    cl->send_error = client_send_error;
    cl->send_header = client_send_header;
    cl->append_header = client_append_header;
    cl->header_end = client_header_end;
    cl->redirect = client_redirect;
    cl->request_done = client_request_done;

    cl->send = client_send;
    cl->printf = uh_printf;
    cl->vprintf = uh_vprintf;
    cl->chunk_send = uh_chunk_send;
    cl->chunk_printf = uh_chunk_printf;
    cl->chunk_vprintf = uh_chunk_vprintf;

    cl->get_version = client_get_version;
    cl->get_method = client_get_method;
    cl->get_peer_addr = client_get_peer_addr;
    cl->get_peer_port = client_get_peer_port;
    cl->get_url = client_get_url;
    cl->get_path = client_get_path;
    cl->get_query = client_get_query;
    cl->get_var = client_get_var;
    cl->get_header = client_get_header;
    cl->get_body = client_get_body;

    if (srv->on_accept)
        srv->on_accept(cl);

    return;
err:
    close(sfd);
}

