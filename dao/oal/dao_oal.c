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





mxml_node_t *whole_root;
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
#if DAO_XML == 1

	FILE *fp;
	mxml_node_t *parent_node,*node;
	printf_debug("create singular xml config file\n");



	parent_node = mxmlFindElement(whole_root, whole_root, parent_element, NULL, NULL,MXML_DESCEND);
	if(parent_node == NULL)  parent_node = mxmlNewElement(whole_root, parent_element);

	node = mxmlFindElement(parent_node, whole_root, element, NULL, NULL,MXML_DESCEND);

	if(node != NULL)  mxmlDelete(node); //找到该元素，删除修改

	node =mxmlNewElement(parent_node, element);

	if(string != NULL) mxmlNewText(node, 0, string);//写字符串

	else   mxmlNewInteger(node, val); //写整型数组

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

void *write_config_file_array(char *file,const char *array,
const char *name,const char *value,const char *element,const char *seed_element,
const char *seed_array,const char *seed_name,const char *seed_value,
int val,char series)
{
#if DAO_XML == 1

	FILE *fp;
	mxml_node_t *parent_node,*node,*seed_node,*s_node;
	printf_debug("create singular xml config file\n");


	parent_node = mxmlFindElement(whole_root, whole_root, array, name, value,MXML_DESCEND);

	if(parent_node == NULL){  
	parent_node = mxmlNewElement(whole_root, array);
	mxmlElementSetAttr(parent_node,name,value);

	}

	node = mxmlFindElement(parent_node, whole_root, element,NULL, NULL,MXML_DESCEND);


	if(series == ARRAY)
	{
		if(node != NULL)  mxmlDelete(node);  //找到该元素，删除修改
		node =mxmlNewElement(parent_node, element);

		mxmlNewInteger(node, val);
	}
	else if(series == ARRAY_PARENT)  
	{
		if(node == NULL)  node = mxmlNewElement(parent_node, element);

		seed_node = mxmlFindElement(node, parent_node, seed_element,NULL, NULL,MXML_DESCEND);

		if(seed_node != NULL)  mxmlDelete(seed_node);  //找到该元素，删除修改

		seed_node =mxmlNewElement(node, seed_element);

		mxmlNewInteger(seed_node, val);
	}
	else if(series == ARRAY_ARRAY)  
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
#if DAO_XML == 1
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


void *write_struc_config_file_array(spectrum         spectrum_fre)
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


const char *read_config_file_single(const char *parent_element,const char *element)
{ 
#if DAO_XML == 1
    mxml_node_t *parent_node,*node;
    //printf_debug("load xml config file\n");


    parent_node= mxmlFindElement(whole_root, whole_root, parent_element, NULL, NULL, MXML_DESCEND);

    node= mxmlFindElement(parent_node, whole_root, element, NULL, NULL, MXML_DESCEND);

    const char *string = mxmlGetText(node, NULL);

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

const char *read_config_file_array(const char *array,const char *name,const char *value,const char *element)
{
#if DAO_XML == 1
    mxml_node_t *parent_node,*node;
    //printf_debug("load xml config file\n");


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
#if DAO_XML == 1
    FILE *fp;
    mxml_node_t *xml;

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
    xml = mxmlNewXML("1.0");
    /*创建版本信息*/
    root = mxmlNewElement(xml, "root");
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
    mxmlNewText(node, 0, "192.168.2.1");
    node = mxmlNewElement(network, "netmask");
    mxmlNewText(node, 0, "255.255.255.0");
    node = mxmlNewElement(network, "ipaddress");
    mxmlNewText(node, 0, "192.168.2.112");
    node = mxmlNewElement(network, "port");
    mxmlNewText(node, 0, "1234");

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
    mxmlNewText(node, 0, "v1.0-20190702-134310");
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
    whole_root = mxmlLoadFile(NULL, fp, MXML_TEXT_CALLBACK);
    
    printf_debug("load config file\n");

    s_config *fre_config;
        
    fre_config = (s_config*)root_config;

    //memcpy(fre_config->oal_config.network.mac,read_config_file_single("network","port"),6);

    fre_config->oal_config.network.port = atoi(read_config_file_single("network","port")) ;
    printf_debug("读取............................port = %d\n",fre_config->oal_config.network.port);

    fre_config->oal_config.multi_freq_point_param[0].cid  = atoi(read_config_file_array("channel","index","0","cid")) ;
    printf_debug("读取............................cid = %d\n",fre_config->oal_config.multi_freq_point_param[0].cid);

    fre_config->oal_config.multi_freq_point_param[0].points[0].center_freq  = atoi(read_config_file_array("freqPoint","index","0","centerFreq"));
    printf_debug("读取............................centerFreq = %d\n",fre_config->oal_config.multi_freq_point_param[0].points[0].center_freq);

    fre_config->oal_config.multi_freq_point_param[0].points[0].bandwidth  = atoi(read_config_file_array("freqPoint","index","0","bandwith"));
    printf_debug("读取............................bandwith = %d\n",fre_config->oal_config.multi_freq_point_param[0].points[0].bandwidth);

    fre_config->oal_config.multi_freq_point_param[0].points[0].freq_resolution  = atof(read_config_file_array("freqPoint","index","0","freqResolution"));
    printf_debug("读取............................freqResolution = %f\n",fre_config->oal_config.multi_freq_point_param[0].points[0].freq_resolution);

    fre_config->oal_config.multi_freq_point_param[0].points[0].fft_size  = atoi(read_config_file_array("freqPoint","index","0","fftSize"));
    printf_debug("读取............................fftSize = %d\n",fre_config->oal_config.multi_freq_point_param[0].points[0].fft_size);

    fre_config->oal_config.multi_freq_point_param[0].points[0].d_method  = atoi(read_config_file_array("freqPoint","index","0","decMethodId"));
    printf_debug("读取............................decMethodId = %d\n",fre_config->oal_config.multi_freq_point_param[0].points[0].d_method);

    fre_config->oal_config.multi_freq_point_param[0].points[0].d_bandwith  = atoi(read_config_file_array("freqPoint","index","0","decBandwidth"));
    printf_debug("读取............................decBandwidth = %d\n",fre_config->oal_config.multi_freq_point_param[0].points[0].d_bandwith);

    fre_config->oal_config.multi_freq_point_param[0].points[0].noise_en  = atoi(read_config_file_array("freqPoint","index","0","muteSwitch"));
    printf_debug("读取............................muteSwitch = %d\n",fre_config->oal_config.multi_freq_point_param[0].points[0].noise_en);

    fre_config->oal_config.multi_freq_point_param[0].points[0].noise_thrh  = atoi(read_config_file_array("freqPoint","index","0","muteThreshold"));
    printf_debug("读取............................muteThreshold = %d\n",fre_config->oal_config.multi_freq_point_param[0].points[0].noise_thrh);


    fre_config->oal_config.rf_para->rf_mode_code = atoi(read_config_file_single("radiofrequency","modeCode")) ;
    printf_debug("读取............................modeCode = %d\n",fre_config->oal_config.rf_para->rf_mode_code);

    fre_config->oal_config.rf_para->gain_ctrl_method = atoi(read_config_file_single("radiofrequency","gainMode")) ;
    printf_debug("读取............................gainMode = %d\n",fre_config->oal_config.rf_para->gain_ctrl_method);

    fre_config->oal_config.rf_para->agc_ctrl_time = atoi(read_config_file_single("radiofrequency","agcCtrlTime")) ;
    printf_debug("读取............................agcCtrlTime = %d\n",fre_config->oal_config.rf_para->agc_ctrl_time);

    fre_config->oal_config.rf_para->agc_mid_freq_out_level = atoi(read_config_file_single("radiofrequency","agcOutPutAmp")) ;
    printf_debug("读取............................agcOutPutAmp = %d\n",fre_config->oal_config.rf_para->agc_mid_freq_out_level);

    fre_config->oal_config.rf_para->mid_bw = atoi(read_config_file_single("radiofrequency","midBw")) ;
    printf_debug("读取............................midBw = %d\n",fre_config->oal_config.rf_para->mid_bw);

    fre_config->oal_config.rf_para->attenuation = atoi(read_config_file_single("radiofrequency","rfAttenuation")) ;
    printf_debug("读取............................rfAttenuation = %d\n",fre_config->oal_config.rf_para->attenuation);



    

    

    fclose(fp);

    }else{

    //whole_root = mxmlNewXML("1.0");	
    printf_debug("create new config file\n");
    root = dao_load_default_config_file(file);
    }

    printf_debug("1\n");
    root_config = dao_load_root(root);
    printf_debug("1\n");
}


