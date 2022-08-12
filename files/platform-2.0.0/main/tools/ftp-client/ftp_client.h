/********************************************************************************
 *      Copyright:  (C) 2019 Wu Yujun<540726307@qq.com>
 *                  All rights reserved.
 *
 *       Filename:  ftp_client.h
 *    Description:  This head file for ftp_client.c
 *
 *        Version:  1.0.0(2019年03月16日)
 *         Author:  Wu Yujun <540726307@qq.com>
 *      ChangeLog:  1, Release initial version on "2019年03月16日 16时00分10秒"
 *                 
 ********************************************************************************/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <libgen.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define TRUE        1
#define ERROR       0
#define BACK_LOG    13
#define BUF_SIZE    512


extern int  accept_client(int listen_fd) ;
extern int creat_socket(char *lis_addr, int port) ;
extern int login(int sock_fd) ;
extern int connect_server(char *ip, int port) ;
extern int ftp_port(int sock_fd,char *ftp_conn_ip, int *ftp_conn_port) ;
extern int ftp_pasv(int sock_fd) ;
extern int ftp_get(int sock_fd) ;
extern int ftp_list(int sock_fd) ;
extern int ftp_put(int sock_fd) ;
extern void ftp_usage_print(void) ; 
extern int ftp_cwd(int sock_fd) ;

