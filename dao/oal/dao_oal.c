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

typedef struct {
	
	int subChannelval;
	int midFreqval;
	int bandwithval;
	int freqResolutionval;
	int centerFreqval;

	mxml_node_t *centerFreq;
	mxml_node_t *array;
	mxml_node_t *infchannel;

	int portval;

	int channelval;
	int windowTypeval;
	int frameDropCntval;
	int freqPointCntval;
	int smoothTimesval;
	int residenceTimeval;
	int residencePolicyval;
	int audioSampleRateval;



	const char *buf;
	const char *macval;
	const char *gatewayval;
	const char *netmaskval;
	const char *ipaddressval;


	mxml_node_t *root;    /* <?xml ... ?> */
	mxml_node_t *tree;
	mxml_node_t *node; 
	mxml_node_t *network;  

	mxml_node_t *mediumfrequency;
	mxml_node_t *freqPoint;
	mxml_node_t *channel;
	mxml_node_t *windowType;
	mxml_node_t *frameDropCnt;	
} spectrum;

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
void *dao_load_default_config_file_singular(char *file,const char *parent_element,const char *element,const char *string,int val)
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
   val:              子元素值

   注:若写入整数，则string 填 NULL，否则 string不为NULL，则表示写入字符串
*/


void *dao_load_default_config_file_array(char *file,const char *array,const char *name,const char *value,const char *element,int val)
{
#if DAO_XML == 1

	FILE *fp;
	mxml_node_t *root,*parent_node,*node;
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

	if(node != NULL)  mxmlDelete(node);  //找到该元素，删除修改

	node =mxmlNewElement(parent_node, element);

	mxmlNewInteger(node, val);

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


const char *dao_load_config_file_singular(char *file,const char *parent_element,const char *element)
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

const char *dao_load_config_file_array(char *file,const char *array,const char *name,const char *value,const char *element)
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




static void *dao_load_default_config_file(char *file)
{
	void *root;
	root = dao_load_default_config_file_singular(file,"network","mac",NULL,87);
	root = dao_load_default_config_file_singular(file,"network","ip",NULL,64);

	root = dao_load_default_config_file_array(file,"infchannel","index","1","centerFreq",2000);
	root = dao_load_default_config_file_array(file,"infchannel","index","1","bandwith",5000);
	root = dao_load_default_config_file_array(file,"infchannel","index","1","freqResolution",155);
	root = dao_load_default_config_file_array(file,"infchannel","index","1","fftSize",23);
	root = dao_load_default_config_file_array(file,"infchannel","index","1","decMethodId",56);

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

        const char *buf = dao_load_config_file_singular(file,"network","mac");
		printf_debug("查找mac地址:%s............\n",buf);       
       
    }else{
        printf_debug("create new config file\n");
		root = dao_load_default_config_file(file);
    }
    printf_debug("1\n");
    root_config = dao_load_root(root);
    printf_debug("1\n");
}

