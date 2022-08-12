#include "ftp_client.h"
int ftp_list(int sock_fd)
{
    int     ftp_pasv_fd = -1 ;
    int     rv = -1 ;
    char    rbuf[256] ;
    
    ftp_pasv_fd = ftp_pasv(sock_fd) ;
    if(ftp_pasv_fd <= 0)
    {
        printf("ftp_pasv failed\n") ;
        return ERROR ;
    }
   if( write(sock_fd, "LIST\r\n", strlen("LIST\r\n")) < 0)
   {
        printf("ftp_list write to ftp failed:%s\n",strerror(errno)) ;
        close(ftp_pasv_fd) ;
        return ERROR ;
   }
    read(sock_fd, rbuf, sizeof(rbuf));
    printf("Read from ftp:%s", rbuf) ;
    
    do{
        memset(rbuf, 0, sizeof(rbuf)) ;
        rv = read(ftp_pasv_fd, rbuf, sizeof(rbuf)) ;
        printf("%s",rbuf) ;
    }while(rv > 0) ;

    printf("\n") ;
    read(sock_fd, rbuf, sizeof(rbuf));
    printf("Read from ftp:%s", rbuf) ;
    close(ftp_pasv_fd) ;
    return TRUE ;
}
