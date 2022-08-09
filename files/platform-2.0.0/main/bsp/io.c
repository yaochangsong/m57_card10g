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
#include <math.h>
#include "config.h"
#include "../executor/spm/spm.h"
#include "../utils/bitops.h"
#include "io.h"


static DECLARE_BITMAP(subch_bmp[CH_TYPE_MAX], MAX_SIGNAL_CHANNEL_NUM);
static DECLARE_BITMAP(ch_bmp[CH_TYPE_MAX], MAX_RADIO_CHANNEL_NUM);


void subch_bitmap_init(void)
{
    for(int i = 0; i< CH_TYPE_MAX; i++)
        bitmap_zero(subch_bmp[i], MAX_SIGNAL_CHANNEL_NUM);
}

void subch_bitmap_set(uint8_t subch, CH_TYPE type)
{
    set_bit(subch, subch_bmp[type]);
}

bool test_subch_iq_on(uint8_t subch)
{
    return test_bit(subch, subch_bmp[CH_TYPE_IQ]);
}

bool test_audio_on(void)
{
    return test_subch_iq_on(CONFIG_AUDIO_CHANNEL);
}


void subch_bitmap_clear(uint8_t subch, CH_TYPE type)
{
    clear_bit(subch, subch_bmp[type]);
}

size_t subch_bitmap_weight(CH_TYPE type)
{
    return bitmap_weight(subch_bmp[type], MAX_SIGNAL_CHANNEL_NUM);
}


const unsigned long *get_ch_bitmap(int index)
{
    return ch_bmp[index];
}

void ch_bitmap_init(void)
{
    for(int i = 0; i< CH_TYPE_MAX; i++)
        bitmap_zero(ch_bmp[i], MAX_RADIO_CHANNEL_NUM);
}
void ch_bitmap_set(uint8_t ch, CH_TYPE type)
{
    set_bit(ch, ch_bmp[type]);
}

void ch_bitmap_clear(uint8_t ch, CH_TYPE type)
{
    clear_bit(ch, ch_bmp[type]);
}

size_t ch_bitmap_weight(CH_TYPE type)
{
    return bitmap_weight(ch_bmp[type], MAX_RADIO_CHANNEL_NUM);
}

bool test_ch_iq_on(uint8_t ch)
{
    return test_bit(ch, ch_bmp[CH_TYPE_IQ]);
}

bool test_ch_fft_on(uint8_t ch)
{
    return test_bit(ch, ch_bmp[CH_TYPE_FFT]);
}

static void  io_get_config_bandwidth_factor(uint32_t anays_band,               /* 输入参数：带宽 */
                                            uint32_t *bw_factor,        /* 输出参数：带宽系数 */
                                            uint32_t *filter_factor,    /* 输出参数：滤波器系数 */
                                            uint32_t *filter_factor2,    /* 输出参数：滤波器系数2 */
                                            struct bindwidth_factor_table *table,/* 输入参数：系数表 */
                                            uint32_t table_len          /* 输入参数：系数表长度 */
                                            )
{
    int found = 0;
    uint32_t i;
    if(anays_band == 0 || table_len == 0){
        *bw_factor = table[0].extract;
        *filter_factor = table[0].filter;
        *filter_factor2 = table[0].filter2;
        printf_note("band=0, set default: extract=%d, extract_filter=%d\n", *bw_factor, *filter_factor);
    }
    for(i = 0; i<table_len; i++){
        if(table[i].bw_hz == anays_band){
            *bw_factor = table[i].extract;
            *filter_factor = table[i].filter;
            *filter_factor2 = table[i].filter2;
            found = 1;
            break;
        }
    }
    if(found == 0){
        *bw_factor = table[0].extract;
        *filter_factor = table[0].filter;
        *filter_factor2 = table[0].filter2;
        printf_debug("[%u]not find band table, set default:[%uHz] extract=%d, extract_filter=%d\n", anays_band, table[0].bw_hz, *bw_factor, *filter_factor);
    }
}



static void  io_get_bandwidth_factor(uint32_t anays_band,               /* 输入参数：带宽 */
                                            uint32_t *bw_factor,        /* 输出参数：带宽系数 */
                                            uint32_t *filter_factor,    /* 输出参数：滤波器系数 */
                                            uint32_t *filter_factor2,    /* 输出参数：滤波器系数2 */
                                            struct  band_table_t *table,/* 输入参数：系数表 */
                                            uint32_t table_len          /* 输入参数：系数表长度 */
                                            )
{
    int found = 0;
    uint32_t i;
    if(anays_band == 0 || table_len == 0){
        *bw_factor = table[0].extract_factor;
        *filter_factor = table[0].filter_factor;
        *filter_factor2 = table[0].filter_factor2;
        printf_info("band=0, set default: extract=%d, extract_filter=%d\n", *bw_factor, *filter_factor);
    }
    for(i = 0; i<table_len; i++){
        if(table[i].band == anays_band){
            *bw_factor = table[i].extract_factor;
            *filter_factor = table[i].filter_factor;
            *filter_factor2 = table[i].filter_factor2;
            found = 1;
            break;
        }
    }
    if(found == 0){
        *bw_factor = table[0].extract_factor;
        *filter_factor = table[0].filter_factor;
        *filter_factor2 = table[0].filter_factor2;
        printf_info("[%u]not find band table, set default:[%uHz] extract=%d, extract_filter=%d\n", anays_band, table[i-1].band, *bw_factor, *filter_factor);
    }
}

void io_reset_fpga_data_link(void){
    #define RESET_ADDR      0x04U
    int32_t ret = 0;
    int32_t data = 0;

    printf_debug("[**REGISTER**]Reset FPGA\n");
#ifdef SET_DATA_RESET
    SET_DATA_RESET(get_fpga_reg(),1);
#endif
}

/*设置带宽因子*/
int32_t io_set_bandwidth(uint32_t ch, uint32_t bandwidth){
    int32_t ret = 0;
    uint32_t set_factor = 0,band_factor = 0, filter_factor = 0, filter_factor2 = 0;
    static int32_t old_ch=-1;
    static uint32_t old_val=0;
    struct  band_table_t *table = NULL;
    struct bindwidth_factor_table *config_table = config_get_config()->oal_config.ctrl_para.bband_bw_factor;
    int table_len = 0;


    if(config_table[0].bw_hz != 0){
       table_len = ARRAY_SIZE(config_get_config()->oal_config.ctrl_para.bband_bw_factor);
       printf_debug("table_len:%d\n", table_len);

       io_get_config_bandwidth_factor(bandwidth, &band_factor,&filter_factor, &filter_factor2, config_table, table_len);
    }
    else
    {
#ifdef CONFIG_FPGA_REGISTER
        table = broadband_factor_table(&table_len);
#endif
        if(table == NULL)
            return -1;
        io_get_bandwidth_factor(bandwidth, &band_factor,&filter_factor, &filter_factor2, table, table_len);
    }

    if((old_val == band_factor) && (ch == old_ch)){
        /* 避免重复设置相同参数 */
        return ret;
    }
    old_val = band_factor;
    old_ch = ch;
#ifdef SET_BROAD_BAND
    SET_BROAD_BAND(get_fpga_reg(),band_factor, ch);
#endif
    printf_note("[**REGISTER**]ch:%d, Set Bandwidth:%u,band_factor=0x%x,set_factor=0x%x\n", ch, bandwidth,band_factor,set_factor);

    return ret;

}

int32_t io_set_noise(uint32_t ch, uint32_t noise_en,int8_t noise_level_tmp){
    uint32_t  noise_level;
    uint32_t  noise_level_cfg = config_get_noise_level();
    if (noise_level_cfg == 0)
        noise_level_cfg = 16000;
    
    if(noise_en == 1)
    {
        if(noise_level_tmp > 0)
        {
            noise_level_tmp = 0;
        }
        noise_level =  (uint32_t)(noise_level_cfg * pow((double)10.0, (double)(noise_level_tmp / 20.0)));
    }
    else 
    {
        noise_level = 0;
    }
    printf_info("[**REGISTER**]ch:%d, Set noise_level:%u noise_en:%d noise_level_tmp:%d\n", ch,noise_level,noise_en,noise_level_tmp);

#if defined(SET_NARROW_NOISE_LEVEL)
    SET_NARROW_NOISE_LEVEL(get_fpga_reg(),ch,noise_level);
#endif
    return 0;
}


/*设置解调带宽因子*/
int32_t io_set_dec_bandwidth(uint32_t ch, uint32_t dec_bandwidth){
    int32_t ret = 0;
    uint32_t set_factor, band_factor, filter_factor, filter_factor2;
    static int32_t old_ch=-1;
    static uint32_t old_val=0;
    struct  band_table_t *table = NULL;
    struct bindwidth_factor_table *config_table = config_get_config()->oal_config.ctrl_para.bband_bw_factor;
    int table_len = 0;

    if(config_table[0].bw_hz != 0){
        table_len = ARRAY_SIZE(config_get_config()->oal_config.ctrl_para.bband_bw_factor);
        io_get_config_bandwidth_factor(dec_bandwidth, &band_factor,&filter_factor, &filter_factor2, config_table, table_len);
     }
    else
    {
#ifdef CONFIG_FPGA_REGISTER
        table = broadband_factor_table(&table_len);
#endif
        if(table == NULL)
            return -1;
        //table= bandtable;
        //table_len = sizeof(bandtable)/sizeof(struct  band_table_t);
        io_get_bandwidth_factor(dec_bandwidth, &band_factor,&filter_factor,&filter_factor2, table, table_len);
    }
    set_factor = band_factor;

    if((old_val == set_factor) && (ch == old_ch)){
        /* 避免重复设置相同参数 */
        return ret;
    }
    old_val = set_factor;
    old_ch = ch;
    printf_note("[**REGISTER**]ch:%d, Set Decode Bandwidth:%u, band_factor=0x%x, set_factor=0x%x, %u\n", ch, dec_bandwidth, band_factor, set_factor, set_factor);
    #ifdef SET_BIQ_BAND
    SET_BIQ_BAND(get_fpga_reg(),band_factor, ch);
    #endif
    printf_note("broard_band[%d]->band:%x\n",ch,band_factor);

    return ret;

}


/*设置解调方式*/
int32_t io_set_dec_method(uint32_t ch, uint8_t dec_method){
    int32_t ret = 0;
    uint32_t d_method = 0;
   // static int32_t old_ch=-1;
   // static uint32_t old_val=0;
   

    if(dec_method == IO_DQ_MODE_AM){
        d_method = 0x0000000;
    }else if(dec_method == IO_DQ_MODE_FM) {
        d_method = 0x0000001;
    }else if(dec_method == IO_DQ_MODE_LSB) {
        d_method = 0x0000002;
    }else if(dec_method == IO_DQ_MODE_USB) {
        d_method = 0x0000002;
    }else if(dec_method == IO_DQ_MODE_ISB) {
        d_method = 0x0000002;
    }else if(dec_method == IO_DQ_MODE_CW) {
        d_method = 0x0000003;
    }else if(dec_method == IO_DQ_MODE_PM) {
        d_method = 0x0000005;
    }else if(dec_method == IO_DQ_MODE_IQ) {
        d_method = 0x0000007;
    }else{
        printf_warn("decode method not support:%d\n",dec_method);
        return -1;
    }
    // if((old_val == d_method) && (ch == old_ch)){
        /* 避免重复设置相同参数 */
   //     return ret;
  //  }
  //  old_val = d_method;
   // old_ch = ch;
    printf_info("[**REGISTER**]ch:%d, Set Decode method:%u, d_method=0x%x\n", ch, dec_method, d_method);
#ifdef  SET_NARROW_DECODE_TYPE
    SET_NARROW_DECODE_TYPE(get_fpga_reg(),ch,dec_method);;
#endif
    return ret;

}


/*解调中心频率计算（需要根据中心频率计算）*/
static uint32_t io_set_dec_middle_freq_reg(uint8_t ch, uint64_t dec_middle_freq, uint64_t middle_freq)
{
        /* delta_freq = (reg* 204800000)/2^32 ==>  reg= delta_freq*2^32/204800000 */
#ifdef CLOCK_FREQ_HZ
#define FREQ_MAGIC1 CLOCK_FREQ_HZ
#else
#define FREQ_MAGIC1 (256000000)
#endif
#define FREQ_MAGIC2 (0x100000000ULL)  /* 2^32 */
    
        uint64_t delta_freq;
        uint32_t reg;
        int32_t ret = 0;

#if defined(CONFIG_BSP_YF21025) || defined(CONFIG_BSP_YF21036) || defined(CONFIG_BSP_YF21038)
        uint64_t board_mid_freq;
        config_read_by_cmd(EX_RF_FREQ_CMD, EX_RF_MID_FREQ_FILTER, ch, &board_mid_freq);
        printf_note("ch%d set subch mid freq, board freq:%"PRIu64"\n", ch, board_mid_freq);
        delta_freq = board_mid_freq +  dec_middle_freq - middle_freq ; 
#else
        if(middle_freq > dec_middle_freq){
            delta_freq = FREQ_MAGIC1 +  dec_middle_freq - middle_freq ;
        }else{
            delta_freq = dec_middle_freq -middle_freq;
        }
#endif

#if defined(CONFIG_BSP_TF713_2CH)
        if(executor_get_bandwidth(ch) == MHZ(20))
            delta_freq = delta_freq + (MHZ(412.5) - MHZ(384));
#endif
        reg = (uint32_t)((delta_freq *FREQ_MAGIC2)/FREQ_MAGIC1);
        
        return reg;
}


/*设置主通道解调中心频率因子*/
int32_t io_set_dec_middle_freq(uint32_t ch, uint64_t dec_middle_freq, uint64_t middle_freq)
{
    uint32_t reg = 0;
    int32_t ret = 0;
    static uint32_t old_val = 0;
    static int32_t old_ch=-1;
    

    if(reg_get()->misc && reg_get()->misc->get_channel_middle_freq)
        middle_freq = reg_get()->misc->get_channel_middle_freq(ch, dec_middle_freq, &middle_freq);

    if(reg_get()->misc && reg_get()->misc->get_channel_by_mfreq)
        ch = reg_get()->misc->get_channel_by_mfreq(dec_middle_freq, &ch);
        
    //middle_freq = _get_channel_middle_freq(ch, dec_middle_freq, &middle_freq);/* 选择主通道频率 */
   // ch =  _get_channel_by_mfreq(dec_middle_freq, &ch);/* 选择主通道号 */
    reg = io_set_dec_middle_freq_reg(ch, dec_middle_freq, middle_freq);
    if((old_val == reg) && (ch == old_ch)){
        /* 避免重复设置相同参数 */
        return ret;
    }
    old_val = reg;
    old_ch = ch;
#ifdef SET_BIQ_SIGNAL_CARRIER
    SET_BIQ_SIGNAL_CARRIER(get_fpga_reg(),reg, ch);
#endif
    printf_note("[**REGISTER**]ch:%d, MiddleFreq =%"PRIu64",Decode MiddleFreq:%"PRIu64",reg=0x%x\n", ch, middle_freq, dec_middle_freq, reg);
    return ret;
}

uint64_t io_get_raw_sample_rate(uint32_t ch, uint64_t middle_freq, uint32_t bw)
{
   // #define _SAMPLE_RATE_ MHZ(512)
    #define DEFAULT_SIDE_BAND_RATE  (1.28)
    float side_rate = 0.0;
    /* 根据带宽获取边带率 */
    if(config_read_by_cmd(EX_CTRL_CMD, EX_CTRL_SIDEBAND,0, &side_rate, bw) == -1){
        printf_info("!!!!!!!!!!!!!SideRate Is Not Set In Config File[bandwidth=%u]!!!!!!!!!!!!!\n", bw);
        side_rate = DEFAULT_SIDE_BAND_RATE;
    }
    uint64_t sample_rate = (uint64_t)bw * side_rate;
    #if defined(CONFIG_BSP_WD_XCR)
    int reminder = sample_rate % MHZ(1.28);
    sample_rate = (sample_rate / MHZ(1.28)) * MHZ(1.28);
    if (reminder > 0 && (sample_rate % MHZ(1))) {
        sample_rate = sample_rate + MHZ(1.28);
    }
    #endif
    
    return sample_rate;
}

int32_t io_set_middle_freq(uint32_t ch, uint64_t middle_freq, uint64_t rel_mfreq)
{

    uint32_t reg = 0;
    reg = io_set_dec_middle_freq_reg(ch, rel_mfreq, middle_freq);
    
    #ifdef SET_BROAD_SIGNAL_CARRIER
        SET_BROAD_SIGNAL_CARRIER(get_fpga_reg(),reg, ch);
    #endif
    printf_debug("ch:%d, freq:%"PRIu64", reg=0x%x\n", ch, middle_freq, reg);
    printf_note(">>>>>>ch:%d, feq:%"PRIu64",rel_middle_freq=%"PRIu64", reg=0x%x\n",ch, middle_freq,rel_mfreq,  reg);
    
    return 0;
}

/*设置子通道解调中心频率因子*/
int32_t io_set_subch_dec_middle_freq(uint32_t ch, uint32_t subch, uint64_t dec_middle_freq, uint64_t middle_freq)
{
    uint32_t reg = 0;
    int32_t ret = 0;


    if(reg_get()->misc && reg_get()->misc->get_channel_middle_freq)
        middle_freq = reg_get()->misc->get_channel_middle_freq(ch, dec_middle_freq, &middle_freq);
    //middle_freq = _get_channel_middle_freq(ch, dec_middle_freq, &middle_freq);
    reg = io_set_dec_middle_freq_reg(ch, dec_middle_freq, middle_freq);
#ifdef SET_NARROW_SIGNAL_CARRIER
    SET_NARROW_SIGNAL_CARRIER(get_fpga_reg(),subch,reg);
#endif

    printf_info("[**REGISTER**]ch:%d, SubChannel Set MiddleFreq =%"PRIu64", Decode MiddleFreq:%"PRIu64", reg=0x%x, ret=%d\n", subch, middle_freq, dec_middle_freq, reg, ret);
    return ret;
}

/*设置子通道开关*/
int32_t io_set_subch_onoff(uint32_t ch, uint32_t subch, uint8_t onoff)
{
    int32_t ret = 0;

    if(onoff){
        uint64_t midfreq = 0;
        config_read_by_cmd(EX_MID_FREQ_CMD, EX_SUB_CH_MID_FREQ, ch, &midfreq, subch);
        //_set_niq_channel(get_fpga_reg(),ch, subch, &midfreq, 0x01);
        if(reg_get()->iif && reg_get()->iif->set_niq_channel)
            reg_get()->iif->set_niq_channel(ch, subch, &midfreq, 0x01);
    }
    else{
        //_set_niq_channel(get_fpga_reg(),ch, subch, NULL, 0x00);
        if(reg_get()->iif && reg_get()->iif->set_niq_channel)
            reg_get()->iif->set_niq_channel(ch, subch, NULL, 0x00);
    }
    printf_debug("[**REGISTER**]ch:%d, SubChannle Set OnOff=%d\n",subch, onoff);
    return ret;
}

/*设置子通道解调带宽因子, 不同解调方式，带宽系数表不一样*/
int32_t io_set_subch_bandwidth(uint32_t subch, uint32_t bandwidth, uint8_t dec_method)
{
    int32_t ret = 0;
    uint32_t band_factor, filter_factor, filter_factor2;
    struct  band_table_t *table = NULL;
    struct bindwidth_factor_table *config_table;
    int table_len = 0;
    uint32_t config_table_len = 0;
    
    static uint32_t old_val = 0;
    static int32_t old_ch=-1;
    static uint8_t old_method = 0xff;
    
    if((old_val == bandwidth) && (subch == old_ch) && (old_method == dec_method)){
        /* 避免重复设置相同参数 */
        return ret;
    }
    old_val = bandwidth;
    old_ch = subch;
    old_method = dec_method;
    
   // dec_method = IO_DQ_MODE_IQ; /* TEST */
    if(dec_method == IO_DQ_MODE_IQ){
#ifdef CONFIG_FPGA_REGISTER
        table = narrowband_iq_factor_table(&table_len);
#endif
        //table= iq_nbandtable;
        //table_len = sizeof(iq_nbandtable)/sizeof(struct  band_table_t);
        config_table = config_get_config()->oal_config.ctrl_para.niq_bw_factor;
        config_table_len = ARRAY_SIZE(config_get_config()->oal_config.ctrl_para.niq_bw_factor);
        printf_debug("config_table_len is:%d\n", config_table_len);
    }else{
#ifdef CONFIG_FPGA_REGISTER
        table = narrowband_demo_factor_table(&table_len);
#endif
        //table= nbandtable;
        //table_len = sizeof(nbandtable)/sizeof(struct  band_table_t);
        config_table = config_get_config()->oal_config.ctrl_para.dem_bw_factor;
        config_table_len = ARRAY_SIZE(config_get_config()->oal_config.ctrl_para.dem_bw_factor);
        printf_debug("config_table_len is:%d\n", config_table_len);

    }
    if(table == NULL)
        return -1;
    if(config_table[0].bw_hz != 0 ){
        io_get_config_bandwidth_factor(bandwidth, &band_factor,&filter_factor, &filter_factor2, config_table, config_table_len);
    }
    else{
        io_get_bandwidth_factor(bandwidth, &band_factor,&filter_factor,&filter_factor2, table, table_len);
    }
    /*设置子通道带宽系数*/
#ifdef SET_NARROW_BAND
    SET_NARROW_BAND(get_fpga_reg(),subch,band_factor);
#endif
#ifdef SET_NARROW_FIR_COEFF
    SET_NARROW_FIR_COEFF(get_fpga_reg(),subch,filter_factor);
#endif
#ifdef SET_NARROW_FIR_COEFF2
    SET_NARROW_FIR_COEFF2(get_fpga_reg(),subch,filter_factor2);
#endif
    printf_info("[**REGISTER**]ch:%d, SubChannle Set Bandwidth=%u, factor=0x%x[%u], filter_factor=0x%x[%u],dec_method=%d,table_len=%d, ret=%d\n",
                subch, bandwidth, band_factor, band_factor,filter_factor,filter_factor, dec_method,  table_len, ret);

    return ret;
}

/*设置子通道解调方式*/
int32_t io_set_subch_dec_method(uint32_t subch, uint8_t dec_method){
    int32_t ret = 0;
   
    uint32_t d_method = 0;
    static int32_t old_ch=-1;
    static int32_t old_val=-1;
   

    d_method = dec_method;
     if((old_val == d_method) && (subch == old_ch)){
        /* 避免重复设置相同参数 */
        return ret;
    }
    old_val = d_method;
    old_ch = subch;

#ifdef SET_NARROW_DECODE_TYPE
    SET_NARROW_DECODE_TYPE(get_fpga_reg(),subch,d_method);
#endif

    return ret;

}

void io_set_subch_audio_sample_rate(uint32_t ch, uint32_t subch,  int rate)
{
    //_set_narrow_audio_sample_rate(get_fpga_reg(), ch, subch, rate);
    if(reg_get()->iif && reg_get()->iif->set_narrow_audio_sample_rate)
        reg_get()->iif->set_narrow_audio_sample_rate(ch, subch, rate);
}

void io_set_subch_audio_gain_mode(uint32_t ch, uint32_t subch,  int mode)
{
    if(reg_get()->iif && reg_get()->iif->set_narrow_audio_gain_mode)
        reg_get()->iif->set_narrow_audio_gain_mode(ch, subch, mode);
   // _set_narrow_audio_gain_mode(get_fpga_reg(), ch, subch, mode);
}


void io_set_subch_audio_gain(uint32_t ch, uint32_t subch,  int gain)
{
    if(reg_get()->iif && reg_get()->iif->set_narrow_audio_gain)
        reg_get()->iif->set_narrow_audio_gain(ch, subch, (float)gain);
    //_set_narrow_audio_gain(get_fpga_reg(), ch, subch, (float)gain);
}

/* 设置加窗类型 */
void io_set_window_type(uint32_t ch, int type)
{
#if defined(CONFIG_SPM_FFT_WINDOWS)
    uint16_t *data, *ptr;
    uint32_t reg_val = 0;
    size_t fsize = 0;
    data = config_get_fft_window_data(type, &fsize);
    fsize = fsize / sizeof(uint16_t);
    ptr = data;
    printf_note("fsize: %zd type=%d\n", fsize, type);
    if(data != NULL && fsize > 0){
        for(int i = 0; i < fsize; i += 4){
            /* reg_val: 高16bit为0,1,2..递增数，低16bit为读取窗文件抽取数据 */
            reg_val = *ptr + ((i / 4) << 16);
            ptr += 4;
            #ifdef SET_FFT_WINDOW_TYPE
            SET_FFT_WINDOW_TYPE(get_fpga_reg(), reg_val, ch);
            #endif
        }
    }
#endif
}


/* 设置平滑类型 */
void io_set_smooth_type(uint32_t ch, int type)
{
#if defined(SET_FFT_SMOOTH_TYPE)
    SET_FFT_SMOOTH_TYPE(get_fpga_reg(),type, ch);
#endif
}

/* 设置平滑门限 */
void io_set_smooth_threshold(uint32_t ch, int val)
{
#if defined(SET_FFT_SMOOTH_THRESHOLD)
    SET_FFT_SMOOTH_THRESHOLD(get_fpga_reg(),val, ch);
#endif
}


/* 设置平滑数 */
void io_set_smooth_time(uint32_t ch, uint16_t stime)
{
    static uint16_t old_val[MAX_RADIO_CHANNEL_NUM] = {0};
    if( ch >= MAX_RADIO_CHANNEL_NUM || old_val[ch] == stime){
        /* 避免重复设置相同参数 */
        return;
    }
    old_val[ch] = stime;

    printf_note("[**REGISTER**]ch:%u, Set Smooth time: factor=%d[0x%x]\n",ch, stime, stime);
#ifdef SET_FFT_MEAN_TIME
    SET_FFT_MEAN_TIME(get_fpga_reg(),stime, ch);
#else
    if((get_spm_ctx() != NULL) && get_spm_ctx()->ops->set_smooth_time)
        get_spm_ctx()->ops->set_smooth_time(stime);
#endif
    
}



/* 设置FPGA校准值 */
void io_set_calibrate_val(uint32_t ch, int32_t  cal_value)
{
    static int32_t old_val[MAX_RADIO_CHANNEL_NUM] = {0};

    if(ch >= MAX_RADIO_CHANNEL_NUM || old_val[ch] == cal_value){
        /* 避免重复设置相同参数 */
        return;
    }
    old_val[ch] = cal_value;
    printf_note("[**REGISTER**][ch=%d]Set Calibrate Val factor=%d[0x%x]\n",ch, cal_value,cal_value);
#ifdef SET_FFT_CALIB
    SET_FFT_CALIB(get_fpga_reg(),(uint32_t)cal_value, ch);
#else
    if((get_spm_ctx()!=NULL) && get_spm_ctx()->ops->set_calibration_value)
        get_spm_ctx()->ops->set_calibration_value(cal_value);
#endif
}

/* 设置频谱增益校准值 */
void io_set_gain_calibrate_val(uint32_t ch, int32_t  gain_val)
{
    static int32_t old_val[MAX_RADIO_CHANNEL_NUM];
    static bool has_inited = false;
    
    if(has_inited == false){
        for(int i = 0; i < MAX_RADIO_CHANNEL_NUM; i++)
            old_val[i] = -1000;
        has_inited = true;
    }
        
    if(old_val[ch] == gain_val || ch >= MAX_RADIO_CHANNEL_NUM){
        /* 避免重复设置相同参数 */
        return;
    }
    old_val[ch] = gain_val;
    printf_note("[**REGISTER**][ch=%d]Set Gain Calibrate Val factor=%u[0x%x]\n",ch, gain_val,gain_val);
#ifdef CONFIG_DEVICE_RF
    struct rf_ctx *ctx = get_rfctx();
    if(ctx && ctx->ops->set_mgc_gain)
        ctx->ops->set_mgc_gain(0, gain_val&0xff);
#endif
}
/* 设置DC OFFSET校准值 */
void io_set_dc_offset_calibrate_val(uint32_t ch, int32_t  val)
{
    static int32_t old_val = -1;
    if(old_val == val){
        /* 避免重复设置相同参数 */
        return;
    }
    old_val = val;
    printf_note("[**REGISTER**][ch=%d]Set DC OFFSET Calibrate Val factor=%u[0x%x]\n",ch, val,val);
#ifdef CONFIG_DEVICE_RF_ADRV
    adrv_set_rx_dc_offset(val&0xff);
#endif
}

void io_set_rf_calibration_source_level(int level)
{
    printf_note("calibration source level = %d\n", level);
    if(reg_get()->rf && reg_get()->rf->set_cali_source_attenuation)
        reg_get()->rf->set_cali_source_attenuation(0, 0, level);
}
void io_set_rf_calibration_source_enable(int ch, int enable)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    if((ch >= MAX_RADIO_CHANNEL_NUM) || (ch < 0))
        return;

    if(reg_get()->rf && reg_get()->rf->set_cali_source_choise)
        reg_get()->rf->set_cali_source_choise(ch, 0, enable);

    usleep(300);
    poal_config->channel[ch].enable.map.bit.cali = (enable == 0 ? 0 : 1);
    printf_debug("cali_source_en: 0x%x\n", poal_config->channel[ch].enable.map.bit.cali);
}
bool is_rf_calibration_source_enable(void)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    int ch = poal_config->cid;
    bool enable = false;
    enable = poal_config->channel[ch].enable.map.bit.cali & 0x01;
    enable = (enable == 0 ? false : true);
    printf_debug("calibration source enable: %s\n", (enable==true ? "true" : "false"));
    return enable;
}

void io_set_fft_size(uint32_t ch, uint32_t fft_size)
{
    uint32_t factor;
    static uint32_t old_value[MAX_RADIO_CHANNEL_NUM] = {0};

    if(ch >= MAX_RADIO_CHANNEL_NUM || old_value[ch] == fft_size){
        return;
    }
    old_value[ch] = fft_size;
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
    printf_info("[**REGISTER**][ch:%d]Set FFT Size=%u, factor=%u[0x%x]\n", ch, fft_size,factor, factor);
#ifdef SET_FFT_FFT_LEN
    SET_FFT_FFT_LEN(get_fpga_reg(),factor, ch);
#endif
}

void io_set_fpga_sys_time(uint32_t time)
{
#ifdef SET_CURRENT_TIME
    SET_CURRENT_TIME(get_fpga_reg(),time);
#endif
}

uint32_t io_get_fpga_sys_ns_time(void)
{
    uint32_t _ns = 0;
#ifdef GET_CURRENT_COUNT
    _ns = GET_CURRENT_COUNT(get_fpga_reg());
    _ns = _ns & 0x0fffffff;
    _ns = (_ns * 1000.0)/153.6;
#endif
    return _ns;
}

uint32_t io_get_fpga_sys_time(void)
{
    uint32_t _sec = 0;
    uint32_t stime = 0;
#ifdef GET_CURRENT_TIME
    _sec = GET_CURRENT_TIME(get_fpga_reg());
#endif
    //uint32_t y , d;
	uint32_t h, m, s;
    uint32_t yue;
    struct tm stm;
    memset (&stm,0,sizeof(stm));
   // y = ((_sec & 0xfc000000) >> 26 );
   //d = (_sec & 0x03fe0000) >> 17;
    h = (_sec & 0x01f000) >> 12;
    m = (_sec & 0x0fc0) >> 6;
    s =  _sec & 0x3f;

    time_t tnow;
    tnow = time(0);
    struct tm *tm_sys;
    tm_sys = localtime(&tnow);
     stm.tm_year = tm_sys->tm_year;
     stm.tm_mon = tm_sys->tm_mon;
     stm.tm_mday = tm_sys->tm_mday;
    stm.tm_hour = h;
    stm.tm_min  = m;
    stm.tm_sec  = s;
    stime = mktime(&stm);

    return stime;
}



void io_set_fpga_sample_ctrl(uint8_t val)
{

    printf_note("[**FPGA**] ifch=%d\n", val);
#ifdef SET_SYS_IF_CH
    SET_SYS_IF_CH(get_fpga_reg(),(val == 0 ? 0 : 1));
#endif
}

int32_t io_set_audio_volume(uint32_t ch,uint8_t volume)
{
    if(reg_get()->misc && reg_get()->misc->set_audio_volume)
        reg_get()->misc->set_audio_volume(ch, volume);
    return 0;
}

static void io_set_dma_SPECTRUM_out_en(int ch, int subch, uint32_t trans_len,uint8_t continuous)
{
    printf_note("SPECTRUM out enable: ch[%d]output en, trans_len=%u, continuous=%d\n",ch, trans_len, continuous);

    if(reg_get()->iif &&reg_get()->iif->set_fft_channel)
        reg_get()->iif->set_fft_channel(ch, 1, NULL);

    if((get_spm_ctx()!=NULL) && get_spm_ctx()->ops->stream_start){
        if(ch_bitmap_weight(CH_TYPE_FFT) == 0){
            printf_note("FFT Stream Start\n");
            get_spm_ctx()->ops->stream_start(ch, subch, trans_len*sizeof(fft_t), continuous, STREAM_FFT);
        } else {
            printf_note("FFT Stream already Started\n");
        }
    }

}


static void io_set_dma_adc_out_en(int ch, int subch, uint32_t trans_len,uint8_t continuous)
{
    printf_debug("ADC out enable: output en\n");
    if((get_spm_ctx()!= NULL) && get_spm_ctx()->ops->stream_start)
        get_spm_ctx()->ops->stream_start(ch, subch, trans_len, continuous, STREAM_ADC_READ);
}

static void io_set_dma_adc_out_disable(int ch, int subch)
{
    printf_debug("ADC out disable\n");
    if((get_spm_ctx()!=NULL) && get_spm_ctx()->ops->stream_stop)
        get_spm_ctx()->ops->stream_stop(ch, subch, STREAM_ADC_READ);
}


static void io_set_dma_SPECTRUM_out_disable(int ch, int subch)
{
    if(reg_get()->iif &&reg_get()->iif->set_fft_channel)
        reg_get()->iif->set_fft_channel(ch, 0, NULL);
    if((get_spm_ctx()!= NULL) && get_spm_ctx()->ops->stream_stop){
        if(ch_bitmap_weight(CH_TYPE_FFT) == 0){
            printf_note("FFT Stream Stop\n");
            get_spm_ctx()->ops->stream_stop(ch, subch, STREAM_FFT);
        }
    }
}

/* 窄带iq禁止 */
static void io_set_NIQ_out_disable(int ch, int subch)
{
    if(subch >= 0){
        io_set_subch_onoff(ch, subch, 0);
        subch_bitmap_clear(subch, CH_TYPE_IQ);
    }
    if(subch_bitmap_weight(CH_TYPE_IQ) == 0 && (get_spm_ctx()!=NULL) && get_spm_ctx()->ops->stream_stop){
        get_spm_ctx()->ops->stream_stop(-1, subch, STREAM_NIQ);
    }
}

/* 窄带iq使能 */
static void io_set_NIQ_out_en(int ch, int subch, uint32_t trans_len,uint8_t continuous)
{
    if((get_spm_ctx()!=NULL) && get_spm_ctx()->ops->stream_start){
        get_spm_ctx()->ops->stream_start(-1, subch, trans_len, continuous, STREAM_NIQ);
    }
    if(subch >= 0){
        io_set_subch_onoff(ch, subch, 1);
        subch_bitmap_set(subch, CH_TYPE_IQ);
    }
}

/* 宽带iq使能 */
static void io_set_BIQ_out_en(int ch, int subch, uint32_t trans_len,uint8_t continuous)
{
    if((get_spm_ctx()!=NULL) && get_spm_ctx()->ops->stream_start)
        get_spm_ctx()->ops->stream_start(ch, subch, trans_len, continuous, STREAM_BIQ);
    if(reg_get()->iif && reg_get()->iif->set_biq_channel)
        reg_get()->iif->set_biq_channel(ch, NULL);

    if(ch >= 0){
        ch_bitmap_set(ch, CH_TYPE_IQ);
    }

}

/* 宽带iq禁止 */
static void io_set_BIQ_out_disable(int ch, int subch)
{
    if(ch >= 0){
        //io_set_subch_onoff(ch, subch, 0);
        ch_bitmap_clear(ch, CH_TYPE_IQ);
    }
    if((get_spm_ctx()!=NULL) && get_spm_ctx()->ops->stream_stop){
        get_spm_ctx()->ops->stream_stop(ch, subch, STREAM_BIQ);
    }
}

void io_set_xdma_disable(int ch, int subch)
{
    printf_note("ch:%d Xdma out disable\n", ch);
#ifndef DEBUG_TEST
    if(reg_get()->iif && reg_get()->iif->set_channel)
        reg_get()->iif->set_channel(ch , 0);
    else
        printf_warn("please realize set channel in reg_if.c\n");
    if((get_spm_ctx()!=NULL) && get_spm_ctx()->ops->stream_stop)
        get_spm_ctx()->ops->stream_stop(ch, subch, STREAM_XDMA_READ);
#endif
}

void io_set_xdma_enable(int ch, int subch)
{
    printf_note("ch:%d Xdma out enable\n", ch);
#ifndef DEBUG_TEST
    if(reg_get()->iif && reg_get()->iif->set_channel)
        reg_get()->iif->set_channel(ch , 1);
    else
        printf_warn("please realize set channel in reg_if.c\n");
    if((get_spm_ctx()!=NULL) && get_spm_ctx()->ops->stream_start)
        get_spm_ctx()->ops->stream_start(ch, subch, 0, 0, STREAM_XDMA_READ);
#endif
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
        case PSD_CON_MODE_ENABLE:
        {
            io_set_dma_SPECTRUM_out_en(ch, subc_ch, fftsize,1);
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
            subc_ch = CONFIG_AUDIO_CHANNEL;
            if(fftsize == 0)
                io_set_NIQ_out_en(ch, subc_ch,512, 1);
            else
                io_set_NIQ_out_en(ch, subc_ch,fftsize, 1);
            break;
        }
        case AUDIO_MODE_DISABLE:
        {
            subc_ch = CONFIG_AUDIO_CHANNEL;
            io_set_NIQ_out_disable(ch, subc_ch);
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
                io_set_NIQ_out_en(ch, subc_ch, poal_config->ctrl_para.iq_data_length, 1);
            }  
            else
                io_set_NIQ_out_en(ch, subc_ch, fftsize, 1);
            break;
        }
        case IQ_MODE_DISABLE:
        {
            io_set_NIQ_out_disable(ch, subc_ch);
            break;
        }
        case BIQ_MODE_ENABLE:
        {
            if(fftsize == 0)
            {
                if(poal_config->ctrl_para.iq_data_length == 0)
                {
                    printf_warn("iq_data_length not set, use 512\n");
                    poal_config->ctrl_para.iq_data_length = 512;
                }    
                io_set_BIQ_out_en(ch, subc_ch, poal_config->ctrl_para.iq_data_length, 1);
            }  
            else
                io_set_BIQ_out_en(ch, subc_ch, fftsize, 1);
            break;
        }
        case BIQ_MODE_DISABLE:
        {
            io_set_BIQ_out_disable(ch, subc_ch);
            break;
        }
        case XDMA_MODE_ENABLE:
        {
            io_set_xdma_enable(ch, subc_ch);
            break;
        }
        case XDMA_MODE_DISABLE:
        {
            io_set_xdma_disable(ch, subc_ch);
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
#ifdef GET_BROAD_AGC_THRESH
    agc_val = GET_BROAD_AGC_THRESH(get_fpga_reg(), ch);
#endif
    return agc_val;
}


int16_t io_get_adc_temperature(void)
{
    float result=0; 

#ifdef CONFIG_ARCH_ARM
    static FILE * fp = NULL;
    int raw_data;

    fp = fopen ("/sys/bus/iio/devices/iio:device0/in_temp0_ps_temp_raw", "r");
    if(!fp){
        //printf("Open file error!\n");
        return -1;
    }
    rewind(fp);
    fscanf(fp, "%d", &raw_data);
    fclose(fp);
    printf_debug("temp: %d\n", raw_data);
    result = ((raw_data * 509.314)/65536.0) - 280.23;
#endif

    return (signed short)result;
}
void io_get_fpga_status(void *args)
{
    struct arg_s{
        uint32_t temp;
        float vol;
        float current;
        uint32_t resources;
    };
    struct arg_s  *param = args;
    uint32_t reg_status = 0, reg_dup;
    
    #define STATUS_MASK (0x03ff)
    #define STATUS_BIT (10)
#if defined(GET_SYS_FPGA_STATUS)
    reg_status = GET_SYS_FPGA_STATUS(get_fpga_reg());
#endif
    //reg_status = get_fpga_reg()->system->fpga_status;
    printf_debug("reg_status=0x%x\n", reg_status);
    reg_dup = reg_status & STATUS_MASK; /* temp */
    param->temp = (uint32_t)((reg_dup * 503.93)/1024.0) - 280.23;
    printf_debug("temp=0x%x, %u\n", reg_dup, param->temp);
    reg_dup = (reg_status >> STATUS_BIT)&STATUS_MASK; /* fpga_int_vol */
    param->vol = 3.0* reg_dup/1024.0;
    printf_debug("fpga_int_vol=0x%x, %f\n", reg_dup, param->vol);
    reg_dup = (reg_status >> (STATUS_BIT*2))&STATUS_MASK; /* fpga_bram_vol */
    param->current = 3.0* reg_dup/1024.0;
    printf_debug("fpga_bram_vol=0x%x, %f\n", reg_dup, param->current);
    param->resources = 65;
    
}
void io_get_board_power(void *args)
{
    struct arg_s{
        float v;
        float i;
    };
    struct arg_s *power = args;
    
    uint32_t reg_status = 0;
#if defined(GET_SYS_FPGA_BOARD_VI)
    reg_status = GET_SYS_FPGA_BOARD_VI(get_fpga_reg());
    printf_debug("0x%x\n", reg_status);
#endif
    power->v = 24.845*(reg_status&0x3ff)/1024.0;
    power->i = 8.585*((reg_status >> 10)&0x3ff)/1024.0;
    printf_debug("power.v=%f, power.i=%f\n", power->v, power->i);
}

void io_set_gps_status(bool is_ok)
{
    FILE * fp = NULL;
    char *status = (is_ok == true ? "ok" : "false");
    bool ret;
    fp = fopen ("/run/status/gps", "w");
    if(!fp){
        printf_err("Open file error!\n");
        return;
    }
    rewind(fp);
    fwrite((void *)status, 1,  strlen(status), fp);
    fclose(fp);
    if(is_ok){
#ifdef GPS_LOCKED
        GPS_LOCKED();
#endif
    }
    else{
#ifdef GPS_UNLOCKED
        GPS_UNLOCKED();
#endif
    }
}
void io_set_rf_status(bool is_ok)
{
    FILE * fp = NULL;
    char *status = (is_ok == true ? "ok" : "false");
    bool ret;
    fp = fopen ("/run/status/rf", "w");
    if(!fp){
        printf_err("Open file error!\n");
        return;
    }
    rewind(fp);
    fwrite((void *)status, 1,  strlen(status), fp);
    fclose(fp);
}
bool io_get_adc_status(void *args)
{
    bool ret = false;
#if 0
    #define STATUS_ADC_OK  "ok"
    FILE * fp = NULL;
    char status[32];
    args = args;
        fp = fopen ("/run/status/adc", "r");
        if(!fp){
            printf_err("Open file error!\n");
            return false;
    }
    rewind(fp);
    fscanf(fp, "%s", status);
    fclose(fp);
    status[sizeof(status) -1] = 0;
    printf_debug("adc status: %s\n", status);
    if(strncmp(STATUS_ADC_OK, status, sizeof(STATUS_ADC_OK)) == 0){
        ret = true;
    }else{
        ret = false;
    }
#else
    if(reg_get()->iif && reg_get()->iif->get_adc_status)
        ret = reg_get()->iif->get_adc_status();
#endif
    return ret;
}

bool io_get_clock_status(void *args)
{
    #define STATUS_IN_CLK     "inner_clock"
    #define STATUS_OUT_CLK    "external_clock"
    #define STATUS_LOCKED     "locked"
    #define STATUS_NO_LOCKED  "no_locked"
    
    FILE * fp = NULL;

    /**** stack smashing detected ***: <unknown> terminated; then exit
      if we use uint8 lock_ok=0, external_clk=0;
    */
    //int lock_ok=0, external_clk=0;
    char external_clk[32], lock_ok[32];
    bool ret = false;
    
        fp = fopen ("/run/status/clock", "r");
        if(!fp){
            printf_err("Open file error!\n");
            return false;
    }
    
    rewind(fp);
    fscanf(fp, "%s %s", external_clk, lock_ok);
    fclose(fp);
    
    external_clk[sizeof(external_clk) -1] = 0;
    lock_ok[sizeof(lock_ok) -1] = 0;
    printf_debug("external_clk:%s, lock_ok: %s\n", external_clk, lock_ok);

    if(strncmp(external_clk, STATUS_OUT_CLK, sizeof(STATUS_OUT_CLK)) == 0){
        *(uint8_t *)args = 1;       /* out */
    }else{
        *(uint8_t *)args = 0;       /* in */
    }
    
    if(strncmp(lock_ok, STATUS_LOCKED, sizeof(STATUS_LOCKED)) == 0){
        ret = true;        /* locked */
    }else{
        ret = false;       /* unlocked */
    }

    return ret;
}

/*  1: out clock 0: in clock */
bool io_get_inout_clock_status(void *args)
{
    bool ret = false, adc_ok = false;
    bool stat = false;

    if(io_get_adc_status(NULL) == false){
        *(uint8_t *)args = 0;
        adc_ok = false;
    }else{
        int16_t temperature = -1;
#ifdef CONFIG_DEVICE_RF
        struct rf_ctx *ctx = get_rfctx();
            if(ctx && ctx->ops->get_status)
                stat = ctx->ops->get_status(0);
#endif
        if(!stat){
            /* GET status from FPGA  false */
            io_get_clock_status(args);
        } else{
            /* GET clk status from RF  */
            if(reg_get()->rf && reg_get()->rf->get_ext_clk)
                ret = reg_get()->rf->get_ext_clk(0, 0);
            *(uint8_t *)args = (((ret & 0x01) == 0) ? 1 : 0);
        }
        adc_ok = true;
    }
    
    return adc_ok;
}

uint32_t get_fpga_version(void)
{
    int32_t ret = 0;
    uint32_t args = 0;
#ifdef GET_SYS_FPGA_VER
    args=GET_SYS_FPGA_VER(get_fpga_reg());
#endif
    return args;
}

int16_t io_get_signal_strength(uint8_t ch)
{
#ifdef GET_NARROW_SIGNAL_VAL
    return GET_NARROW_SIGNAL_VAL(get_fpga_reg(), ch);
#endif
    return 0;
}


/* ---Disk-related function--- */
void io_set_backtrace_mode(int ch, bool args)
{
    if(args){
        if(reg_get()->misc && reg_get()->misc->set_ssd_mode)
            reg_get()->misc->set_ssd_mode(ch, 1);
    }else{
        if(reg_get()->misc && reg_get()->misc->set_ssd_mode)
            reg_get()->misc->set_ssd_mode(ch, 0);
    }
}


int32_t io_start_backtrace_file(void *arg){
    int32_t ret = 0;

    int ch;
    ch = *(int*)arg;
    io_set_backtrace_mode(ch, true);
    if((get_spm_ctx()!= NULL) && get_spm_ctx()->ops->stream_start)
        get_spm_ctx()->ops->stream_start(ch, 0, 0x1000, 1, STREAM_ADC_WRITE);
    
    return ret;
}

int32_t io_stop_backtrace_file(void *arg){
    int32_t ret = 0;
    int ch;
    ch = *(int*)arg;
    io_set_backtrace_mode(ch, false);
    if((get_spm_ctx()!=NULL) &&  get_spm_ctx()->ops->stream_stop)
        get_spm_ctx()->ops->stream_stop(ch, 0, STREAM_ADC_WRITE);
    
    return ret;
}

#ifdef CONFIG_ARCH_ARM
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
    return 0;
}

void io_init(void)
{
    printf_info("io init!\n");
}



