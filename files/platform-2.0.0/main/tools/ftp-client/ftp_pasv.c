#include "ftp_client.h"
int ftp_pasv(int sock_fd)
{
    char    pasv_ip[64] ; 
    char    ip0[4],ip1[4],ip2[4],ip3[4] ;
    char    port0[4], port1[4] ;
    int     port ;
    char    ip[32] ;
    int     pasv_fd = -1 ;

    char    *ip_start = NULL, *ip_end =NULL ;
    if( write(sock_fd, "PASV\r\n", strlen("PASV\r\n")) <= 0 )
    {
        printf("write to ftp failed: %s\n", strerror(errno)) ;
        return ERROR ;
    }
    memset(pasv_ip, 0, sizeof(pasv_ip)) ;
    read(sock_fd, pasv_ip, sizeof(pasv_ip)) ;
    //printf("%s\n", pasv_ip) ;

    ip_start = strstr(pasv_ip, "(") ;
    ip_start ++ ;   
    ip_end = strstr(ip_start, ",") ;
    memset(ip0, 0, sizeof(ip0)) ;
    strncpy(ip0, ip_start, (ip_end - ip_start)) ;

    ip_start = ip_end + 1 ;
    ip_end = strstr(ip_start, ",") ;
    memset(ip1, 0, sizeof(ip1)) ;
    strncpy(ip1, ip_start, (ip_end-ip_start));

    ip_start = ip_end + 1 ;
    ip_end = strstr(ip_start, ",") ;
    memset(ip2, 0, sizeof(ip2)) ;
    strncpy(ip2, ip_start, (ip_end-ip_start)) ;
    
    ip_start = ip_end + 1 ;
    ip_end = strstr(ip_start, ",") ;
    memset(ip3, 0, sizeof(ip3)) ;
    strncpy(ip3, ip_start, (ip_end-ip_start)) ;

    snprintf(ip, sizeof(ip),"%s.%s.%s.%s\n", ip0,ip1,ip2,ip3) ;

    ip_start = ip_end + 1 ;
    ip_end = strstr(ip_start, ",") ;
    memset(port0, 0, sizeof(port0)) ;
    strncpy(port0, ip_start, (ip_end-ip_start)) ;
    
    ip_start = ip_end+1 ;
    ip_end = strstr(ip_start, ")") ;
    memset(port1, 0, sizeof(port1)) ;
    strncpy(port1, ip_start, ip_end- ip_start) ;
    port = atoi(port0)*256+atoi(port1) ;

    pasv_fd = connect_server(ip, port) ;
    if(pasv_fd < 0)
    {
        printf("connect ftp pasv fail!\n") ;
        return ERROR ;
    }

    return pasv_fd ;
    

}
