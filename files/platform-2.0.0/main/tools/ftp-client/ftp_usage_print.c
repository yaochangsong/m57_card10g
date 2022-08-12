#include "ftp_client.h"
void ftp_usage_print(void) 
{
    printf("ftp_client version 1.0.0:\n") ;
    printf("help:        show the help fro use ftp_client\n") ;
    printf("ls:          to list ftp server file\n") ;
    printf("put:         upload file to ftp server\n") ; 
    printf("get:         download file from ftp server\n") ;
    printf("cd:          change work path\n") ; 
    printf("quit:        to exit ftp_client\n") ;

}
