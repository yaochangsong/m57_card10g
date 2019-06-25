/*
 * Copyright (C) 2019 xwkj

 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <uhttpd.h>
#include <string.h>


static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [option]\n"
        "          -p port  # Default port is 8080\n"
        "          -v       # verbose\n", prog);
    exit(1);
}

int main(int argc, char **argv)
{
    bool verbose = false;
    int port = 8080;
    int opt;

    while ((opt = getopt(argc, argv, "p:v")) != -1) {
        switch (opt)
        {
        case 'p':
            port = atoi(optarg);
            break;
        case 'v':
            verbose = true;
            break;
        default: /* '?' */
            usage(argv[0]);
        }
    }

    uloop_init();

    srv = un_server_new("0.0.0.0", port);
    if (!srv)
        goto done;
        uloop_run();
    
    done:
        uloop_done();
    
        if (srv) {
            srv->free(srv);
            free(srv);
        }
        
        return 0;
    }

}



