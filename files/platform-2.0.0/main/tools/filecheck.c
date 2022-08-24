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
#include <stdbool.h>
#include <sys/statfs.h>
#include <getopt.h>
#include <errno.h>
#include <sys/mman.h>

#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))

static bool check_int8_inc_file(char *filename)
{
    FILE *file;

    unsigned char data = 0, buffer;
    long long size;
    file = fopen(filename, "rb");
    if(!file){
        printf("Open file error!\n");
        return false;
    }
    
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    printf("file size=%llu\n", size);
    fseek(file, 0, SEEK_SET);
    for(long long i = 0; i < size; i++){
        fread(&buffer, 1, 1, file);
        if(data != buffer){
            printf("offset=%llu[0x%llx], 0x%x!=0x%x\n", i, i, data, buffer);
            return false;
        }
        data ++;
    }

    fclose(file);
    return true;
}

static bool check_int32_inc_file(char *filename)
{
    FILE *file;

    unsigned int data = 0, buffer;
    size_t rc = 0;
    long long size;
    file = fopen(filename, "rb");
    if(!file){
        printf("Open file error!\n");
        return false;
    }
    
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    printf("file size=%llu\n", size);
    fseek(file, 0, SEEK_SET);
    for(long long i = 0; i < size; i+=4){
        rc= fread(&buffer, sizeof(unsigned int), 1, file);
        if(rc == 0)
            break;
        if(data != buffer){
            printf("offset=%llu[0x%llx], 0x%x!=0x%x\n", i, i, data, buffer);
            return false;
        }
        data ++;
    }

    fclose(file);
    return true;
}



static ssize_t _find_header(uint8_t *ptr, uint16_t header, size_t len)
{
    ssize_t offset = 0;
    do{
        if(ptr != NULL && *(uint16_t *)ptr != header){
            ptr += 1;
            offset += 1;
        }else{
            break;
        }
    }while(offset < len);

    if(offset >= len || ptr == NULL)
        return -1;
    offset += 2;
    return offset;
}

ssize_t _writen(int fd, const uint8_t *buf, size_t buflen)
{
    ssize_t ret = 0, len;

    if (!buflen)
        return 0;

    while (buflen) {
        len = write(fd, buf, buflen);

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

static int m57_vpx_write_data(uint8_t *buffer, size_t len, char *wfile)
{
    #define M57_VPX_MIN_PKT_LEN 256
    int ifd;
    uint8_t *ptr = buffer;
    ssize_t consume_len = 0, _len = len;
    ssize_t offset = 0;
    
    ifd = open(wfile, O_WRONLY|O_CREAT, 0644);
    if(ifd == -1){
        printf ("Can't open %s: %s\n", wfile, strerror(errno));
        return -1;
    }

    do{
        offset = 0 ;
        offset = _find_header(ptr, 0x5157, _len);
        if(offset == -1){
            break;
        }
        if(offset > 0){
            ptr += offset;
            _len -= offset;
        }
        
        consume_len = min(_len, M57_VPX_MIN_PKT_LEN);
        _writen(ifd, ptr, consume_len);
        ptr += consume_len;
        _len -= consume_len;
       // printf("ptr:%p[%ld, %ld, %ld], %p[%lu]\n", ptr, _len, offset, consume_len, buffer, len);
        if(_len <= 0 || (ssize_t)(ptr - buffer) >= len){
            printf("write over, %ld\n", _len);
            break;
        }
    }while(1);
        
    close(ifd);
    return 0;
}




int m57_vpx_data_check_write(char *filename, char *outfilename)
{
    struct stat sbuf;
    unsigned char *ptr;
    int ifd, ret = 0;

    if(!outfilename){
        printf ("Output file is null\n");
        return -1;
    }

    ifd = open(filename, O_RDONLY);
    if(!ifd){
        printf ("Can't open %s: %s\n", filename, strerror(errno));
        return -1;
    }

    if (fstat(ifd, &sbuf) < 0) {
        close(ifd);
        printf ("Can't stat %s: %s\n", filename, strerror(errno));
        return -1;
    }

    ptr = (unsigned char *) mmap(0, sbuf.st_size, PROT_READ, MAP_SHARED, ifd, 0);
    if ((caddr_t)ptr == (caddr_t)-1) {
        close(ifd);
        printf ("Can't mmap %s: %s\n", filename, strerror(errno));
        return -1;
    }
    ret = m57_vpx_write_data(ptr, sbuf.st_size, outfilename);
    
    munmap(ptr, sbuf.st_size);
    close(ifd);

    return ret;
}


static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [option]\n"
        "          -t --type [N]          N=0(default): check 32bit increase file; "
        "                                 N=1: check and output M57 VPX data file; \n"
        "                                 N=2: check 8bit increase file;\n"
        "          -o --of   [file]       output file after removing information\n"
        "          -i --if   [file]       raw input file\n"
        "          -h --help\n", prog);
    exit(1);
}


int main(int argc, char **argv)
{
    char *outfile = NULL, *infile = NULL;
    int type = 0, cmd_opt = 0;
    const char *short_opts = "ht:o:i:";
    const struct option long_opts[] =   {  
            {"help", no_argument, NULL, 'h'},    
            {"type", required_argument, NULL, 't'},
            {"of", required_argument, NULL, 'o'},
            {"if", required_argument, NULL, 'i'},
            {0, 0, 0, 0} 
        };
    while ((cmd_opt = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1)
    {
        switch (cmd_opt)
        {
            case 'o':
                outfile = strdup(optarg);
                break;
            case 'i':
                infile = strdup(optarg);
                break;
            case 't':
                type = atoi(optarg);
                break;
            case 'h':
            default:
                usage(argv[0]);
                exit(1);
        }
    }
    if(infile == NULL){
        usage(argv[0]);
        exit(1);
    }
    if(type == 0){
        if(check_int32_inc_file(infile)){
            printf("OK\n");
        }else{
            printf("Failed\n");
        }
    } else if(type == 1) {
        if(m57_vpx_data_check_write(infile, outfile) == 0){
            printf("OK\n");
        } else {
            printf("Failed\n");
        }
    } else if(type == 2){
        if(check_int8_inc_file(infile)){
            printf("OK\n");
        }else{
            printf("Failed\n");
        }
    }else {
        usage(argv[0]);
    }
    return 0;
}


