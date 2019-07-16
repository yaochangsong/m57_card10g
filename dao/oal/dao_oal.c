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
static void *dao_load_default_config_file_singular(char *file,char *parent_element,char *element,char *string,int val)
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


static void *dao_load_default_config_file_array(char *file,char *array,char *name,char *value,char *element,int val)
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



static void *dao_load_config_file(FILE *fp)
{
    if(fp == NULL){
        return NULL;
    }
#if DAO_XML == 1
    //mxml_node_t *root;
    printf_debug("load xml config file\n");

	
	spectrum_fre.tree = mxmlLoadFile(NULL, fp,MXML_TEXT_CALLBACK);

	
    spectrum_fre.node= mxmlFindElement(spectrum_fre.tree, spectrum_fre.tree, "mac", NULL, NULL, MXML_DESCEND);
    spectrum_fre.macval = mxmlGetText(spectrum_fre.node, NULL);
    printf_debug("mac结构体读取:%s\n",spectrum_fre.macval);


	spectrum_fre.node= mxmlFindElement(spectrum_fre.tree, spectrum_fre.tree, "gateway", NULL, NULL, MXML_DESCEND);
    spectrum_fre.gatewayval = mxmlGetText(spectrum_fre.node, NULL);
    printf_debug("gateway结构体读取:%s\n",spectrum_fre.gatewayval);

	spectrum_fre.node= mxmlFindElement(spectrum_fre.tree, spectrum_fre.tree, "netmask", NULL, NULL, MXML_DESCEND);
    spectrum_fre.netmaskval = mxmlGetText(spectrum_fre.node, NULL);
    printf_debug("netmask结构体读取:%s\n",spectrum_fre.netmaskval);

	spectrum_fre.node= mxmlFindElement(spectrum_fre.tree, spectrum_fre.tree, "ipaddress", NULL, NULL, MXML_DESCEND);
    spectrum_fre.ipaddressval = mxmlGetText(spectrum_fre.node, NULL);
    printf_debug("ipaddress结构体读取:%s\n",spectrum_fre.ipaddressval);
   
    
    spectrum_fre.node= mxmlFindElement(spectrum_fre.tree, spectrum_fre.tree, "port",NULL, NULL,MXML_DESCEND);
    spectrum_fre.buf = mxmlGetText(spectrum_fre.node,NULL);
    spectrum_fre.portval = atoi(spectrum_fre.buf);
    printf_debug("port结构体读取:%d\n",spectrum_fre.portval);


	spectrum_fre.node= mxmlFindElement(spectrum_fre.tree, spectrum_fre.tree, "channel",NULL, NULL,MXML_DESCEND);
    spectrum_fre.buf = mxmlGetText(spectrum_fre.node,NULL);
    spectrum_fre.channelval = atoi(spectrum_fre.buf);
    printf_debug("channel结构体读取:%d\n",spectrum_fre.channelval);

	spectrum_fre.node= mxmlFindElement(spectrum_fre.tree, spectrum_fre.tree, "windowType",NULL, NULL,MXML_DESCEND);
    spectrum_fre.buf = mxmlGetText(spectrum_fre.node,NULL);
    spectrum_fre.windowTypeval = atoi(spectrum_fre.buf);
    printf_debug("windowType结构体读取:%d\n",spectrum_fre.windowTypeval);

	spectrum_fre.node= mxmlFindElement(spectrum_fre.tree, spectrum_fre.tree, "frameDropCnt",NULL, NULL,MXML_DESCEND);
    spectrum_fre.buf = mxmlGetText(spectrum_fre.node,NULL);
    spectrum_fre.frameDropCntval = atoi(spectrum_fre.buf);
    printf_debug("frameDropCnt结构体读取:%d\n",spectrum_fre.frameDropCntval);

	spectrum_fre.node= mxmlFindElement(spectrum_fre.tree, spectrum_fre.tree, "freqPointCnt",NULL, NULL,MXML_DESCEND);
    spectrum_fre.buf = mxmlGetText(spectrum_fre.node,NULL);
    spectrum_fre.freqPointCntval = atoi(spectrum_fre.buf);
    printf_debug("freqPointCnt结构体读取:%d\n",spectrum_fre.freqPointCntval);

	spectrum_fre.node= mxmlFindElement(spectrum_fre.tree, spectrum_fre.tree, "smoothTimes",NULL, NULL,MXML_DESCEND);
    spectrum_fre.buf = mxmlGetText(spectrum_fre.node,NULL);
    spectrum_fre.smoothTimesval = atoi(spectrum_fre.buf);
    printf_debug("smoothTimes结构体读取:%d\n",spectrum_fre.smoothTimesval);


	spectrum_fre.node= mxmlFindElement(spectrum_fre.tree, spectrum_fre.tree, "residenceTime",NULL, NULL,MXML_DESCEND);
    spectrum_fre.buf = mxmlGetText(spectrum_fre.node,NULL);
    spectrum_fre.residenceTimeval = atoi(spectrum_fre.buf);
    printf_debug("residenceTime结构体读取:%d\n",spectrum_fre.residenceTimeval);

	spectrum_fre.node= mxmlFindElement(spectrum_fre.tree, spectrum_fre.tree, "residencePolicy",NULL, NULL,MXML_DESCEND);
    spectrum_fre.buf = mxmlGetText(spectrum_fre.node,NULL);
    spectrum_fre.residencePolicyval = atoi(spectrum_fre.buf);
    printf_debug("residencePolicy结构体读取:%d\n",spectrum_fre.residencePolicyval);

	spectrum_fre.node= mxmlFindElement(spectrum_fre.tree, spectrum_fre.tree, "audioSampleRate",NULL, NULL,MXML_DESCEND);
    spectrum_fre.buf = mxmlGetText(spectrum_fre.node,NULL);
    spectrum_fre.audioSampleRateval = atoi(spectrum_fre.buf);
    printf_debug("audioSampleRate结构体读取:%d\n",spectrum_fre.audioSampleRateval);



	spectrum_fre.array = mxmlFindElement(spectrum_fre.tree, spectrum_fre.tree, "array",NULL, NULL,MXML_DESCEND);
	spectrum_fre.infchannel = mxmlFindElement(spectrum_fre.array, spectrum_fre.tree, "infchannel", "index","1", MXML_DESCEND);

	spectrum_fre.centerFreq = mxmlFindElement(spectrum_fre.infchannel, spectrum_fre.tree, "centerFreq",NULL, NULL,MXML_DESCEND);

	printf_debug("centerFreq = %s\n",mxmlGetText(spectrum_fre.centerFreq, NULL));

	spectrum_fre.infchannel = mxmlFindElement(spectrum_fre.array, spectrum_fre.tree, "infchannel", "index","2",MXML_DESCEND);

	spectrum_fre.centerFreq = mxmlFindElement(spectrum_fre.infchannel, spectrum_fre.tree, "centerFreq",NULL, NULL,MXML_DESCEND);

	printf_debug("centerFreq2 = %s\n",mxmlGetText(spectrum_fre.centerFreq, NULL));
	


    mxmlDelete(spectrum_fre.tree);

	
    //root = mxmlLoadFile(NULL, fp, MXML_OPAQUE_CALLBACK);
    /*mxml_node_t * tree;
	tree = mxmlLoadFile(NULL, fp,MXML_TEXT_CALLBACK);

    return (void *)tree;*/
#elif DAO_JSON == 1

#else
    #error "NOT SUPPORT DAO FORMAT"
#endif
    return NULL;
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
    printf_debug("1\n");
    root_config = dao_load_root(root);
    printf_debug("1\n");
}

