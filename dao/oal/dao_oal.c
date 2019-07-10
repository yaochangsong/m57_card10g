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
*  Rev 1.0   06 July 2019   yaochangsong
*  Initial revision.
******************************************************************************/

#include "config.h"

static void *dao_load_default_config_file(char *file)
{
#if DAO_XML == 1
    FILE *fp;
    mxml_node_t *root;    /* <?xml ... ?> */
    mxml_node_t *data;   /* <data> */
    mxml_node_t *node;   /* <node> */
    mxml_node_t *group;  /* <group> */
    printf_debug("create xml config file\n");
    root = mxmlNewXML("1.0");
    data = mxmlNewElement(root, "data");

    node = mxmlNewElement(data, "node");
    mxmlNewText(node, 0, "val1");
    node = mxmlNewElement(data, "node");
    mxmlNewText(node, 0, "val2");
    node = mxmlNewElement(data, "node");
    mxmlNewText(node, 0, "val3");

    group = mxmlNewElement(data, "group");

    node = mxmlNewElement(group, "node");
    mxmlNewText(node, 0, "val4");
    node = mxmlNewElement(group, "node");
    mxmlNewText(node, 0, "val5");
    node = mxmlNewElement(group, "node");
    mxmlNewText(node, 0, "val6");

    node = mxmlNewElement(data, "node");
    mxmlNewText(node, 0, "val7");
    node = mxmlNewElement(data, "node");
    mxmlNewText(node, 0, "val8");
    printf_debug("%s\n", file);
    fp = fopen(file, "w");
    mxmlSaveFile(root, fp, MXML_NO_CALLBACK);
    fclose(fp);
    return (void *)root;
#elif DAO_JSON == 1

#else
    #error "NOT SUPPORT DAO FORMAT"
#endif
    return NULL;
}

static void *dao_load_config_file(FILE *fp)
{
    if(fp == NULL){
        return NULL;
    }
#if DAO_XML == 1
    mxml_node_t *root;
    printf_debug("load xml config file\n");
    root = mxmlLoadFile(NULL, fp, MXML_OPAQUE_CALLBACK);
    return (void *)root;
#elif DAO_JSON == 1

#else
    #error "NOT SUPPORT DAO FORMAT"
#endif
    return NULL;
}

static void *dao_load_root(void *root)
{
    if(root == NULL){
        return NULL;
    }
#if DAO_XML == 1

#elif DAO_JSON == 1

#else
#error "NOT SUPPORT DAO FORMAT"
#endif
}


void dao_read_create_config_file(char *file, void *root_config)
{
    void *root;
    FILE *fp;
    /* read/write or create file */
    fp = fopen(file, "r");
    if(fp != NULL){
        printf_debug("load config file\n");
        root = dao_load_config_file(fp);
        fclose(fp);
    }else{
        printf_debug("create new config file\n");
        root = dao_load_default_config_file(file);
    }
    root_config = dao_load_root(root);
}

