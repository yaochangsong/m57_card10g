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

static int io_ctrl_fd = -1;

static struct  band_table_t bandtable[] ={
    {0x70fa0, 0, 5000},
    {0x70320, 0, 25000},
    {0x70190, 0, 50000},
    {0x700c8, 0, 100000},
    {0x70050, 0, 250000},
    {0x70028, 0, 500000},
    {0x70014, 0, 1000000},
    {0x70008, 0, 2500000},
    {0x70004, 0, 5000000},
//    {0x30004, 0, 10000000},
    {0x70000, 0, 20000000},
    {0x30000, 0, 40000000},
    {0x10000, 0, 80000000},
    {0,       0, 160000000},
}; 

static void  io_compute_extract_factor_by_fftsize(uint32_t anays_band,uint32_t *extract, uint32_t *extract_filter)
{
    int found = 0;
    uint32_t i;
    if(anays_band == 0){
        *extract = bandtable[0].extract_factor;
        *extract_filter = bandtable[0].filter_factor;
        printf_info("band=0, set default: extract=%d, extract_filter=%d\n", *extract, *extract_filter);
    }
    for(i = 0; i<sizeof(bandtable)/sizeof(struct  band_table_t); i++){
        if(bandtable[i].band == anays_band){
            *extract = bandtable[i].extract_factor;
            *extract_filter = bandtable[i].filter_factor;
            found = 1;
            break;
        }
    }
    if(found == 0){
        *extract = bandtable[i-1].extract_factor;
        *extract_filter = bandtable[i-1].filter_factor;
        printf_info("[%u]not find band table, set default: extract=%d, extract_filter=%d\n", anays_band, *extract, *extract_filter);
    }
}

int32_t io_set_sta_info_param(STATION_INFO *data){
    int32_t ret = 0;
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    ret = ioctl(io_ctrl_fd,IOCTL_STA_INFO_PARAM,data);
#endif
    return ret;
}

int32_t io_set_refresh_keepalive_time(uint32_t index){
    int32_t ret = 0;
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    ret = ioctl(io_ctrl_fd,IOCTL_KEEPALIVE_PARAM,index);
#endif
    return ret;
}

int32_t io_save_net_param(SNIFFER_DATA_REPORT_ST *data){
    int32_t ret = 0;

    if(io_ctrl_fd<=0){
        return 0;
    }
    ret = ioctl(io_ctrl_fd,IOCTL_RUN_NET_PARAM,data);
    return ret;
}

#ifdef SUPPORT_NET_WZ
int32_t io_set_local_10g_net(uint32_t ip, uint16_t port)
{
    struct net_10g_local_param_t{
        uint32_t local_ip_10g;
        uint16_t local_port_10g;
    };

    int32_t ret = 0;
    if(io_ctrl_fd<=0){
        return 0;
    }
    
    struct net_10g_local_param_t nl;
    /*万兆设备ip网段在千兆口网段基础上加1,端口和千兆口端口一样*/
    nl.local_ip_10g = ip+(1 << 8);
    nl.local_port_10g = port;
    printf_note("ipaddress[0x%x], port=%d\n", nl.local_ip_10g, nl.local_port_10g);
    ret = ioctl(io_ctrl_fd,IOCTL_NET_10G_LOCAL_SET,&nl);
    return ret;
}

/* 万兆开关 */
int32_t io_set_10ge_net_onoff(uint8_t onoff)
{
    int32_t ret = 0;
    printf_note("[**NET**]10GE net %s\n", onoff == 0 ? "off":"on");
    ret = ioctl(io_ctrl_fd,IOCTL_NET_10G_ONOFF_SET,&onoff);
    return ret;
}
#endif

int32_t io_set_1ge_net_onoff(uint8_t onoff)
{
    int32_t ret = 0;
    printf_note("[**NET**]1GE net %s\n", onoff == 0 ? "off":"on");
    ret = ioctl(io_ctrl_fd,IOCTL_NET_1G_IQ_ONOFF_SET,&onoff);
    return ret;
}



void io_reset_fpga_data_link(void){
    #define RESET_ADDR      0x04U
    int32_t ret = 0;
    int32_t data = 0;
    if(io_ctrl_fd<=0){
        return ;
    }
    printf_debug("[**REGISTER**]Reset FPGA\n");
    data = (RESET_ADDR &0xffff)<<16;
    ret = ioctl(io_ctrl_fd,IOCTL_SET_DDC_REGISTER_VALUE,&data);
}

/*设置带宽因子*/
int32_t io_set_bandwidth(uint32_t ch, uint32_t bandwidth){
    int32_t ret = 0;
    uint32_t set_factor,band_factor, filter_factor;
    static int32_t old_ch=-1;
    static uint32_t old_val=0;
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    io_compute_extract_factor_by_fftsize(bandwidth,&band_factor, &filter_factor);
    set_factor = band_factor|0x1000000;
    if((old_val == set_factor) && (ch == old_ch)){
        /* 避免重复设置相同参数 */
        return ret;
    }
    old_val = set_factor;
    old_ch = ch;
    printf_debug("[**REGISTER**]ch:%d, Set Bandwidth:%u,band_factor=0x%x,set_factor=0x%x\n", ch, bandwidth,band_factor,set_factor);
    ret = ioctl(io_ctrl_fd, IOCTL_EXTRACT_CH0, set_factor);
#endif
    return ret;

}

/*设置解调带宽因子*/
int32_t io_set_dec_bandwidth(uint32_t ch, uint32_t dec_bandwidth){
    int32_t ret = 0;
    uint32_t set_factor, band_factor, filter_factor;
    static int32_t old_ch=-1;
    static uint32_t old_val=0;
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    io_compute_extract_factor_by_fftsize(dec_bandwidth,&band_factor, &filter_factor);
    set_factor = band_factor|0x2000000;
    if((old_val == set_factor) && (ch == old_ch)){
        /* 避免重复设置相同参数 */
        return ret;
    }
    old_val = set_factor;
    old_ch = ch;
    printf_note("[**REGISTER**]ch:%d, Set Decode Bandwidth:%u, band_factor=0x%x, set_factor=0x%x\n", ch, dec_bandwidth, band_factor, set_factor);
    ret = ioctl(io_ctrl_fd, IOCTL_EXTRACT_CH0, set_factor);
#endif
    return ret;

}


/*设置解调方式*/
int32_t io_set_dec_method(uint32_t ch, uint8_t dec_method){
    int32_t ret = 0;
    uint32_t d_method = 0;
    static int32_t old_ch=-1;
    static uint32_t old_val=0;
   
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    if(dec_method == DQ_MODE_AM){
        d_method = 0x4000000;
    }else if(dec_method == DQ_MODE_FM) {
        d_method = 0x4000001;
    }else if(dec_method == DQ_MODE_LSB) {
        d_method = 0x4000002;
    }else if(dec_method == DQ_MODE_USB) {
        d_method = 0x4000002;
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
    ret = ioctl(io_ctrl_fd,IOCTL_EXTRACT_CH0,d_method);
#endif
    return ret;

}


/*解调实际参数下发：内核发送数据头使用*/
int32_t io_set_dec_parameter(uint32_t ch, uint64_t dec_middle_freq, uint8_t dec_method, uint32_t dec_bandwidth)
{
    FIXED_FREQ_ANYS_D_PARAM_ST dq;
    dq.cid = ch;
    dq.bandwidth = dec_bandwidth;
    dq.center_freq = dec_middle_freq;
    dq.d_method = dec_method;
    ioctl(io_ctrl_fd,IOCTL_RUN_DEC_PARAM,&dq);
}



/*解调中心频率计算（需要根据中心频率计算）*/
uint32_t io_set_dec_middle_freq_reg(uint64_t dec_middle_freq, uint64_t middle_freq)
{
        /* delta_freq = (reg* 204800000)/2^32 ==>  reg= delta_freq*2^32/204800000 */
#define FREQ_MAGIC1 (204800000)
#define FREQ_MAGIC2 (0x100000000ULL)  /* 2^32 */
    
        uint64_t delta_freq;
        uint32_t reg;
        int32_t ret = 0;
    
#if defined(SUPPORT_SPECTRUM_KERNEL) 
        if(middle_freq > dec_middle_freq){
            delta_freq = FREQ_MAGIC2 +  dec_middle_freq - middle_freq ;
        }else{
            delta_freq = dec_middle_freq -middle_freq;
        }
        reg = (uint32_t)((delta_freq *FREQ_MAGIC2)/FREQ_MAGIC1);
#endif
        return reg;
}


/*设置主通道解调中心频率因子*/
int32_t io_set_dec_middle_freq(uint32_t ch, uint64_t dec_middle_freq, uint64_t middle_freq)
{
    uint32_t reg;
    int32_t ret = 0;
    static uint32_t old_val = 0;
    static int32_t old_ch=-1;
    
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    reg = io_set_dec_middle_freq_reg(dec_middle_freq, middle_freq);
    if((old_val == reg) && (ch == old_ch)){
        /* 避免重复设置相同参数 */
        return ret;
    }
    old_val = reg;
    old_ch = ch;
    printf_debug("[**REGISTER**]ch:%d, MiddleFreq =%llu, Decode MiddleFreq:%llu, reg=0x%x\n", ch, middle_freq, dec_middle_freq, reg);
    ret = ioctl(io_ctrl_fd, IOCTL_DECODE_MID_FREQ, reg);
#endif
    return ret;
}


/*设置子通道解调中心频率因子*/
int32_t io_set_subch_dec_middle_freq(uint32_t subch, uint64_t dec_middle_freq, uint64_t middle_freq)
{
        uint32_t reg;
        int32_t ret = 0;
        struct  ioctl_data_t odata;
#if defined(SUPPORT_SPECTRUM_KERNEL) 
        reg = io_set_dec_middle_freq_reg(dec_middle_freq, middle_freq);
        odata.ch = subch;
        memcpy(odata.data,&reg,sizeof(reg));
        ret = ioctl(io_ctrl_fd, IOCTL_SUB_CH_MIDDLE_FREQ, &odata);
        printf_debug("[**REGISTER**]ch:%d, SubChannel Set MiddleFreq =%llu, Decode MiddleFreq:%llu, reg=0x%x, ret=%d\n", subch, middle_freq, dec_middle_freq, reg, ret);
        
#endif
        return ret;
}

/*设置子通道开关*/
int32_t io_set_subch_onoff(uint32_t subch, uint8_t onoff)
{
    struct  ioctl_data_t odata;
    int32_t ret = 0;
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    odata.ch = subch;
    memcpy(odata.data,&onoff,sizeof(onoff));
    ret = ioctl(io_ctrl_fd, IOCTL_SUB_CH_ONOFF, &odata);
    printf_debug("[**REGISTER**]ch:%d, SubChannle Set OnOff=%d, ret=%d\n",subch, onoff, ret);
#endif
    return ret;
}

/*设置子通道解调带宽因子*/
int32_t io_set_subch_bandwidth(uint32_t subch, uint32_t bandwidth)
{
    int32_t ret = 0;
    uint32_t band_factor, filter_factor;
    struct  ioctl_data_t odata;
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    io_compute_extract_factor_by_fftsize(bandwidth,&band_factor, &filter_factor);
    odata.ch = subch;
    memcpy(odata.data,&band_factor,sizeof(band_factor));
    ret = ioctl(io_ctrl_fd, IOCTL_SUB_CH_BANDWIDTH, &odata);
    printf_debug("[**REGISTER**]ch:%d, SubChannle Set Bandwidth=%u, factor=0x%x ret=%d\n",subch, bandwidth, band_factor, ret);
#endif
    return ret;
}


static void io_set_common_param(uint8_t type, uint8_t *buf,uint32_t buf_len)
{
    printf_debug("set common param: type=%d,data_len=%d\n",type,buf_len);
#if defined(SUPPORT_SPECTRUM_KERNEL)  
    COMMON_PARAM_ST c_p_param;
    c_p_param.type = type;
    if(buf_len >0){
        memcpy(c_p_param.buf,buf,buf_len);
    }
    ioctl(io_ctrl_fd, IOCTL_COMMON_PARAM_CMD, &c_p_param);
#endif
}

/* 设置平滑数 */
void io_set_smooth_time(uint16_t stime)
{
    printf_debug("[**REGISTER**]Set Smooth time: factor=%d[0x%x]\n",stime, stime);
#if defined(SUPPORT_SPECTRUM_KERNEL)
    //smooth mode
    ioctl(io_ctrl_fd,IOCTL_SMOOTH_CH0,0x10000);
    //smooth value
    ioctl(io_ctrl_fd,IOCTL_SMOOTH_CH0,stime);  
#endif
}


/* 设置FPGA校准值 */
void io_set_calibrate_val(uint32_t ch, uint32_t  cal_value)
{
    printf_debug("[**REGISTER**][ch=%d]Set Calibrate Val factor=%u[0x%x]\n",ch, cal_value,cal_value);
#if defined(SUPPORT_SPECTRUM_KERNEL)
    ioctl(io_ctrl_fd,IOCTL_CALIBRATE_CH0,&cal_value);
#endif
}

void io_set_dq_param(void *pdata)
{
    struct io_decode_param_st *dec_para = (struct io_decode_param_st *)pdata;
    uint64_t tmp_freq;
    uint32_t bandwidth;
    uint64_t fix_value1 = (0x100000000ULL);
    uint64_t fix_value2 = (102400000);
    uint32_t freq_factor,band_factor, filter_factor;

    uint32_t d_method = 0;
    uint16_t ch, sub_ch = 0;

    unsigned char convert_buf[32]={0};    
    //mid frequency  map value
    tmp_freq = fix_value2 ;
    tmp_freq = tmp_freq*fix_value1;
    tmp_freq /= fix_value2;
    freq_factor = (uint32_t)tmp_freq;

    //banwidth 
    ch = dec_para->cid;
    bandwidth = dec_para->d_bandwidth;
    d_method = dec_para->d_method;

    io_compute_extract_factor_by_fftsize(bandwidth,&band_factor, &filter_factor);
    printf_debug("ch:%d, sub_ch:%d,bandwidth=%u,d_method=%d, band_factor=%u, filter_factor=%u\n",ch, sub_ch, bandwidth, d_method, band_factor, filter_factor);


    //first close iq
    band_factor |= 0x1000000;


    FIXED_FREQ_ANYS_D_PARAM_ST dq;
    dq.bandwidth = bandwidth;
    dq.center_freq = dec_para->center_freq;
    dq.d_method = dec_para->d_method;
#if defined(SUPPORT_SPECTRUM_KERNEL)
    ioctl(io_ctrl_fd,IOCTL_RUN_DEC_PARAM,&dq);
#endif
    
    memcpy(convert_buf,(uint8_t *)(&ch),sizeof(uint8_t));
    memcpy(convert_buf+1,(uint8_t *)(&sub_ch),sizeof(uint16_t));
    memcpy(convert_buf+3,(uint8_t *)(&freq_factor),sizeof(uint32_t));
    memcpy(convert_buf+3+sizeof(uint32_t),(uint8_t *)(&band_factor),sizeof(uint32_t));
    memcpy(convert_buf+3+2*sizeof(uint32_t),(uint8_t *)(&d_method),sizeof(uint32_t));
    printf_debug("[**REGISTER**]ch=%d, sub_ch=%d, d_freq_factor=%u[0x%x]\n", ch, sub_ch, freq_factor, freq_factor);
    printf_debug("[**REGISTER**]ch=%d, sub_ch=%d, d_method=%u[0x%x]\n", ch, sub_ch,d_method, d_method);
    printf_debug("[**REGISTER**]ch=%d, sub_ch=%d, d_band_factor=%u[0x%x]\n", ch, sub_ch, band_factor, band_factor);
    io_set_common_param(3,convert_buf,sizeof(uint32_t)*3+3);
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
#if defined(SUPPORT_SPECTRUM_KERNEL)
    if (io_ctrl_fd > 0) {
        ctrl_val = ((con&0xff)<<16) | ((data_offset & 0xFF) << 8) | 1;
        ioctl(io_ctrl_fd,IOCTL_ENABLE_DISABLE,ctrl_val);
    }
#endif
}

static void io_dma_dev_trans_len(uint32_t ch, uint8_t type, uint32_t len)
{
    TRANS_LEN_PARAMETERS tran_parameter;
    uint8_t data_offset = (ch<<2)|type;
    printf_debug("[**REGISTER**]ch=%d, type=%s, data_offset=%x Transfer len:%d\n", ch, type==0?"IQ":"FFT", data_offset, len);
    uint32_t ctrl_val = 0;
#if defined(SUPPORT_SPECTRUM_KERNEL)
    if (io_ctrl_fd > 0) {
        ctrl_val = (((data_offset & 0xF) << 28) | (len & 0xFFFFFF));
        ioctl(io_ctrl_fd,IOCTL_TRANSLEN, ctrl_val);
    }
#endif

}

void io_set_fft_size(uint32_t ch, uint32_t fft_size)
{
    printf_debug("set fft size:%d\n",ch, fft_size);
    uint32_t factor;

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
    if(io_ctrl_fd<=0){
        return;
    }
    printf_debug("[**REGISTER**][ch:%d]Set FFT Size=%u, factor=%u[0x%x]\n", ch, fft_size,factor, factor);
#if defined(SUPPORT_SPECTRUM_KERNEL)
    ioctl(io_ctrl_fd,IOCTL_FFT_SIZE_CH0,factor);
#endif
}

static void io_dma_dev_disable(uint32_t ch,uint8_t type)
{
    uint32_t ctrl_val = 0;
    int ret = -1;
    
#if defined(SUPPORT_SPECTRUM_KERNEL)
    uint8_t data_offset = (ch<<2)|type;
    if (io_ctrl_fd > 0) {
        ctrl_val = (data_offset & 0xFF) << 8;
        ret = ioctl(io_ctrl_fd,IOCTL_ENABLE_DISABLE,ctrl_val);
    }
    printf_debug("[**REGISTER**]ch=%d, type=%s data_offset=%x Disable, ret=%d\n", ch, type==0?"IQ":"FFT", data_offset, ret);
#endif
}


static void io_set_dma_SPECTRUM_out_en(uint8_t ch, uint32_t trans_len,uint8_t continuous)
{
    printf_debug("SPECTRUM out enable: ch[%d]output en\n",ch);
    io_dma_dev_disable(ch,IO_SPECTRUM_TYPE);
    io_dma_dev_enable(ch,IO_SPECTRUM_TYPE,continuous);
    io_dma_dev_trans_len(ch,IO_SPECTRUM_TYPE, trans_len);
}

static void io_set_dma_SPECTRUM_out_disable(uint8_t ch)
{
    io_dma_dev_disable(ch, IO_SPECTRUM_TYPE);
}


static void io_set_IQ_out_disable(uint8_t ch)
{
    io_dma_dev_disable(ch, IO_IQ_TYPE);
}

static void io_set_IQ_out_en(uint8_t ch,uint32_t trans_len,uint8_t continuous)
{
    printf_debug("SPECTRUM out enable: ch[%d]output en, trans_len=0x%u\n",ch, trans_len);
    io_dma_dev_disable(ch,IO_IQ_TYPE);
    io_dma_dev_enable(ch,IO_IQ_TYPE,continuous);
    io_dma_dev_trans_len(ch,IO_IQ_TYPE, trans_len);
}


static void io_set_dma_DQ_out_en(uint8_t ch,uint32_t trans_len,uint8_t continuous)
{
    io_dma_dev_disable(ch,IO_DQ_TYPE);
    io_dma_dev_enable(ch,IO_DQ_TYPE,continuous);
    io_dma_dev_trans_len(ch,IO_DQ_TYPE, trans_len);
}

static void  io_set_dma_DQ_out_dis(uint8_t ch)
{
    io_dma_dev_disable(ch,IO_DQ_TYPE);
}


int8_t io_set_para_command(uint8_t type, uint8_t ch, void *data)
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


int8_t io_set_enable_command(uint8_t type, uint8_t ch, uint32_t fftsize)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    switch(type)
    {
        case PSD_MODE_ENABLE:
        {
            io_set_dma_SPECTRUM_out_en(ch,fftsize*2,0);
            break;
        }
        case PSD_MODE_DISABLE:
        {
            io_set_dma_SPECTRUM_out_disable(ch);
            break;
        }
        case AUDIO_MODE_ENABLE:
        {
            if(fftsize == 0)
                io_set_dma_DQ_out_en(ch, 512, 1);
            else
                io_set_dma_DQ_out_en(ch, fftsize, 1);
            break;
        }
        case AUDIO_MODE_DISABLE:
        {
            io_set_dma_DQ_out_dis(ch);
            break;
        }
        case IQ_MODE_ENABLE:
        {
            if(fftsize == 0)
                io_set_IQ_out_en(ch, 32768, 1);
            else
                io_set_IQ_out_en(ch, fftsize, 1);
            break;
        }
        case IQ_MODE_DISABLE:
        {
            io_set_IQ_out_disable(ch);
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
        case FREQUENCY_BAND_ENABLE_DISABLE:
        {
            ioctl(io_ctrl_fd,IOCTL_FREQUENCY_BAND_CONFIG0,0);
            break;
        }
    }
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
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    /* filename */
    ret = ioctl(io_ctrl_fd,IOCTL_DISK_FORMAT,arg);
#endif
    return ret;
}


int32_t io_set_refresh_disk_file_buffer(void *arg){
    int32_t ret = 0;
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    /* filename */
    ret = ioctl(io_ctrl_fd,IOCTL_DISK_REFRESH_FILE_BUFFER,arg);
#endif
    return ret;
}

int32_t io_get_read_file_info(void *arg){
    int32_t ret = 0;
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    ret = ioctl(io_ctrl_fd,IOCTL_DISK_READ_FILE_INFO,arg);
#endif
    return ret;
}

int32_t io_get_disk_info(void *arg){
    int32_t ret = 0;
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    ret = ioctl(io_ctrl_fd,IOCTL_DISK_GET_INFO,arg);
#endif
    return ret;
}

int32_t io_find_file_info(void *arg){
    int32_t ret = 0;
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    ret = ioctl(io_ctrl_fd,IOCTL_DISK_FIND_FILE_INFO,arg);
#endif
    return ret;
}

int32_t io_delete_file(void *arg){
    int32_t ret = 0;
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    ret = ioctl(io_ctrl_fd,IOCTL_DISK_DELETE_FILE_INFO,arg);
#endif
    return ret;
}


int32_t io_start_save_file(void *arg){
    int32_t ret = 0;
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    ret = ioctl(io_ctrl_fd,IOCTL_DISK_START_SAVE_FILE_INFO,arg);
#endif
    return ret;
}

int32_t io_stop_save_file(void *arg){
    int32_t ret = 0;
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    ret = ioctl(io_ctrl_fd,IOCTL_DISK_STOP_SAVE_FILE_INFO,arg);
#endif
    return ret;
}

int32_t io_start_backtrace_file(void *arg){
    int32_t ret = 0;
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    SW_TO_BACKTRACE_MODE();
    ret = ioctl(io_ctrl_fd,IOCTL_DISK_START_BACKTRACE_FILE_INFO,arg);
#endif
    return ret;
}

int32_t io_stop_backtrace_file(void *arg){
    int32_t ret = 0;
#if defined(SUPPORT_SPECTRUM_KERNEL) 
    SW_TO_AD_MODE();
    ret = ioctl(io_ctrl_fd,IOCTL_DISK_STOP_BACKTRACE_FILE_INFO,arg);
#endif
    return ret;
}



int32_t io_set_assamble_kernel_header_response_data(void *data){

    int32_t ret = 0;
    DATUM_SPECTRUM_HEADER_ST *pdata;
    pdata = (DATUM_SPECTRUM_HEADER_ST *)data;
#if defined(SUPPORT_SPECTRUM_KERNEL)
    if(io_ctrl_fd <= 0){
        return 0;
    }
    ret = ioctl(io_ctrl_fd, IOCTL_FFT_HDR_PARAM,data);
#endif
    return ret;
}

#ifdef SUPPORT_PLATFORM_ARCH_ARM
#define do_system(cmd)   safe_system(cmd)
#else
#define do_system(cmd)
#endif

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
    printf_info("receive a signal, signalnum:%d\n", signum);
    sem_post(&work_sem.kernel_sysn);
}

int io_get_fd(void)
{
    return io_ctrl_fd;
}


void io_init(void)
{
    printf_info("io init!\n");
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
}



