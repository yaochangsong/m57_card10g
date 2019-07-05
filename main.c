/*
 * Copyright (C) 2017 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#include "config.h"

static void on_accept(struct uh_client *cl)
{
    ULOG_INFO("New connection from: %s:%d\n", cl->get_peer_addr(cl), cl->get_peer_port(cl));
}

static int on_request(struct uh_client *cl)
{
    const char *path = cl->get_path(cl);
    int body_len = 0;

    if (strcmp(path, "/hello"))
        return UH_REQUEST_CONTINUE;

    cl->send_header(cl, 200, "OK", -1);
    cl->append_header(cl, "Myheader", "Hello");
    cl->header_end(cl);

    cl->chunk_printf(cl, "<h1>Hello Libuhttpd %s</h1>", UHTTPD_VERSION_STRING);
    cl->chunk_printf(cl, "<h1>REMOTE_ADDR: %s</h1>", cl->get_peer_addr(cl));
    cl->chunk_printf(cl, "<h1>URL: %s</h1>", cl->get_url(cl));
    cl->chunk_printf(cl, "<h1>PATH: %s</h1>", cl->get_path(cl));
    cl->chunk_printf(cl, "<h1>QUERY: %s</h1>", cl->get_query(cl));
    cl->chunk_printf(cl, "<h1>VAR name: %s</h1>", cl->get_var(cl, "name"));
    cl->chunk_printf(cl, "<h1>BODY:%s</h1>", cl->get_body(cl, &body_len));
    cl->request_done(cl);

    return UH_REQUEST_DONE;
}

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [option]\n"
        "          -p port  # Default port is 8080\n"
        "          -s       # SSl on\n"
        "          -v       # verbose\n", prog);
    exit(1);
}

#if 0
struct uloop_timeout timeout;
int frequency = 1;
static void timeout_cb(struct uloop_timeout *t)
{
    printf("time is over!\n");
    uloop_timeout_set(t, frequency * 1000);
}
int main()
{
    uloop_init();
    timeout.cb = timeout_cb;
    uloop_timeout_set(&timeout, frequency * 1000);
    uloop_run();
}
#endif

#if 1
int main(int argc, char **argv)
{
    struct uh_server *srv = NULL;
    struct server *tcpsrv = NULL;
    bool verbose = false;
    bool ssl = false;
    int port = 8981;
    int opt;

    while ((opt = getopt(argc, argv, "p:vs")) != -1) {
        switch (opt)
        {
        case 'p':
            port = atoi(optarg);
            break;
        case 's':
            ssl = true;
            break;
        case 'v':
            verbose = true;
            break;
        default: /* '?' */
            usage(argv[0]);
        }
    }

    if (!verbose)
        ulog_threshold(LOG_ERR);
    
    ULOG_INFO("libuhttpd version: %s\n", UHTTPD_VERSION_STRING);

    uloop_init();

    

    tcpsrv = tcp_server_new("0.0.0.0", 1234);
    if (!tcpsrv)
        goto done;
    tcpsrv->on_accept = on_accept;

    
    srv = uh_server_new("0.0.0.0", port);
    if (!srv)
        goto done;

#if (!UHTTPD_SSL_SUPPORT)
    if (ssl)
        ULOG_ERR("SSl is not compiled in\n");
#else
    if (ssl && srv->ssl_init(srv, "server-key.pem", "server-cert.pem") < 0)
        goto done;
#endif

    ULOG_INFO("Listen on: %s *:%d\n", srv->ssl ? "https" : "http", port);

    srv->on_accept = on_accept;
    srv->on_request = on_request;
    
    uloop_run();
done:
    uloop_done();

    if (srv) {
        srv->free(srv);
        free(srv);
    }
    
    return 0;
}
#endif
