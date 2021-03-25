#ifndef __REG_H__
#define __REG_H__

#include <stdint.h>

#define FPGA_REG_DEV "/dev/mem"

#include <stdint.h>
#include "../../utils/utils.h"
#include "../../log/log.h"
#include "../../protocol/oal/poal.h"

#ifndef MHZ
#define MHZ(x) ((long long)(x*1000000.0+0.5))
#endif
#ifndef GHZ
#define GHZ(x) ((long long)(x*1000000000.0+0.5))
#endif

#define FPGA_REG_BASE           0xb0000000
#define FPGA_SYSETM_BASE        FPGA_REG_BASE
#define FPGA_ADC_BASE 	        (FPGA_REG_BASE+0x1000)
#define FPGA_SIGNAL_BASE 	    (FPGA_REG_BASE+0x2000)
#define FPGA_BRAOD_BAND_BASE    (FPGA_REG_BASE+0x3000)
#define FPGA_NARROR_BAND_BASE   (FPGA_REG_BASE+0x4000)
#define FPGA_RF_BASE            (FPGA_REG_BASE+0x0100)
#define FPGA_AUDIO_BASE   		FPGA_REG_BASE

#define SYSTEM_CONFG_REG_OFFSET	0x0
#define SYSTEM_CONFG_REG_LENGTH 0x100
#define BROAD_BAND_REG_OFFSET 	0x100
#define BROAD_BAND_REG_LENGTH 	0x100
#define NARROW_BAND_REG_BASE 	0xa0001000
#define NARROW_BAND_REG_LENGTH 	0x100
#define NARROW_BAND_CHANNEL_MAX_NUM 17
#define CONFG_REG_LEN 0x100
#define CONFG_AUDIO_OFFSET       0x800

#ifndef MAX_SCAN_FREQ_HZ
#define MAX_SCAN_FREQ_HZ (6000000000)
#endif


#include "platform.h"

typedef struct _SYSTEM_CONFG_REG_
{
	uint32_t version;
	uint32_t system_reset;
    uint32_t reserve1[0x2];
    uint32_t fpga_status;
    uint32_t board_vi;
    uint32_t reserve2[0x6];
    uint32_t if_ch;
    uint32_t ssd_mode;

}SYSTEM_CONFG_REG;

typedef struct _ADC_REG_
{   
    uint32_t reserve[0x20];
	uint32_t DDS_REF;
	uint32_t RESET;
	uint32_t DEC;
	uint32_t dc_offset_cal;
    uint32_t reserve1[0xc];
    uint32_t STATE;
}ADC_REG;


typedef struct _BROAD_BAND_REG_
{
	uint32_t signal_carrier;
	uint32_t enable;
	uint32_t band;
	uint32_t fft_mean_time;
	uint32_t fft_smooth_type;
	uint32_t fft_calibration;
	uint32_t reserves;
	uint32_t fft_lenth;
	uint32_t reserve;
	uint32_t agc_thresh;
	uint32_t reserves1[0x3];
	uint32_t fir_coeff;
}BROAD_BAND_REG;

typedef struct _NARROW_BAND_REG_
{
	uint32_t signal_carrier;
	uint32_t enable;
	uint32_t band;
    uint32_t reserve[0x5];
    uint32_t sigal_val;
	uint32_t reserve2[0x3];
	uint32_t decode_type;
	uint32_t fir_coeff;
	uint32_t noise_level;
	uint32_t audio_cfg;
}NARROW_BAND_REG;


typedef struct _SIGNAL_REG_
{
	uint32_t reserve;
	uint32_t data_path_reset;
    uint32_t reserve1[0x3];
    uint32_t trig;
    uint32_t reserve2[0x7];
    uint32_t reserve3[0xf];
    uint32_t current_time;
    uint32_t current_count;
    uint32_t trig_time;
    uint32_t trig_count;
}SIGNAL_REG;

typedef struct _RF_REG_
{
    uint32_t freq_khz;      //频率控制
    uint32_t rf_minus;       //射频衰减
    uint32_t midband_minus;  //中频衰减
    uint32_t rf_mode;        //射频模式
    uint32_t mid_band;       //中频带宽
    uint32_t reserve[0x2];
    uint32_t input;          //输入选择：0 为外部射频输入，1 为校正源输入
    uint32_t direct_control; //直采控制数据
    uint32_t revise_minus;   //校正衰减控制数据
    uint32_t direct_minus;   //直采衰减控制数据
    uint32_t  temperature;    //射频温度
    uint32_t clk_lock;       //时钟锁定
    uint32_t in_out_clk;     //内外时钟
}RF_REG;

typedef struct _AUDIO_REG_
{
    uint32_t volume;      //音量大小控制       byte 0 = 0x9a; byte 1 = 0xc0 + 31*dat/100;  dat为0-100的音量值
}AUDIO_REG;

typedef struct _FPGA_CONFIG_REG_
{
	SYSTEM_CONFG_REG *system;
    ADC_REG         *adcReg;
    SIGNAL_REG      *signal;
    RF_REG          *rfReg[MAX_RF_NUM];
    AUDIO_REG		*audioReg;
	BROAD_BAND_REG  *broad_band[BROAD_CH_NUM];
	NARROW_BAND_REG *narrow_band[NARROW_BAND_CHANNEL_MAX_NUM];
}FPGA_CONFIG_REG;

/*****system*****/
/*GET*/
#define GET_SYS_FPGA_VER(reg)				(reg->system->version)
#define GET_SYS_FPGA_STATUS(reg)			(reg->system->fpga_status)
#define GET_SYS_FPGA_BOARD_VI(reg)			(reg->system->board_vi)
/*SET*/
#define SET_SYS_RESET(reg,v) 				(reg->system->system_reset=v)
#define SET_SYS_IF_CH(reg,v) 				(reg->system->if_ch=v)
#define SET_SYS_SSD_MODE(reg,v) 			(reg->system->ssd_mode=v)

/*****broad band*****/
/*GET*/
#define GET_BROAD_AGC_THRESH(reg, ch) 			(reg->broad_band[0]->agc_thresh)
/*SET*/
#define SET_BROAD_SIGNAL_CARRIER(reg,v, ch) 	(reg->broad_band[0]->signal_carrier=v)
#define SET_BROAD_ENABLE(reg,v, ch) 			(reg->broad_band[0]->enable=v)
#define SET_BROAD_BAND(reg,v, ch) 				(reg->broad_band[0]->band=v)
#define SET_BROAD_FIR_COEFF(reg,v, ch) 			(reg->broad_band[0]->fir_coeff=v)
/*fft smooth */
#define SET_FFT_SMOOTH_TYPE(reg,v, ch) 			(reg->broad_band[0]->fft_smooth_type=v)
#define SET_FFT_MEAN_TIME(reg,v, ch) 			(reg->broad_band[0]->fft_mean_time=v)
#define SET_FFT_CALIB(reg,v, ch) 				(reg->broad_band[0]->fft_calibration=v)
#define SET_FFT_FFT_LEN(reg,v, ch) 				(reg->broad_band[0]->fft_lenth=v)
/* NIQ */
#define SET_NIQ_SIGNAL_CARRIER(reg,v, ch) 		(reg->broad_band[1]->signal_carrier=v)
#define SET_NIQ_BAND(reg,v, ch) 				(reg->broad_band[1]->band=v)

/*****narrow band*****/
/*GET*/
#define GET_NARROW_SIGNAL_VAL(reg,id) 		(reg->narrow_band[id]->sigal_val)
/*SET*/
#define SET_NARROW_BAND(reg,id,v) 			(reg->narrow_band[id]->band=v)
#define SET_NARROW_FIR_COEFF(reg,id,v) 		(reg->narrow_band[id]->fir_coeff=v)
#define SET_NARROW_DECODE_TYPE(reg,id,v) 	(reg->narrow_band[id]->decode_type=v)
//#define SET_NARROW_ENABLE(reg,id,v) 		(reg->narrow_band[id]->enable=v)
#define SET_NARROW_SIGNAL_CARRIER(reg,id,v) (reg->narrow_band[id]->signal_carrier=v)
#define SET_NARROW_NOISE_LEVEL(reg,id,v) 	(reg->narrow_band[id]->noise_level=v)

/* others */
#define SET_CURRENT_TIME(reg,v)			    (reg->signal->current_time=v)
#define SET_DATA_RESET(reg,v)			    (reg->signal->data_path_reset=v)
#define SET_CURRENT_COUNT(reg,v)		    (reg->signal->current_count=v)
#define SET_TRIG_TIME(reg,v)			    (reg->signal->trig_time=v)
#define SET_TRIG_COUNT(reg,v)			    (reg->signal->trig_count=v)

#define AUDIO_REG(reg)                      (&reg->audioReg->volume)

static inline void _set_narrow_channel(FPGA_CONFIG_REG *reg, int ch, int subch, int enable)
{
    uint32_t _reg;

    _reg = enable &0x01;
    reg->narrow_band[subch]->enable = _reg;
}

static inline uint64_t _get_middle_freq_by_channel(int ch, void *args)
{
    /*
    ch   中心频率（MHz）
    0	165
    1	280
    2	395
    3	510
*/
    struct  freq_rang_t{
        int ch;
        uint64_t m_freq;
    }freq_rang [] = {
            {0, MHZ(165)},
            {1, MHZ(280)},
            {2, MHZ(395)},
            {3, MHZ(510)},
        };
    int _ch = ch;
    
    args = args;
    for(int i = 0; i< ARRAY_SIZE(freq_rang); i++){
        if(ch == freq_rang[i].ch)
            return freq_rang[i].m_freq;
    }
    //printf_warn("ch[%d]can't find frequeny, use default:%"PRIu64"Hz\n",  freq_rang[0].m_freq);
    return freq_rang[0].m_freq;
}

static inline uint64_t _get_middle_freq(int ch, uint64_t rang_freq, void *args)
{
    /*
    ch	 频段范围（MHz）
    0	108～228
    1	220～340
    2	335～455
    3	450～570
    */
    struct	freq_rang_t{
        int ch;
        uint64_t s_freq;
        uint64_t e_freq;
    }freq_rang [] = {
        {0, MHZ(105), MHZ(222.5)},//105~225
        {1, MHZ(222.5), MHZ(337.5)},//220~340
        {2, MHZ(337.5), MHZ(452.5)},//335~455
        {3, MHZ(452.5), MHZ(570)},//450~570
    };
        
    args = args;
    for(int i = 0; i< ARRAY_SIZE(freq_rang); i++){
        if(rang_freq >= freq_rang[i].s_freq && rang_freq < freq_rang[i].e_freq){
            return _get_middle_freq_by_channel(freq_rang[i].ch,NULL);
        }
    }
    return 0;
}


static inline void _set_channel(FPGA_CONFIG_REG *reg, int ch, void *args)
{
/*
    ch   频段范围（MHz）
    0	108～228
    1	220～340
    2	335～455
    3	450～570
*/
    struct  freq_rang_t{
        int ch;
        uint64_t s_freq;
        uint64_t e_freq;
    }freq_rang [] = {
            {0, MHZ(108), MHZ(228)},
            {1, MHZ(220), MHZ(340)},
            {2, MHZ(335), MHZ(455)},
            {3, MHZ(450), MHZ(570)},
        };
    uint32_t _reg;
    uint32_t val;
    int _ch = ch;

    if(args != NULL && _ch < 0){
        uint64_t freq = *(uint64_t *)args;
        for(int i = 0; i< ARRAY_SIZE(freq_rang); i++){
            if(freq >= freq_rang[i].s_freq && freq <= freq_rang[i].e_freq){
                _ch = freq_rang[i].ch;
                break;
            }
        }
    }

    _reg = reg->broad_band[0]->enable;
    val = _ch << 8;
    _reg = (_reg & 0x00ff)|val;
    reg->broad_band[0]->enable = _reg;
    //printf_note("[ch:%d]set reg: 0x%x, val=0x%x 0x%x\n", ch,  _reg, val, reg->broad_band[0]->enable);
}

/*
    @back:1 playback  0: normal
*/
static inline void _set_ssd_mode(FPGA_CONFIG_REG *reg, int ch,int back)
{
    uint32_t _reg = 0;
#if 0
    if(ch >= 0)
        _reg = ((back &0x01) << ch);
    else
        _reg = back &0x01;
#endif
    if(back)
        _reg = 0x1;
    else
        _reg = 0x0;
    reg->system->ssd_mode = _reg;
}


extern pthread_mutex_t rf_param_mutex[MAX_RF_NUM];


static inline void set_rf_safe(int ch, uint32_t *reg, uint32_t val)
{
    pthread_mutex_lock(&rf_param_mutex[ch]);
    *reg = val;
    usleep(100);
    pthread_mutex_unlock(&rf_param_mutex[ch]);
}

static inline uint32_t get_rf_safe(int ch, uint32_t *reg)
{
    pthread_mutex_lock(&rf_param_mutex[ch]);
    uint32_t val;
    val = *reg;
    usleep(500);
    pthread_mutex_unlock(&rf_param_mutex[ch]);
    return val;
}


/* RF */
/*GET*/
/* 
@ch: rf control channel
@index: a channel may have multiple RF controls
*/
static inline int32_t _reg_get_rf_temperature(int ch, int index, FPGA_CONFIG_REG *reg)
{
#define RF_TEMPERATURE_FACTOR 0.0625
    int16_t  rf_temp_reg = 0;
    uint32_t rf_temperature, tye_count = 0;
    bool read_ok = false, time_out = false;

    index = index;
    if(reg->rfReg[ch] == NULL || ch >= MAX_RF_NUM)
        return 0;

    do{
        rf_temp_reg = get_rf_safe(ch, &reg->rfReg[ch]->temperature);
        /* if temperature <30 or > 100, we think read false; try again 
          NOTE: reg*0.0625
        */
        if(rf_temp_reg > 1600 || rf_temp_reg < 480 || rf_temp_reg == 0){
            read_ok = false;
        }else{
            read_ok = true;
        }
        if(++tye_count > 4)
            time_out = true;
    }while(!read_ok && !time_out);
    printf_debug("read ch=%d,rf_temperature = 0x%x, tye_count=%d, read_ok=%d\n",
                ch, rf_temp_reg,  tye_count, read_ok);
    
    if(read_ok == false)
        return -1;
    
    rf_temperature = rf_temp_reg * RF_TEMPERATURE_FACTOR;
    return rf_temperature;
}


/* 获取射频不同工作模式下放大倍数 
    @ch: 工作通道
    @index: 射频模式
    @args: 通道参数指针
*/
static inline int32_t _get_rf_magnification(int ch, int index,void *args, uint64_t mid_freq)
{
    struct poal_config *config = args;

    if(config == NULL || ch >= MAX_RADIO_CHANNEL_NUM || index >= RF_MODE_NUMBER)
        return 0;

    if(ch == 0)
       return config->channel[ch].rf_para.rf_mode.mag[index];
    else
        return 0;
}


static inline bool _reg_get_rf_ext_clk(int ch, int index, FPGA_CONFIG_REG *reg)
{
    int32_t  inout = 0;
    bool is_ext = false;

    index = index;
    if(reg->rfReg[ch] == NULL || ch >= MAX_RADIO_CHANNEL_NUM)
        return false;
    
    //inout = reg->rfReg[ch]->in_out_clk;
    inout = get_rf_safe(ch, &reg->rfReg[ch]->in_out_clk);
    inout = get_rf_safe(ch, &reg->rfReg[ch]->in_out_clk);
    //inout = reg->rfReg[ch]->in_out_clk;
    /*  1: out clock 0: in clock */
    is_ext = (((inout & 0x01) == 0) ? false : true);
    
    return is_ext;
}

static inline bool _reg_get_rf_lock_clk(int ch, int index, FPGA_CONFIG_REG *reg)
{
    int32_t  lock = 0;
    bool is_lock = false;

    index = index;
    if(reg->rfReg[ch] == NULL || ch >= MAX_RADIO_CHANNEL_NUM)
        return false;
    
    lock = get_rf_safe(ch, &reg->rfReg[ch]->clk_lock);
    lock = get_rf_safe(ch, &reg->rfReg[ch]->clk_lock);
    
    is_lock = (lock == 0 ? false : true);
    return is_lock;
}

/* RF */
/*SET*/
static inline void _reg_set_rf_frequency(int ch, int index, uint64_t freq_hz, FPGA_CONFIG_REG *reg)
{
    uint32_t freq_khz = freq_hz/1000;
    set_rf_safe(ch, &reg->rfReg[ch]->freq_khz, freq_khz);
    set_rf_safe(ch, &reg->rfReg[ch]->freq_khz, freq_khz);
}

static inline void _reg_set_rf_bandwidth(int ch, int index, uint32_t bw_hz, FPGA_CONFIG_REG *reg)
{
    int i, found = 0;
    uint8_t set_val = 0;
    if(ch >= MAX_RADIO_CHANNEL_NUM)
        return;
    /* 400KH(0) 4MHz(0x01) 40MHz(0x02) 200MHz(0x03)  */
    struct rf_bw_reg{
        uint32_t bw_hz;
        uint8_t reg_val;
    };
    struct rf_bw_reg _reg[] = {
            {400000,    0x00},
            {4000000,   0x01},
            {40000000,  0x02},
            {200000000, 0x03},
    };

    for(i = 0; i < ARRAY_SIZE(_reg); i++){
        if(bw_hz == _reg[i].bw_hz){
            set_val = _reg[i].reg_val;
            found ++;
        }
    }
    if(found == 0){
        printf("NOT found bandwidth %uHz in tables,use default[200Mhz]\n", bw_hz);
        set_val = 0x03; /* default 200MHz */
    }
    
    if(reg->rfReg[ch] == NULL)
        return;
    
    set_rf_safe(ch, &reg->rfReg[ch]->mid_band, set_val);
    set_rf_safe(ch, &reg->rfReg[ch]->mid_band, set_val);
}

static inline void _reg_set_rf_mode_code(int ch, int index, uint8_t code, FPGA_CONFIG_REG *reg)
{
        int i, found = 0;
        uint8_t set_val = 0;
        /*这里需要转换 =>常规模式(0) 低噪声模式(0x01) 低失真模式(0x02)    */
        struct _reg{
            uint8_t mode;
            uint8_t reg_val;
        };
        
        struct _reg _reg[] = {
                {0,     0x02},
                {1,     0x00},
                {2,     0x01},
        };

        if(ch >= MAX_RADIO_CHANNEL_NUM)
            return;
        
        for(i = 0; i < ARRAY_SIZE(_reg); i++){
            if(code == _reg[i].mode){
                set_val = _reg[i].reg_val;
                found ++;
            }
        }
        if(found == 0){
            printf("NOT found noise mode %uHz in tables,use default normal mode[0]\n", code);
            set_val = 0x00; /* default normal mode */
        }
        
        if(reg->rfReg[ch] == NULL)
            return;

        set_rf_safe(ch, &reg->rfReg[ch]->rf_mode, set_val);
        set_rf_safe(ch, &reg->rfReg[ch]->rf_mode, set_val);
}

static inline void _reg_set_rf_mgc_gain(int ch, int index, uint8_t gain, FPGA_CONFIG_REG *reg)
{
    if(ch >= MAX_RADIO_CHANNEL_NUM)
        return;
    
    if(gain > 30)
        gain = 30;
    else if(gain < 0)
        gain = 0;
    
    if(reg->rfReg[ch] == NULL)
        return;

    set_rf_safe(ch, &reg->rfReg[ch]->midband_minus, gain);
    set_rf_safe(ch, &reg->rfReg[ch]->midband_minus, gain);
}


static inline void _reg_set_rf_attenuation(int ch, int index, uint8_t atten, FPGA_CONFIG_REG *reg)
{
    if(ch >= MAX_RADIO_CHANNEL_NUM)
        return;
    if(atten > 30)
        atten = 30;
    else if(atten < 0)
        atten = 0;
    
    if(reg->rfReg[ch] == NULL)
        return;
    
    set_rf_safe(ch, &reg->rfReg[ch]->rf_minus, atten);
    set_rf_safe(ch, &reg->rfReg[ch]->rf_minus, atten);
}

static inline void _reg_set_rf_cali_source_attenuation(int ch, int index, uint8_t level, FPGA_CONFIG_REG *reg)
{
    /*  射频模块-90～-30dBm 功率可调，步进 1dB 
        接收机校正衰减有效范围为 寄存器0 dB～60dB
    */
    int reg_val;
    index = index;
    
    if(ch >= MAX_RADIO_CHANNEL_NUM)
        return;
    
    if(level > -30)
        level = -30;
    else if(level < -90)
        level = -90;
    level = -level;
    reg_val = level - 30;

    if(reg->rfReg[ch] == NULL)
        return;
    
    set_rf_safe(ch, &reg->rfReg[ch]->revise_minus, reg_val);
    set_rf_safe(ch, &reg->rfReg[ch]->revise_minus, reg_val);
}

static inline void _reg_set_rf_direct_sample_ctrl(int ch, int index, uint8_t val, FPGA_CONFIG_REG *reg)
{
    uint8_t data;

    
    index = index;
    if(ch >= MAX_RADIO_CHANNEL_NUM)
        return;
    data = (val == 0 ? 0 : 0x01);
    /* 0关闭直采，1开启直采 */
    if(reg->rfReg[ch] == NULL)
        return;
    set_rf_safe(ch, &reg->rfReg[ch]->direct_control, data);
    set_rf_safe(ch, &reg->rfReg[ch]->direct_control, data);
}

static inline void _reg_set_rf_cali_source_choise(int ch, int index, uint8_t val, FPGA_CONFIG_REG *reg)
{
    int _reg;
    if(ch >= MAX_RADIO_CHANNEL_NUM)
        return;
    
    /* 0 为外部射频输入，1 为校正源输入 */
    _reg = (val == 0 ? 0 : 1);
    if(reg->rfReg[ch] == NULL)
        return;
    set_rf_safe(ch, &reg->rfReg[ch]->input, _reg);
    set_rf_safe(ch, &reg->rfReg[ch]->input, _reg);
}

//#平台信息查询回复命令(0x17)
static inline int _reg_get_fpga_info(int id, void **args)
{
    uint8_t data[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                             0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00};
    uint8_t *ptr;
    ptr = malloc(sizeof(data));
    if(ptr == NULL){
        printf_err("malloc err\n");
        return -1;
    }
    memcpy(ptr, data, sizeof(data));

    *args = (void *)ptr;
    return sizeof(data);
}

//读取板卡信息回复(0x13)
static inline int _reg_get_board_info(int id, void **args)
{
    uint8_t data[] = {0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x77, 0xba, 
        0xf6, 0x45, 0x1a, 0x81, 0x5e, 0x6d, 0xa5, 0x32, 0x9b, 0x52, 0xfa, 0x2d, 0xe3, 0x05, 
        0x02, 0xd9, 0xe3, 0x26, 0x9e, 0x28, 0x7f, 0xb0, 0x26, 0x30, 0x46, 0xd4, 0xb1, 0x45,
        0x07, 0x43, 0x62, 0x0b, 0x5c, 0x9d, 0x95, 0xe1, 0x35, 0xc2, 0x9c, 0x7d, 0xde, 0xcb, 
        0xf9, 0x45, 0x12, 0x06, 0x5b, 0xe8, 0x11, 0xcc, 0x4f, 0x85, 0x1c, 0x43, 0x23, 0x00, 
        0x87, 0xf4, 0xaa, 0x09, 0x3b, 0x2d, 0xae, 0x4e, 0x5f, 0xdd, 0x20, 0x64, 0xcd, 0xbe,
        0xc5, 0x25, 0x89, 0x08, 0xfa, 0x4a, 0x82, 0xc7, 0x0c, 0x2c, 0x2b, 0x6d, 0xbb, 0x3c,
        0x91, 0xec, 0xa3, 0xd1, 0x1b, 0x14, 0x8d, 0xbc, 0xc3, 0x51, 0xaa, 0x58, 0x6b, 0xb5, 
        0x6d, 0x67, 0xb5, 0xf9, 0x54, 0xd4, 0x24, 0xb9, 0x77, 0x84, 0xce, 0xb9, 0xda, 0x2f,
        0xc2, 0xef, 0x8b, 0x6d, 0x66, 0xd9, 0x99, 0xbc, 0x68, 0x6b, 0x17, 0x85, 0xb7, 0x53, 
        0x88, 0xef, 0xaa, 0x53, 0x6f, 0xab, 0xc9, 0x57, 0xf0, 0x33, 0x92, 0x9f, 0xb5, 0xe0, 
        0xbc, 0x6f, 0x76, 0x99, 0xfc, 0xe5, 0x1f, 0x6c, 0xf6, 0xb1, 0xda, 0x93, 0x73, 0xc1, 
        0xe9, 0xf7, 0x15, 0x4e, 0xf1, 0xe0, 0xdd, 0x89, 0x1e, 0x10, 0x62, 0x94, 0x19, 0x76, 
        0xa9, 0x68, 0xc7, 0xc1, 0x77, 0x56, 0x69, 0x23, 0x24, 0x20, 0x7e, 0x3f, 0x6f, 0x67, 
        0x30, 0x23, 0x5e, 0xb9, 0x12, 0x65, 0x0d, 0x57, 0x3b, 0x9a, 0x53, 0xea, 0xb3, 0xba, 
        0xe5, 0x97, 0x4d, 0xb7, 0x3a, 0xff, 0x35, 0x6b, 0x59, 0x66, 0xe5, 0x70, 0x7f, 0xc3, 
        0x8e, 0x5a, 0x11, 0xb0, 0xef, 0x0d, 0xb5, 0x44, 0xb5, 0xda, 0x21, 0xc0, 0x91, 0x40, 
        0xd5, 0x9e, 0x9a, 0x92, 0x77, 0x1f, 0x58, 0x7b, 0xad, 0xad, 0x4c, 0xf6, 0x81, 0x3e, 
        0xb6, 0xda, 0xbf, 0x8b, 0x8f, 0xa9, 0x4c, 0xbc, 0x65, 0x88, 0x5f, 0x88, 0xc9, 0x42, 
        0x2d, 0x47, 0x76, 0xc5, 0xde, 0xc3, 0x42, 0x6b, 0xb9, 0xef, 0xad, 0x59, 0x1c, 0x67, 
        0x09, 0xc4, 0x69, 0x18, 0xdf, 0xc5, 0x9d, 0x71, 0x56, 0xb6, 0xe5, 0xd4, 0xc0, 0x8f, 
        0x68, 0xc9, 0x52, 0x11, 0x27, 0x57, 0x15, 0xdc, 0xc2, 0xf5, 0x1e, 0x97, 0x60, 0xb7, 
        0x53, 0x51, 0x6d, 0x0b, 0xc2, 0xd7, 0xe7, 0xd3, 0xa6, 0xe8, 0x5c, 0xf4, 0xfb, 0x2c, 
        0xac, 0xbd, 0xf1, 0x6a, 0xbc, 0xc4, 0xd8, 0x53, 0x1d};
    
    uint8_t *ptr;
    ptr = malloc(sizeof(data));
    if(ptr == NULL){
        printf_err("malloc err\n");
        return -1;
    }
    memcpy(ptr, data, sizeof(data));

    *args = (void *)ptr;
    return sizeof(data);
    
}

static inline int _get_set_load(void)
{
    return 0;
}

extern FPGA_CONFIG_REG *get_fpga_reg(void);
extern void fpga_io_init(void);
extern void fpga_io_close(void);
#endif

