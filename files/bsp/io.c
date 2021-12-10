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



/* 窄带表：解调方式为IQ系数表 */
static struct  band_table_t iq_nbandtable[] ={
    {985540, 0, 5000, 1.28},
    {198608, 0, 25000, 1.28},
    {197608, 0, 50000, 1.28},
    {197108, 0, 100000, 1.28},
    {196941, 0, 150000, 1.281281},
    {196808, 0, 250000, 1.28},
    {196708, 0, 500000, 1.28},
    {196658, 0, 1000000, 1.28},
    {196628, 0, 2500000, 1.28},
    {196618, 0, 5000000, 1.28},
    {196613, 0, 10000000, 1.28},
    {65541, 0, 20000000, 1.28},
    {5,      0, 40000000, 1.28},
}; 

/* 窄带表：解调方式为非IQ系数表 */
static struct  band_table_t nbandtable[] ={
    {198608, 128,   600, 0},
    {198608, 128,   1500, 0},
    {198608, 129,   2400, 0},
    {198608, 131,   6000, 0},
    {198608, 133,   9000, 0},
    {198608, 135,   12000, 0},
    {198608, 137,   15000, 0},
    {197608, 0,     30000, 0},
    {197108, 0,     50000, 0},
    {196858, 0,     100000, 0},
    {196733, 0,     150000, 0},
#if 0
    {198208, 130,   5000},
    {197408, 128,   25000},
    {197008, 128,   50000},
    {196808, 0,     100000},
    {196708, 0,     250000},
    {196658, 0,     500000},
#endif
}; 
/* 宽带系数表 */
static struct  band_table_t bandtable[] ={
#if 0//defined(SUPPORT_PROJECT_WD_XCR)
    {458752, 0, 25000000},
    {196608, 0, 50000000},
    {65536,  0, 100000000},
    {0,      0, 200000000},
#else
    {196632, 0,  1250000, 1.28},
    {196620, 0,  2500000, 1.28},
    {196614, 0,  5000000, 1.28},
    {65542, 0,  10000000, 1.28},
    {1114112, 0, 20000000, 1.28},
    {1048576, 0, 40000000, 1.28},
    {65536, 0,   60000000, 1.28},
#endif
}; 

static DECLARE_BITMAP(subch_bmp[CH_TYPE_MAX], MAX_SIGNAL_CHANNEL_NUM);
static DECLARE_BITMAP(ch_bmp[CH_TYPE_MAX], MAX_RADIO_CHANNEL_NUM);
static DECLARE_BITMAP(cl_sock_bmp, MAX_CLINET_SOCKET_NUM);
static DECLARE_BITMAP(cards_status_bmp, MAX_FPGA_CARD_SLOT_NUM);

void cards_status_bitmap_init(void)
{
    bitmap_zero(cards_status_bmp, MAX_FPGA_CARD_SLOT_NUM);
}

void cards_status_set(int index)
{
    set_bit(index, cards_status_bmp);
}

void cards_status_clear(int index)
{
    clear_bit(index, cards_status_bmp);
}

size_t cards_status_weight(void)
{
    return bitmap_weight(cards_status_bmp, MAX_FPGA_CARD_SLOT_NUM);
}

bool cards_status_test(int bit)
{
    return test_bit(bit, cards_status_bmp);
}

uint32_t get_cards_status(void)
{
    uint32_t mask = 0;
    size_t bit = 0;
    for_each_set_bit(bit, cards_status_bmp, MAX_FPGA_CARD_SLOT_NUM){
        mask |= (1<<bit);
    }
    printf_note("mask=%x\n", mask);
    return mask;
}

const unsigned long *cards_status_get_bitmap(void)
{
    return cards_status_bmp;
}


void socket_bitmap_init(void)
{
    bitmap_zero(cl_sock_bmp, MAX_CLINET_SOCKET_NUM);
}

void socket_bitmap_set(int index)
{
    set_bit(index, cl_sock_bmp);
}

void socket_bitmap_clear(int index)
{
    clear_bit(index, cl_sock_bmp);
}

size_t socket_bitmap_weight(void)
{
    return bitmap_weight(cl_sock_bmp, MAX_CLINET_SOCKET_NUM);
}


ssize_t socket_bitmap_find_index(void)
{
    return find_first_zero_bit(cl_sock_bmp, MAX_CLINET_SOCKET_NUM);
}

const unsigned long *socket_get_bitmap(void)
{
    return cl_sock_bmp;
}


void io_socket_set_id(int id)
{
    id = id;
}


void io_socket_set_sub(int id, uint16_t chip_id, uint16_t func_id, uint16_t port)
{
    id = id;
    chip_id = chip_id;
    func_id = func_id;
    port = port;
}


void io_socket_set_unsub(int id, uint16_t chip_id, uint16_t func_id, uint16_t port)
{
    
}



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


float io_get_narrowband_iq_factor(uint32_t bindwidth)
{
    int i;
    float factor = 1.28;
    for(i = 0; i < ARRAY_SIZE(iq_nbandtable); i++){
        if(bindwidth == iq_nbandtable[i].band){
            factor = iq_nbandtable[i].factor;
            break;
        }
    }
    return factor;
}

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

void io_reset_fpga_data_link(void){
    #define RESET_ADDR      0x04U
    int32_t ret = 0;
    int32_t data = 0;

    printf_debug("[**REGISTER**]Reset FPGA\n");
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2) 
    #if defined(SUPPORT_SPECTRUM_FPGA)
    SET_DATA_RESET(get_fpga_reg(),1);
//    get_fpga_reg()->system->data_path_reset = 1;
    #endif
#endif
#endif /* SUPPORT_PLATFORM_ARCH_ARM */
}

/*设置带宽因子*/
int32_t io_set_bandwidth(uint32_t ch, uint32_t bandwidth){
    int32_t ret = 0;
    uint32_t set_factor = 0,band_factor = 0, filter_factor = 0;
    static int32_t old_ch=-1;
    static uint32_t old_val=0;
    struct  band_table_t *table;
    uint32_t table_len = 0;

    table= bandtable;
    table_len = sizeof(bandtable)/sizeof(struct  band_table_t);
    io_get_bandwidth_factor(bandwidth, &band_factor,&filter_factor, table, table_len);
    if((old_val == band_factor) && (ch == old_ch)){
        /* 避免重复设置相同参数 */
        return ret;
    }
    old_val = band_factor;
    old_ch = ch;
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2) 
    #if defined(SUPPORT_SPECTRUM_FPGA)
    //get_fpga_reg()->broad_band->band = band_factor;
    SET_BROAD_BAND(get_fpga_reg(),band_factor, ch);
    #endif
#endif
#endif /* SUPPORT_PLATFORM_ARCH_ARM */
    printf_debug("[**REGISTER**]ch:%d, Set Bandwidth:%u,band_factor=0x%x,set_factor=0x%x\n", ch, bandwidth,band_factor,set_factor);

    return ret;

}

int32_t io_set_noise(uint32_t ch, uint32_t noise_en,int8_t noise_level_tmp){
    uint32_t  noise_level;
    if(noise_en == 1)
    {
        if(noise_level_tmp > 0)
        {
            noise_level_tmp = 0;
        }
        noise_level =  (uint32_t)(16000.0 * pow((double)10.0, (double)(noise_level_tmp / 20.0)));
    }
    else 
    {
        noise_level = 0;
    }
    printf_note("[**REGISTER**]ch:%d, Set noise_level:%u noise_en:%d noise_level_tmp:%d\n", ch,noise_level,noise_en,noise_level_tmp);
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_FPGA)
    SET_NARROW_NOISE_LEVEL(get_fpga_reg(),ch,noise_level);
#endif
#endif
    return 0;
}


/*设置解调带宽因子*/
int32_t io_set_dec_bandwidth(uint32_t ch, uint32_t dec_bandwidth){
    int32_t ret = 0;
    uint32_t set_factor, band_factor, filter_factor;
    static int32_t old_ch=-1;
    static uint32_t old_val=0;
    struct  band_table_t *table;
    uint32_t table_len = 0;

    table= bandtable;
    table_len = sizeof(bandtable)/sizeof(struct  band_table_t);
    io_get_bandwidth_factor(dec_bandwidth, &band_factor,&filter_factor, table, table_len);
    set_factor = band_factor;
    if((old_val == set_factor) && (ch == old_ch)){
        /* 避免重复设置相同参数 */
        return ret;
    }
    old_val = set_factor;
    old_ch = ch;
    printf_note("[**REGISTER**]ch:%d, Set Decode Bandwidth:%u, band_factor=0x%x, set_factor=0x%x, %u\n", ch, dec_bandwidth, band_factor, set_factor, set_factor);
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2) 
    #if defined(SUPPORT_SPECTRUM_FPGA)
   // get_fpga_reg()->narrow_band[ch]->band = band_factor;   //0x30190
    SET_NIQ_BAND(get_fpga_reg(),band_factor, ch);
    #endif
    printf_note("broard_band[%d]->band:%x\n",ch,band_factor);
#endif
#endif
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
    }else if(dec_method == IO_DQ_MODE_CW) {
        d_method = 0x0000003;
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
    printf_note("[**REGISTER**]ch:%d, Set Decode method:%u, d_method=0x%x\n", ch, dec_method, d_method);
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2) 
    #if defined(SUPPORT_SPECTRUM_FPGA)
    SET_NARROW_DECODE_TYPE(get_fpga_reg(),ch,dec_method);;
    #endif
    //get_fpga_reg()->narrow_band[ch]->decode_type = dec_method;
#endif
#endif
    return ret;

}



/*解调中心频率计算（需要根据中心频率计算）*/
static uint32_t io_set_dec_middle_freq_reg(uint64_t dec_middle_freq, uint64_t middle_freq)
{
        /* delta_freq = (reg* 204800000)/2^32 ==>  reg= delta_freq*2^32/204800000 */
#if defined(SUPPORT_PROJECT_WD_XCR) || defined(SUPPORT_PROJECT_WD_XCR_40G)
#define FREQ_MAGIC1 (256000000)
#elif defined(SUPPORT_PROJECT_TF713_4CH)
#define FREQ_MAGIC1 (153600000)
#else
#define FREQ_MAGIC1 (204800000)
#endif
#define FREQ_MAGIC2 (0x100000000ULL)  /* 2^32 */
    
        uint64_t delta_freq;
        uint32_t reg;
        int32_t ret = 0;
#if defined(SUPPORT_DIRECT_SAMPLE)
#define FREQ_ThRESHOLD_HZ (100000000)
#define MID_FREQ_MAGIC1 (128000000)
#define DIRECT_FREQ_THR (200000000)
        if(dec_middle_freq< DIRECT_FREQ_THR ){
            uint32_t reg;
            int32_t val;
            if(dec_middle_freq >= MID_FREQ_MAGIC1){
                reg = (dec_middle_freq - MID_FREQ_MAGIC1)*FREQ_MAGIC2/FREQ_MAGIC1;
            }else{
                val = dec_middle_freq - MID_FREQ_MAGIC1;
                reg = (val+FREQ_MAGIC1)*FREQ_MAGIC2/FREQ_MAGIC1;
            }
            printf_debug("feq:%"PRIu64", reg=0x%x\n", dec_middle_freq, reg);
            return reg;
        }
#endif
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
#if defined(SUPPORT_SPECTRUM_V2) 
    #if defined(SUPPORT_SPECTRUM_FPGA)
    SET_NIQ_SIGNAL_CARRIER(get_fpga_reg(),reg, ch);
    #endif
#endif
#endif
    printf_note("[**REGISTER**]ch:%d, MiddleFreq =%"PRIu64",Decode MiddleFreq:%"PRIu64",reg=0x%x\n", ch, middle_freq, dec_middle_freq, reg);
    return ret;
}


uint64_t io_get_raw_sample_rate(uint32_t ch, uint64_t middle_freq)
{
    return get_clock_frequency();
    
}
int32_t io_set_middle_freq(uint32_t ch, uint64_t middle_freq, uint64_t rel_mfreq)
{
#if defined(SUPPORT_DIRECT_SAMPLE)
#define DIRECT_FREQ_THR (200000000) /* 直采截止频率 */
#define FREQ_MAGIC2 (0x100000000ULL)  /* 2^32 */
#define FREQ_MAGIC1 (256000000)
#define MID_FREQ_MAGIC1 (128000000)/* 直采采样频率 */
    uint32_t reg = 0;
    if(middle_freq < DIRECT_FREQ_THR ){
        int32_t val;
        if(middle_freq >= MID_FREQ_MAGIC1){
            reg = (middle_freq - MID_FREQ_MAGIC1)*FREQ_MAGIC2/FREQ_MAGIC1;
        }else{
            val = middle_freq - MID_FREQ_MAGIC1;
            printf_note("val:%d\n", val);
            reg = (val+FREQ_MAGIC1)*FREQ_MAGIC2/FREQ_MAGIC1;
        }
    }
#else
    uint32_t reg = io_set_dec_middle_freq_reg(rel_mfreq, middle_freq);
    #if defined(SUPPORT_SPECTRUM_FPGA)
    SET_BROAD_SIGNAL_CARRIER(get_fpga_reg(),reg, ch);
    #endif
    printf_debug("ch:%d, freq:%"PRIu64", reg=0x%x\n", ch, middle_freq, reg);
    printf_debug(">>>>>>ch:%d, feq:%"PRIu64",rel_middle_freq=%"PRIu64", reg=0x%x\n",ch, middle_freq,rel_mfreq,  reg);
#endif
    return 0;
}


/*设置子通道解调中心频率因子*/
int32_t io_set_subch_dec_middle_freq(uint32_t subch, uint64_t dec_middle_freq, uint64_t middle_freq)
{
        uint32_t reg;
        int32_t ret = 0;
        
        reg = io_set_dec_middle_freq_reg(dec_middle_freq, middle_freq);
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2) 
        #if defined(SUPPORT_SPECTRUM_FPGA)
        SET_NARROW_SIGNAL_CARRIER(get_fpga_reg(),subch,reg);
        #endif
#endif
#endif
        printf_note("[**REGISTER**]ch:%d, SubChannel Set MiddleFreq =%"PRIu64", Decode MiddleFreq:%"PRIu64", reg=0x%x, ret=%d\n", subch, middle_freq, dec_middle_freq, reg, ret);
        return ret;
}

/*设置子通道开关*/
int32_t io_set_subch_onoff(uint32_t ch, uint32_t subch, uint8_t onoff)
{
    int32_t ret = 0;
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2) 
    #if defined(SUPPORT_SPECTRUM_FPGA)
    if(onoff){
        uint64_t midfreq = 0;
        config_read_by_cmd(EX_MID_FREQ_CMD, EX_SUB_CH_MID_FREQ, ch, &midfreq, subch);
        _set_niq_channel(get_fpga_reg(),subch, &midfreq, 0x01);
    }
    else{
        _set_niq_channel(get_fpga_reg(),subch, NULL, 0x00);
    }
    #else

    #endif
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
   // dec_method = IO_DQ_MODE_IQ; /* TEST */
    if(dec_method == IO_DQ_MODE_IQ){
        table= iq_nbandtable;
        table_len = sizeof(iq_nbandtable)/sizeof(struct  band_table_t);
        
    }else{
        table= nbandtable;
        table_len = sizeof(nbandtable)/sizeof(struct  band_table_t);
    }
    io_get_bandwidth_factor(bandwidth, &band_factor,&filter_factor, table, table_len);
    /*设置子通道带宽系数*/
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2) 
    #if defined(SUPPORT_SPECTRUM_FPGA)
    SET_NARROW_BAND(get_fpga_reg(),subch,band_factor);
    SET_NARROW_FIR_COEFF(get_fpga_reg(),subch,filter_factor);
    #endif
    //get_fpga_reg()->narrow_band[subch]->fir_coeff = filter_factor;
#endif
    printf_note("[**REGISTER**]ch:%d, SubChannle Set Bandwidth=%u, factor=0x%x[%u], filter_factor=0x%x[%u],dec_method=%d,table_len=%d, ret=%d\n",
                subch, bandwidth, band_factor, band_factor,filter_factor,filter_factor, dec_method,  table_len, ret);

#endif /* SUPPORT_PLATFORM_ARCH_ARM */
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
    
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2) 
    #if defined(SUPPORT_SPECTRUM_FPGA)
    //get_fpga_reg()->narrow_band[subch]->decode_type = d_method;
    SET_NARROW_DECODE_TYPE(get_fpga_reg(),subch,d_method);
    #endif
#endif
#endif
    return ret;

}

void io_set_subch_audio_sample_rate(uint32_t ch, uint32_t subch,  int rate)
{
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_FPGA)
    _set_narrow_audio_sample_rate(get_fpga_reg(), ch, subch, rate);
#endif
#endif
}

void io_set_subch_audio_gain_mode(uint32_t ch, uint32_t subch,  int mode)
{
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_FPGA)
    _set_narrow_audio_gain_mode(get_fpga_reg(), ch, subch, mode);
#endif
#endif
}


void io_set_subch_audio_gain(uint32_t ch, uint32_t subch,  int gain)
{
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_FPGA)
    _set_narrow_audio_gain(get_fpga_reg(), ch, subch, (float)gain);
#endif
#endif
}

/* 设置加窗类型 */
void io_set_window_type(uint32_t ch, int type)
{
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_FPGA)
    uint16_t *data, *ptr;
    uint32_t reg_val = 0;
    size_t fsize = 0;
    data = config_get_fft_window_data(type, &fsize);
    fsize = fsize / sizeof(uint16_t);
    ptr = data;
    printf_note("fsize: %ld type=%d\n", fsize, type);
    if(data != NULL && fsize > 0){
        for(int i = 0; i < fsize; i += 4){
            /* reg_val: 高16bit为0,1,2..递增数，低16bit为读取窗文件抽取数据 */
            reg_val = *ptr + ((i / 4) << 16);
            ptr += 4;
            SET_FFT_WINDOW_TYPE(get_fpga_reg(), reg_val, ch);
        }
    }
#endif
#endif
}


/* 设置平滑类型 */
void io_set_smooth_type(uint32_t ch, int type)
{
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_FPGA)
    SET_FFT_SMOOTH_TYPE(get_fpga_reg(),type, ch);
#endif
#endif
}

/* 设置平滑门限 */
void io_set_smooth_threshold(uint32_t ch, int val)
{
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_FPGA)
    SET_FFT_SMOOTH_THRESHOLD(get_fpga_reg(),val, ch);
#endif
#endif
}


/* 设置平滑数 */
void io_set_smooth_time(uint32_t ch, uint16_t stime)
{
    static uint16_t old_val[MAX_RADIO_CHANNEL_NUM] = {0};
    
    if(old_val[ch] == stime || ch >= MAX_RADIO_CHANNEL_NUM){
        /* 避免重复设置相同参数 */
        return;
    }
    old_val[ch] = stime;

    printf_note("[**REGISTER**]ch:%u, Set Smooth time: factor=%d[0x%x]\n",ch, stime, stime);
    
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2) 
    #if defined(SUPPORT_SPECTRUM_FPGA)
    //SET_FFT_SMOOTH_TYPE(get_fpga_reg(),0, ch);
    SET_FFT_MEAN_TIME(get_fpga_reg(),stime, ch);
    #elif defined(SUPPORT_SPECTRUM_CHIP) 
    if((get_spm_ctx() != NULL) && get_spm_ctx()->ops->set_smooth_time)
        get_spm_ctx()->ops->set_smooth_time(stime);
    #endif
#endif
#endif
}



/* 设置FPGA校准值 */
void io_set_calibrate_val(uint32_t ch, int32_t  cal_value)
{
    static int32_t old_val[MAX_RADIO_CHANNEL_NUM] = {0};

    if(old_val[ch] == cal_value || ch >= MAX_RADIO_CHANNEL_NUM){
        /* 避免重复设置相同参数 */
        return;
    }
    old_val[ch] = cal_value;
    printf_note("[**REGISTER**][ch=%d]Set Calibrate Val factor=%d[0x%x]\n",ch, cal_value,cal_value);
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2) 
    #if defined(SUPPORT_SPECTRUM_FPGA)
    SET_FFT_CALIB(get_fpga_reg(),(uint32_t)cal_value, ch);
    #elif defined(SUPPORT_SPECTRUM_CHIP) 
    if((get_spm_ctx()!=NULL) && get_spm_ctx()->ops->set_calibration_value)
        get_spm_ctx()->ops->set_calibration_value(cal_value);
    #endif
#endif
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
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2) 
    #if defined(SUPPORT_SPECTRUM_FPGA)

    #elif defined(SUPPORT_SPECTRUM_CHIP) 
        #ifdef SUPPORT_RF_ADRV
            adrv_set_rx_gain(gain_val&0xff);
        #endif
    #endif
#endif
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
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2) 
    #if defined(SUPPORT_SPECTRUM_FPGA)

    #elif defined(SUPPORT_SPECTRUM_CHIP) 
        #ifdef SUPPORT_RF_ADRV
            adrv_set_rx_dc_offset(val&0xff);
        #endif
    #endif
#endif
#endif
}

void io_set_rf_calibration_source_level(int level)
{
    
    printf_note("calibration source level = %d\n", level);
#if defined(SUPPORT_SPECTRUM_FPGA)
    _reg_set_rf_cali_source_attenuation(0, 0, level, get_fpga_reg());
#endif
}
void io_set_rf_calibration_source_enable(int ch, int enable)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    if((ch >= MAX_RADIO_CHANNEL_NUM) || (ch < 0))
        return;
    
#if defined(SUPPORT_SPECTRUM_FPGA)
    _reg_set_rf_cali_source_choise(ch, 0, enable, get_fpga_reg());
#endif
    usleep(300);
    poal_config->channel[ch].enable.cali_source_en = (enable == 0 ? 0 : 1);
    printf_debug("cali_source_en: 0x%x\n", poal_config->channel[ch].enable.cali_source_en);
}
bool is_rf_calibration_source_enable(void)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    int ch = poal_config->cid;
    bool enable = false;
    enable = poal_config->channel[ch].enable.cali_source_en & 0x01;
    enable = (enable == 0 ? false : true);
    printf_debug("calibration source enable: %s\n", (enable==true ? "true" : "false"));
    return enable;
}

void io_set_fft_size(uint32_t ch, uint32_t fft_size)
{
    uint32_t factor;
    static uint32_t old_value[MAX_RADIO_CHANNEL_NUM] = {0};

    if(old_value[ch] == fft_size || ch >= MAX_RADIO_CHANNEL_NUM){
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
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2) 
    #if defined(SUPPORT_SPECTRUM_FPGA)
    SET_FFT_FFT_LEN(get_fpga_reg(),factor, ch);
    //get_fpga_reg()->broad_band->fft_lenth = factor;
    #endif
#endif
#endif
}

void io_set_fpga_sys_time(uint32_t time)
{
#if defined(SUPPORT_SPECTRUM_V2) 
    #if defined(SUPPORT_SPECTRUM_FPGA)
    SET_CURRENT_TIME(get_fpga_reg(),time);
    //get_fpga_reg()->signal->current_time = time;
    #endif
#endif
}

uint32_t io_get_fpga_sys_ns_time(void)
{
    uint32_t _ns = 0;
#if defined(SUPPORT_SPECTRUM_V2) 
    #if defined(SUPPORT_SPECTRUM_FPGA)
    _ns = GET_CURRENT_COUNT(get_fpga_reg());
    _ns = _ns & 0x0fffffff;
    _ns = (_ns * 1000.0)/153.6;
    #endif
#endif
    return _ns;
}

uint32_t io_get_fpga_sys_time(void)
{
    uint32_t _sec = 0;
    uint32_t stime = 0;
#if defined(SUPPORT_SPECTRUM_V2) 
    #if defined(SUPPORT_SPECTRUM_FPGA)
    _sec = GET_CURRENT_TIME(get_fpga_reg());
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
     stm.tm_hour = h ;
     stm.tm_min  = m;
     stm.tm_sec  = s;
  // printf_note(" y=%lu,m=%lu,d =%lu,h=%lu,m=%lu, s=%lu\n",stm.tm_year + 1900 ,stm.tm_mon+1,stm.tm_mday,
                                                    //  stm.tm_hour,stm.tm_min, stm.tm_sec );

    stime = mktime(&stm);
  //  printf_note("tnow =%d mktime(&stm)=%ld\n", tnow,mktime(&stm));
    #endif
#endif
    return stime;
}
void io_set_fpga_sample_ctrl(uint8_t val)
{
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2)
    #if defined(SUPPORT_SPECTRUM_FPGA)
    printf_note("[**FPGA**] ifch=%d\n", val);
    SET_SYS_IF_CH(get_fpga_reg(),(val == 0 ? 0 : 1));
    //get_fpga_reg()->system->if_ch = (val == 0 ? 0 : 1);
    #endif
#endif
#endif
}

int32_t io_set_audio_volume(uint32_t ch,uint8_t volume)
{
#if defined(SUPPORT_SPECTRUM_FPGA)
     volume_set((intptr_t)AUDIO_REG(get_fpga_reg()), volume);
     //volume_set(get_fpga_reg()->audioReg,volume);
#endif
    return 0;
}

static void io_set_dma_SPECTRUM_out_en(int ch, int subch, uint32_t trans_len,uint8_t continuous)
{
    printf_debug("SPECTRUM out enable: ch[%d]output en, trans_len=%u\n",ch, trans_len);
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2)
    _set_fft_channel(get_fpga_reg(),ch, NULL);
    if((get_spm_ctx()!=NULL) && get_spm_ctx()->ops->stream_start){
        get_spm_ctx()->ops->stream_start(ch, subch, trans_len*sizeof(fft_t), continuous, STREAM_FFT);
    }
#endif
#endif
}


static void io_set_dma_adc_out_en(int ch, int subch, uint32_t trans_len,uint8_t continuous)
{
    printf_debug("ADC out enable: output en\n");
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2)
    if((get_spm_ctx()!= NULL) && get_spm_ctx()->ops->stream_start)
    get_spm_ctx()->ops->stream_start(ch, subch, trans_len, continuous, STREAM_ADC_READ);
#endif
#endif
}

static void io_set_dma_adc_out_disable(int ch, int subch)
{
    printf_debug("ADC out disable\n");
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2)
    if((get_spm_ctx()!=NULL) && get_spm_ctx()->ops->stream_stop)
    get_spm_ctx()->ops->stream_stop(ch, subch, STREAM_ADC_READ);
#endif
#endif
}


static void io_set_dma_SPECTRUM_out_disable(int ch, int subch)
{
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2)
    if((get_spm_ctx()!= NULL) && get_spm_ctx()->ops->stream_stop)
    get_spm_ctx()->ops->stream_stop(ch, subch, STREAM_FFT);
#endif
#endif

}

/* 窄带iq禁止 */
static void io_set_NIQ_out_disable(int ch, int subch)
{
    if(subch >= 0){
        io_set_subch_onoff(ch, subch, 0);
        subch_bitmap_clear(subch, CH_TYPE_IQ);
    }
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2) 
    if(subch_bitmap_weight(CH_TYPE_IQ) == 0 && (get_spm_ctx()!=NULL) && get_spm_ctx()->ops->stream_stop){
        get_spm_ctx()->ops->stream_stop(-1, subch, STREAM_NIQ);
    }
#endif 
#endif

}

/* 窄带iq使能 */
static void io_set_NIQ_out_en(int ch, int subch, uint32_t trans_len,uint8_t continuous)
{
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2) 
    if((get_spm_ctx()!=NULL) && get_spm_ctx()->ops->stream_start){
        get_spm_ctx()->ops->stream_start(-1, subch, trans_len, continuous, STREAM_NIQ);
    }
#endif
#endif
    if(subch >= 0){
        io_set_subch_onoff(ch, subch, 1);
        subch_bitmap_set(subch, CH_TYPE_IQ);
    }
}

/* 宽带iq使能 */
static void io_set_BIQ_out_en(int ch, int subch, uint32_t trans_len,uint8_t continuous)
{
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2) 
    if((get_spm_ctx()!=NULL) && get_spm_ctx()->ops->stream_start)
        get_spm_ctx()->ops->stream_start(ch, subch, trans_len, continuous, STREAM_BIQ);
#endif
#endif
    if(ch >= 0){
        //io_set_subch_onoff(ch, subch, 1);
        ch_bitmap_set(ch, CH_TYPE_IQ);
    }

}

/* 宽带iq禁止 */
static void io_set_BIQ_out_disable(int ch, int subch)
{
    if(subch >= 0){
        //io_set_subch_onoff(ch, subch, 0);
        ch_bitmap_clear(ch, CH_TYPE_IQ);
    }
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2) 
    if((get_spm_ctx()!=NULL) && get_spm_ctx()->ops->stream_stop){
        get_spm_ctx()->ops->stream_stop(ch, subch, STREAM_BIQ);
    }
#endif 
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
                //io_set_dma_DQ_out_en(ch, subc_ch, 512, 1);
                io_set_NIQ_out_en(ch, subc_ch,512, 1);
            else
                //io_set_dma_DQ_out_en(ch, subc_ch,fftsize, 1);
                io_set_NIQ_out_en(ch, subc_ch,fftsize, 1);
            break;
        }
        case AUDIO_MODE_DISABLE:
        {
            subc_ch = CONFIG_AUDIO_CHANNEL;
            //io_set_dma_DQ_out_dis(ch, subc_ch);
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
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2) 
    #if defined(SUPPORT_SPECTRUM_FPGA)
    agc_val = GET_BROAD_AGC_THRESH(get_fpga_reg(), ch);
    //agc_val = get_fpga_reg()->broad_band->agc_thresh;
    #endif
#endif
#endif
    return agc_val;
}


int16_t io_get_adc_temperature(void)
{
    float result=0; 

//#ifdef SUPPORT_PLATFORM_ARCH_ARM
    FILE * fp = NULL;
    int raw_data, raw_data2;

        //fp = fopen ("/sys/bus/iio/devices/iio:device0/in_temp0_ps_temp_raw", "r");
    fp = fopen ("/sys/devices/platform/PHYT0008:00/PHYT000D:00/hwmon/hwmon0/temp1_input", "r");
    if(!fp){
            printf("Open file error!\n");
            return -1;
    }
    rewind(fp);
    fscanf(fp, "%d", &raw_data);
    fclose(fp);
    
    fp = fopen ("/sys/devices/platform/PHYT0008:00/PHYT000D:00/hwmon/hwmon0/temp2_input", "r");
    if(!fp){
            printf("Open file error!\n");
            return -1;
    }
    rewind(fp);
    fscanf(fp, "%d", &raw_data2);
    fclose(fp);
    printf_note("raw_data=%d, raw_data2=%d\n", raw_data, raw_data2);
    raw_data = max(raw_data, raw_data2);
    result = raw_data/1000.0;
    printf_note("temp: %d, result: %f\n", raw_data, result);
    //result = ((raw_data * 509.314)/65536.0) - 280.23;
//#endif

    return (signed short)result;
}

int io_get_adc_temperature_ex(int16_t *temp)
{
    float result=0; 

//#ifdef SUPPORT_PLATFORM_ARCH_ARM
    FILE * fp = NULL;
    int raw_data, raw_data2;

        //fp = fopen ("/sys/bus/iio/devices/iio:device0/in_temp0_ps_temp_raw", "r");
    fp = fopen ("/sys/devices/platform/PHYT0008:00/PHYT000D:00/hwmon/hwmon0/temp1_input", "r");
    if(!fp){
            printf("Open file error!\n");
            return -1;
    }
    rewind(fp);
    fscanf(fp, "%d", &raw_data);
    fclose(fp);
    
    fp = fopen ("/sys/devices/platform/PHYT0008:00/PHYT000D:00/hwmon/hwmon0/temp2_input", "r");
    if(!fp){
            printf("Open file error!\n");
            return -1;
    }
    rewind(fp);
    fscanf(fp, "%d", &raw_data2);
    fclose(fp);
    printf_note("raw_data=%d, raw_data2=%d\n", raw_data, raw_data2);
    raw_data = max(raw_data, raw_data2);
    result = raw_data/1000.0;
    printf_note("temp: %d, result: %f\n", raw_data, result);
    *temp = (signed short)result;
    //result = ((raw_data * 509.314)/65536.0) - 280.23;
//#endif

    return 0;
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
    uint32_t reg_status, reg_dup;
#ifdef SUPPORT_PLATFORM_ARCH_ARM
    #if defined(SUPPORT_SPECTRUM_FPGA)
    #define STATUS_MASK (0x03ff)
    #define STATUS_BIT (10)
    reg_status = GET_SYS_FPGA_STATUS(get_fpga_reg());
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
#endif
#endif
}
void io_get_board_power(void *args)
{
    struct arg_s{
        float v;
        float i;
    };
    struct arg_s *power = args;
#ifdef SUPPORT_PLATFORM_ARCH_ARM
    #if defined(SUPPORT_SPECTRUM_FPGA)
    uint32_t reg_status;
    reg_status = GET_SYS_FPGA_BOARD_VI(get_fpga_reg());
    //reg_status = get_fpga_reg()->system->board_vi;
    power->v = 24.845*(reg_status&0x3ff)/1024.0;
    power->i = 8.585*((reg_status >> 10)&0x3ff)/1024.0;
    printf_debug("[0x%x]power.v=%f, power.i=%f\n", reg_status, power->v, power->i);
    #endif
#endif
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
    if(is_ok)
        GPS_LOCKED();
    else
        GPS_UNLOCKED();
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
#ifdef SUPPORT_PLATFORM_ARCH_ARM
    #if defined(SUPPORT_SPECTRUM_FPGA)
    ret = _get_adc_status(get_fpga_reg());
    #endif
#endif
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
    bool ret = false, is_lock_ok = false;
#if defined(SUPPORT_SPECTRUM_FPGA)
    if(io_get_adc_status(NULL) == false){
        *(uint8_t *)args = 0;
        is_lock_ok = false;
    }else{
        int32_t rf_temp =  _reg_get_rf_temperature(1, 0, get_fpga_reg());
        if(rf_temp < 0){
            /* GET status from FPGA  false */
            io_get_clock_status(args);
        } else{
            /* GET status from RF  */
            ret = _reg_get_rf_ext_clk(0, 0, get_fpga_reg());
            *(uint8_t *)args = (((ret & 0x01) == 0) ? 1 : 0);
            _reg_get_rf_lock_clk(0, 0, get_fpga_reg());
        }
        is_lock_ok = true;
    }
#endif
    return is_lock_ok;
}

uint32_t get_fpga_version(void)
{
    int32_t ret = 0;
    uint32_t args = 0;
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2) 
    #if defined(SUPPORT_SPECTRUM_FPGA)
    args=GET_SYS_FPGA_VER(get_fpga_reg());
    //args = get_fpga_reg()->system->version;
    #endif
#endif
#else
    args=1;
#endif
    return args;
}

int16_t io_get_signal_strength(uint8_t ch)
{
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2) 
    #if defined(SUPPORT_SPECTRUM_FPGA)
    return GET_NARROW_SIGNAL_VAL(get_fpga_reg(), ch);
    #endif
    return 0;
#endif
#endif
    return 0;
}


/* ---Disk-related function--- */
void io_set_backtrace_mode(int ch, bool args)
{
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2) 
    #if defined(SUPPORT_SPECTRUM_FPGA)
    if(args){
        _set_ssd_mode(get_fpga_reg(), ch, 1);
    }else{
        _set_ssd_mode(get_fpga_reg(), ch, 0);
    }
    #endif
#endif
#endif

}


int32_t io_start_backtrace_file(void *arg){
    int32_t ret = 0;
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2) 
    int ch;
    ch = *(int*)arg;
    io_set_backtrace_mode(ch, true);
    if((get_spm_ctx()!= NULL) && get_spm_ctx()->ops->stream_start)
        get_spm_ctx()->ops->stream_start(ch, 0, 0x1000, 1, STREAM_ADC_WRITE);
#endif
#endif
    return ret;
}

int32_t io_stop_backtrace_file(void *arg){
    int32_t ret = 0;
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_SPECTRUM_V2) 
    int ch;
    ch = *(int*)arg;
    io_set_backtrace_mode(ch, false);
    if((get_spm_ctx()!=NULL) &&  get_spm_ctx()->ops->stream_stop)
        get_spm_ctx()->ops->stream_stop(ch, 0, STREAM_ADC_WRITE);
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
    return 0;
}

void io_set_xdma_disable(int ch, int subch)
{
    printf_note("ch:%d Xdma out disable\n", ch);
    _set_xdma_channel(get_fpga_reg(), ch, 0);
    if((get_spm_ctx()!=NULL) && get_spm_ctx()->ops->stream_stop)
        get_spm_ctx()->ops->stream_stop(ch, subch, XDMA_STREAM);
}

void io_set_xdma_enable(int ch, int subch)
{
    printf_note("ch:%d Xdma out enable\n", ch);
    _set_xdma_channel(get_fpga_reg(), ch, 1);
    if((get_spm_ctx()!=NULL) && get_spm_ctx()->ops->stream_start)
        get_spm_ctx()->ops->stream_start(ch, subch, 0, 0, XDMA_STREAM);
}

bool io_get_xdma_fpga_status(void)
{
    uint8_t *info = NULL, *ptr;
    int nbyte = 0;
    bool ret = false;
    nbyte = _reg_get_fpga_info_(get_fpga_reg(), 0, (void **)&info);
    ptr = info;
    if(nbyte > 0){
        for(int i = 0; i < nbyte; i++)
            if(*ptr++ != 0){
                ret = true;
                break;
            }
    }
    safe_free(info);
    return ret;
}


void io_reg_rf_set_ndata(uint8_t *ptr, int len)
{
    if(ptr == NULL)
        return;
#ifdef SUPPORT_RF_SPI_M57
    for(int i = 0; i< len; i++)
        SET_RF_BYTE_DATA(get_fpga_reg(), *ptr++);
#endif
}

void io_reg_rf_start_tranfer(uint32_t len)
{
#ifdef SUPPORT_RF_SPI_M57
    SET_RF_BYTE_CMD(get_fpga_reg(), (uint16_t)len);
#endif
}

bool io_reg_rf_is_busy(void)
{
    bool busy = false;
#ifdef SUPPORT_RF_SPI_M57
    busy = ((GET_RF_STATUS(get_fpga_reg()) & 0x01) == 0 ? false : true);
#endif
    return busy;
}

void io_reg_rf_get_data(uint8_t *buffer, int len)
{
#ifdef SUPPORT_RF_SPI_M57
    uint8_t *ptr = buffer;
    if(buffer == NULL)
        return;
    for(int i = 0; i < len; i++)
        *ptr++ = GET_RF_BYTE_READ(get_fpga_reg());
#endif
}


/* 检测槽位是否有效 
    @chipid: 芯片id
    return true / false
*/
bool io_xdma_is_valid_chipid(int chipid)
{
    int id = 0;
    id = CARD_SLOT_NUM(chipid);
    return cards_status_test(id);
}

bool io_xdma_is_valid_addr(int slot_id)
{
    return _reg_is_fpga_addr_ok(get_fpga_reg(), slot_id,  NULL);
}

uint32_t io_xdma_get_slot_version(int slot_id)
{
    return _reg_get_fpga_version(get_fpga_reg(), slot_id, NULL);
}


void io_init(void)
{
    printf_info("io init!\n");
    socket_bitmap_init();
    cards_status_bitmap_init();
    //ch_bitmap_init();
    //subch_bitmap_init();
}



