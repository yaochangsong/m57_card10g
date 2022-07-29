#include <string.h>  
#include <stdlib.h>  
#include <stdio.h>  
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <getopt.h>
#include <arpa/inet.h>
#include "nr_1800.h" 
#include "nr_config.h" 
#include "../../dao/json/cJSON.h"


struct nr_config{
    char *path;
    srio_route_rule **rtable;
    size_t rtable_len;
};

static cJSON* nr_root_json = NULL;

static int nr_json_print(cJSON *root, int do_format)
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


static cJSON* nr_json_read_file(const char *filename, cJSON* root)
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

static int nr_json_parse_config_param(const cJSON* root, struct nr_config *config)
{
    cJSON *route = NULL, *node, *val;
    route = cJSON_GetObjectItem(root, "route");
    if(route == NULL){
        printf("not found route table\n");
        return -1;
    }
    int len = cJSON_GetArraySize(route);
    if(len <= 0){
        printf_debug("route table len is %d\n", len);
        config->rtable_len = 0;
        config->rtable = NULL;
        return -1;
    }
    config->rtable = (srio_route_rule **)calloc(1, sizeof(config->rtable) * len);
    for(int i = 0; i < len; i++){
        config->rtable[i] = (srio_route_rule *)calloc(1, sizeof(srio_route_rule));
    }
    config->rtable_len = len;
    for(int i = 0; i < len; i++){
        printf_debug("index:%d ", i);
        node = cJSON_GetArrayItem(route, i);
        val = cJSON_GetObjectItem(node, "port_num");
        if(cJSON_IsNumber(val)){
            config->rtable[i]->port_num = val->valueint;
            printf_debug("port_num:%d, ",  config->rtable[i]->port_num);
        }
        val = cJSON_GetObjectItem(node, "dest_id");
        if(cJSON_IsNumber(val)){
            config->rtable[i]->dest_id = val->valueint;
            printf_debug("dest_id:%d, ", config->rtable[i]->dest_id);
        }
        val = cJSON_GetObjectItem(node, "dest_port");
        if(cJSON_IsNumber(val)){
            config->rtable[i]->dest_port = val->valueint;
            printf_debug("dest_port:%d, ", config->rtable[i]->dest_port);
        }
        printf_debug("\n");
    }
    return 0;
}


static int nr_json_read_config_file(const void *config)
{
    char *file;
    struct nr_config *conf = (struct nr_config*)config;
    
    file = conf->path;
    if(file == NULL || config == NULL)
        exit(1);
    
    nr_root_json = nr_json_read_file(file, nr_root_json);
    if(nr_root_json == NULL){
        printf("json read error:%s\n", file);
        exit(1);
    }
    //nr_json_print(nr_root_json, 1);
    if(nr_json_parse_config_param(nr_root_json, conf) == -1){
        exit(1);
    }
    return 0;
}

static struct nr_config nrconfig;

srio_route_rule  **nr_get_route_table(int *count)
{
    if(count)
        *count = nrconfig.rtable_len;
    return nrconfig.rtable;
}



void nr_config_init(void)
{
    char *path;
    #define NR_CONFIG_PATH "nr_config.json"
    printf_debug("nr_config_init.\n");
    path = get_config_path();
    if( path != NULL && strlen(path) > 0)
        nrconfig.path = path;
    else
        nrconfig.path = strdup(NR_CONFIG_PATH);
    printf_debug("config file path: %s\n", nrconfig.path);
    nr_json_read_config_file(&nrconfig);
}

