#include "ftp_client.h"

int accept_client(int listen_fd) //accept new client
{
    int                     client_fd ;
    struct sockaddr_in      cli_addr ;
    socklen_t               cli_addr_len ;
    memset(&cli_addr, 0, sizeof(cli_addr)) ;
    client_fd = accept(listen_fd,(struct sockaddr*)&cli_addr, &cli_addr_len) ;//block untill a new client connect 
    if(client_fd < 0)
    {
        printf("accept failure: %s\n", strerror(errno)) ;
        return ERROR ;
    }
    printf("accept sucessful, client [%s:%d] \n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port) ) ;
    return client_fd ; 
}
