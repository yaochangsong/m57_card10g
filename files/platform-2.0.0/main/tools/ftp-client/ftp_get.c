#include "ftp_client.h"
int ftp_get(int sock_fd) 
{
    int     ftp_pasv_fd = -1 ;
    int     rv = -1 ;
    char    file_name[256] ;
    char    ftp_download_cmd[128] ;
    char    rbuf[512] ;
    int     fd ;
    
    printf("Please input file name you want to download:") ;
    scanf("%s", file_name) ;
    ftp_pasv_fd = ftp_pasv(sock_fd) ;
    if(ftp_pasv_fd == ERROR)
    {
        printf("ftp_pasv failed");
        return ERROR ;
    }
    snprintf(ftp_download_cmd,sizeof(ftp_download_cmd),"RETR %s\r\n",file_name) ;
    if( write(sock_fd, ftp_download_cmd, strlen(ftp_download_cmd)) <= 0)
    {
        printf("Send RETR to ftp failed: %s\n", strerror(errno)) ;
        return ERROR ;
    }
    memset(rbuf, 0, sizeof(rbuf)) ;
    read(sock_fd, rbuf, sizeof(rbuf)) ;
    if(strcmp(rbuf,"550 Failed to open file.\r\n") == 0)
    {
    	printf("Don't exist this file\n") ;
       return ERROR ;	
    }

    fd = open(file_name,O_WRONLY|O_CREAT,0755) ;
    if(fd < 0)
    {
        printf("Open failed:%s\n", strerror(errno)) ; 
        return ERROR ;
    }
    
    do{
        memset(rbuf, 0, sizeof(rbuf)) ;
        rv = read(ftp_pasv_fd, rbuf, sizeof(rbuf)) ;
        write(fd, rbuf, rv) ;
    }while(rv > 0) ;
    
    read(sock_fd, rbuf, sizeof(rbuf)) ;
    printf("Read from ftp:%s\n", rbuf) ;
    
    close(fd) ;
    close(ftp_pasv_fd) ;
    return TRUE ;
}
