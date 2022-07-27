#include "ftp_common.h"
#include "ftp_server.h"
#include <pthread.h>

void parse_command(char *cmdstring, Command *cmd);

//Communication
void* communication(void* _c) {
    chdir("/");
    int connection = *(int*)_c , bytes_read;
    char buffer[BSIZE];
    Command *cmd = malloc(sizeof(Command));
    State *state = malloc(sizeof(State));
    state->username = NULL; //add by zhaoyou
    memset(buffer,0,BSIZE);
    char welcome[BSIZE] = "220 ";
    if(strlen(welcome_message)<BSIZE-4){
        strcat(welcome,welcome_message);
    }else{
        strcat(welcome, "Welcome to nice FTP service.");
    }

    /* Write welcome message */
    strcat(welcome,"\n");
    write(connection, welcome,strlen(welcome));

    /* Read commands from client */
    while (bytes_read = read(connection,buffer,BSIZE) > 0){
        if(!(bytes_read>BSIZE) && bytes_read > 0){
            /* TODO: output this to log */
            printf("User %s sent command: %s\n",(state->username==0)?"unknown":state->username,buffer);
            parse_command(buffer,cmd);
            state->connection = connection;

            /* Ignore non-ascii char. Ignores telnet command */
            if(buffer[0]<=127 || buffer[0]>=0){
                response(cmd,state);
            }
            memset(buffer,0,BSIZE);
            memset(cmd,0,sizeof(cmd));
        }else{
            /* Read error */
            perror("server:read");
        }
    }
    close(connection);
    printf("Client disconnected.\n");
    return NULL ;
}

/** 
* Sets up server and handles incoming connections
* @param port Server port
*/
void ftp_server(int port)
{
    //set defaul ftp server root work path.You can customize the path.
   if(chroot(SERVERROOTPATH) !=0 )
   {
       printf("%s","chroot erro：please run as root!\n");
       exit(0);
   }
    int sock = create_socket(port);
    struct sockaddr_in client_address;
    int len = sizeof(client_address);
    while(1){
        int *connection = (int*)malloc(sizeof(int));
        *connection = accept(sock, (struct sockaddr*) &client_address,&len);

        pthread_t pid;
        pthread_create(&pid, NULL, communication, (void*) (connection));
    }
}

/**
* Creates socket on specified port and starts listening to this socket
* @param port Listen on this port
* @return int File descriptor for new socket
*/
int create_socket(int port)
{
    int sock;
    int reuse = 1;
    struct sockaddr_in server_address = (struct sockaddr_in){
        AF_INET,
                htons(port),
                (struct in_addr){INADDR_ANY}
    };


    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "Cannot open socket");
        exit(EXIT_FAILURE);
    }

    /* Address can be reused instantly after program exits */
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);

    /* Bind socket to server address */
    if(bind(sock,(struct sockaddr*) &server_address, sizeof(server_address)) < 0){
        fprintf(stderr, "Cannot bind socket to address");
        exit(EXIT_FAILURE);
    }

    listen(sock,5);
    return sock;
}


/**
* Parses FTP command string into struct
* @param cmdstring Command string (from ftp client)
* @param cmd Command struct
*/
void parse_command(char *cmdstring, Command *cmd)
{
    sscanf(cmdstring,"%s %s",cmd->command,cmd->arg);
}

