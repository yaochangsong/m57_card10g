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
#include <errno.h>
#define SNDSIZE 1024*1024
#define SERVER_PORT 5000 // 服务器端口
#define FILE_NAME "rawdata"

int recv_cnt = 0;
unsigned int write_byte = 0;

void stop_exit(int signo)
{
    printf("stop recv over: %d ,write_byte=%uByte\n", recv_cnt,write_byte);
    exit(0);
}

char login_data[] = {
	0x51,0x57,0x96, 0x32, 0x38, 0x00, 0x00, 0x00, 0x01, 0x00, 
	0x30,0x00,0x00 ,0x00,0x00,0x00,0x01, 0x02, 0x30, 0x46,0x44,
	0x39,0x35,0x33,0x46,0x39,0x2d,0x33,0x37,0x45,0x32,0x2d,0x34,
	0x63,0x30,0x35,0x2d,0x41,0x41,0x46,0x33,0x2d,0x44,0x42,0x34,
	0x41,0x43,0x37,0x30,0x35,0x42,0x35,0x45,0x37,0x00,0x00
};

static int send_data(int fd, const char *buf, int buflen)
{
    ssize_t ret = 0, len;

    if (!buflen)
        return 0;

    while (buflen) {
        len = send(fd, buf, buflen, 0);

        if (len < 0) {
            printf("[fd:%d]-send len : %ld, %d[%s]\n", fd, len, errno, strerror(errno));
            if (errno == EINTR)
                continue;

            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOTCONN)
                break;

            return -1;
        }

        ret += len;
        buf += len;
        buflen -= len;
    }
    
    return ret;
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
	sleep(1);
	printf("send logins\n");
	send_data(fd_client, login_data, sizeof(login_data));
    file_fd = fopen(FILE_NAME, "w+b");
    char buf[SNDSIZE];
    int readn;
    while(memset(buf,0,SNDSIZE),(readn = recv(fd_client,buf,SNDSIZE,0)) > 0)
    {
        //fwrite((void *)buf,1, readn, file_fd);
        recv_cnt++;
        write_byte+=readn;
    }
    printf("recv over: %d ,write_byte=%uByte\n", recv_cnt,write_byte);
    fclose(file_fd);
    close(fd_client);
    return 0;
}

