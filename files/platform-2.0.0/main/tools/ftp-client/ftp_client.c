/*********************************************************************************
 *      Copyright:  (C) 2019 Wu Yujun<540726307@qq.com>
 *                  All rights reserved.
 *
 *       Filename:  ftp_client.c
 *    Description:  This file is ftp client 
 *                 
 *        Version:  1.0.0(2019年03月16日)
 *         Author:  Wu Yujun <540726307@qq.com>
 *      ChangeLog:  1, Release initial version on "2019年03月16日 15时29分29秒"
 *                 
 ********************************************************************************/

#include "ftp_client.h"

int g_stop = 0 ;


void print_usage(const char *program_name)
{
    printf("\n%s -- (2019.3.16)\n", program_name);
    printf(" Usage: %s -i <server_ip>/<ftp_server_hostname> -p <ftp_server_port>  [-h <use_help>]\n", program_name);
    printf("        -p --port       the port of the ftp server you want to connect\n") ;
    printf("        -i --ip         the ip address or hostname of the ftp server you want to connect\n") ;
    printf("        -h --help       the ftp_client how to use\n");
    printf("After Loin OK:\n") ;
    ftp_usage_print() ;
                                    
    return ;
}
void sighandler(int sig_num)
{
    if(sig_num == SIGUSR1)
    {
        g_stop = 1 ;
    }
}

int main(int argc, char **argv)
{
    char                *program_name ;
    int                 port = 0 ;
    int                 opt = -1 ;
    char                *ip = NULL;
    struct hostent *    hostnp ;
    int                 no_ipport ;
    char                buf[BUF_SIZE] ;
    int                 sock_fd = -1 ;
    //int                 rv = 0 ;
    char                cmd[32] ;


    program_name = basename(argv[0]) ;


    const char *short_opts = "i:n:p:h";                    
    const struct option long_opts[] =   {  
            {"help", no_argument, NULL, 'h'},    
            {"ip", required_argument, NULL, 'i'},
            { "port", required_argument, NULL, 'p'},  
            {0, 0, 0, 0} 
        };  
    while ((opt= getopt_long(argc, argv, short_opts, long_opts,NULL)) != -1) 
    {
        switch (opt) 
        {
            case 'i':
                ip = optarg ;
                break ;
            case 'p':
                port = atoi(optarg) ;
                break ;
            case 'h':
                print_usage(program_name) ;
                return 0 ;
        }
     }
    
    no_ipport = ( (!ip) || (!port) ) ;            //port or ip is NULL, no_ipprot = 1 ;
    if( no_ipport )
    {    
        print_usage(program_name);
        return 0;
    }

    /*  connect server get host by name    */
    if ( inet_addr(ip)== INADDR_NONE )
    {
        if( (hostnp = gethostbyname(ip) ) == NULL )
        {
            printf("get host by name failure: %s\n", strerror(h_errno)) ;
            return -1 ;
        }
        ip = inet_ntoa( * (struct in_addr *)hostnp->h_addr );
    }

    signal(SIGUSR1, sighandler);


    if( (sock_fd=connect_server(ip, port)) < 0 )
    {
        printf("connect_server() failed\n") ;
        return -2 ;
    }

    memset(buf, 0, sizeof(buf)) ;
    if( read(sock_fd, buf, sizeof(buf)) <= 0)
    {
        printf("Read information from ftp_server failed:%s\n", strerror(errno)) ;
        goto cleanup ;
    }
    printf("ftp Information: %s\n", buf) ;
   
    //while(rv<=0)
    //{   
    //    rv = login(sock_fd) ;
    //}
    

    while(!g_stop)
    {
        memset(cmd, 0, sizeof(cmd)) ;
        printf("\nFTP: ") ;
        scanf("%s", cmd) ;
        if(!strcmp(cmd,"ls"))
        {
            if( ftp_list(sock_fd) == ERROR )
                continue ;
        }
        else if(!strcmp(cmd,"get"))
        {
            if( ftp_get(sock_fd) == ERROR)
                continue ;
        }
        else if(!strcmp(cmd,"put"))
        {
            if( ftp_put(sock_fd) ==ERROR)
            {
                continue ;
            }
            read(sock_fd, buf, sizeof(buf));
            printf("Read from ftp:%s\n",buf) ;
        }
        else if(!strcmp(cmd,"quit"))
        {
            write(sock_fd, "quit\r\n", strlen("quit\r\n")) ;
            read(sock_fd, buf, sizeof(buf)) ;
            printf("Read from ftp: %s",buf) ;
            goto cleanup ;
        }
        else if(!strcmp(cmd,"cd"))
        {
            ftp_cwd(sock_fd) ;
        }
        else{
            ftp_usage_print() ;
            continue ;
        }

    }

cleanup:
    printf("program %s is stop\n", program_name) ;
    close(sock_fd) ;
    return 0 ;
/*  End Of main */
}

