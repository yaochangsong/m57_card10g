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
*  Rev 2.0   22 July 2019   yonglang
*  Initial revision.
******************************************************************************/

#include "config.h"
#include "../mxml-3.0/config.h"
#include "../mxml-3.0/mxml-private.h"
#ifndef _WIN32
#  include <unistd.h>
#endif /* !_WIN32 */
#include <fcntl.h>
#ifndef O_BINARY
#  define O_BINARY 0
#endif /* !O_BINARY */


mxml_node_t *whole_root;
mxml_node_t *calibration_root;

spectrum spectrum_array;
spectrum_single spectrum_sing;

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
#ifdef SUPPORT_DAO_XML
    printf_debug("开始写入\n");
	FILE *fp;
    char buf[32];
	mxml_node_t *parent_node,*node;
	printf_debug("create singular xml config file\n");

    if(whole_root == NULL) 
    {
      printf_debug("xml文件为空，不能写入\n");
      return NULL;
    }

	parent_node = mxmlFindElement(whole_root, whole_root, parent_element, NULL, NULL,MXML_DESCEND);
	if(parent_node == NULL)  parent_node = mxmlNewElement(whole_root, parent_element);

	node = mxmlFindElement(parent_node, whole_root, element, NULL, NULL,MXML_DESCEND);

    if(node != NULL)  
    {
        if(string == NULL)  
        {
            sprintf(buf,"%d",val);
            mxmlSetText(node, 0, buf);//写字符串
        }
        else mxmlSetText(node, 0, string);//写字符串
    }//mxmlDelete(node); //找到该元素，删除修改
    else
    {
        node =mxmlNewElement(parent_node, element);
        if(string == NULL)  
        {
            sprintf(buf,"%d",val);
            mxmlNewText(node, 0, buf);//写字符串
        }
        else mxmlNewText(node, 0, string);//写字符串
    }

    

	

	

	printf_debug("%s\n", file);
	fp = fopen(file, "w");
	mxmlSaveFile(whole_root, fp, MXML_NO_CALLBACK);
	fclose(fp);
    
	printf_debug("1\n");
    printf_debug("写入完成\n");
	return (void *)whole_root;
#elif defined SUPPORT_DAO_JSON

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
#ifdef SUPPORT_DAO_XML
    printf_debug("开始写入\n");
	FILE *fp;
    char buff[32];
	mxml_node_t *parent_node,*node,*seed_node,*s_node;
	printf_debug("create singular xml config file\n");

    if(whole_root == NULL) 
    {
      printf_debug("xml文件为空，不能写入\n");
      return NULL;
    }

	parent_node = mxmlFindElement(whole_root, whole_root, array, name, value,MXML_DESCEND);

	if(parent_node == NULL){  
	parent_node = mxmlNewElement(whole_root, array);
	mxmlElementSetAttr(parent_node,name,value);

	}

	node = mxmlFindElement(parent_node, whole_root, element,NULL, NULL,MXML_DESCEND);


	if(series == ARRAY)
	{
        if(node != NULL)   
        {
            sprintf(buff,"%d",val);
            mxmlSetText(node, 0, buff);//写字符串
        }
        else
        {
            node =mxmlNewElement(parent_node, element);
            sprintf(buff,"%d",val);
            mxmlNewText(node, 0, buff);//写字符串
        }
	}
	else if(series == ARRAY_PARENT)  
	{
		if(node == NULL)  node = mxmlNewElement(parent_node, element);

        

		seed_node = mxmlFindElement(node, parent_node, seed_element,NULL, NULL,MXML_DESCEND);

        if(seed_node != NULL)   
        {
            sprintf(buff,"%d",val);
            mxmlSetText(seed_node, 0, buff);//写字符串
        }
        else
        {
            seed_node =mxmlNewElement(node, seed_element);
		    sprintf(buff,"%d",val);
            mxmlNewText(seed_node, 0, buff);
        }
	}
	else if(series == ARRAY_ARRAY)  
	{

		if(node == NULL)  node =  mxmlNewElement(parent_node, element);

        //s_node = mxmlFindElement(node, parent_node, seed_array,NULL, NULL,MXML_DESCEND);

		//if(s_node == NULL) s_node =  mxmlNewElement(node, seed_array);

        //if(s_node == NULL)  mxmlElementSetAttr(s_node,seed_name,seed_value);

		//else s_node = mxmlFindElement(node, parent_node, seed_array, seed_name, seed_value,MXML_DESCEND);

        s_node = mxmlFindElement(node, parent_node, seed_array, seed_name, seed_value,MXML_DESCEND);

        if(s_node == NULL)
        {
         
           s_node =  mxmlNewElement(node, seed_array);
           mxmlElementSetAttr(s_node,seed_name,seed_value);
        }


		seed_node = mxmlFindElement(s_node, node, seed_element,NULL, NULL,MXML_DESCEND);

        if(seed_node != NULL)   
        {
            sprintf(buff,"%d",val);
            mxmlSetText(seed_node, 0, buff);//写字符串
        }
        else
        {
          seed_node =mxmlNewElement(s_node, seed_element);
		  sprintf(buff,"%d",val);
          mxmlNewText(seed_node, 0, buff);
        }
	}


	printf_debug("%s\n", file);
	fp = fopen(file, "w");
	mxmlSaveFile(whole_root, fp, MXML_NO_CALLBACK);
	fclose(fp);

	printf_debug("1\n");
    printf_debug("写入完成\n");
	return (void *)whole_root;
#elif defined SUPPORT_DAO_JSON

#else
    #error "NOT SUPPORT DAO FORMAT"
#endif
    return NULL;
}



/* 
   功能:               将各个变量写入或修改到XML文件里
   file:             xml文件名
   parent_element :  父元素
   element :         子元素
   string :          要写入的字符串
   val :             要写入的整数

   注:若写入整数，则string 填 NULL
*/




void *write_struc_config_file_single(spectrum_single          spectrum_fre)
{
#ifdef SUPPORT_DAO_XML
    FILE *fp;
    mxml_node_t *parent_node,*node,*seed_node,*s_node;;
    printf_debug("create singular xml config file\n");

    parent_node = mxmlFindElement(whole_root, whole_root, spectrum_fre.parent_element, NULL, NULL,MXML_DESCEND);
    if(parent_node == NULL)  parent_node = mxmlNewElement(whole_root, spectrum_fre.parent_element);

    node = mxmlFindElement(parent_node, whole_root, spectrum_fre.element, NULL, NULL,MXML_DESCEND);

    if(spectrum_fre.series == ONE_LEVEL)
    {
        if(node != NULL)  mxmlDelete(node); //找到该元素，删除修改

        node =mxmlNewElement(parent_node, spectrum_fre.element);

        if(spectrum_fre.string != NULL) mxmlNewText(node, 0, spectrum_fre.string);//写字符串

        else   mxmlNewInteger(node, spectrum_fre.val); //写整型数组
    }
    else if(spectrum_fre.series == TWO_LEVEL)
    {
        if(node == NULL)  node =mxmlNewElement(parent_node, spectrum_fre.element);

        s_node = mxmlFindElement(node, parent_node, spectrum_fre.s_element, NULL, NULL,MXML_DESCEND);

        if(s_node != NULL)  mxmlDelete(s_node); //找到该元素，删除修改

        s_node =mxmlNewElement(node, spectrum_fre.s_element);

        if(spectrum_fre.string != NULL) mxmlNewText(s_node, 0, spectrum_fre.string);//写字符串

        else   mxmlNewInteger(s_node, spectrum_fre.val); //写整型数组
    }
    else if(spectrum_fre.series == THREE_LEVEL)
    {
        if(node == NULL)  node =mxmlNewElement(parent_node, spectrum_fre.element);

        s_node = mxmlFindElement(node, parent_node, spectrum_fre.s_element, NULL, NULL,MXML_DESCEND);

        if(s_node == NULL)  s_node =mxmlNewElement(node, spectrum_fre.s_element);

        seed_node = mxmlFindElement(s_node, node, spectrum_fre.seed_element, NULL, NULL,MXML_DESCEND);


        if(seed_node != NULL)  mxmlDelete(seed_node); //找到该元素，删除修改

        seed_node =mxmlNewElement(s_node, spectrum_fre.seed_element);

        if(spectrum_fre.string != NULL) mxmlNewText(seed_node, 0, spectrum_fre.string);//写字符串

        else   mxmlNewInteger(seed_node, spectrum_fre.val); //写整型数组
    }

    printf_debug("%s\n", spectrum_fre.file);
    fp = fopen(spectrum_fre.file, "w");
    mxmlSaveFile(whole_root, fp, MXML_NO_CALLBACK);
    fclose(fp);

    printf_debug("1\n");
    return (void *)whole_root;
#elif defined SUPPORT_DAO_JSON

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


void *write_struc_config_file_array(spectrum          spectrum_fre)
{
#ifdef SUPPORT_DAO_XML

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
#elif defined SUPPORT_DAO_JSON

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


const char *read_config_file_single(const char *parent_element,const char *element)
{ 
#ifdef SUPPORT_DAO_XML
    printf_debug("开始读取single\n");

    mxml_node_t *parent_node,*node;
    //printf_debug("load xml config file\n");

    parent_node = mxmlFindElement(whole_root, whole_root, parent_element, NULL, NULL, MXML_DESCEND);

    if(parent_node == NULL) return NULL; 
    node= mxmlFindElement(parent_node, whole_root, element, NULL, NULL, MXML_DESCEND);
    if(node == NULL) return NULL;

    const char *string = mxmlGetText(node, NULL);

    printf_debug("读取结束\n");
    return string;
    
#elif defined SUPPORT_DAO_JSON

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

const char *read_config_file_array(mxml_node_t *root, const char *array,const char *name,const char *value,const char *element)
{
#ifdef SUPPORT_DAO_XML
    mxml_node_t *parent_node,*node;
    //printf_debug("load xml config file\n");

    parent_node = mxmlFindElement(root, root, array, name,value, MXML_DESCEND);
    if(parent_node == NULL) return NULL;

    node = mxmlFindElement(parent_node,root, element,NULL, NULL,MXML_DESCEND);
    if(node == NULL) return NULL;

    const char *string = mxmlGetText(node, NULL);
    return string;
#elif defined SUPPORT_DAO_JSON

#else
    #error "NOT SUPPORT DAO FORMAT"
#endif
    return NULL;
}




static void *dao_load_default_config_file(char *file)
{
#ifdef SUPPORT_DAO_XML
    FILE *fp;

    mxml_node_t *root;    /* <?xml ... ?> */
    mxml_node_t *child;   /* <data> */
    mxml_node_t *node;   /* <node> */
    mxml_node_t *group;  /* <group> */
    printf_debug("create xml config file\n");

    /*二级节点*/
    mxml_node_t *network;
    mxml_node_t *controlPara;
    mxml_node_t *statusPara;
    mxml_node_t *fileStorePara;
    mxml_node_t *channel;

    /*控制参数下节点*/
    mxml_node_t *dataOutPutEn;
    /*状态参数下节点的定义*/
   // mxml_node_t *channelInfo;
    mxml_node_t *softVersion;
    mxml_node_t *diskInfo;
    mxml_node_t *clkInfo;
    mxml_node_t *adInfo;
   // mxml_node_t *rfInfo;
    mxml_node_t *fpgaInfo;
    mxml_node_t *powerstate;
    //mxml_node_t *diskNode;
    mxml_node_t *version;

    /*root下的一级节点*/
    printf_debug("create xml config file\n");
    whole_root = mxmlNewXML("1.0");
    /*创建版本信息*/
    root = mxmlNewElement(whole_root, "root");
    version = mxmlNewElement(root, "version");
    mxmlNewText(version, 0, "1.0");

   
    network= mxmlNewElement(root, "network");
    controlPara = mxmlNewElement(root, "controlPara");
    statusPara = mxmlNewElement(root, "statusPara");
    fileStorePara = mxmlNewElement(root, "fileStorePara");

     /*创建网络参数*/
    node = mxmlNewElement(network, "mac");
    mxmlNewText(node, 0, "38:2c:4a:ba:57:1d");
    node = mxmlNewElement(network, "gateway");
    mxmlNewText(node, 0, "192.168.1.1");
    node = mxmlNewElement(network, "netmask");
    mxmlNewText(node, 0, "255.255.255.0");
    node = mxmlNewElement(network, "ipaddress");
    mxmlNewText(node, 0, "192.168.1.111");
    node = mxmlNewElement(network, "port");
    mxmlNewText(node, 0, "1325");

    /*中频参数*/
    char p[10]={0};
    char temp[10]={0};
    int i=0;
    int j=0;
    for(i=0;i<1;i++)
    {
        memset(p,0,sizeof(char)*10);
        sprintf(p,"%d",i);
        channel=mxmlNewElement(root,"channel");
        mxmlElementSetAttr(channel,"index",p);
        node = mxmlNewElement(channel, "cid");
        mxmlNewText(node, 0, "1");
        node = mxmlNewElement(channel, "mediumfrequency");
        for(j=0;j<1;j++)
        {
            memset(temp,0,sizeof(char)*10);
            sprintf(temp,"%d",j);
            group = mxmlNewElement(node, "freqPoint");
            mxmlElementSetAttr(group,"index",temp);
            child = mxmlNewElement(group, "centerFreq");
            mxmlNewText(child, 0, "1000000");
            child = mxmlNewElement(group, "bandwith");
            mxmlNewText(child, 0, "1000");
            child = mxmlNewElement(group, "freqResolution");
            mxmlNewText(child, 0, "20");
            child = mxmlNewElement(group, "fftSize");
            mxmlNewText(child, 0, "64");
            child = mxmlNewElement(group, "decMethodId");
            mxmlNewText(child, 0, "1");
            child = mxmlNewElement(group, "decBandwidth");
            mxmlNewText(child, 0, "16");
            child = mxmlNewElement(group, "muteSwitch");
            mxmlNewText(child, 0, "0");
            child = mxmlNewElement(group, "muteThreshold");
            mxmlNewText(child, 0, "50");

        }
        node = mxmlNewElement(channel, "radiofrequency");
        group = mxmlNewElement(node, "status");
        mxmlNewText(group, 0, "1");
        group = mxmlNewElement(node, "modeCode");
        mxmlNewText(group, 0, "0");
        group = mxmlNewElement(node, "gainMode");
        mxmlNewText(group, 0, "0");
        group = mxmlNewElement(node, "agcCtrlTime");
        mxmlNewText(group, 0, "10");
        group = mxmlNewElement(node, "agcOutPutAmp");
        mxmlNewText(group, 0, "1");
        group = mxmlNewElement(node, "midBw");
        mxmlNewText(group, 0, "100");
        group = mxmlNewElement(node, "rfAttenuation");
        mxmlNewText(group, 0, "10");
        group = mxmlNewElement(node, "temprature");
        mxmlNewText(group, 0, "50");
         
    }


    
    /*控制参数*/
    node = mxmlNewElement(controlPara, "ctrlMode");
    mxmlNewText(node, 0, "1");
    node = mxmlNewElement(controlPara, "calibration");
    mxmlNewText(node, 0, "1");
    node = mxmlNewElement(controlPara, "timeSet");
    mxmlNewText(node, 0, "1");
    dataOutPutEn = mxmlNewElement(controlPara, "dataOutPutEn");
    /*控制参数下dataOutPutEn的子节点*/
    node = mxmlNewElement(dataOutPutEn, "enable");
    mxmlNewText(node, 0, "1");
    node = mxmlNewElement(dataOutPutEn, "subChannel");
    mxmlNewText(node, 0, "-1");
    node = mxmlNewElement(dataOutPutEn, "psdEnable");
    mxmlNewText(node, 0, "0");
    node = mxmlNewElement(dataOutPutEn,  "audioEnable");
    mxmlNewText(node, 0, "1");
    node = mxmlNewElement(dataOutPutEn, "IQEnable");
    mxmlNewText(node, 0, "0");
    node = mxmlNewElement(dataOutPutEn, "spectrumAnalysisEn");
    mxmlNewText(node, 0, "0");
    node = mxmlNewElement(dataOutPutEn, "directionEn");
    mxmlNewText(node, 0, "0");
    /*状态参数的子节点*/ 
    //channelInfo = mxmlNewElement(statusPara, "channelInfo");
    softVersion = mxmlNewElement(statusPara, "softVersion");
    diskInfo = mxmlNewElement(statusPara, "diskInfo");
    clkInfo = mxmlNewElement(statusPara, "clkInfo");
    adInfo = mxmlNewElement(statusPara, "adInfo");
    //rfInfo = mxmlNewElement(statusPara, "rfInfo");
    fpgaInfo = mxmlNewElement(statusPara, "fpgaInfo");
    powerstate = mxmlNewElement(statusPara, "powerstate");
    /*状态参数下softVersion的子节点*/
    node = mxmlNewElement(softVersion, "app");
    mxmlNewText(node, 0, get_version_string());
    node = mxmlNewElement(softVersion, "kernel");
    mxmlNewText(node, 0, "v1.0-20190702-134310");
    node = mxmlNewElement(softVersion, "uboot");
    mxmlNewText(node, 0, "v1.0-20190702-134310");

    /*状态参数下diskInfo的子节点*/
    node = mxmlNewElement(diskInfo, "diskNum");
    mxmlNewText(node, 0, "1");
    node = mxmlNewElement(diskInfo, "diskNode");
    group = mxmlNewElement(node, "totalSpace");
    mxmlNewText(group, 0, "100000000");
    group = mxmlNewElement(node, "freeSpace");
    mxmlNewText(group, 0, "5665641");
    /*状态参数下clkInfo的子节点*/
    node = mxmlNewElement(clkInfo, "inout");
    mxmlNewText(node, 0, "1");
    /*状态参数下adInfo的子节点*/
    node = mxmlNewElement(adInfo, "status");
    mxmlNewText(node, 0, "1");
    /*状态参数下fpgaInfo的子节点*/
    node = mxmlNewElement(fpgaInfo, "temprature");
    mxmlNewText(node, 0, "67");
    /*状态参数下 powerstate的子节点*/
    node = mxmlNewElement(powerstate, "diskNum");
    mxmlNewText(node, 0, "1");
    node = mxmlNewElement(powerstate, "diskNode");
    group = mxmlNewElement(node, "totalSpace");
    mxmlNewText(group, 0, "100000000");
    group = mxmlNewElement(node, "freeSpace");
    mxmlNewText(group, 0, "5665641");

    /*状态参数下fileStorePara的子节点*/
    node = mxmlNewElement(fileStorePara, "iqDataStore");
    mxmlNewText(node, 0, "1");
    node = mxmlNewElement(fileStorePara, "iqDataBackTrace");
    mxmlNewText(node, 0, "1");
    node = mxmlNewElement(fileStorePara, "fileQuery");
    mxmlNewText(node, 0, "1");
    node = mxmlNewElement(fileStorePara, "fileDelete");
    mxmlNewText(node, 0, "1");
    node = mxmlNewElement(fileStorePara, "fileStrategy");
    mxmlNewText(node, 0, "1");

    printf_debug("4\n");


   
    printf_debug("%s\n", file);
    fp = fopen(file, "w");
    mxmlSaveFile(root, fp, NULL);
    fclose(fp);
    return (void *)root;
#elif defined SUPPORT_DAO_JSON


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
#ifdef SUPPORT_DAO_XML 

#elif defined SUPPORT_DAO_JSON 

#else
#error "NOT SUPPORT DAO FORMAT"
#endif
}

static  int8_t dao_read_float_data_array_value(void *root, const char *array_name, 
                                     const char *index_name, const char *index_value, 
                                     const char *element, float *result)
{
    char *pdata;
    float fdata;
#ifdef SUPPORT_DAO_XML
    pdata = read_config_file_array(root, array_name,index_name,index_value,element);
    if(pdata != NULL){
        fdata = atof(pdata);
        *result = fdata;
        //*result = atof(pdata); /* Bus error ??? */
        return 0;
    }
    return -1;
#endif
}

static inline int8_t dao_read_long_data_array_value(void *root, const char *array_name, 
                                     const char *index_name, const char *index_value, 
                                     const char *element, uint32_t *result)
{
    char *pdata;
#ifdef SUPPORT_DAO_XML 
    pdata = read_config_file_array(root, array_name,index_name,index_value,element);
    if(pdata != NULL){
        *result = atol(pdata);
        return 0;
    }
    return -1;
#endif
}

static inline int8_t dao_read_int_data_array_value(void *root, const char *array_name, 
                                     const char *index_name, const char *index_value, 
                                     const char *element,  int32_t *result)
{
    char *pdata;
#ifdef SUPPORT_DAO_XML 
        pdata = read_config_file_array(root, array_name,index_name,index_value,element);
        if(pdata != NULL){
            *result = atoi(pdata);
            return 0;
        }
        return -1;
#endif
}

static inline int8_t dao_read_int_data_single_value(const char *parent, const char *element,  int32_t *result)
{
 char *pdata;
#ifdef SUPPORT_DAO_XML 
     pdata = read_config_file_single(parent,element);
     if(pdata != NULL){
         *result = atoi(pdata);
         return 0;
     }
     return -1;
#endif
}

const char *
whitespace_cb(mxml_node_t *node, int where)
{
    if (where == MXML_WS_BEFORE_OPEN || where == MXML_WS_AFTER_CLOSE)
      return ("\n");
    else return NULL;
}

const char *				/* O - Whitespace string or NULL */
whitespace_cb_x(mxml_node_t *node,	/* I - Element node */
              int         where)	/* I - Open or close tag? */
{
  mxml_node_t	*parent;		/* Parent node */
  int		level;			/* Indentation level */
  const char	*name;			/* Name of element */
  static const char *tabs = "\t\t\t\t\t\t\t\t";
					/* Tabs for indentation */

   // printf_warn("whitespace_cb_x-------------------");
 /*
  * We can conditionally break to a new line before or after any element.
  * These are just common HTML elements...
  */

  name = node->value.element.name;
  printf_warn("name:%s\n", name);
  if (!strcmp(name, "html") || !strcmp(name, "head") || !strcmp(name, "body") ||
      !strcmp(name, "pre") || !strcmp(name, "p") ||
      !strcmp(name, "h1") || !strcmp(name, "h2") || !strcmp(name, "h3") ||
      !strcmp(name, "h4") || !strcmp(name, "h5") || !strcmp(name, "h6"))
  {
   /*
    * Newlines before open and after close...
    */

    if (where == MXML_WS_BEFORE_OPEN || where == MXML_WS_AFTER_CLOSE)
      return ("\n");
  }
  else if (!strcmp(name, "dl") || !strcmp(name, "ol") || !strcmp(name, "ul"))
  {
   /*
    * Put a newline before and after list elements...
    */

    return ("\n");
  }
  else if (!strcmp(name, "dd") || !strcmp(name, "dt") || !strcmp(name, "li"))
  {
   /*
    * Put a tab before <li>'s, <dd>'s, and <dt>'s, and a newline after them...
    */

    if (where == MXML_WS_BEFORE_OPEN)
      return ("\t");
    else if (where == MXML_WS_AFTER_CLOSE)
      return ("\n");
  }
  else if (!strncmp(name, "?xml", 4))
  {
    if (where == MXML_WS_AFTER_OPEN)
      return ("\n");
    else
      return (NULL);
  }
  else if (where == MXML_WS_BEFORE_OPEN ||
           ((!strcmp(name, "choice") || !strcmp(name, "option")) &&
	    where == MXML_WS_BEFORE_CLOSE))
  {
    for (level = -1, parent = node->parent;
         parent;
	 level ++, parent = parent->parent);

    if (level > 8)
      level = 8;
    else if (level < 0)
      level = 0;

    return (tabs + 8 - level);
  }
  else if (where == MXML_WS_AFTER_CLOSE ||
           ((!strcmp(name, "group") || !strcmp(name, "option") ||
	     !strcmp(name, "choice")) &&
            where == MXML_WS_AFTER_OPEN))
    return ("\n");
  else if (where == MXML_WS_AFTER_OPEN && !node->child)
    return ("\n");

 /*
  * Return NULL for no added whitespace...
  */

  return (NULL);
}


void dao_version_check(mxml_node_t *root, char *file)
{
    FILE *fp;
    mxml_node_t *node = mxmlFindElement(root, "softVersion", "app",NULL, NULL,MXML_DESCEND);
    if(node != NULL){
        printf_warn("--%s\n", mxmlGetText(node ,NULL));
        if(!strcmp(mxmlGetText(node ,NULL), get_version_string())){
            return;
        }
        mxmlSetText(node, 0, get_version_string());
        fp = fopen(file, "w");
        mxmlSaveFile(root, fp, whitespace_cb_x);
        fclose(fp);
    }
    

    
    //if(!strcmp(node_value, get_version_string())){
    //    return;
    //}
}


void read_calibration_file(mxml_node_t *root, void *config)
{
    s_config *sys_config;
    struct calibration_info_st *cali_config;
    char indexvalue[32] = {0};
    sys_config = (s_config*)config;
    
    cali_config = &sys_config->oal_config.cal_level; 

    mxml_node_t *node;
    char *node_value;
    node = mxmlFindElement(root, root,"calibration","psd_power", NULL,MXML_DESCEND);
    node_value = mxmlElementGetAttr(node,"psd_power");
    if(node_value != NULL){
        cali_config->specturm.global_roughly_power_lever = atoi(node_value);
    }else{
        cali_config->specturm.global_roughly_power_lever = 0;
    }
    printf_debug("specturm.global_roughly_power_lever=%d\n", cali_config->specturm.global_roughly_power_lever);

    node = mxmlFindElement(root, root,"calibration","analysis_power", NULL,MXML_DESCEND);
    node_value = mxmlElementGetAttr(node,"analysis_power");
    if(node_value != NULL){
        cali_config->analysis.global_roughly_power_lever = atoi(node_value);
    }else{
        cali_config->analysis.global_roughly_power_lever = 0;
    }
    printf_debug("analysis.global_roughly_power_lever=%d\n", cali_config->analysis.global_roughly_power_lever);

    /* low noise */
    node = mxmlFindElement(root, root,"calibration","low_noise_power", NULL,MXML_DESCEND);
    node_value = mxmlElementGetAttr(node,"low_noise_power");
    if(node_value != NULL){
        cali_config->low_noise.global_power_val = atoi(node_value);
    }else{
        cali_config->low_noise.global_power_val = 0;
    }
    printf_debug("low_noise.global_power_val=%d\n", cali_config->low_noise.global_power_val);
    /* read lo_leakage globe calibration value  */
    node = mxmlFindElement(root, root,"calibration","lo_leakage_threshold", NULL,MXML_DESCEND);
    node_value = mxmlElementGetAttr(node,"lo_leakage_threshold");
    if(node_value != NULL){
        cali_config->lo_leakage.global_threshold = atoi(node_value);
    }else{
        cali_config->lo_leakage.global_threshold = 0;
    }
    /* read lo_leakage renew_data_len globe calibration value  */
    node = mxmlFindElement(root, root,"calibration","lo_leakage_renew_data_len", NULL,MXML_DESCEND);
    node_value = mxmlElementGetAttr(node,"lo_leakage_renew_data_len");
    if(node_value != NULL){
        cali_config->lo_leakage.global_renew_data_len = atoi(node_value);
    }else{
        cali_config->lo_leakage.global_renew_data_len = 0;
    }
    
    /* read mgc_gain globe calibration value  */
    node = mxmlFindElement(root, root,"calibration","mgc_gain", NULL,MXML_DESCEND);
    node_value = mxmlElementGetAttr(node,"mgc_gain");
    if(node_value != NULL){
        cali_config->mgc.global_gain_val = atoi(node_value);
    }else{
        cali_config->mgc.global_gain_val = 0;
    }
    
    printf_debug("sizeof = %d\n", sizeof(cali_config->specturm.start_freq_khz)/sizeof(uint32_t));
    /* read psd  calibration value in different frequency */
    for(int i = 0; i<sizeof(cali_config->specturm.start_freq_khz)/sizeof(uint32_t); i++){
        sprintf(indexvalue, "%d", i);
        

        if(dao_read_long_data_array_value(root, "psd_power", "index", indexvalue, 
                                    "start_requency",  &cali_config->specturm.start_freq_khz[i]) == -1)
           break;
        if(dao_read_long_data_array_value(root, "psd_power", "index", indexvalue, 
                                    "end_requency",  &cali_config->specturm.end_freq_khz[i]) == -1)
           break;
        if(dao_read_int_data_array_value(root, "psd_power", "index", indexvalue, 
                                    "level",  &cali_config->specturm.power_level[i]) == -1)
           break;

        printf_debug("indexvalue=%s\n", indexvalue);

        printf_debug("read calbration start_freq[%d] = %uKHz\n",i,cali_config->specturm.start_freq_khz[i]);
        printf_debug("read calbration end_freq[%d] = %uKHz\n",i,cali_config->specturm.end_freq_khz[i]);
        printf_debug("read calbration power_level[%d] = %d\n",i,cali_config->specturm.power_level[i]);
    }
    
     /* read analysis calibration value in different frequency */
    for(int i = 0; i<sizeof(cali_config->analysis.start_freq_khz)/sizeof(uint32_t); i++){
        sprintf(indexvalue, "%d", i);

        if(dao_read_long_data_array_value(root, "analysis_power", "index", indexvalue, 
                                    "start_requency",  &cali_config->analysis.start_freq_khz[i]) == -1)
           break;
        if(dao_read_long_data_array_value(root, "analysis_power", "index", indexvalue, 
                                    "end_requency",  &cali_config->analysis.end_freq_khz[i]) == -1)
           break;
        if(dao_read_int_data_array_value(root, "analysis_power", "index", indexvalue, 
                                    "level",  &cali_config->analysis.power_level[i]) == -1)
           break;

        printf_debug("analysis_power read calbration start_freq[%d] = %uKHz\n",i,cali_config->analysis.start_freq_khz[i]);
        printf_debug("analysis_power read calbration end_freq[%d] = %uKHz\n",i,cali_config->analysis.end_freq_khz[i]);
        printf_debug("analysis_power read calbration power_level[%d] = %d\n",i,cali_config->analysis.power_level[i]);
    }
    /* read lo_leakage calibration value */
   
    for(int i = 0; i<sizeof(cali_config->lo_leakage.fft_size)/sizeof(uint32_t); i++){
         sprintf(indexvalue, "%d", i);
         if(dao_read_int_data_array_value(root, "lo_leakage", "index", indexvalue, 
                                         "fft_size",  &cali_config->lo_leakage.fft_size[i]) == -1)
                break;
         if(dao_read_int_data_array_value(root, "lo_leakage", "index", indexvalue, 
                                        "threshold",  &cali_config->lo_leakage.threshold[i]) == -1)
                break;
         if(dao_read_int_data_array_value(root, "lo_leakage", "index", indexvalue, 
                                        "renew_data_len",  &cali_config->lo_leakage.renew_data_len[i]) == -1)
                break;
        printf_debug("lo_leakage.fft_size[%d] = %u\n",i, cali_config->lo_leakage.fft_size[i]);
        printf_debug("lo_leakage.threshold[%d] = %d\n",i, cali_config->lo_leakage.threshold[i]);
        printf_debug("lo_leakage.renew_data_len[%d] = %d\n",i, cali_config->lo_leakage.renew_data_len[i]);
    }
    
    /* read mgc calibration value */
    for(int i = 0; i<sizeof(cali_config->mgc.start_freq_khz)/sizeof(uint32_t); i++){
        sprintf(indexvalue, "%d", i);
        if(dao_read_long_data_array_value(root, "mgc", "index", indexvalue, 
                                         "start_requency",  &cali_config->mgc.start_freq_khz[i]) == -1)
                break;
        if(dao_read_long_data_array_value(root, "mgc", "index", indexvalue, 
                                        "end_requency",  &cali_config->mgc.end_freq_khz[i]) == -1)
                break;
        if(dao_read_int_data_array_value(root, "mgc", "index", indexvalue, 
                                        "gain",  &cali_config->mgc.gain_val[i]) == -1)
                break;
        printf_debug("mgc.start_freq[%d] = %uKHz\n",i, cali_config->mgc.start_freq_khz[i]);
        printf_debug("mgc.end_freq[%d] = %uKHz\n",i, cali_config->mgc.end_freq_khz[i]);
        printf_debug("mgc.gain_val[%d] = %d\n",i, cali_config->mgc.gain_val[i]);
    }
}

void read_scan_param_info(mxml_node_t *root, void *config)
{
    s_config *sys_config;
    struct scan_bindwidth_info *scanbw;
    mxml_node_t *parent_node;
    char indexvalue[32] = {0};
    sys_config = (s_config*)config;
    
    scanbw = &sys_config->oal_config.ctrl_para.scan_bw; 

    mxml_node_t *node;
    char *node_value;

    /* read scan bindwidth value */
    scanbw->work_fixed_bindwidth_flag = false;
    for(int i = 0; i<sizeof(scanbw->bindwidth_hz)/sizeof(uint32_t); i++){
        sprintf(indexvalue, "%d", i);
        if(dao_read_long_data_array_value(root, "scanBindWidthInfo", "index", indexvalue, 
                                         "bindWidth",  &scanbw->bindwidth_hz[i]) == -1)
                break;
        if(dao_read_float_data_array_value(root, "scanBindWidthInfo", "index", indexvalue, 
                                 "sideBandRate",  &scanbw->sideband_rate[i]) == -1)
                break;
        
        printf_debug("Scan bindWidth[%d] = %d, sideband rate[%d]=%f\n",i, scanbw->bindwidth_hz[i], i,scanbw->sideband_rate[i]);


        parent_node = mxmlFindElement(root, root, "scanBindWidthInfo", "index",indexvalue, MXML_DESCEND);
        node = mxmlFindElement(root, parent_node,"scanBindWidthInfo","fixed", NULL,MXML_DESCEND);
        node_value = mxmlElementGetAttr(node,"fixed");
        if(node_value != NULL){
            scanbw->fixed_bindwidth_flag[i] =  (atoi(node_value)==0 ? false:true);
        }else{
            scanbw->fixed_bindwidth_flag[i] = false;
        }
        printf_debug("scanbw->fixed_bindwidth_flag[%d]=%d\n", i, scanbw->fixed_bindwidth_flag[i]);
        if(scanbw->fixed_bindwidth_flag[i] == true){
           scanbw->work_fixed_bindwidth_flag = true;
           scanbw->work_sideband_rate = scanbw->sideband_rate[i];
           scanbw->work_bindwidth_hz = scanbw->bindwidth_hz[i];
           printf_warn("work fixed bindwidth flag=%d, work bindwidth_hz=%u,work sideband rate=%f\n",scanbw->work_fixed_bindwidth_flag, scanbw->work_bindwidth_hz, scanbw->work_sideband_rate);
            break;
        }else{
            scanbw->work_sideband_rate = scanbw->sideband_rate[i]; /* the last value ; work_bindwidth_hz is set by remote*/
        }
    }
    printf_debug("scanbw->fixed_bindwidth_flag=%d,scanbw->work_bindwidth_hz=%u\n", scanbw->work_fixed_bindwidth_flag, scanbw->work_bindwidth_hz);
}

void read_config(void *root_config)
{
    printf_debug("load config file\n");

    s_config *fre_config;

    mxml_node_t *root;
    root = whole_root;

    fre_config = (s_config*)root_config;

     if(root == NULL)  
    {
        printf_debug("读取失败\n");
        return NULL;
    }

     struct sockaddr_in saddr;
     
     saddr.sin_addr.s_addr=inet_addr(read_config_file_single("network","gateway"));
     fre_config->oal_config.network.gateway = saddr.sin_addr.s_addr;
     printf_debug("读取............................gateway = %x\n",fre_config->oal_config.network.gateway);
     
     
     saddr.sin_addr.s_addr=inet_addr(read_config_file_single("network","netmask"));
     fre_config->oal_config.network.netmask = saddr.sin_addr.s_addr;
     printf_debug("读取............................netmask = %x\n",fre_config->oal_config.network.netmask);
     
     
     saddr.sin_addr.s_addr=inet_addr(read_config_file_single("network","ipaddress"));
     fre_config->oal_config.network.ipaddress = saddr.sin_addr.s_addr;
     printf_debug("读取............................ipaddress = %x\n",fre_config->oal_config.network.ipaddress);

        
    if(dao_read_int_data_single_value("network","port", &fre_config->oal_config.network.port)!=-1){
         printf_debug("读取............................port = %d\n",fre_config->oal_config.network.port);
    }


    if(dao_read_int_data_single_value("dataOutPutEn","enable", &fre_config->oal_config.enable.bit_en)!=-1){
         printf_debug("读取............................enable = %d\n",fre_config->oal_config.enable.bit_en);
    }


    if(dao_read_int_data_single_value("dataOutPutEn","subChannel", &fre_config->oal_config.enable.sub_id)!=-1){
         printf_debug("读取............................subChannel = %d\n",fre_config->oal_config.enable.sub_id);
    }


    if(dao_read_int_data_single_value("dataOutPutEn","psdEnable", &fre_config->oal_config.enable.psd_en)!=-1){
         printf_debug("读取............................psdEnable = %d\n",fre_config->oal_config.enable.psd_en);
    }

    
    if(dao_read_int_data_single_value("dataOutPutEn","audioEnable", &fre_config->oal_config.enable.audio_en)!=-1){
         printf_debug("读取............................audioEnable = %d\n",fre_config->oal_config.enable.audio_en);
    }


    if(dao_read_int_data_single_value("dataOutPutEn","IQEnable", &fre_config->oal_config.enable.iq_en)!=-1){
         printf_debug("读取............................IQEnable = %d\n",fre_config->oal_config.enable.iq_en);
    }


    if(dao_read_int_data_single_value("dataOutPutEn","spectrumAnalysisEn", &fre_config->oal_config.enable.spec_analy_en)!=-1){
         printf_debug("读取............................spectrumAnalysisEn = %d\n",fre_config->oal_config.enable.spec_analy_en);
    }


    if(dao_read_int_data_single_value("dataOutPutEn","directionEn", &fre_config->oal_config.enable.direction_en)!=-1){
         printf_debug("读取............................directionEn = %d\n",fre_config->oal_config.enable.direction_en);
    }   

    
    if(dao_read_int_data_single_value("controlPara","spectrum_time_interval", &fre_config->oal_config.ctrl_para.spectrum_time_interval)!=-1){
         printf_debug("spectrum_timer_interval = %u\n",fre_config->oal_config.ctrl_para.spectrum_time_interval);
    }

    if(dao_read_int_data_array_value(root, "channel", "index", "0", 
                                "cid",&fre_config->oal_config.multi_freq_point_param[0].cid)!=-1){
        printf_debug("读取............................cid = %d\n",fre_config->oal_config.multi_freq_point_param[0].cid);
       };


    if(dao_read_int_data_array_value(root, "freqPoint", "index", "0", 
                                "centerFreq",&fre_config->oal_config.multi_freq_point_param[0].points[0].center_freq)!=-1){
        printf_debug("读取............................centerFreq = %llu\n",fre_config->oal_config.multi_freq_point_param[0].points[0].center_freq);
       };


    if(dao_read_int_data_array_value(root, "freqPoint", "index", "0", 
                                "bandwith",&fre_config->oal_config.multi_freq_point_param[0].points[0].bandwidth) !=-1){
        printf_debug("读取............................bandwith = %llu\n",fre_config->oal_config.multi_freq_point_param[0].points[0].bandwidth);
       };

    if(dao_read_float_data_array_value(root, "freqPoint", "index", "0", 
                                "freqResolution",&(fre_config->oal_config.multi_freq_point_param[0].points[0].freq_resolution)) !=-1){
        printf_debug("读取............................freqResolution = %f\n",fre_config->oal_config.multi_freq_point_param[0].points[0].freq_resolution);
    };
    if(dao_read_int_data_array_value(root, "freqPoint", "index", "0", 
                                "fftSize",&fre_config->oal_config.multi_freq_point_param[0].points[0].fft_size) !=-1){
        printf_debug("读取............................fftSize = %u\n",fre_config->oal_config.multi_freq_point_param[0].points[0].fft_size);
       };

    if(dao_read_int_data_array_value(root, "freqPoint", "index", "0", 
                                "decMethodId",&fre_config->oal_config.multi_freq_point_param[0].points[0].d_method) !=-1){
        printf_debug("读取............................decMethodId = %d\n",fre_config->oal_config.multi_freq_point_param[0].points[0].d_method);
       };


    if(dao_read_int_data_array_value(root, "freqPoint", "index", "0", 
                                "decBandwidth",&fre_config->oal_config.multi_freq_point_param[0].points[0].d_bandwith) !=-1){
        printf_debug("读取............................decBandwidth = %u\n",fre_config->oal_config.multi_freq_point_param[0].points[0].d_bandwith);
       };


    if(dao_read_int_data_array_value(root, "freqPoint", "index", "0", 
                                "muteSwitch",&fre_config->oal_config.multi_freq_point_param[0].points[0].noise_en) !=-1){
        printf_debug("读取............................muteSwitch = %d\n",fre_config->oal_config.multi_freq_point_param[0].points[0].noise_en);
       };



    if(dao_read_int_data_array_value(root, "freqPoint", "index", "0", 
                                "muteThreshold",&fre_config->oal_config.multi_freq_point_param[0].points[0].noise_thrh) !=-1){
        printf_debug("读取............................muteThreshold = %d\n",fre_config->oal_config.multi_freq_point_param[0].points[0].noise_thrh);
       };


    
    if(dao_read_int_data_single_value("radiofrequency","modeCode", &fre_config->oal_config.rf_para->rf_mode_code)!=-1){
         printf_debug("读取............................modeCode = %d\n",fre_config->oal_config.rf_para->rf_mode_code);
    }


    if(dao_read_int_data_single_value("radiofrequency","gainMode", &fre_config->oal_config.rf_para->gain_ctrl_method)!=-1){
         printf_debug("读取............................gainMode = %d\n",fre_config->oal_config.rf_para->gain_ctrl_method);
    }
 

    if(dao_read_int_data_single_value("radiofrequency","agcCtrlTime", &fre_config->oal_config.rf_para->agc_ctrl_time)!=-1){
         printf_debug("读取............................agcCtrlTime = %u\n",fre_config->oal_config.rf_para->agc_ctrl_time);
    }


    if(dao_read_int_data_single_value("radiofrequency","agcOutPutAmp", &fre_config->oal_config.rf_para->agc_mid_freq_out_level)!=-1){
         printf_debug("读取............................agcOutPutAmp = %d\n",fre_config->oal_config.rf_para->agc_mid_freq_out_level);
    }


    if(dao_read_int_data_single_value("radiofrequency","midBw", &fre_config->oal_config.rf_para->mid_bw)!=-1){
         printf_debug("读取............................midBw = %u\n",fre_config->oal_config.rf_para->mid_bw);
    }


    if(dao_read_int_data_single_value("radiofrequency","rfAttenuation", &fre_config->oal_config.rf_para->attenuation)!=-1){
         printf_debug("读取............................rfAttenuation = %d\n",fre_config->oal_config.rf_para->attenuation);
    }

    read_calibration_file(root, root_config);
    read_scan_param_info(root, root_config);
}

void dao_read_create_config_file(char *file, void *root_config)
{
    void *root;
    FILE *fp;
    /* read/write or create file */
    fp = fopen(file, "r");
    if(fp != NULL){
        whole_root = mxmlLoadFile(NULL, fp, MXML_TEXT_CALLBACK);//MXML_TEXT_CALLBACK);
        //dao_version_check(whole_root, file);
        read_config(root_config);
        fclose(fp);
    }else{
        whole_root = mxmlNewXML("1.0");	
        printf_debug("create new config file\n");
        root = dao_load_default_config_file(file);
        read_config(root_config);
    
    }
    root_config = dao_load_root(root);
}

void dao_read_calibration_file(char *file, void *config)
{
    void *root;
    FILE *fp;
    /* read/write or create file */
    fp = fopen(file, "r");
    if(fp != NULL){
        calibration_root = mxmlLoadFile(NULL, fp, MXML_TEXT_CALLBACK);
        read_calibration_file(calibration_root, config);
        fclose(fp);
    }else{
        printf_err("The calibration file[%s] is not valid!!\n", file);
    }
}


