#include <stdlib.h>  
#include <stdio.h>  
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <getopt.h>
#include <arpa/inet.h>


void create_increase_32bit_data_file(FILE *file, size_t len)
{
    uint32_t *ptr, *buffer = calloc(1, len);
    if(!buffer)
        return;
    ptr = buffer;
    for(int i = 0; i < len/4; i++){
        *ptr ++ = i;
    }
    fwrite((void *)buffer, 4,  len/4, file);
    free(buffer);
}

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [option]\n"
        "          -i --increase        32bit increase type data\n"
        "          -o --output          output data filename\n"
        "          -n --length          output data length\n"
        "          -h --help\n", prog);
    exit(1);
}

int main(int argc, char *argv[])
{
    int cmd_opt,  increase32_type = 0;
    FILE *file = NULL;
    size_t length = 0;
    char *output = NULL;

    const char *short_opts = "ho:in:";
    const struct option long_opts[] =   {  
            {"help", no_argument, NULL, 'h'},    
            {"output", required_argument, NULL, 'o'},
            {"increase", no_argument, NULL, 'i'},
            {"length", required_argument, NULL, 'n'},
            {0, 0, 0, 0} 
        };

    while ((cmd_opt = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1)
    {
        switch (cmd_opt)
        {
            case 'o':
                output = strdup(optarg);
                break;
            case 'i':
                increase32_type = 1;
                break;
            case 'n':
                length = atoi(optarg);
                printf("length:%ld\n", length);
                break;
            case 'h':
            default:
                usage(argv[0]);
                exit(0);
        }
    }
    if(!output || strlen(output) == 0){
        printf("Please input output filename\n");
        exit(1);
    }
        
    file = fopen (output, "w");
    if(!file){
        printf("Open file[%s] error!\n", output);
        exit(1);
    }

    if(increase32_type){
        if(length > 0){
            create_increase_32bit_data_file(file, length);
        } else{
            printf("Please input data length\n");
        }
    } else {
        printf("Please input type\n");
    }
    fclose(file);

    return 0;
}


