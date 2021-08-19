#include "ftp_client.h"
int ftp_put(int sock_fd)
{
    int     ftp_pasv_fd = -1 ;
    int     rv = -1 ;
    char    file_name[256] ;
    char    ftp_upload_cmd[128] ;
    int     fd = -1 ;
    char    rbuf[128] ;
    
    
  /*    
    send(sock_fd, "TYPE I\r\n", strlen("TYPE I\r\n"), 0);
    recv(sock_fd, rbuf, sizeof(rbuf), 0);
    printf("%s\n", rbuf);
    memset(rbuf,0,sizeof(rbuf)) ;
*/
    printf("Please input file name you want to upload:") ;
    scanf("%s", file_name) ;
    fd = open(file_name,O_RDONLY) ;
    if(fd < 0)
    {
        printf("Open failed:%s\n", strerror(errno)) ; 
        return ERROR ;
    }
    
    ftp_pasv_fd = ftp_pasv(sock_fd) ;
    if(ftp_pasv <= 0)
    {
        printf("ftp_pasv return ERROR\n") ;
        return ERROR ;
    }

    snprintf(ftp_upload_cmd,sizeof(ftp_upload_cmd),"STOR %s\r\n",file_name) ;
    if( write(sock_fd, ftp_upload_cmd, strlen(ftp_upload_cmd)) <= 0)
    {
        printf("Send STOR to ftp failed: %s\n", strerror(errno)) ;
        return ERROR ;
    }
    
    read(sock_fd, rbuf, sizeof(rbuf)) ;
    if(strcmp(rbuf, "150 Ok to send data.\r\n")!=0)
    {
        printf("Send data error or Could not create file\n") ;
        close(fd) ;
        close(ftp_pasv_fd) ;
        return ERROR ;
    }
    
    do{
        memset(rbuf, 0, sizeof(rbuf)) ;
        rv = read(fd, rbuf, sizeof(rbuf)) ;
        if( write(ftp_pasv_fd, rbuf, rv) < 0)
        {
            printf("ftp_put write to ftp faile:%s\n", strerror(errno)) ;
            return ERROR ;
        }
    }while(rv > 0) ;

    printf("finish upload!\n") ;


    close(fd) ;
    close(ftp_pasv_fd) ;
    return TRUE ;
}
