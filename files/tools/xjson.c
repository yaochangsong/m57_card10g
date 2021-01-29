/******************************************************************************
    json文件修改工具：
******************************************************************************/
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
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include "../dao/json/cJSON.h"

/* xjson -w -f /etc/config.json -n ipaddress,network -v $DEFAULT_IP */
/* xjson -r -f /etc/config.json -n ipaddress,network  */
static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [option]\n"
        "          -w write value \n"
        "          -r read value \n"
        "          -f input json file \n"
        "          -n node[ipaddress,network] \n"
        "          -v value \n "
        "          \n", prog);
    exit(1);
}

static bool parse_number(char * const input_buffer, double *ddata, int *idata)
{
    double number = 0;
    unsigned char *after_end = NULL;
    unsigned char number_c_string[64];
    unsigned char decimal_point = '.';
    size_t i = 0;
    #define access_at_index(buffer, index) ((buffer!=NULL)&&(buffer[index]!='\n'))
    
    if ((input_buffer == NULL))
    {
        return false;
    }

    /* copy the number into a temporary buffer and replace '.' with the decimal point
     * of the current locale (for strtod)
     * This also takes care of '\0' not necessarily being available for marking the end of the input */
    for (i = 0; (i < (sizeof(number_c_string) - 1)) && access_at_index(input_buffer, i); i++)
    {
        switch (input_buffer[i])
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '+':
            case '-':
            case 'e':
            case 'E':
                number_c_string[i] = input_buffer[i];
                break;

            case '.':
                number_c_string[i] = decimal_point;
                break;

            default:
                goto loop_end;
        }
    }
loop_end:
    number_c_string[i] = '\0';

    number = strtod((const char*)number_c_string, (char**)&after_end);
    if (number_c_string == after_end)
    {
        return false; /* parse_error */
    }
    
    if(strlen(input_buffer) != after_end-number_c_string){
        return false;
    }
    *ddata = number;

    /* use saturation in case of overflow */
    if (number >= INT_MAX)
    {
        *idata = INT_MAX;
    }
    else if (number <= (double)INT_MIN)
    {
        *idata = INT_MIN;
    }
    else
    {
        *idata = (int)number;
    }
    return true;
}

static cJSON* json_read_file(const char *filename, cJSON* root)
{
    long len = 0;
    char *JSON_content;

    FILE* fp = fopen(filename, "rb+");
    if(!fp)
    {
        printf("open file %s failed.\n", filename);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    if(0 == len)
    {
        fclose(fp);
        return NULL;
    }

    fseek(fp, 0, SEEK_SET);
    JSON_content = (char*) malloc(sizeof(char) * len);
    fread(JSON_content, 1, len, fp);

    fclose(fp);
    root = cJSON_Parse(JSON_content);
    if (root == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return NULL;
    }
    free(JSON_content);

    return root;
}

static inline  int json_write_double_param(cJSON* root_json, const char * const grandfather, const char * const parents, const char * const child, double data)
{
    cJSON* root = root_json;
    if(root == NULL || parents == NULL || child == NULL)
        return -1;
    cJSON *grand_object = NULL, *object = NULL;
    if(grandfather != NULL){
        grand_object = cJSON_GetObjectItem(root, grandfather);
        object = cJSON_GetObjectItem(grand_object, parents);
    }else{
        object = cJSON_GetObjectItem(root, parents);
    }
    if(object)
        cJSON_GetObjectItem(object,child)->valuedouble = data;
    else
        return -1;

    return 0;
}

static inline  int json_write_string_param(cJSON* root_json, const char * const grandfather, const char * const parents, const  char * const child, char *data)
{
    cJSON* root = root_json;
    if(root == NULL || parents == NULL || child == NULL || data == NULL)
        return -1;
    cJSON *grand_object = NULL, *object = NULL;
    if(grandfather != NULL){
        grand_object = cJSON_GetObjectItem(root, grandfather);
        object = cJSON_GetObjectItem(grand_object, parents);
    }else{
        object = cJSON_GetObjectItem(root, parents);
    }
    if(object)
        cJSON_ReplaceItemInObject(object, child, cJSON_CreateString((char *)data));
    else
        return -1;
    return 0;
}

#if 0
static int json_print(cJSON *root, int do_format)
{
    if(root == NULL){
        return -1;
    }
    char *printed_json = NULL;
    if (do_format)
    {
        printed_json = cJSON_Print(root);
    }
    else
    {
        printed_json = cJSON_PrintUnformatted(root);
    }
        
    if (printed_json == NULL)
    {
        return -1;
    }
    printf("%s\n", printed_json);
    free(printed_json);
    return 0;
}
#endif

static int json_write_file(char *filename, cJSON *root)
{
    FILE *fp;
    char *printed_json = NULL;
    int do_format = 1;

    if(filename == NULL || root == NULL){
        return -1;
    }
    
    if (do_format)
    {
        printed_json = cJSON_Print(root);
    }
    else
    {
        printed_json = cJSON_PrintUnformatted(root);
    }
    
    fp = fopen(filename, "w");
    if(!fp){
        printf("Open file error!\n");
        return -1;
    }

    fwrite(printed_json,1,strlen(printed_json),fp);

    free(printed_json);
    fclose(fp);
    
    return 0;
}



/* xjson -r -f /etc/config.json -n network,ipaddress -v $DEFAULT_IP */
int main(int argc, char **argv)
{
    #define JSON_NODE_SEPARATOR ","
    #define JSON_NODE_MAX_NUM 4
    char *json_node_buffer[JSON_NODE_MAX_NUM];
    int opt, i;
    double ddata = 0;
    int idata = 0;
    char *file, *nodevaule, *nodestr;
    static cJSON* root_json = NULL;
    int action = 0; /* 0: read, 1:write */

    while ((opt = getopt(argc, argv, "rwf:n:v:")) != -1) {
        switch (opt)
        {
            case 'r':
                action = 0;
                break;
            case 'w':
                action = 1;
                break;
            case 'f':
                file = optarg;
                break;
            case 'n':
                nodestr = optarg;
                break;
            case 'v':
                nodevaule = optarg;
                break;
            default: /* '?' */
                usage(argv[0]);
                exit(0);
        }
    }
    if (argc <= 1){
        usage(argv[0]);
        exit(1);
    }

    for(i = 0; i<JSON_NODE_MAX_NUM ;i++)
        json_node_buffer[i]=NULL;

    root_json = json_read_file(file, root_json);
    if(root_json == NULL){
        printf("json read error\n");
        exit(1);
    }

    char *str_cpy = NULL,*saveptr = NULL, *cur_ptr=NULL;
    int param_num = 0;
    
    str_cpy = strdup(nodestr);
    cur_ptr = strtok_r( str_cpy, JSON_NODE_SEPARATOR, &saveptr );
    json_node_buffer[param_num] = cur_ptr;
    param_num++;
    while (cur_ptr != NULL){
        if(param_num >= JSON_NODE_MAX_NUM){
            printf("json node num is too long: %d\n", param_num);
            exit(0);
        }
        cur_ptr = strtok_r( NULL, JSON_NODE_SEPARATOR, &saveptr );
        json_node_buffer[param_num] = cur_ptr;
        param_num ++;
    }
    
    if(action == 1){ /* write */
        if(parse_number(nodevaule, &ddata, &idata)){
        if(json_write_double_param(root_json, json_node_buffer[2], json_node_buffer[1], json_node_buffer[0], ddata) == 0){
            printf("[%s.%s.%s] number value:%f write ok!\n", json_node_buffer[2], json_node_buffer[1], json_node_buffer[0], ddata);
        }else{
            printf("[%s.%s.%s] number value:%f write faild!\n", json_node_buffer[2], json_node_buffer[1], json_node_buffer[0], ddata);
        }
    }else{
            if(json_write_string_param(root_json, json_node_buffer[2], json_node_buffer[1], json_node_buffer[0], nodevaule) == 0){
                printf("[%s.%s.%s] string value:%s write ok!\n", json_node_buffer[2], json_node_buffer[1], json_node_buffer[0], nodevaule);
            }else{
                printf("[%s.%s.%s] string value:%s write faild!\n", json_node_buffer[2], json_node_buffer[1], json_node_buffer[0], nodevaule);
            }
        }
        //json_print(root_json, 1);
        json_write_file(file, root_json);
    }else{  /* read */
        cJSON *obj = NULL;
        for(i=param_num-2; i>=0; i--){
            if(json_node_buffer[i] == NULL)
                continue;
            if(i == param_num-2)
                obj = root_json;
            obj = cJSON_GetObjectItem(obj, json_node_buffer[i]);
        }
         if(obj!= NULL && cJSON_IsString(obj)){
            printf("%s\n",obj->valuestring);
         }else if(obj!= NULL && cJSON_IsNumber(obj)){
            printf("%d\n",obj->valueint);
         }
    }
    free(str_cpy);
}


