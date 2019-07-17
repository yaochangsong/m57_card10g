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


#define ARRAY        0
#define ARRAY_PARENT 1
#define ARRAY_ARRAY  2

/* 
   功能:               将各个变量写入或修改到XML文件里
   file:             xml文件名
   parent_element :  父元素
   element :         子元素
   string :          要写入的字符串
   val :             要写入的整数

   注:若写入整数，则string 填 NULL
*/
void *write_config_file_single(char *file,const char *parent_element,const char *element,const char *string,int val)
{
#if DAO_XML == 1

	FILE *fp;
	mxml_node_t *root,*parent_node,*node;
	printf_debug("create singular xml config file\n");


	fp = fopen(file, "r");
	if(fp != NULL) {	
	root = mxmlLoadFile(NULL, fp, MXML_TEXT_CALLBACK);
	fclose(fp);
	}
	else root = mxmlNewXML("1.0");

	parent_node = mxmlFindElement(root, root, parent_element, NULL, NULL,MXML_DESCEND);
	if(parent_node == NULL)  parent_node = mxmlNewElement(root, parent_element);

	node = mxmlFindElement(parent_node, root, element, NULL, NULL,MXML_DESCEND);

	if(node != NULL)  mxmlDelete(node); //找到该元素，删除修改

	node =mxmlNewElement(parent_node, element);

	if(string != NULL) mxmlNewText(node, 0, string);//写字符串

	else   mxmlNewInteger(node, val); //写整型数组

	printf_debug("%s\n", file);
	fp = fopen(file, "w");
	mxmlSaveFile(root, fp, MXML_NO_CALLBACK);
	fclose(fp);

	printf_debug("1\n");
	return (void *)root;
#elif DAO_JSON == 1

#else
    #error "NOT SUPPORT DAO FORMAT"
#endif
    return NULL;
}

/* 
   功能:               将数组变量写入或修改到XML文件里
   file:             xml文件名
   array :           数组名  
   name :            属性名称
   value :           属性值
   element :         子元素
   seed_element:     下级子元素
   seed_array:       下级数组名
   seed_name:        下级属性名
   seed_value:       下级属性值
   val:              子元素值
   series:           建立类型  
*/

void *write_config_file_array(char *file,const char *array,
const char *name,const char *value,const char *element,const char *seed_element,
const char *seed_array,const char *seed_name,const char *seed_value,
int val,char series)
{
#if DAO_XML == 1

	FILE *fp;
	mxml_node_t *root,*parent_node,*node,*seed_node,*s_node;
	printf_debug("create singular xml config file\n");

	fp = fopen(file, "r");
	if(fp != NULL){
	root = mxmlLoadFile(NULL, fp, MXML_TEXT_CALLBACK);
	fclose(fp);
	}
	else root = mxmlNewXML("1.0");

	parent_node = mxmlFindElement(root, root, array, name, value,MXML_DESCEND);

	if(parent_node == NULL){  
	parent_node = mxmlNewElement(root, array);
	mxmlElementSetAttr(parent_node,name,value);
	}

	node = mxmlFindElement(parent_node, root, element,NULL, NULL,MXML_DESCEND);


	if(series == 0)
	{
		if(node != NULL)  mxmlDelete(node);  //找到该元素，删除修改
		node =mxmlNewElement(parent_node, element);

		mxmlNewInteger(node, val);
	}
	else if(series == 1)  
	{
		if(node == NULL)  node = mxmlNewElement(parent_node, element);

		seed_node = mxmlFindElement(node, parent_node, seed_element,NULL, NULL,MXML_DESCEND);

		if(seed_node != NULL)  mxmlDelete(seed_node);  //找到该元素，删除修改

		seed_node =mxmlNewElement(node, seed_element);

		mxmlNewInteger(seed_node, val);
	}
	else if(series == 2)  
	{

		if(node == NULL)  node =  mxmlNewElement(parent_node, element);

		s_node =  mxmlNewElement(node, seed_array);

		mxmlElementSetAttr(s_node,seed_name,seed_value);

		seed_node = mxmlFindElement(s_node, node, seed_element,NULL, NULL,MXML_DESCEND);

		if(seed_node != NULL)  mxmlDelete(seed_node);  //找到该元素，删除修改

		seed_node =mxmlNewElement(s_node, seed_element);

		mxmlNewInteger(seed_node, val);
	}


	printf_debug("%s\n", file);
	fp = fopen(file, "w");
	mxmlSaveFile(root, fp, MXML_NO_CALLBACK);
	fclose(fp);

	printf_debug("1\n");
	return (void *)root;
#elif DAO_JSON == 1

#else
    #error "NOT SUPPORT DAO FORMAT"
#endif
    return NULL;
}


/* 
   功能:               从xml文件里解析出各个变量
   file:             xml文件名
   parent_element :  父元素
   element :         子元素

   函数将返回变量值
*/


const char *read_config_file_single(char *file,const char *parent_element,const char *element)
{
    FILE *fp;
    
#if DAO_XML == 1
	mxml_node_t *tree,*parent_node,*node;
	printf_debug("load xml config file\n");

	fp = fopen(file, "r");
	if(fp != NULL){
	tree = mxmlLoadFile(NULL, fp,MXML_TEXT_CALLBACK);
	fclose(fp);
	}
	parent_node= mxmlFindElement(tree, tree, parent_element, NULL, NULL, MXML_DESCEND);

	node= mxmlFindElement(parent_node, tree, element, NULL, NULL, MXML_DESCEND);

	const char *string = mxmlGetText(node, NULL);

	//mxmlDelete(tree);

	return string;
#elif DAO_JSON == 1

#else
    #error "NOT SUPPORT DAO FORMAT"
#endif
    return NULL;
}

/* 
   功能:               从xml里查找数组元素变量
   file:             xml文件名
   array :           数组名  
   name :            属性名称
   value :           属性值
   element :         子元素

   函数将返回数组元素变量值
*/

const char *read_config_file_array(char *file,const char *array,const char *name,const char *value,const char *element)
{
	FILE *fp;
#if DAO_XML == 1
	mxml_node_t *tree,*parent_node,*node;
	printf_debug("load xml config file\n");

	fp = fopen(file, "r");
	if(fp != NULL){
	tree = mxmlLoadFile(NULL, fp,MXML_TEXT_CALLBACK);
	fclose(fp);
	}

	parent_node = mxmlFindElement(tree, tree, array, name,value, MXML_DESCEND);

	node = mxmlFindElement(parent_node,tree, element,NULL, NULL,MXML_DESCEND);

	const char *string = mxmlGetText(node, NULL);

	return string;
#elif DAO_JSON == 1

#else
    #error "NOT SUPPORT DAO FORMAT"
#endif
    return NULL;
}

#define ARRAY        0
#define ARRAY_PARENT 1
#define ARRAY_ARRAY  2

static void *dao_load_default_config_file(char *file)
{
	void *root;
	root = write_config_file_single(file,"network","mac",NULL,87);
	root = write_config_file_single(file,"network","ip",NULL,64);

	root = write_config_file_array(file,"channel","index","0","cid",NULL,NULL,NULL,NULL,1,ARRAY);


	root = write_config_file_array(file,"channel","index","0","mediumfrequency","centerFreq","freqPoint","indexx","33",77,ARRAY_ARRAY);

	root = write_config_file_array(file,"channel","index","0","radiofrequency","status",NULL,NULL,NULL,99,ARRAY_PARENT);

	return root;
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
		fclose(fp);
        printf_debug("load config file\n");

        const char *buf = read_config_file_single(file,"network","mac");
		printf_debug("查找mac地址:%s............\n",buf);       
       
    }else{
        printf_debug("create new config file\n");
		root = dao_load_default_config_file(file);
    }
    printf_debug("1\n");
    root_config = dao_load_root(root);
    printf_debug("1\n");
}

