#include "ftp_client.h"

int connect_server(char *ip, int port)
{
    int                     conn_fd ;
    struct sockaddr_in      serv_addr ;
    conn_fd = socket( AF_INET, SOCK_STREAM, 0 ) ;
    if(conn_fd < 0)
    {
        printf("creat socket failure : %s\n", strerror(errno)) ;
        return -1 ;
    }
    //printf("socket sucessful,connect file descriptor[%d]\n" ,conn_fd) ;
    memset(&serv_addr, 0, sizeof(serv_addr)) ;
    serv_addr.sin_port = htons(port) ;
    serv_addr.sin_family = AF_INET ;
    inet_aton(ip, &serv_addr.sin_addr);


    if( connect(conn_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0 )
    {
        printf("connect failure[%s:%d] : %s\n",  inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port), strerror(errno)) ;
        goto cleanup ;
    }
                                                    
    //printf("connect server[%s:%d] sucessful!\n",  inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port)) ;
    return conn_fd ;

cleanup:
        close(conn_fd) ;
        return -2 ;


}
