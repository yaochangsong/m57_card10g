#include "ftp_client.h"
int ftp_port(int sock_fd,char *ftp_conn_ip, int *ftp_conn_port)
{
    char                ip0[4],ip1[4],ip2[4],ip3[4] ;
    int                 port0,port1 ;
    char                *ip_start = NULL,*ip_end = NULL ;
    int                 listen_fd ;
    char                portcmd[64];
    struct hostent *    hostnp ;
    char                buf[BUF_SIZE] ;

    printf("\nIP address or hostname for data send:") ;
    scanf("%s", ftp_conn_ip) ;
    printf("PORT for data send:") ;
    scanf("%d",ftp_conn_port) ;

 /*  connect server get host by name    */
    if ( inet_addr(ftp_conn_ip)== INADDR_NONE )
    {
        if( (hostnp = gethostbyname(ftp_conn_ip) ) == NULL )
        {
            printf("get host by name failure: %s\n", strerror(h_errno)) ;
            return -1 ;
        }
        ftp_conn_ip = inet_ntoa( * (struct in_addr *)hostnp->h_addr );
    }

    listen_fd = creat_socket(ftp_conn_ip,*ftp_conn_port) ;
    if(listen_fd == ERROR)
    {
        return ERROR ;
    }


    ip_start = ftp_conn_ip ;
    ip_end = strstr(ftp_conn_ip, ".") ;
    memset(ip0, 0, sizeof(ip0)) ;
    strncpy(ip0, ip_start, (ip_end-ip_start)) ;

    ip_end++ ;
    ip_start = ip_end ;
    ip_end = strstr(ip_start, ".") ;
    memset(ip1, 0, sizeof(ip1)) ;
    strncpy(ip1, ip_start, (ip_end-ip_start)) ;


    ip_end++ ;          
    ip_start = ip_end ; 
    ip_end = strstr(ip_start, ".") ;
    memset(ip2, 0, sizeof(ip2)) ;
    strncpy(ip2, ip_start, (ip_end-ip_start)) ;
    

    ip_end++ ;          
    memset(ip3, 0, sizeof(ip3)) ;
    strncpy(ip3, ip_end, strlen(ip_end)) ;

    port0 = *ftp_conn_port / 256 ;
    port1 = *ftp_conn_port % 256 ;
    
    snprintf(portcmd, sizeof(portcmd),"PORT %s,%s,%s,%s,%d,%d\r\n",ip0,ip1,ip2,ip3,port0,port1);

    if(write(sock_fd,portcmd,strlen(portcmd)) <= 0)
    {
        close(listen_fd);
        return -1;
    }
    read(sock_fd, buf, sizeof(buf)) ;
    printf("Read from ftp: %s\n", buf) ;

    return listen_fd ;
}
