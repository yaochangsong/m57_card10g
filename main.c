/******************************************************************************
*  Copyright 2019, Showay Technology Dev Co.,Ltd.
*  ---------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Showay Technology Dev Co.,Ltd. (C) 2019
******************************************************************************/
/*****************************************************************************     
*  Rev 1.0   06 July 2019   yaochangsong
*  Initial revision.
******************************************************************************/

#include "config.h"




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

#if 0
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
void thread_ping(void *arg)
{
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;
    struct timespec timeout;

    while (1) {
        /* Make sure we check the servers at the very begining */
        printf_debug("Running ping()");
        //ping();

        /* Sleep for config.checkinterval seconds... */
        //timeout.tv_sec = time(NULL) checkinterval;
        //timeout.tv_nsec = 0;

        /* Mutex must be locked for pthread_cond_timedwait... */
        pthread_mutex_lock(&cond_mutex);

        /* Thread safe "sleep" */
        pthread_cond_timedwait(&cond, &cond_mutex, &timeout);

        /* No longer needs to be locked */
        pthread_mutex_unlock(&cond_mutex);
    }
}


/******************************************************************************
* FUNCTION:
*     main
*
* DESCRIPTION:
*     
*     
*
* PARAMETERS
*     not used
*
* RETURNS
*     none
******************************************************************************/
int main(int argc, char **argv)
{
    bool verbose = false;
    bool ssl = false;
    int port = 8081;
    int opt;

    if (!verbose)
        log_init(log_debug);
    
    printf_note("VERSION:%s\n",SPCTRUM_VERSION_STRING);
    config_init();
    uloop_init();
    if(server_init() == -1){
        printf_err("server init fail!\n");
        goto done;
    }
    uloop_run();
done:
    uloop_done();
    return 0;
}


