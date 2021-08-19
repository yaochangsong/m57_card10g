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


 
void usage(char *prog)
{
    printf("usage: %s\n",prog);
    printf("%s -c [filename] -b [byte]\n", prog);
    printf("%s -t [filename]\n", prog);
}


static uint32_t getopt_integer(char *optarg)
{
  int rc;
  uint32_t value;
  rc = sscanf(optarg, "0x%x", &value);
  if (rc <= 0)
    rc = sscanf(optarg, "%ul", &value);
    
  return value;
}

static int create_inc_file(char *filename, long long size)
{
    FILE *file;
    unsigned char data = 0;
    file = fopen(filename, "wb");
    if(!file){
        printf("Open file error!\n");
        return -1;
    }
    fseek(file, 0, SEEK_SET);
    for(long long i = 0; i < size; i++){
        fwrite(&data, 1, 1, file);
        data ++;
    }

    fclose(file);
    return 0;
}

static bool check_inc_file(char *filename)
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
        if(data != buffer)
            return false;
        data ++;
    }

    fclose(file);
    return true;
}


/*  -c:  创建递增序列文件
            -b 文件大小 单位字节
   filecheck -c test.data -b 10000
   
    -t: 文件检测是否为递增序列 返回: ture / false
  filecheck -t test.data
*/
int main(int argc, char **argv)
{
    char *filename = NULL;
    uint32_t file_size = 0;
    int  cmd_opt;
    bool create_file=false, checkfile=false;
    while ((cmd_opt = getopt_long(argc, argv, "hc:b:t:", NULL, NULL)) != -1)
    {
        switch (cmd_opt)
        { 
            case 'h':
                usage(argv[0]);
                return 0;
            case 't':
                filename = optarg;
                printf("check file: %s\n", filename);
                checkfile = true;
                break;
            case 'c':
                filename = optarg;
                printf("create file: %s\n", filename);
                create_file = true;
                break;
            case 'b':
                file_size = getopt_integer(optarg);
                printf("file size: %ubyte\n", file_size);
                break;
            default:
            	usage(argv[0]);
            exit(0);
            break;
        }
    }
    if(create_file && filename != NULL && file_size > 0){
        if(create_inc_file(filename, file_size) != 0){
            printf("create file failed!\n");
        }else{
            printf("create file ok!\n");
        }
    } else if(checkfile && filename != NULL){
        if(check_inc_file(filename)){
            printf("file check ok!\n");
        } else{
            printf("file check failed!\n");
        }
    }
    return 0;
}


