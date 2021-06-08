#ifndef __REG_H__
#define __REG_H__

#include <stdint.h>

#define FPGA_REG_DEV "/dev/mem"

#include <stdint.h>
#include <math.h>
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
#define NARROW_BAND_CHANNEL_MAX_NUM MAX_SIGNAL_CHANNEL_NUM
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
	uint32_t fft_smooth_threshold;
	uint32_t fft_lenth;
	uint32_t amplitude;     /* 通道幅度值 */
	uint32_t agc_thresh;    /* RF AGC幅度值 */
	uint32_t reserves1[0x3];
	uint32_t fir_coeff;
	uint32_t reserves2[0x2];
	uint32_t fft_win;
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


#define GET_ADC_STATUS(reg)			        (reg->adcReg->STATE)

/*****broad band*****/
/*GET*/
#define GET_BROAD_AGC_THRESH(reg, ch) 			(reg->broad_band[ch+2]->agc_thresh)
/*SET*/
#define SET_BROAD_SIGNAL_CARRIER(reg,v, ch) 	(reg->broad_band[0]->signal_carrier=v)
#define SET_BROAD_ENABLE(reg,v, ch) 			(reg->broad_band[0]->enable=v)
#define SET_BROAD_BAND(reg,v, ch) 				(reg->broad_band[0]->band=v)
#define SET_BROAD_FIR_COEFF(reg,v, ch) 			(reg->broad_band[0]->fir_coeff=v)
/*fft smooth */
#define SET_FFT_SMOOTH_TYPE(reg,v, ch) 			(reg->broad_band[0]->fft_smooth_type=v)
#define SET_FFT_SMOOTH_THRESHOLD(reg,v, ch) 	(reg->broad_band[0]->fft_smooth_threshold=v)
#define SET_FFT_MEAN_TIME(reg,v, ch) 			(reg->broad_band[0]->fft_mean_time=v)
#define SET_FFT_CALIB(reg,v, ch) 				(reg->broad_band[0]->fft_calibration=v)
#define SET_FFT_FFT_LEN(reg,v, ch) 				(reg->broad_band[0]->fft_lenth=v)
#define SET_FFT_WINDOW_TYPE(reg,v, ch) 			(reg->broad_band[0]->fft_win=v)

/* NIQ */
#define SET_NIQ_SIGNAL_CARRIER(reg,v, ch) 		(reg->broad_band[1]->signal_carrier=v)
#define SET_NIQ_BAND(reg,v, ch) 				(reg->broad_band[1]->band=v)
#define SET_NIQ_ENABLE(reg,v, ch) 				(reg->broad_band[1]->enable=v)


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

#define GET_CURRENT_TIME(reg)			    (reg->signal->current_time)
#define GET_CURRENT_COUNT(reg)			    (reg->signal->current_count)

#define AUDIO_REG(reg)                      (&reg->audioReg->volume)

static inline bool _get_adc_status(FPGA_CONFIG_REG *reg)
{
    uint32_t _reg = GET_ADC_STATUS(reg);
    //printf_note("get adc status:0x%x\n", _reg);
    if ((_reg & 0x0f) == 0x0f) {
        return true;
    } else {
        return false;
    }
}
static inline void _set_narrow_channel(FPGA_CONFIG_REG *reg, int ch, int subch, int enable)
{
    uint32_t _reg;
    uint32_t val;
    val = reg->narrow_band[subch]->enable;
    _reg = (enable &0x01)|val;
    reg->narrow_band[subch]->enable = _reg;
    printf_debug("[subch:%d] _ch: %d, reg: 0x%x, val=0x%x enable:0x%x,%d\n", subch, ch, _reg, val, reg->narrow_band[subch]->enable, enable);
}

static inline void _set_narrow_audio_sample_rate(FPGA_CONFIG_REG *reg, int ch, int subch, int rate)
{
    struct  sample_rate_t{
        int val;
        uint32_t sample_rate_khz;
    }sample_rate_table [] = {
        {0, 32},
        {1, 16},
        {3, 8},
    };

    uint32_t _reg, _val;
    int found = 0;
    
    for(int i = 0; i< ARRAY_SIZE(sample_rate_table); i++){
        if(rate == sample_rate_table[i].sample_rate_khz){
            found ++;
            _val = sample_rate_table[i].val;
            break;
        }
    }
    if(found == 0){
        printf_warn("sample rate:%dkhz not find in table!",  rate);
        return;
    }
    
    _reg = reg->narrow_band[subch]->audio_cfg;
    _reg = _reg & 0x0ffff;
    printf_note("sample rate raw val = %dKhz[0x%x]\n",  rate, _val);
    _val = (_val << 16) & 0x0ff0000;
    _reg = _reg | _val;
    printf_note("[subch:%d]sample rate reg val: regval=0x%x, val = 0x%x\n", subch,  _reg, _val);
    reg->narrow_band[subch]->audio_cfg = _reg;
    
}

static inline void _set_narrow_audio_gain(FPGA_CONFIG_REG *reg, int ch, int subch, float gain)
{
    #define SUB_CH_MAX_GAIN_VAL 90//(57.855)
    #define SUB_CH_MIN_GAIN_VAL 0 //(-30.0)
    uint32_t _reg;
    int32_t _val;
    /*  
        gain: 0dbm~90dbm
        reg = 10^(gain/20); 
    */
    printf_note("set audio gain: raw gain = %f\n",  gain);
    if(f_sgn(gain-SUB_CH_MAX_GAIN_VAL) > 0){
        gain = SUB_CH_MAX_GAIN_VAL;
    } else if(f_sgn(gain-SUB_CH_MIN_GAIN_VAL) < 0){
        gain = SUB_CH_MIN_GAIN_VAL;
    }

    _reg = reg->narrow_band[subch]->audio_cfg;
    _reg = _reg & 0xff8000;
    _val = (int32_t)(pow(10, (float)gain/20.0));
    _reg = _reg | _val;
    printf_note("[subch:%d]set audio gain: %f, regval=0x%x, val = 0x%x\n", subch, gain, _reg, _val);
    reg->narrow_band[subch]->audio_cfg = _reg;
}


static inline void _set_narrow_audio_gain_mode(FPGA_CONFIG_REG *reg, int ch, int subch, int mode)
{
    #define SUB_CH_AGC_VAL 80.0//(57.855) /* 57dbm */
    uint32_t _reg, _val;
    _val = (mode == POAL_MGC_MODE ? 1 : 0); /* val=0: agc;  val=1, mgc */
    _reg = reg->narrow_band[subch]->audio_cfg;
    _reg = _reg & (~0x08000);
    _reg = _reg & 0x0ffffff;
    _val = (_val << 15) & 0x08000;
    _reg = _reg | _val;
    printf_note("[subch:%d]set audio gain mode: %s, regval=0x%x, val = 0x%x\n", subch, mode == POAL_MGC_MODE ? "mgc" : "agc", _reg, _val);
    reg->narrow_band[subch]->audio_cfg = _reg;
    if(mode == POAL_AGC_MODE){  /* agc */
        printf_note("AGC Mode,Set Agc gain: %fDbm\n", SUB_CH_AGC_VAL);
        _set_narrow_audio_gain(reg, ch, subch, SUB_CH_AGC_VAL);
    }
}




static inline void _set_niq_channel(FPGA_CONFIG_REG *reg, int subch, void *args, int enable)
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
        {0, MHZ(105), MHZ(222.5)},//105~225
        {1, MHZ(222.5), MHZ(337.5)},//220~340
        {2, MHZ(337.5), MHZ(452.5)},//335~455
        {3, MHZ(452.5), MHZ(570)},//450~570
        };
    uint32_t _reg;
    uint32_t val;
    int _ch = 0;

    if(args != NULL && subch >= 0){
        uint64_t freq = *(uint64_t *)args;
        for(int i = 0; i< ARRAY_SIZE(freq_rang); i++){
            if(freq >= freq_rang[i].s_freq && freq <= freq_rang[i].e_freq){
                _ch = freq_rang[i].ch;
                break;
            }
        }
    }
    if(enable)
        _reg = 0x01;
    else
        _reg = 0x00;
    val = _ch << 8;
    _reg = (_reg & 0x00ff)|val;
    reg->narrow_band[subch]->enable = _reg;
    
    if(enable)
        printf_debug("[subch:%d] _ch: %d, set reg: 0x%x, val=0x%x 0x%x, freq=%"PRIu64"\n", subch, _ch, _reg, val, reg->narrow_band[subch]->enable, *(uint64_t *)args);
    else
        printf_debug("[subch:%d] _ch: %d, set reg: 0x%x, val=0x%x 0x%x\n", subch, _ch, _reg, val, reg->narrow_band[subch]->enable);
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
            {0, MHZ(165) - MHZ(0.075)},
            {1, MHZ(280) - MHZ(0.1)},
            {2, MHZ(395) - MHZ(0.05)},
            {3, MHZ(510)},
        };
    int _ch = ch;
    
    args = args;
    for(int i = 0; i< ARRAY_SIZE(freq_rang); i++){
        if(ch == freq_rang[i].ch){
            return freq_rang[i].m_freq;
        }
            
    }
   // printf_warn("ch[%d]can't find frequeny, use default:%"PRIu64"Hz\n",  freq_rang[0].m_freq);
    return freq_rang[0].m_freq;
}


static inline uint64_t _get_rel_middle_freq_by_channel(int ch, void *args)
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
        if(ch == freq_rang[i].ch){
            return freq_rang[i].m_freq;
        }
            
    }
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
static inline int _get_channel_by_mfreq(uint64_t rang_freq, void *args)
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
            return freq_rang[i].ch;
        }
    }
    return 0;
}

static inline void _set_fft_channel(FPGA_CONFIG_REG *reg, int ch, void *args)
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
    printf_debug("[ch:%d]set reg: 0x%x, val=0x%x 0x%x\n", ch,  _reg, val, reg->broad_band[0]->enable);
}



static inline void _set_biq_channel(FPGA_CONFIG_REG *reg, int ch, void *args)
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

    _reg = reg->broad_band[1]->enable;
    val = _ch << 8;
    _reg = (_reg & 0x00ff)|val;
    reg->broad_band[1]->enable = _reg;
    printf_note(" BIQ [ch:%d]set reg: 0x%x, val=0x%x 0x%x\n", ch,  _reg, val, reg->broad_band[1]->enable);
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
                {1,     0x01},
                {2,     0x00},
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


extern FPGA_CONFIG_REG *get_fpga_reg(void);
extern void fpga_io_init(void);
extern void fpga_io_close(void);
#endif

