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
*  Rev 1.0   09 July 2019   yaochangsong
*  Initial revision.
******************************************************************************/
#include "config.h"
#include "spm/io_fpga.h"
#include "spm/spm.h"


static int io_ctrl_fd = -1;

/* 窄带表：解调方式为IQ系数表 */
static struct  band_table_t iq_nbandtable[] ={
    {204608, 0, 5000},
    {198208, 0, 25000},
    {197408, 0, 50000},
    {197008, 0, 100000},
    {196768, 0, 250000},
    {196688, 0, 500000},
    {196648, 0, 1000000},
    {196624, 0, 2500000},
    {196616, 0, 5000000},
    {196612, 0, 10000000},
    {458752, 0, 20000000},
    {196608, 0, 40000000},
}; 

/* 窄带表：解调方式为非IQ系数表 */
static struct  band_table_t nbandtable[] ={
    {198208, 130,   5000},
    {197408, 128,   25000},
    {197008, 128,   50000},
    {196808, 0,     100000},
    {196708, 0,     250000},
    {196658, 0,     500000},
}; 
/* 宽带系数表 */
static struct  band_table_t bandtable[] ={
#if 0//defined(SUPPORT_PROJECT_WD_XCR)
    {458752, 0, 25000000},
    {196608, 0, 50000000},
    {65536,  0, 100000000},
    {0,      0, 200000000},
#else
    {458752, 0,  20000000},
    {458752, 0,  25000000},
    {196608, 0,  40000000},
    {196608, 0,  50000000},
    {65536,  0,  80000000},
    {65536,  0,  100000000},
    {0,      0,  160000000},
    {0,      0,  175000000},
    {0,      0,  200000000},
#endif
}; 

static void  io_get_bandwidth_factor(uint32_t anays_band,               /* 输入参数：带宽 */
                                            uint32_t *bw_factor,        /* 输出参数：带宽系数 */
                                            uint32_t *filter_factor,    /* 输出参数：滤波器系数 */
                                            struct  band_table_t *table,/* 输入参数：系数表 */
                                            uint32_t table_len          /* 输入参数：系数表长度 */
                                            )
{
    int found = 0;
    uint32_t i;
    if(anays_band == 0 || table_len == 0){
        *bw_factor = table[0].extract_factor;
        *filter_factor = table[0].filter_factor;
        printf_info("band=0, set default: extract=%d, extract_filter=%d\n", *bw_factor, *filter_factor);
    }
    for(i = 0; i<table_len; i++){
        if(table[i].band == anays_band){
            *bw_factor = table[i].extract_factor;
            *filter_factor = table[i].filter_factor;
            found = 1;
            break;
        }
    }
    if(found == 0){
        *bw_factor = table[i-1].extract_factor;
        *filter_factor = table[i-1].filter_factor;
        printf_info("[%u]not find band table, set default:[%uHz] extract=%d, extract_filter=%d\n", anays_band, table[i-1].band, *bw_factor, *filter_factor);
    }
}


int io_set_udp_client_info(void *arg)
{
    int32_t ret = 0;
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    ret = ioctl(io_ctrl_fd,IOCTL_UDP_CLIENT_INFO_NOTIFY,arg);
#endif
#endif /* SUPPORT_PLATFORM_ARCH_ARM */
    return ret;
}

int set_1g_network_ipaddress(char *ipaddr, char *mask,char *gateway)
{
    return set_ipaddress(NETWORK_EHTHERNET_POINT, ipaddr, mask,gateway);
}

#ifdef SUPPORT_NET_WZ
int set_10g_network_ipaddress(char *ipaddr, char *mask,char *gateway)
{
    return set_ipaddress(NETWORK_10G_EHTHERNET_POINT, ipaddr, mask,gateway);
}

int32_t io_set_local_10g_net(uint32_t ip, uint32_t nw,uint32_t gw,uint16_t port)
{
    struct net_10g_local_param_t{
        uint32_t local_ip_10g;
        uint16_t local_port_10g;
    };
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    int32_t ret = 0;
    if(io_ctrl_fd<=0){
        return 0;
    }
    
    struct net_10g_local_param_t nl;
    /*万兆设备ip网段在千兆口网段基础上加1,端口和千兆口端口一样*/
    nl.local_ip_10g = ip;
    nl.local_port_10g = port;
    printf_note("ipaddress[0x%x], port=%d\n", nl.local_ip_10g, nl.local_port_10g);
    ret = ioctl(io_ctrl_fd,IOCTL_NET_10G_LOCAL_SET,&nl);
#else
    struct in_addr ipaddr, netmask, gateway;
    char s_ipaddr[16], s_netmask[16], s_gateway[16];
    ipaddr.s_addr = ip;

    port = port;
    
    memcpy(&s_ipaddr, inet_ntoa(ipaddr), sizeof(s_ipaddr));
    printf_note("set 10g ipaddress[%s], %s\n", inet_ntoa(ipaddr), s_ipaddr);

    netmask.s_addr = nw;
    memcpy(&s_netmask, inet_ntoa(netmask), sizeof(s_netmask));
    printf_note("set 10g netmask[%s], %s\n", inet_ntoa(netmask), s_netmask);

    gateway.s_addr = gw;
    memcpy(&s_gateway, inet_ntoa(gateway), sizeof(s_gateway));
    printf_note("set 10g gateway[%s], %s\n", inet_ntoa(gateway), s_gateway);

    set_10g_network_ipaddress(s_ipaddr, s_netmask, s_gateway);
#endif
    return 0;
}

/* 万兆开关 */
int32_t io_set_10ge_net_onoff(uint8_t onoff)
{
    int32_t ret = 0;
    printf_note("[**NET**]10GE net %s\n", onoff == 0 ? "off":"on");
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    ret = ioctl(io_ctrl_fd,IOCTL_NET_10G_ONOFF_SET,&onoff);
#endif
    return ret;
}

#endif /* end SUPPORT_NET_WZ */

int32_t io_set_1ge_net_onoff(uint8_t onoff)
{
    int32_t ret = 0;
    printf_note("[**NET**]1GE net %s\n", onoff == 0 ? "off":"on");
#if defined(SUPPORT_SPECTRUM_KERNEL)
    ret = ioctl(io_ctrl_fd,IOCTL_NET_1G_IQ_ONOFF_SET,&onoff);
#endif
    return ret;
}



void io_reset_fpga_data_link(void){
    #define RESET_ADDR      0x04U
    int32_t ret = 0;
    int32_t data = 0;

    printf_debug("[**REGISTER**]Reset FPGA\n");
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    if(io_ctrl_fd<=0){
        return ;
    }
    data = (RESET_ADDR &0xffff)<<16;
    ret = ioctl(io_ctrl_fd,IOCTL_SET_DDC_REGISTER_VALUE,&data);
#elif defined(SUPPORT_SPECTRUM_V2) 
//    get_fpga_reg()->system->data_path_reset = 1;
#endif
#endif /* SUPPORT_PLATFORM_ARCH_ARM */
}

/*设置带宽因子*/
int32_t io_set_bandwidth(uint32_t ch, uint32_t bandwidth){
    int32_t ret = 0;
    uint32_t set_factor,band_factor, filter_factor;
    static int32_t old_ch=-1;
    static uint32_t old_val=0;
    struct  band_table_t *table;
    uint32_t table_len = 0;

    table= &bandtable;
    table_len = sizeof(bandtable)/sizeof(struct  band_table_t);
    io_get_bandwidth_factor(bandwidth, &band_factor,&filter_factor, table, table_len);
    if((old_val == band_factor) && (ch == old_ch)){
        /* 避免重复设置相同参数 */
        return ret;
    }
    old_val = band_factor;
    old_ch = ch;
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    set_factor = band_factor|0x1000000;
    ret = ioctl(io_ctrl_fd, IOCTL_EXTRACT_CH0, set_factor);
#elif defined(SUPPORT_SPECTRUM_V2) 
    get_fpga_reg()->broad_band->band = band_factor;
#endif
#endif /* SUPPORT_PLATFORM_ARCH_ARM */
    printf_note("[**REGISTER**]ch:%d, Set Bandwidth:%u,band_factor=0x%x,set_factor=0x%x\n", ch, bandwidth,band_factor,set_factor);

    return ret;

}

/*根据边带率,设置数据长度 
    @rate: 边带率>0;实际下发边带率为整数；放大100倍（内核不处理浮点数）
*/
int32_t io_set_side_rate(uint32_t ch, float *rate){
    #define RATE_GAIN 1000
    int32_t ret = 0;
    static uint32_t old_val=0;
    uint32_t irate = 0;
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    irate = (uint32_t)(*(float *)rate * (float)RATE_GAIN);
    if(irate == old_val){
        /* 避免重复设置相同参数 */
        return ret;
    }
    old_val = irate;
    printf_note("[**REGISTER**]ch:%d, side rate%u\n", ch, irate);
    ret = ioctl(io_ctrl_fd, IOCTL_BAND_SIDE_RATE, irate);
#endif
#endif
    return ret;

}


/*设置解调带宽因子*/
int32_t io_set_dec_bandwidth(uint32_t ch, uint32_t dec_bandwidth){
    int32_t ret = 0;
    uint32_t set_factor, band_factor, filter_factor;
    static int32_t old_ch=-1;
    static uint32_t old_val=0;
    struct  band_table_t *table;
    uint32_t table_len = 0;

    table= &nbandtable;
    table_len = sizeof(nbandtable)/sizeof(struct  band_table_t);
    io_get_bandwidth_factor(dec_bandwidth, &band_factor,&filter_factor, table, table_len);
    set_factor = band_factor|0x2000000;
    if((old_val == set_factor) && (ch == old_ch)){
        /* 避免重复设置相同参数 */
        return ret;
    }
    old_val = set_factor;
    old_ch = ch;
    printf_note("[**REGISTER**]ch:%d, Set Decode Bandwidth:%u, band_factor=0x%x, set_factor=0x%x\n", ch, dec_bandwidth, band_factor, set_factor);
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    ret = ioctl(io_ctrl_fd, IOCTL_EXTRACT_CH0, set_factor);
#elif defined(SUPPORT_SPECTRUM_V2) 
    printf_warn("NOT DEFINE...\n");
#endif
#endif
    return ret;

}


/*设置解调方式*/
int32_t io_set_dec_method(uint32_t ch, uint8_t dec_method){
    int32_t ret = 0;
    uint32_t d_method = 0;
    static int32_t old_ch=-1;
    static uint32_t old_val=0;
   

    if(dec_method == IO_DQ_MODE_AM){
        d_method = 0x4000000;
    }else if(dec_method == IO_DQ_MODE_FM) {
        d_method = 0x4000001;
    }else if(dec_method == IO_DQ_MODE_LSB) {
        d_method = 0x4000002;
    }else if(dec_method == IO_DQ_MODE_USB) {
        d_method = 0x4000002;
    }else if(dec_method == IO_DQ_MODE_CW) {
        d_method = 0x4000003;
    }else if(dec_method == IO_DQ_MODE_IQ) {
        d_method = 0x4000007;
    }else{
        printf_warn("decode method not support:%d\n",dec_method);
        return -1;
    }
     if((old_val == d_method) && (ch == old_ch)){
        /* 避免重复设置相同参数 */
        return ret;
    }
    old_val = d_method;
    old_ch = ch;
    printf_note("[**REGISTER**]ch:%d, Set Decode method:%u, d_method=0x%x\n", ch, dec_method, d_method);
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    ret = ioctl(io_ctrl_fd,IOCTL_EXTRACT_CH0,d_method);
#elif defined(SUPPORT_SPECTRUM_V2) 
    printf_warn("NOT DEFINE...\n");
#endif
#endif
    return ret;

}


/*解调实际参数下发：内核发送数据头使用*/
int32_t io_set_dec_parameter(uint32_t ch, uint64_t dec_middle_freq, uint8_t dec_method, uint32_t dec_bandwidth)
{
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    FIXED_FREQ_ANYS_D_PARAM_ST dq;
    dq.cid = ch;
    dq.bandwidth = dec_bandwidth;
    dq.center_freq = dec_middle_freq;
    dq.d_method = dec_method;
    ioctl(io_ctrl_fd,IOCTL_RUN_DEC_PARAM,&dq);
#endif
#endif
    return 0;
}



/*解调中心频率计算（需要根据中心频率计算）*/
static uint32_t io_set_dec_middle_freq_reg(uint64_t dec_middle_freq, uint64_t middle_freq)
{
        /* delta_freq = (reg* 204800000)/2^32 ==>  reg= delta_freq*2^32/204800000 */
#if defined(SUPPORT_PROJECT_WD_XCR)
#define FREQ_MAGIC1 (256000000)
#else
#define FREQ_MAGIC1 (204800000)
#endif
#define FREQ_MAGIC2 (0x100000000ULL)  /* 2^32 */
    
        uint64_t delta_freq;
        uint32_t reg;
        int32_t ret = 0;
    
        if(middle_freq > dec_middle_freq){
            delta_freq = FREQ_MAGIC1 +  dec_middle_freq - middle_freq ;
        }else{
            delta_freq = dec_middle_freq -middle_freq;
        }
        reg = (uint32_t)((delta_freq *FREQ_MAGIC2)/FREQ_MAGIC1);
        
        return reg;
}


/*设置主通道解调中心频率因子*/
int32_t io_set_dec_middle_freq(uint32_t ch, uint64_t dec_middle_freq, uint64_t middle_freq)
{
    uint32_t reg;
    int32_t ret = 0;
    static uint32_t old_val = 0;
    static int32_t old_ch=-1;

    reg = io_set_dec_middle_freq_reg(dec_middle_freq, middle_freq);
    if((old_val == reg) && (ch == old_ch)){
        /* 避免重复设置相同参数 */
        return ret;
    }
    old_val = reg;
    old_ch = ch;
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    ret = ioctl(io_ctrl_fd, IOCTL_DECODE_MID_FREQ, reg);
#elif defined(SUPPORT_SPECTRUM_V2) 
    get_fpga_reg()->broad_band->signal_carrier = reg;
#endif
#endif
    printf_debug("[**REGISTER**]ch:%d, MiddleFreq =%llu, Decode MiddleFreq:%llu, reg=0x%x\n", ch, middle_freq, dec_middle_freq, reg);
    return ret;
}


/*设置子通道解调中心频率因子*/
int32_t io_set_subch_dec_middle_freq(uint32_t subch, uint64_t dec_middle_freq, uint64_t middle_freq)
{
        uint32_t reg;
        int32_t ret = 0;
        struct  ioctl_data_t odata;

        reg = io_set_dec_middle_freq_reg(dec_middle_freq, middle_freq);
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL) 
        odata.ch = subch;
        memcpy(odata.data,&reg,sizeof(reg));
        ret = ioctl(io_ctrl_fd, IOCTL_SUB_CH_MIDDLE_FREQ, &odata);
#elif defined(SUPPORT_SPECTRUM_V2) 
        get_fpga_reg()->narrow_band[subch]->signal_carrier = reg;
#endif
#endif
        printf_note("[**REGISTER**]ch:%d, SubChannel Set MiddleFreq =%llu, Decode MiddleFreq:%llu, reg=0x%x, ret=%d\n", subch, middle_freq, dec_middle_freq, reg, ret);
        return ret;
}

/*设置子通道开关*/
int32_t io_set_subch_onoff(uint32_t subch, uint8_t onoff)
{
    int32_t ret = 0;
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    struct  ioctl_data_t odata;
    odata.ch = subch;
    memcpy(odata.data,&onoff,sizeof(onoff));
    ret = ioctl(io_ctrl_fd, IOCTL_SUB_CH_ONOFF, &odata);
#elif defined(SUPPORT_SPECTRUM_V2) 
    printf_debug("subch=%u, onoff=%d, ptr=%p\n", subch, onoff, get_fpga_reg()->narrow_band[subch]);
    if(onoff)
        get_fpga_reg()->narrow_band[subch]->enable =0x01;
    else
        get_fpga_reg()->narrow_band[subch]->enable = 0x00;
#endif
#endif
    printf_debug("[**REGISTER**]ch:%d, SubChannle Set OnOff=%d\n",subch, onoff);
    return ret;
}

/*设置子通道解调带宽因子, 不同解调方式，带宽系数表不一样*/
int32_t io_set_subch_bandwidth(uint32_t subch, uint32_t bandwidth, uint8_t dec_method)
{
    int32_t ret = 0;
    uint32_t band_factor, filter_factor;
    struct  ioctl_data_t odata;
    struct  band_table_t *table;
    uint32_t table_len = 0;
    
    static uint32_t old_val = 0;
    static int32_t old_ch=-1;
    
    if((old_val == bandwidth) && (subch == old_ch)){
        /* 避免重复设置相同参数 */
        return ret;
    }
    old_val = bandwidth;
    old_ch = subch;
    dec_method = IO_DQ_MODE_IQ; /* TEST */
    if(dec_method == IO_DQ_MODE_IQ){
        table= &iq_nbandtable;
        table_len = sizeof(iq_nbandtable)/sizeof(struct  band_table_t);
        
    }else{
        table= &nbandtable;
        table_len = sizeof(nbandtable)/sizeof(struct  band_table_t);
    }
    io_get_bandwidth_factor(bandwidth, &band_factor,&filter_factor, table, table_len);
    /*设置子通道带宽系数*/
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    odata.ch = subch;
    memcpy(odata.data,&band_factor,sizeof(band_factor));
    ret = ioctl(io_ctrl_fd, IOCTL_SUB_CH_BANDWIDTH, &odata);

    /*设置子通道滤波器系数*/
    odata.ch = subch;
    memcpy(odata.data,&filter_factor,sizeof(filter_factor));
    ret = ioctl(io_ctrl_fd, IOCTL_SUB_CH_FILTER_COEFF, &odata);
#elif defined(SUPPORT_SPECTRUM_V2) 
    get_fpga_reg()->narrow_band[subch]->band = band_factor;
    get_fpga_reg()->narrow_band[subch]->fir_coeff = filter_factor;
#endif
    printf_note("[**REGISTER**]ch:%d, SubChannle Set Bandwidth=%u, factor=0x%x[%u], filter_factor=0x%x[%u],dec_method=%d,table_len=%d, ret=%d\n",
                subch, bandwidth, band_factor, band_factor,filter_factor,filter_factor, dec_method,  table_len, ret);

#endif /* SUPPORT_PLATFORM_ARCH_ARM */
    return ret;
}

/*设置子通道解调方式*/
int32_t io_set_subch_dec_method(uint32_t subch, uint8_t dec_method){
    int32_t ret = 0;
    struct  ioctl_data_t odata;
    uint32_t d_method = 0;
    static int32_t old_ch=-1;
    static uint32_t old_val=0;
   

    d_method = dec_method;
     if((old_val == d_method) && (subch == old_ch)){
        /* 避免重复设置相同参数 */
        return ret;
    }
    old_val = d_method;
    old_ch = subch;
    
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    odata.ch = subch;
    memcpy(odata.data,&d_method,sizeof(d_method));
    printf_note("[**REGISTER**]subch:%d, Set Decode method:%u, d_method=0x%x\n", subch, dec_method, d_method);
    ret = ioctl(io_ctrl_fd, IOCTL_SUB_CH_DECODE_TYPE, &odata);
#elif defined(SUPPORT_SPECTRUM_V2) 
    get_fpga_reg()->narrow_band[subch]->decode_type = d_method;
#endif
#endif
    return ret;

}

static void io_set_common_param(uint8_t type, uint8_t *buf,uint32_t buf_len)
{
    printf_debug("set common param: type=%d,data_len=%d\n",type,buf_len);
    
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL)  
    COMMON_PARAM_ST c_p_param;
    c_p_param.type = type;
    if(buf_len >0){
        memcpy(c_p_param.buf,buf,buf_len);
    }
    ioctl(io_ctrl_fd, IOCTL_COMMON_PARAM_CMD, &c_p_param);
#endif
#endif
}

/* 设置平滑数 */
void io_set_smooth_time(uint16_t stime)
{
    static uint16_t old_val = 0;
    
    if(old_val == stime){
        /* 避免重复设置相同参数 */
        return;
    }
    old_val = stime;

    printf_note("[**REGISTER**]Set Smooth time: factor=%d[0x%x]\n",stime, stime);
    
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL)
    //smooth mode
    ioctl(io_ctrl_fd,IOCTL_SMOOTH_CH0,0x10000);
    //smooth value
    ioctl(io_ctrl_fd,IOCTL_SMOOTH_CH0,stime);
#elif defined(SUPPORT_SPECTRUM_V2) 
    get_fpga_reg()->broad_band->fft_smooth_type = 0;
    get_fpga_reg()->broad_band->fft_mean_time = stime;
#endif
#endif
}

int8_t io_fill_mid_rf_param(uint8_t gain_mode, uint8_t gain_val)
{
    int ret = -1;
#if defined(SUPPORT_SPECTRUM_KERNEL)
    typedef struct _DEVICE_RF_INFO{
        uint8_t cid;
        uint8_t patten_id;
        uint8_t gain_patten_id;
        uint16_t gain_val;
        uint16_t auto_gain_time;
        int8_t mid_freq_amplitude;
        uint8_t bandwith_id;
        uint8_t antenna_sel;
    }__attribute__ ((packed)) DEVICE_RF_INFO_ST;
    
    DEVICE_RF_INFO_ST args;
    args.gain_patten_id = gain_mode;
    args.gain_val = gain_val;
    if (io_ctrl_fd > 0)
        ret = ioctl(io_ctrl_fd,IOCTL_RF_PARAM, &args);
#endif
    return ret;
}


/* 设置FPGA校准值 */
void io_set_calibrate_val(uint32_t ch, uint32_t  cal_value)
{
    static uint32_t old_val = 0, flag = 0;
    
    if((old_val == cal_value) && (flag != 0)){
        /* 避免重复设置相同参数 */
        flag = 1;
        return;
    }
    old_val = cal_value;

    printf_note("[**REGISTER**][ch=%d]Set Calibrate Val factor=%u[0x%x]\n",ch, cal_value,cal_value);
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL)
    ioctl(io_ctrl_fd,IOCTL_CALIBRATE_CH0,&cal_value);
#elif defined(SUPPORT_SPECTRUM_V2) 
    get_fpga_reg()->broad_band->fft_calibration = cal_value;
#endif
#endif
}


void io_dma_dev_enable(uint32_t ch, uint8_t type, uint8_t continuous)
{
    uint32_t ctrl_val = 0;
    uint32_t con ;
    /*data map: 
     +------------------------------------------------------------------+
     |                            data(ctrl_val) map                            |
     +---------------------------------+-------------+-----------------+
     |  continuous mode    |       ch       |fft|iq|dq(type)|      enable       |
     +---------------- - +--------------+------------+------------------+
     32bit              16bit              10bit          8bit                0bit
     |<-continuous(16bit)->|<-----data_offset(8bit)------>|<---enable(8bit)-->|
    */
    uint8_t data_offset = (ch<<2)|type;
    printf_debug("[**REGISTER**]ch=%d, type=%s, data_offset=%x, Enable\n", ch, type==0?"IQ":"FFT", data_offset);
    con = continuous;
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL)
    if (io_ctrl_fd > 0) {
        ctrl_val = ((con&0xff)<<16) | ((data_offset & 0xFF) << 8) | 1;
        ioctl(io_ctrl_fd,IOCTL_ENABLE_DISABLE,ctrl_val);
    }
#endif
#endif
}

static void io_dma_dev_trans_len(uint32_t ch, uint8_t type, uint32_t len)
{
    TRANS_LEN_PARAMETERS tran_parameter;
    uint8_t data_offset = (ch<<2)|type;
    printf_debug("[**REGISTER**]ch=%d, type=%s, data_offset=%x Transfer len:%d\n", ch, type==0?"IQ":"FFT", data_offset, len);
    uint32_t ctrl_val = 0;
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL)
    if (io_ctrl_fd > 0) {
        ctrl_val = (((data_offset & 0xF) << 28) | (len & 0xFFFFFF));
        ioctl(io_ctrl_fd,IOCTL_TRANSLEN, ctrl_val);
    }
#endif
#endif

}

void io_set_fft_size(uint32_t ch, uint32_t fft_size)
{
    printf_debug("set fft size:%d\n",ch, fft_size);
    uint32_t factor;
    static uint32_t old_value = 0;

    if(old_value == fft_size){
        return;
    }
    old_value = fft_size;
    factor = 0xc;
    if(fft_size == FFT_SIZE_256){
        factor = 0x8;
    }else if(fft_size == FFT_SIZE_512){
        factor = 0x9;
    }else if(fft_size == FFT_SIZE_1024){
        factor = 0xa;
    }else if(fft_size == FFT_SIZE_2048){
        factor = 0xb;
    }else if(fft_size == FFT_SIZE_4096){
        factor = 0xc;
    }else if(fft_size == FFT_SIZE_8192){
        factor = 0xd;
    }else if(fft_size == FFT_SIZE_16384){
        factor = 0xe;
    }else if(fft_size == FFT_SIZE_32768){
        factor = 0xf;
    }
    printf_note("[**REGISTER**][ch:%d]Set FFT Size=%u, factor=%u[0x%x]\n", ch, fft_size,factor, factor);
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL)
    if(io_ctrl_fd<=0){
        return;
    }
    ioctl(io_ctrl_fd,IOCTL_FFT_SIZE_CH0,factor);
#elif defined(SUPPORT_SPECTRUM_V2) 
    get_fpga_reg()->broad_band->fft_lenth = factor;
#endif
#endif
}

void io_set_fpga_sys_time(uint32_t time)
{
#if defined(SUPPORT_SPECTRUM_KERNEL)
    if(io_ctrl_fd<=0){
        return;
    }
    ioctl(io_ctrl_fd,IOCTL_SET_SYS_TIME,time);
#endif
}

void io_set_fpga_sample_ctrl(uint8_t val)
{
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2)
    get_fpga_reg()->system->if_ch = (val == 0 ? 0 : 1);
#endif
#endif
}

static void io_dma_dev_disable(uint32_t ch,uint8_t type)
{
    uint32_t ctrl_val = 0;
    int ret = -1;
    
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL)
    uint8_t data_offset = (ch<<2)|type;
    if (io_ctrl_fd > 0) {
        ctrl_val = (data_offset & 0xFF) << 8;
        ret = ioctl(io_ctrl_fd,IOCTL_ENABLE_DISABLE,ctrl_val);
    }
    printf_debug("[**REGISTER**]ch=%d, type=%s data_offset=%x Disable, ret=%d\n", ch, type==0?"IQ":"FFT", data_offset, ret);
#endif
#endif
}


static void io_set_dma_SPECTRUM_out_en(int ch, int subch, uint32_t trans_len,uint8_t continuous)
{
    printf_debug("SPECTRUM out enable: ch[%d]output en\n",ch);
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL)
    io_dma_dev_disable(ch,IO_SPECTRUM_TYPE);
    io_dma_dev_enable(ch,IO_SPECTRUM_TYPE,continuous);
    io_dma_dev_trans_len(ch,IO_SPECTRUM_TYPE, trans_len*2);
#elif defined(SUPPORT_SPECTRUM_V2)
    get_spm_ctx()->ops->stream_start(trans_len*sizeof(fft_t), continuous, STREAM_FFT);
#endif
#endif
}


static void io_set_dma_adc_out_en(int ch, int subch, uint32_t trans_len,uint8_t continuous)
{
    printf_debug("ADC out enable: output en\n");
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL)

#elif defined(SUPPORT_SPECTRUM_V2)
    get_spm_ctx()->ops->stream_start(trans_len, continuous, STREAM_ADC);
#endif
#endif
}

static void io_set_dma_adc_out_disable(int ch, int subch)
{
    printf_debug("ADC out disable\n",ch);
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL)

#elif defined(SUPPORT_SPECTRUM_V2)
    get_spm_ctx()->ops->stream_stop(STREAM_ADC);
#endif
#endif
}


static void io_set_dma_SPECTRUM_out_disable(int ch, int subch)
{
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL)
    io_dma_dev_disable(ch, IO_SPECTRUM_TYPE);
#elif defined(SUPPORT_SPECTRUM_V2)
    get_spm_ctx()->ops->stream_stop(STREAM_FFT);
#endif
#endif

}


static void io_set_IQ_out_disable(int ch, int subch)
{
    if(subch >= 0){
        io_set_subch_onoff(subch, 0);
    }
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL)
    io_dma_dev_disable(ch, IO_IQ_TYPE);
#elif defined(SUPPORT_SPECTRUM_V2) 
    get_spm_ctx()->ops->stream_stop(STREAM_IQ);
#endif 
#endif

}

static void io_set_IQ_out_en(int ch, int subch, uint32_t trans_len,uint8_t continuous)
{
    if(subch >= 0){
        io_set_subch_onoff(subch, 1);
    }
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL)
    printf_debug("SPECTRUM out enable: ch[%d]output en, trans_len=0x%u\n",ch, trans_len);
    io_dma_dev_disable(0,IO_IQ_TYPE);
    io_dma_dev_enable(0,IO_IQ_TYPE,continuous);
    io_dma_dev_trans_len(0,IO_IQ_TYPE, trans_len);
#elif defined(SUPPORT_SPECTRUM_V2) 
    get_spm_ctx()->ops->stream_start(trans_len, continuous, STREAM_IQ);
#endif
#endif
}


static void io_set_dma_DQ_out_en(int ch, int subch, uint32_t trans_len,uint8_t continuous)
{
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL)
    io_dma_dev_disable(ch,IO_DQ_TYPE);
    io_dma_dev_enable(ch,IO_DQ_TYPE,continuous);
    io_dma_dev_trans_len(ch,IO_DQ_TYPE, trans_len);
#elif defined(SUPPORT_SPECTRUM_V2) 
    
#endif
#endif
}

static void  io_set_dma_DQ_out_dis(int ch, int subch)
{
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL)
    io_dma_dev_disable(ch,IO_DQ_TYPE);
#elif defined(SUPPORT_SPECTRUM_V2) 

#endif
#endif

}


int8_t io_set_para_command(uint8_t type, int ch, void *data)
{
    int ret = 0;
    struct poal_config *poal_config = &(config_get_config()->oal_config);

    switch(type)
    {
        case EX_CHANNEL_SELECT:
            printf_debug("[**REGISTER**]Set Channel Select, ch=%d\n", *(uint8_t *)data);
            io_set_common_param(7, data,sizeof(uint8_t));
            break;
        case EX_AUDIO_SAMPLE_RATE:
        {
            SUB_AUDIO_PARAM paudio;
            paudio.cid= ch;
            paudio.sample_rate = *(uint32_t *)data;
            printf_debug("[**REGISTER**]Set Audio Sample Rate, ch=%d, rate:%u[0x%x]\n", paudio.cid, paudio.sample_rate, paudio.sample_rate);
            io_set_common_param(9, &paudio,sizeof(SUB_AUDIO_PARAM));
            break;
        }
        default:
            printf_err("invalid type[%d]", type);
            ret =  -1;
     break;
    }
    return ret;
}

int8_t io_set_work_mode_command(void *data)
{
    io_set_assamble_kernel_header_response_data(data);
    return 0;
}


int8_t io_set_enable_command(uint8_t type, int ch, int subc_ch, uint32_t fftsize)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    switch(type)
    {
        case PSD_MODE_ENABLE:
        {
            io_set_dma_SPECTRUM_out_en(ch, subc_ch, fftsize,0);
            break;
        }
        case PSD_MODE_DISABLE:
        {
            io_set_dma_SPECTRUM_out_disable(ch, subc_ch);
            break;
        }
        case ADC_MODE_ENABLE:
        {
            io_set_dma_adc_out_en(ch, subc_ch, 512,1);
            break;
        }
        case ADC_MODE_DISABLE:
        {
            io_set_dma_adc_out_disable(ch, subc_ch);
            break;
        }
        case AUDIO_MODE_ENABLE:
        {
            if(fftsize == 0)
                io_set_dma_DQ_out_en(ch, subc_ch, 512, 1);
            else
                io_set_dma_DQ_out_en(ch, subc_ch,fftsize, 1);
            break;
        }
        case AUDIO_MODE_DISABLE:
        {
            io_set_dma_DQ_out_dis(ch, subc_ch);
            break;
        }
        case IQ_MODE_ENABLE:
        {
            if(fftsize == 0)
            {
                if(poal_config->ctrl_para.iq_data_length == 0)
                {
                    printf_warn("iq_data_length not set, use 512\n");
                    poal_config->ctrl_para.iq_data_length = 512;
                }    
                io_set_IQ_out_en(ch, subc_ch, poal_config->ctrl_para.iq_data_length, 1);
            }  
            else
                io_set_IQ_out_en(ch, subc_ch, fftsize, 1);
            break;
        }
        case IQ_MODE_DISABLE:
        {
            io_set_IQ_out_disable(ch, subc_ch);
            break;
        }
        case SPCTRUM_MODE_ANALYSIS_ENABLE:
        {
            break;
        }
        case SPCTRUM_MODE_ANALYSIS_DISABLE:
        {
            break;
        }
        case DIRECTION_MODE_ENABLE:
        {
            break;
        }
        case DIRECTION_MODE_ENABLE_DISABLE:
        {
            break;
        }
    }
    return 0;
}

int32_t io_get_agc_thresh_val(int ch)
{
    int nread = 0;
    uint16_t agc_val = 0;
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL)
    nread = read(io_ctrl_fd, &agc_val, ch+1);
    if(nread <= 0){
        printf_err("read agc error[%d]\n", nread);
        return -1;
    }
#elif defined(SUPPORT_SPECTRUM_V2) 
    agc_val = get_fpga_reg()->broad_band->agc_thresh;
#endif
#endif
    return agc_val;
}


int16_t io_get_adc_temperature(void)
{
    float result=0; 
#ifdef SUPPORT_PLATFORM_ARCH_ARM
    char  path[128], upset[20];  
    char value=0;  
    int fd = -1, offset;
    float raw_data=0;
    

    sprintf(path,"/sys/bus/iio/devices/iio:device0/in_temp0_raw");
    fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        printf_note("[get_adc_temperature]: Cannot open in_temp0_raw path\n");
        return -1;
    }
    offset=0;  
    while(offset<5)	
    {  
        lseek(fd,offset,SEEK_SET);  
        read(fd,&value,sizeof(char));	 
        upset[offset]=value;  
        offset++;  
    }	 
    upset[offset]='\0'; 
    raw_data=atoi(upset);
    result = ((raw_data * 503.975)/4096) - 273.15;
    close(fd);
#endif
    return (signed short)result;

}

uint32_t get_fpga_version(void)
{
    int32_t ret = 0;
    uint32_t args = 0;
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    /* filename */
    ret = ioctl(io_ctrl_fd,IOCTL_GET_FPGA_VERSION,&args);
#elif defined(SUPPORT_SPECTRUM_V2) 
    args = get_fpga_reg()->system->version;
#endif
#else
    args=1;
#endif
    return args;
}

int16_t io_get_signal_strength(uint8_t ch)
{
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    struct signal_amp_t{
        uint8_t cid;
        uint16_t sig_amp;
    }__attribute__ ((packed));

    struct signal_amp_t sig_amp;
    sig_amp.cid = ch;
    if (io_ctrl_fd > 0) {
        ioctl(io_ctrl_fd,IOCTL_GET_CH_SIGNAL_AMP,&sig_amp);
        return sig_amp.sig_amp;
    }
#elif defined(SUPPORT_SPECTRUM_V2) 
    return 0;
#endif
#endif
}


/* ---Disk-related function--- */
int io_read_more_info_by_name(const char *name, void *info, int32_t (*iofunc)(void *))
{
    int ret = 0;
    if(name == NULL || info == NULL)
        return -1;
    strcpy((char *)info, name);
    ret = iofunc(info);
    return ret;
}

int32_t io_set_format_disk(void *arg){
    int32_t ret = 0;
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    /* filename */
    ret = ioctl(io_ctrl_fd,IOCTL_DISK_FORMAT,arg);
#endif
#endif
    return ret;
}


int32_t io_set_refresh_disk_file_buffer(void *arg){
    int32_t ret = 0;
    
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    /* filename */
    ret = ioctl(io_ctrl_fd,IOCTL_DISK_REFRESH_FILE_BUFFER,arg);
#endif
#endif
    return ret;
}

int32_t io_get_read_file_info(void *arg){
    int32_t ret = 0;
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    ret = ioctl(io_ctrl_fd,IOCTL_DISK_READ_FILE_INFO,arg);
#endif
#endif
    return ret;
}

int32_t io_get_disk_info(void *arg){
    int32_t ret = 0;
    
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    ret = ioctl(io_ctrl_fd,IOCTL_DISK_GET_INFO,arg);
#endif
#endif
    return ret;
}

int32_t io_find_file_info(void *arg){
    int32_t ret = 0;
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    ret = ioctl(io_ctrl_fd,IOCTL_DISK_FIND_FILE_INFO,arg);
#endif
#endif
    return ret;
}

int32_t io_delete_file(void *arg){
    int32_t ret = 0;
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    ret = ioctl(io_ctrl_fd,IOCTL_DISK_DELETE_FILE_INFO,arg);
#endif
#endif
    return ret;
}


int32_t io_start_save_file(void *arg){
    int32_t ret = 0;
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    ret = ioctl(io_ctrl_fd,IOCTL_DISK_START_SAVE_FILE_INFO,arg);
#endif
#endif
    return ret;
}

int32_t io_stop_save_file(void *arg){
    int32_t ret = 0;
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    ret = ioctl(io_ctrl_fd,IOCTL_DISK_STOP_SAVE_FILE_INFO,arg);
#endif
#endif
    return ret;
}

int32_t io_start_backtrace_file(void *arg){
    int32_t ret = 0;
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    SW_TO_BACKTRACE_MODE();
    ret = ioctl(io_ctrl_fd,IOCTL_DISK_START_BACKTRACE_FILE_INFO,arg);
#endif
#endif
    return ret;
}

int32_t io_stop_backtrace_file(void *arg){
    int32_t ret = 0;

#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    SW_TO_AD_MODE();
    ret = ioctl(io_ctrl_fd,IOCTL_DISK_STOP_BACKTRACE_FILE_INFO,arg);
#endif
#endif
    return ret;
}


int32_t io_dma_data_read(void *arg){
    int32_t ret = 0;
    //ret = ioctl(io_ctrl_fd,IOCTL_DMA_GET_ASYN_READ_INFO,arg);
    return ret;
}


int32_t io_set_assamble_kernel_header_response_data(void *data){

    int32_t ret = 0;
    DATUM_SPECTRUM_HEADER_ST *pdata;
    pdata = (DATUM_SPECTRUM_HEADER_ST *)data;
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL)
    if(io_ctrl_fd <= 0){
        return 0;
    }
    ret = ioctl(io_ctrl_fd, IOCTL_FFT_HDR_PARAM,data);
#endif
#endif
    return ret;
}

#ifdef SUPPORT_PLATFORM_ARCH_ARM
#define do_system(cmd)   safe_system(cmd)
#else
#define do_system(cmd)
#endif

uint8_t  io_restart_app(void)
{
    printf_debug("restart app\n");
    sleep(1);
    do_system("/etc/init.d/platform.sh restart &");
    sleep(1);
}


uint8_t  io_set_network_to_interfaces(void *netinfo)
{

        
    #ifdef SUPPORT_PLATFORM_ARCH_ARM
    #define NETWORK_INTERFACES_FILE_PATH  "/etc/network/interfaces"
    #else
    #define NETWORK_INTERFACES_FILE_PATH  "./etc/network/interfaces"
    #endif
    
    struct in_addr ip_sin_addr, mask_sin_addr, gw_sin_addr;
    struct network_st *pnetwork = netinfo;
    char cmd[256];
    uint8_t ret = 0;
    char *s_ip=NULL;
    char *s_gw=NULL;
    char *s_mask=NULL;
    

    printf_debug("ipaddress=%x, netmask=%x,gateway=%x\n", pnetwork->ipaddress, pnetwork->netmask, pnetwork->gateway);

    sprintf(cmd, "echo \"auto lo\" > %s", NETWORK_INTERFACES_FILE_PATH);
    printf_debug("%s\n", cmd);
    do_system(cmd);

    sprintf(cmd, "echo \"iface lo inet loopback\" >> %s", NETWORK_INTERFACES_FILE_PATH);
    printf_debug("%s\n", cmd);
    do_system(cmd);

    sprintf(cmd, "echo \"auto %s\" >> %s", NETWORK_EHTHERNET_POINT, NETWORK_INTERFACES_FILE_PATH);
    printf_debug("%s\n", cmd);
    do_system(cmd);
    
    sprintf(cmd, "echo \"iface %s inet static\" >> %s", NETWORK_EHTHERNET_POINT, NETWORK_INTERFACES_FILE_PATH);
    printf_debug("%s\n", cmd);
    do_system(cmd);

    
    //ip_sin_addr.s_addr = htonl(pnetwork->ipaddress);
    ip_sin_addr.s_addr = pnetwork->ipaddress;
    s_ip = inet_ntoa(ip_sin_addr);

    sprintf(cmd, "echo \"address %s\" >> %s", s_ip, NETWORK_INTERFACES_FILE_PATH);
    printf_debug("%s\n", cmd);
    do_system(cmd);

   // mask_sin_addr.s_addr = htonl(pnetwork->netmask);
    mask_sin_addr.s_addr = pnetwork->netmask;
    s_mask = inet_ntoa(mask_sin_addr);

    sprintf(cmd, "echo \"netmask %s\" >> %s", s_mask, NETWORK_INTERFACES_FILE_PATH);
    printf_debug("%s\n", cmd);
    do_system(cmd);

   // gw_sin_addr.s_addr = htonl(pnetwork->gateway);
    gw_sin_addr.s_addr = pnetwork->gateway;
    s_gw = inet_ntoa(gw_sin_addr);

    sprintf(cmd, "echo \"gateway %s\" >> %s", s_gw, NETWORK_INTERFACES_FILE_PATH);
    do_system(cmd);
#ifndef SUPPORT_GDB_DEBUG
    sprintf(cmd, "ip addr flush dev %s", NETWORK_EHTHERNET_POINT);/* 删除接口上所有地址 */
    do_system(cmd);
#endif
    sprintf(cmd, "/etc/init.d/networking restart");
    printf_debug("%s\n", cmd);
    do_system(cmd);
    return ret;
    
}

static void io_asyn_signal_handler(int signum)
{
    printf_debug("receive a signal, signalnum:%d\n", signum);
    sem_post(&work_sem.kernel_sysn);
}

int io_get_fd(void)
{
    return io_ctrl_fd;
}


void io_init(void)
{
    printf_info("io init!\n");
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_KERNEL)
    int Oflags;
    
    if (io_ctrl_fd > 0) {
        return;
    }
    io_ctrl_fd = open(DMA_DEVICE, O_RDWR);
    if(io_ctrl_fd < 0) {
        perror("open");
        return;
    }
    /* Asynchronous notification initial */
    signal(SIGIO, io_asyn_signal_handler);
    fcntl(io_ctrl_fd, F_SETOWN, getpid());
    Oflags = fcntl(io_ctrl_fd, F_GETFL);
    fcntl(io_ctrl_fd, F_SETFL, Oflags | FASYNC|FNONBLOCK);
#endif
#endif
}



