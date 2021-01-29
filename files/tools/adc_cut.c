#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <poll.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <poll.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <semaphore.h>
#include <net/if.h>
#include <sys/stat.h>
#include <termios.h>
#include <errno.h>
#include <linux/spi/spidev.h>
#include <time.h>



int read_file(void *pdata, unsigned int data_len, char *filename)
{
        
    FILE *file;
   // unsigned int *pdata_offset;

    if(pdata == NULL){
        return -1;
    }

    file = fopen(filename, "r");
    if(!file){
        printf("Open file error!\n");
        return -1;
    }

    //fread(pdata,data_len,1,file);
    fread(pdata,1,data_len,file);
    fclose(file);

    return 0;
}

static FILE *file_open(char *filename, char *attr)
{
    FILE *file;
    file = fopen(filename, attr);
    if(!file){
        printf("Open file error!\n");
        return -1;
    }
    return file;
}

static inline int file_savedata(FILE *file, uint8_t *pdata, unsigned int data_len)
{
    fwrite((void *)pdata,1,data_len,file);
    return 0;
}

static inline size_t  file_readdata(FILE *file, uint8_t *pdata, unsigned int data_len)
{
    return fread(pdata,1,data_len,file);
}



static int file_close(FILE *file)
{
    fclose(file);
	return 0;
}

static inline void swap(uint32_t *d1, uint32_t *d2)
{
    uint32_t tmp;
    tmp = *d1;
    *d1 = *d2;
    *d2 = tmp;
}

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [option]\n"
        "          -i input filename \n"
        "          -o output filename \n"
        "          -t type,1g or 10g [1, 10], default 1g \n "
        "          \n", prog);
    exit(1);
}


int main(int argc, char **argv)
{
    int opt;
    char *i_filename = NULL, *o_filename = NULL;
    FILE *i_file= NULL, *o_file=NULL;
    uint64_t buffer[512]={0};
    uint64_t *pbuffer = NULL;
    char type;
    size_t  nread = 0;
    while ((opt = getopt(argc, argv, "i:o:t:")) != -1) {
        switch (opt)
        {
        case 'i':
            printf("input file=%s\n", optarg);
            i_filename = optarg;
            break;
        case 'o':
            printf("output file=%s\n", optarg);
            o_filename = optarg;
            break;
        case 't':
            printf("type=%dg\n", atoi(optarg));
            type = atoi(optarg);
            break;
        default: /* '?' */
            usage(argv[0]);
            exit(0);
        }
    }
    memset(buffer, 0,sizeof(buffer));
    i_file = file_open(i_filename, "r+b");
    o_file = file_open(o_filename, "w+b");
  //  int cnt = 0, start_header_offset = 0;
	int cnt = 0;
    fseek(i_file, 0, SEEK_SET);
    

   // start_header_offset = 0;
    for(;;){
           memset(buffer, 0,sizeof(buffer));
           nread = file_readdata(i_file, (uint8_t*)buffer, sizeof(buffer));
           if(nread == 0){
               break;
           }
           pbuffer = (uint64_t *)buffer;
           if((pbuffer[0]&0x7f00000000000000) != 0x7f00000000000000){
                printf("--header err [%d]0x%lx, 0x%lx\n",cnt, pbuffer[0], pbuffer[1]);
                exit(1);
           }
           //printf("--header [%d]0x%llx, 0x%llx\n",cnt, pbuffer[0], pbuffer[1]);
           cnt ++;
           
       }
    fseek(i_file, 0, SEEK_SET);
    #if 1
    for(;;){
        memset(buffer, 0,sizeof(buffer));
        nread = file_readdata(i_file, (uint8_t*)buffer, sizeof(buffer));
        pbuffer = (uint32_t *)buffer;
        if(nread == 0){
            break;
        }
        file_savedata(o_file, (uint8_t*)buffer+1, sizeof(buffer)-8);
        
    }
    #endif
    file_close(i_file);
    file_close(o_file);
}

