#include "ftp_client.h"

int creat_socket(char *lis_addr, int port)
{
    int                     serv_fd ;
    struct sockaddr_in      serv_addr ;
    int                     reval = 0 ;
                            
    serv_fd= socket(AF_INET, SOCK_STREAM, 0) ;
    if(serv_fd < 0)
    {
        printf("socket() failed: %s\n", strerror(errno));
        return  ERROR ;
    }
    
    memset(&serv_addr, 0, sizeof(serv_addr)) ;
    serv_addr.sin_family = AF_INET ;
    serv_addr.sin_port = htons(port) ;
    if(!lis_addr)
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY) ;
    else 
        inet_aton(lis_addr, &serv_addr.sin_addr) ;
                                                     
    int reuse = 1 ;
    setsockopt(serv_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)); //reuseaddr
    if( bind(serv_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr) ) < 0)
    {
        printf("bind() failed: %s\n", strerror(errno)) ;
        reval = -1 ;
        
        goto cleanup ;
    }

    if(listen(serv_fd, BACK_LOG) < 0)
    {
        printf("listen() failed: %s\n", strerror(errno));
        reval = -2 ;
        
        goto cleanup ;
    }
    
cleanup:
    if(reval < 0)
    {   
        close(serv_fd) ;
        return ERROR ;
    }
    else
        return serv_fd ;
}

