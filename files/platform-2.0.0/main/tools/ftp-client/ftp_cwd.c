#include "ftp_client.h"
int ftp_cwd(int sock_fd)
{
    char    path[64] ;
    char    ftp_cwd_cmd[64] ;
    char    rbuf[64] ;

    printf("Input path you want to change:") ;
    memset(path,0,sizeof(path)) ;
    scanf("%s", path) ;

    memset(ftp_cwd_cmd,0,sizeof(ftp_cwd_cmd)) ;
    snprintf(ftp_cwd_cmd,sizeof(ftp_cwd_cmd),"CWD %s\r\n",path);
    if(write(sock_fd, ftp_cwd_cmd, strlen(ftp_cwd_cmd)) < 0)
    {
        printf("write CWD to ftp failed:%s\n", strerror(errno));
        return ERROR ;
    }
    read(sock_fd, rbuf, sizeof(rbuf)) ;
    printf("Read from ftp:%s\n", rbuf) ;
    return TRUE ;

}
