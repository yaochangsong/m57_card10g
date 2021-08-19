#include "ftp_client.h"
int login(int sock_fd)
{
    int         rv = 0 ;
    char        buf[256] ;
    char        scanf_buf[256] ;
    char        *password ;


    printf("\n/************ START LOGIN *************/\n") ;
    /*  Input ftp username  */
    printf("Please input User_name:") ;
    memset(scanf_buf, 0, sizeof(scanf_buf)) ;
    scanf("%s",scanf_buf) ;
    memset(buf, 0 ,sizeof(buf)) ;
    snprintf(buf, sizeof(buf),"USER %s\r\n",scanf_buf) ;
    if(write(sock_fd, buf, strlen(buf))< 0 )
    {
        printf("write to ftp server failed:%s\n",strerror(errno)) ;
        return ERROR ;
    }
    memset(buf, 0, sizeof(buf)) ;
    if( (rv =read(sock_fd, buf, sizeof(buf)) ) <= 0)
    {
        printf("Read from ftp_server failed:%s\n", strerror(errno)) ;
        return ERROR ;
    }
    //printf("Read %d from ftp: %s\n",rv, buf) ; 
    if(strcmp(buf,"331 Please specify the password.\r\n")!= 0)
    {
    	printf("Invaild user name\n") ;
	return ERROR ;
    }

    /*  input ftp user passwd     */
    password = getpass("Please input Password:");
    memset(buf, 0,sizeof(buf)) ;
    snprintf(buf,sizeof(buf),"PASS %s\r\n",password);
    if(write(sock_fd, buf, strlen(buf))< 0 )
    {         
        printf("write to ftp server failed:%s\n",strerror(errno)) ;
        return ERROR ;
    }          
    if( (rv = read(sock_fd, buf, sizeof(buf))) <= 0)
    {
        printf("Read from ftp_server failed:%s\n", strerror(errno)) ;
        return ERROR ;
    }
    //printf("Read %d byte from ftp: %s\n",rv, buf) ;
    if(strcmp(buf,"230 Login successful.\r\n")== 0)
    {
        printf("You are Welcome!\n") ;
        printf("/************ FINISH LOGIN *************/\n") ;
        return TRUE ;
    }
    else 
    {
        printf("Invaild username or password, Please try again\n") ;
        return ERROR ;
    }
}
