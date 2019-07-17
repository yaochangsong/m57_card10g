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


mxml_node_t *whole_root;
spectrum spectrum_fre;

/* 
   功能:               将各个变量写入或修改到XML文件里
   file:             xml文件名
   parent_element :  父元素
   element :         子元素
   string :          要写入的字符串
   val :             要写入的整数

   注:若写入整数，则string 填 NULL
*/
void *write_config_file_single(char *file,const char *parent_element,const char *element,
const char *s_element,const char *seed_element,
const char *string,int val,char series)
{
#if DAO_XML == 1
    FILE *fp;
	mxml_node_t *parent_node,*node,*seed_node,*s_node;;
	printf_debug("create singular xml config file\n");

	parent_node = mxmlFindElement(whole_root, whole_root, parent_element, NULL, NULL,MXML_DESCEND);
	if(parent_node == NULL)  parent_node = mxmlNewElement(whole_root, parent_element);

	node = mxmlFindElement(parent_node, whole_root, element, NULL, NULL,MXML_DESCEND);
	
	if(series ==0)
	{
      if(node != NULL)  mxmlDelete(node); //找到该元素，删除修改

	  node =mxmlNewElement(parent_node, element);

	  if(string != NULL) mxmlNewText(node, 0, string);//写字符串

	  else   mxmlNewInteger(node, val); //写整型数组
	}
	else if(series == 1)
	{
       if(node == NULL)  node =mxmlNewElement(parent_node, element);

	   s_node = mxmlFindElement(node, parent_node, s_element, NULL, NULL,MXML_DESCEND);

	   if(s_node != NULL)  mxmlDelete(s_node); //找到该元素，删除修改

	   s_node =mxmlNewElement(node, s_element);
	  
	   if(string != NULL) mxmlNewText(s_node, 0, string);//写字符串

	   else   mxmlNewInteger(s_node, val); //写整型数组
	}
	else if(series == 2)
	{
       if(node == NULL)  node =mxmlNewElement(parent_node, element);

	   s_node = mxmlFindElement(node, parent_node, s_element, NULL, NULL,MXML_DESCEND);

	   if(s_node == NULL)  s_node =mxmlNewElement(node, s_element);

	   seed_node = mxmlFindElement(s_node, node, seed_element, NULL, NULL,MXML_DESCEND);

	   
	   if(seed_node != NULL)  mxmlDelete(seed_node); //找到该元素，删除修改

	   seed_node =mxmlNewElement(s_node, seed_element);
	  
	   if(string != NULL) mxmlNewText(seed_node, 0, string);//写字符串

	   else   mxmlNewInteger(seed_node, val); //写整型数组
	}

	

	printf_debug("%s\n", file);
	fp = fopen(file, "w");
	mxmlSaveFile(whole_root, fp, MXML_NO_CALLBACK);
	fclose(fp);

	printf_debug("1\n");
	return (void *)whole_root;
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






void *write_config_file_array(spectrum        spectrum_fre)
{
#if DAO_XML == 1

	FILE *fp;
	mxml_node_t *parent_node,*node,*seed_node,*s_node;
	printf_debug("create singular xml config file\n");

	parent_node = mxmlFindElement(whole_root, whole_root, spectrum_fre.array, spectrum_fre.name, spectrum_fre.value,MXML_DESCEND);

	if(parent_node == NULL){  
	parent_node = mxmlNewElement(whole_root, spectrum_fre.array);
	mxmlElementSetAttr(parent_node,spectrum_fre.name,spectrum_fre.value);
	}

	node = mxmlFindElement(parent_node, whole_root, spectrum_fre.element,NULL, NULL,MXML_DESCEND);


	if(spectrum_fre.series == ARRAY)
	{
		if(node != NULL)  mxmlDelete(node);  //找到该元素，删除修改
		node =mxmlNewElement(parent_node, spectrum_fre.element);

		mxmlNewInteger(node, spectrum_fre.val);
	}
	else if(spectrum_fre.series == ARRAY_PARENT)  
	{
		if(node == NULL)  node = mxmlNewElement(parent_node, spectrum_fre.element);

		seed_node = mxmlFindElement(node, parent_node, spectrum_fre.seed_element,NULL, NULL,MXML_DESCEND);

		if(seed_node != NULL)  mxmlDelete(seed_node);  //找到该元素，删除修改

		seed_node =mxmlNewElement(node, spectrum_fre.seed_element);

		mxmlNewInteger(seed_node, spectrum_fre.val);
	}
	else if(spectrum_fre.series == ARRAY_ARRAY)  
	{

		if(node == NULL)  node =  mxmlNewElement(parent_node, spectrum_fre.element);

		s_node =  mxmlNewElement(node, spectrum_fre.seed_array);

		mxmlElementSetAttr(s_node,spectrum_fre.seed_name,spectrum_fre.seed_value);

		seed_node = mxmlFindElement(s_node, node, spectrum_fre.seed_element,NULL, NULL,MXML_DESCEND);

		if(seed_node != NULL)  mxmlDelete(seed_node);  //找到该元素，删除修改

		seed_node =mxmlNewElement(s_node, spectrum_fre.seed_element);

		mxmlNewInteger(seed_node, spectrum_fre.val);
	}


	printf_debug("%s\n", spectrum_fre.file);
	fp = fopen(spectrum_fre.file, "w");
	mxmlSaveFile(whole_root, fp, MXML_NO_CALLBACK);
	fclose(fp);

	printf_debug("1\n");
	return (void *)whole_root;
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
#if DAO_XML == 1
	mxml_node_t *parent_node,*node;
	printf_debug("load xml config file\n");


	parent_node= mxmlFindElement(whole_root, whole_root, parent_element, NULL, NULL, MXML_DESCEND);

	node= mxmlFindElement(parent_node, whole_root, element, NULL, NULL, MXML_DESCEND);

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
#if DAO_XML == 1
	mxml_node_t *parent_node,*node;
	printf_debug("load xml config file\n");


	parent_node = mxmlFindElement(whole_root, whole_root, array, name,value, MXML_DESCEND);

	node = mxmlFindElement(parent_node,whole_root, element,NULL, NULL,MXML_DESCEND);

	const char *string = mxmlGetText(node, NULL);

	return string;
#elif DAO_JSON == 1

#else
    #error "NOT SUPPORT DAO FORMAT"
#endif
    return NULL;
}



static void *dao_load_default_config_file(char *file)
{
	void *root;
	
	root = write_config_file_single(file,"network","mac","asd",NULL,NULL,87,1);
	root = write_config_file_single(file,"network","aa","bb","cc",NULL,87,2);
	root = write_config_file_single(file,"network","ip",NULL,NULL,NULL,64,0);




	spectrum_fre.file = file;
	spectrum_fre.array = "channel";
	spectrum_fre.name = "index";
	spectrum_fre.value = "0";
	
    spectrum_fre.element = "mediumfrequency";
	spectrum_fre.seed_element = "centerFreq";
	spectrum_fre.seed_array = "freqPoint";
	spectrum_fre.seed_name = "indexx";
	spectrum_fre.seed_value = "37";
	spectrum_fre.val = 90;   
	spectrum_fre.series = ARRAY_ARRAY;
	root = write_config_file_array(spectrum_fre);

    spectrum_fre.element = "cid";
	spectrum_fre.val = 3;   
	spectrum_fre.series = ARRAY;
	root = write_config_file_array(spectrum_fre);

	
	//root = write_config_file_array(file,"channel","index","0","mediumfrequency","centerFreq","freqPoint","indexx","33",77,ARRAY_ARRAY);
	//root = write_config_file_array(file,"channel","index","0","radiofrequency","status",NULL,NULL,NULL,99,ARRAY_PARENT);

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
		whole_root = mxmlLoadFile(NULL, fp, MXML_TEXT_CALLBACK);
		
        printf_debug("load config file\n");
        const char *buf = read_config_file_single(file,"network","cc");
		printf_debug("查找cc地址:%s............\n",buf);   
		fclose(fp);
       
    }else{//
		
		whole_root = mxmlNewXML("1.0");	
        printf_debug("create new config file\n");
		root = dao_load_default_config_file(file);
    }
	
    printf_debug("1\n");
    root_config = dao_load_root(root);
    printf_debug("1\n");
}

