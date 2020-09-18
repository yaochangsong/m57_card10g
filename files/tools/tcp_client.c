#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/wait.h>
#define SNDSIZE 1024*1024
#define SERVER_PORT 6080 // 服务器端口
#define FILE_NAME "rawdata"

int recv_cnt = 0;
unsigned int write_byte = 0;

void stop_exit(int signo)
{
    printf("stop recv over: %d ,write_byte=%uByte\n", recv_cnt,write_byte);
    exit(0);
}

int main(int argc, char *argv[]) 
{
    FILE *file_fd;

    signal(SIGINT, stop_exit);
    /* socket */
    int fd_client;
    fd_client = socket(AF_INET, SOCK_STREAM, 0);
    if(fd_client == -1)
    {
        perror("socket");
        exit(1);
    }
    /* 存放所连服务器信息的结构体 */
    struct sockaddr_in server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if(argv[1] == NULL){
        printf("Please set server ip!!\n");
        exit(1);
    }
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    
    /* connect */
    while(connect(fd_client, (struct sockaddr *)&server_addr,sizeof(server_addr)) == -1)
    {
        /* 当connect返回-1时，说明服务器还没有启动 */
        sleep(1);
        printf("connecting... \n");
    }
    printf("connect success! \n");
    file_fd = fopen(FILE_NAME, "w+b");
    char buf[SNDSIZE];
    int readn;
    while(memset(buf,0,SNDSIZE),(readn = recv(fd_client,buf,SNDSIZE,0)) > 0)
    {
        fwrite((void *)buf,1, readn, file_fd);
        recv_cnt++;
        write_byte+=readn;
    }
    printf("recv over: %d ,write_byte=%uByte\n", recv_cnt,write_byte);
    fclose(file_fd);
    close(fd_client);
    return 0;
}

