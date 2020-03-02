/******************************************************************************
*  Copyright 2019, Showay Technology Dev Co.,Ltd.
*  ---------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Showay Technology Dev Co.,Ltd. (C) 2019
******************************************************************************/
/*****************************************************************************     
*  Rev 1.0   29 Feb 2020   yaochangsong
*  Initial revision.
******************************************************************************/
#include "config.h"

int parse_json_rf_multi_value(const char * const body)
{
    printf_note("...\n");
    char *data = "{\"version\": \"1.0\"}";
    cJSON *root = cJSON_Parse(data);
    cJSON *node, *value;
    printf_note("...\n");
    if (root == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return -1;
    }
    printf_note("...\n");
    node = cJSON_GetObjectItem(root, "version");
    printf_note("...\n");
    if(node != NULL && cJSON_IsString(node)){
        printf_note("version [%s]\n",node->valuestring);
    }
    printf_note("...\n");
    return 0;
}

int parse_json_if_multi_value(const char * const body)
{
    return 0;
}

int parse_json_multi_band(const char * const body)
{
    return 0;
}

int parse_json_muti_point(const char * const body)
{
    return 0;
}
int parse_json_demodulation(const char * const body)
{
    return 0;
}


/* NOTE: 调用该函数后，需要free返回指针 */
char *assemble_json_response(int err_code, const char *message)
{
    char *str_json = NULL;
    cJSON *root = cJSON_CreateObject();
    
    cJSON_AddNumberToObject(root, "code", err_code);
    cJSON_AddStringToObject(root, "message", message);
    
    str_json = cJSON_PrintUnformatted(root);
    return str_json;
}


/* NOTE: 调用该函数后，需要free返回指针 */
char *assemble_json_data_response(int err_code, const char *message, const char * const data)
{
    char *str_json = NULL, *body = NULL;
    cJSON *root, *node;

    str_json = assemble_json_response(err_code, message);
    root = cJSON_Parse(str_json);
    node = cJSON_Parse(data);
    if (root != NULL){
        cJSON_AddItemToObject(root, "data", node);
    }
    body = cJSON_PrintUnformatted(root);
    return body;
}




