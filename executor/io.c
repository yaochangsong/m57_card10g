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
    {459085, 134, 600},
    {459085, 133, 1500},
    {459085, 132, 2400},
    {459085, 131, 5000},
    {459085, 130, 6000},
    {459085, 129, 9000},
    {459085, 128, 12000},
    {459085, 0, 15000},
    {458852, 0, 25000},
    {458852, 0, 50000},
    {458802, 0, 100000},
    {458785, 0, 150000},
    {458772, 0, 250000},
    {458772, 0, 500000},
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
        printf_info("not find band table, set default: extract=%d, extract_filter=%d\n", *extract, *extract_filter);
    }
}


static void io_set_common_param(uint8_t type, uint8_t *buf,uint32_t buf_len)
{
    printf_debug("set common param: type=%d,data_len=%d\n",type,buf_len);
#ifdef PLAT_FORM_ARCH_ARM
    COMMON_PARAM_ST c_p_param;
    c_p_param.type = type;
    if(buf_len >0){
        memcpy(c_p_param.buf,buf,buf_len);
    }
    ioctl(io_ctrl_fd, IOCTL_COMMON_PARAM_CMD, &c_p_param);
#endif    
}


void io_set_smooth_factor(uint32_t factor)
{
    printf_info("set smooth_factor: factor=%d\n",factor);
#ifdef PLAT_FORM_ARCH_ARM
    //smooth mode
    ioctl(io_ctrl_fd,IOCTL_SMOOTH_CH0,0x10000);
    //smooth value
    ioctl(io_ctrl_fd,IOCTL_SMOOTH_CH0,factor);
#endif    
}


void io_set_calibrate_val(uint32_t ch, uint32_t  factor)
{
    printf_debug("factor = %08x,%d\n",factor,factor);
#ifdef PLAT_FORM_ARCH_ARM
    ioctl(io_ctrl_fd,IOCTL_CALIBRATE_CH0,&factor);
#endif
}

void io_set_dq_param(void)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    
    uint64_t tmp_freq;
    uint32_t bindwith;
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
    ch = poal_config->cid;
    sub_ch = 0;
    bindwith = poal_config->multi_freq_point_param[ch].points[sub_ch].d_bandwith;
    d_method = poal_config->multi_freq_point_param[ch].points[sub_ch].d_method;

    io_compute_extract_factor_by_fftsize(bindwith,&band_factor, &filter_factor);
    printf_info("ch:%d, sub_ch:%d,bindwith=%u,d_method=%d, band_factor=%u, filter_factor=%u\n",ch, sub_ch, bindwith, d_method, band_factor, filter_factor);


    //first close iq
    band_factor |= 0x1000000;


    FIXED_FREQ_ANYS_D_PARAM_ST dq;
    dq.bandwith = bindwith;
    dq.center_freq = poal_config->multi_freq_point_param[ch].points[sub_ch].center_freq;
    dq.d_method = poal_config->multi_freq_point_param[ch].points[sub_ch].d_method;
    
#ifdef PLAT_FORM_ARCH_ARM
    ioctl(io_ctrl_fd,IOCTL_RUN_DEC_PARAM,&dq);
    
    memcpy(convert_buf,(uint8_t *)(&ch),sizeof(uint8_t));
    memcpy(convert_buf+1,(uint8_t *)(&sub_ch),sizeof(uint16_t));
    memcpy(convert_buf+3,(uint8_t *)(&freq_factor),sizeof(uint32_t));
    memcpy(convert_buf+3+sizeof(uint32_t),(uint8_t *)(&band_factor),sizeof(uint32_t));
    memcpy(convert_buf+3+2*sizeof(uint32_t),(uint8_t *)(&d_method),sizeof(uint32_t));

    io_set_common_param(3,convert_buf,sizeof(uint32_t)*3+3);
#endif 
}

void io_set_dma_DQ_out_en(uint8_t ch, uint8_t outputen,uint32_t trans_len,uint8_t continious)
{
    //for 8 channel device only one sub channel 
    uint16_t sub_ch = 0;
    //for iq and dq dma .for 512 fixed length ,continious mode
    printf_info("dq output enable, ch:%d, en:%x\n", ch, outputen);
    if((outputen&(D_OUT_MASK|IQ_OUT_MASK)) > 0){
        uint8_t convert_buf[512] = {0};
        memcpy(convert_buf,(uint8_t *)(&ch),sizeof(uint8_t));
        memcpy(convert_buf+1,(uint8_t *)(&sub_ch),sizeof(uint16_t));
        memcpy(convert_buf+3,(uint8_t *)(&outputen),sizeof(uint8_t));
        io_set_common_param(1,convert_buf,4);
    }
}

void io_set_dma_DQ_out_disable(uint8_t ch)
{
    uint8_t convert_buf[512] = {0};
    uint8_t outputen = 0;
    //for 8 channel device only one sub channel 
    uint16_t sub_ch = 0;
    printf_info("dq output disable, ch:%d\n", ch);
    memcpy(convert_buf,(uint8_t *)(&ch),sizeof(uint8_t));
    memcpy(convert_buf+1,(uint8_t *)(&sub_ch),sizeof(uint16_t));
    memcpy(convert_buf+3,(uint8_t *)(&outputen),sizeof(uint8_t));
    io_set_common_param(1,convert_buf,4);
}

void io_dma_dev_enable(uint32_t ch,uint8_t continuous)
{
#ifdef PLAT_FORM_ARCH_ARM
    uint32_t ctrl_val = 0;
    uint32_t con ;
    con = continuous;
    if (io_ctrl_fd > 0) {
        ctrl_val = ((con&0xff)<<16) | ((ch & 0xFF) << 8) | 1;
        ioctl(io_ctrl_fd,IOCTL_ENABLE_DISABLE,ctrl_val);
    }
#endif    
}

static void io_dma_dev_trans_len(uint32_t ch, uint32_t len)
{
#ifdef PLAT_FORM_ARCH_ARM
    TRANS_LEN_PARAMETERS tran_parameter;
    if (io_ctrl_fd > 0) {
        tran_parameter.ch = ch;
        tran_parameter.len = len;
        tran_parameter.type = FAST_SCAN;
        ioctl(io_ctrl_fd,IOCTL_TRANSLEN, &tran_parameter);
    }
#endif
}

void io_set_fft_size(uint32_t ch, uint32_t fft_size)
{
    printf_info("set fft size:%d\n",ch, fft_size);
#ifdef PLAT_FORM_ARCH_ARM
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
    ioctl(io_ctrl_fd,IOCTL_FFT_SIZE_CH0,factor);
#endif
}


static void io_set_dma_SPECTRUM_out_en(uint8_t cid, uint8_t outputen,uint32_t trans_len,uint8_t continuous)
{
    uint8_t ch = cid;
    printf_info("SPECTRUM out enable: ch[%d]output en[outputen:%x]\n",cid, outputen);
    if((outputen&SPECTRUM_MASK) > 0){
        io_dma_dev_enable(ch,continuous);
        io_dma_dev_trans_len(ch,trans_len);
        }
    }

static void io_dma_dev_disable(uint32_t ch)
{
#ifdef PLAT_FORM_ARCH_ARM
    uint32_t ctrl_val = 0;

    if (io_ctrl_fd > 0) {
        ctrl_val = (ch & 0xFF) << 8;
        ioctl(io_ctrl_fd,IOCTL_ENABLE_DISABLE,ctrl_val);
    }
#endif
}

static void io_set_dma_SPECTRUM_out_disable(uint8_t ch)
{
    printf_info("SPECTRUM out disable: ch[%d]\n",ch);
    io_dma_dev_disable(ch);
}

int8_t io_set_para_command(uint8_t type, uint8_t ch, void *data)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);

    switch(type)
    {
        case EX_CHANNEL_SELECT:
            printf_debug("select ch:%d\n", *(uint8_t *)data);
            io_set_common_param(7, data,sizeof(uint8_t));
            break;
        case EX_AUDIO_SAMPLE_RATE:
        {
            SUB_AUDIO_PARAM paudio;
            paudio.cid= ch;
            paudio.sample_rate = (uint32_t)*(float *)data;
            printf_info("set audio sample, ch:%d rate:%u\n", paudio.cid, paudio.sample_rate);
            io_set_common_param(9, &paudio,sizeof(SUB_AUDIO_PARAM));
            break;
        }
        default:
            printf_err("invalid type[%d]", type);
        return -1;
     break;
    }
    return 0;
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
            uint8_t outputen;
            outputen = poal_config->enable.bit_en;
            io_set_dma_SPECTRUM_out_en(ch, outputen,fftsize*2,0);
            break;
        }
        case PSD_MODE_DISABLE:
        {
            io_set_dma_SPECTRUM_out_disable(ch);
            break;
        }
        case AUDIO_MODE_ENABLE:
        case IQ_MODE_ENABLE:
        {
            uint8_t outputen;
            outputen = poal_config->enable.bit_en;
            io_set_dma_DQ_out_en(ch, outputen, 512, 0);
            break;
        }
        case AUDIO_MODE_DISABLE:
        case IQ_MODE_DISABLE:
        {
            io_set_dma_DQ_out_disable(ch);
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
}



int16_t io_get_adc_temperature(void)
{
    float result=0; 
#ifdef PLAT_FORM_ARCH_ARM
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

int32_t io_set_assamble_kernel_header_response_data(void *data){

    int32_t ret = 0;
#ifdef PLAT_FORM_ARCH_ARM
    DATUM_SPECTRUM_HEADER_ST *pdata;
    pdata = (DATUM_SPECTRUM_HEADER_ST *)data;
    if(io_ctrl_fd <= 0){
        return 0;
    }
    ret = ioctl(io_ctrl_fd, IOCTL_FFT_HDR_PARAM,data);
 #endif
    return ret;
}

#define do_system(cmd)   // system(cmd)

uint8_t  io_set_network_to_interfaces(void *netinfo)
{

        
    #ifdef PLAT_FORM_ARCH_X86
    #define NETWORK_INTERFACES_FILE_PATH  "./etc/network/interfaces"
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
    
    sprintf(cmd, "/etc/init.d/networking restart");
    printf_debug("%s\n", cmd);
    //system(cmd);
    return ret;
    
}

static void io_asyn_signal_handler(int signum)
{
    printf_info("receive a signal, signalnum:%d\n", signum);
    sem_post(&work_sem.kernel_sysn);
}



void io_init(void)
{
    printf_info("io init!\n");
#ifdef PLAT_FORM_ARCH_ARM
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



