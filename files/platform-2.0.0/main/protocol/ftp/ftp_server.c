#include "ftp_common.h"
#include "ftp_server.h"
#include <stdbool.h>
#include "../../../main/executor/spm/spm.h"
#include <pthread.h>

void parse_command(char *cmdstring, Command *cmd);

//Communication
void* communication(void* _c) {
#ifdef CONFIG_PROTOCOL_FTP_PATH
    chdir(CONFIG_PROTOCOL_FTP_PATH);
#else
    chdir("/home");
#endif
    
    ftp_client_t *c = (ftp_client_t *)_c;
    int connection = c->fd;
    int bytes_read;
    char buffer[BSIZE];
    Command *cmd = malloc(sizeof(Command));
    State *state = malloc(sizeof(State));
    state->username = NULL; //add by zhaoyou
    state->private_args = c;
    state->logged_in = 1;
    memset(buffer,0,BSIZE);
    char welcome[BSIZE] = "220 ";
    if(strlen(welcome_message)<BSIZE-4){
        strcat(welcome,welcome_message);
    }else{
        strcat(welcome, "Welcome to nice FTP service.");
    }
    sprintf(buffer ," [%s:%d]\n", inet_ntoa(c->client_address.sin_addr), c->client_address.sin_port);
    strcat(welcome, buffer);
    /* Write welcome message */
    strcat(welcome,"\n");
    write(connection, welcome,strlen(welcome));

    memset(buffer,0,BSIZE);
    /* Read commands from client */
    while ((bytes_read = read(connection,buffer,BSIZE)) > 0){
        if(!(bytes_read>BSIZE) && bytes_read > 0){
            /* TODO: output this to log */
            printf_note("User %s sent command: %s\n",(state->username==NULL)?"unknown":state->username,buffer);
            parse_command(buffer,cmd);
            state->connection = connection;

            /* Ignore non-ascii char. Ignores telnet command */
            if(buffer[0]<=127 || buffer[0]>=0){
                response(cmd,state);
            }
            memset(buffer,0,BSIZE);
            memset(cmd,0,sizeof(*cmd));
        }else{
            /* Read error */
            perror("server:read");
        }
    }
    printf_debug("Client disconnected.\n");
    close(connection);
    free(c);
    c = NULL;
    free(cmd);
    cmd = NULL;
    if(state->username){
        free(state->username);
        state->username = NULL;
    }
    free(state);
    state = NULL;
    return NULL ;
}


void *ftp_server_thread_loop(void *s)
{
    //set defaul ftp server root work path.You can customize the path.
#if 0
   if(chroot(SERVERROOTPATH) !=0 )
   {
       printf("%s","chroot erro：please run as root!\n");
       exit(0);
   }
#endif
    ftp_server_t *server = s;
    int sock = create_socket(server->port);
    struct sockaddr_in client_address;
    unsigned int len = sizeof(client_address);
    pthread_detach(pthread_self());
    while(1){
        ftp_client_t *c = malloc(sizeof(ftp_client_t));
        c->fd = accept(sock, (struct sockaddr*) &client_address,&len);
        c->server = server;
        memcpy(&c->client_address, &client_address, len);
        pthread_t pid;
        pthread_create(&pid, NULL, communication, (void*) (c));
    }
}

void ftp_server_init(int port)
{
    pthread_t pid;
    struct misc_ops  *m= misc_get();
    ftp_server_t *server = malloc(sizeof(ftp_server_t));
    memset(server, 0, sizeof(*server));
    server->port = port;
    ftp_client_init_idx(server);
    if(!m)
        return;
    if(m->pre_handle)
        server->pre_cb = m->pre_handle;
    if(m->post_handle)
        server->post_cb = m->post_handle;
    if(m->write_handle)
        server->downlink_cb = m->write_handle;
    if(m->read_handle)
        server->uplink_cb = m->read_handle;
    if(m->usr1_handle)
        server->usr1_handle = m->usr1_handle;

    int ret;
    pthread_t work_id;
    ret=pthread_create(&work_id, NULL, ftp_server_thread_loop, (void *)server);
    if(ret!=0){
        perror("pthread create");
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

