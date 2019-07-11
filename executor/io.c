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

struct  band_table_t bandtable[] ={
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

void io_init(void)
{
    int Oflags,i;
    
    if (io_ctrl_fd > 0) {
        return;
    }
    io_ctrl_fd = open(DMA_DEVICE, O_RDWR);
    if(io_ctrl_fd < 0) {
        perror("open");
        return;
    }
    fcntl(io_ctrl_fd, F_SETOWN, getpid());
    Oflags = fcntl(io_ctrl_fd, F_GETFL);
    fcntl(io_ctrl_fd, F_SETFL, Oflags | FASYNC|FNONBLOCK);
}

void io_set_common_param(uint8_t type, uint8_t *buf,uint32_t buf_len)
{
    printf_debug("set common param: type=%d,data_len=%d\n",type,buf_len);
    COMMON_PARAM_ST c_p_param;
    c_p_param.type = type;
    if(buf_len >0){
        memcpy(c_p_param.buf,buf,buf_len);
    }
    ioctl(io_ctrl_fd, IOCTL_COMMON_PARAM_CMD, &c_p_param);
}


void io_set_smooth_factor(uint32_t factor)
{
    //smooth mode
    ioctl(io_ctrl_fd,IOCTL_SMOOTH_CH0,0x10000);
    //smooth value
    ioctl(io_ctrl_fd,IOCTL_SMOOTH_CH0,factor);
}


void io_set_calibrate_val(uint32_t ch, uint32_t  factor)
{
    printf_debug("factor = %08x,%d\n",factor,factor);
    ioctl(io_ctrl_fd,IOCTL_CALIBRATE_CH0,&factor);
}

void io_set_dq_param(uint32_t ch,FIXED_FREQ_ANYS_D_PARAM_ST dq)
{

    uint64_t tmp_freq,center_freq;
    uint64_t fix_value1 = (0x100000000ULL);
    uint64_t fix_value2 = (102400000);
    uint32_t freq_factor,band_factor;

    uint32_t d_method = 0;

    unsigned char convert_buf[32]={0};
    //for 8 channel device only one sub channel 
    uint16_t sub_ch = 0;

    printf_debug("ch:%d, decode method:%d\n",ch, dq.d_method);
    if(dq.d_method == DQ_MODE_AM){
        d_method = 0x0;
    }else if(dq.d_method == DQ_MODE_FM || dq.d_method == DQ_MODE_WFM) {
        d_method = 0x1;
    }else if(dq.d_method == DQ_MODE_LSB || dq.d_method == DQ_MODE_USB) {
        d_method = 0x2;
    }else if(dq.d_method == DQ_MODE_CW) {
        d_method = 0x3;
    }else if(dq.d_method == DQ_MODE_IQ) {
        d_method = 0x7;
    }else{
        printf_info("decode method not support:%d\n",dq.d_method);
        return;
    }
    //mid frequency  map value
    tmp_freq = fix_value2 ;
    tmp_freq = tmp_freq*fix_value1;
    tmp_freq /= fix_value2;
    freq_factor = (uint32_t)tmp_freq;
#if 0
	//banwidth 
	compute_extract_fftsize(dq.bandwith,&band_factor);
	printf_info("bd=%u,factor=%x\n",dq.bandwith,band_factor);
	
	//first close iq
	band_factor |= 0x1000000;

	ioctl(dma_fd,IOCTL_RUN_DEC_PARAM,&dq);

	memcpy(convert_buf,(uint8_t *)(&ch),sizeof(uint8_t));
	memcpy(convert_buf+1,(uint8_t *)(&sub_ch),sizeof(uint16_t));
	memcpy(convert_buf+3,(uint8_t *)(&freq_factor),sizeof(uint32_t));
	memcpy(convert_buf+3+sizeof(uint32_t),(uint8_t *)(&band_factor),sizeof(uint32_t));
	memcpy(convert_buf+3+2*sizeof(uint32_t),(uint8_t *)(&d_method),sizeof(uint32_t));

	set_common_param(3,convert_buf,sizeof(uint32_t)*3+3);
#endif
}


int8_t io_set_command(uint8_t cmd, uint8_t type, void *data)
{
    switch(cmd)
    {
        case EX_CHANNEL_SELECT:
            printf_debug("select ch:%d\n", *(uint8_t *)data);
            io_set_common_param(7, &data,sizeof(uint8_t));
            break;
        case EX_SMOOTH_TIME:
            printf_debug("smooth time:%d\n", *(uint16_t *)data);
            io_set_smooth_factor(*(uint16_t *)data);
            break;
        default:
            printf_err("invalid cmd[%d]", cmd);
            return -1;
    }
    return 0;
}

int16_t io_get_adc_temperature(void)
{
    char  path[128], upset[20];  
    char value=0;  
    int fd = -1, offset;
    float raw_data=0;
    float result=0; 

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
    
    return (signed short)result;
}


