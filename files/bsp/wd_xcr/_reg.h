#ifndef __REG_H__
#define __REG_H__

#include <stdint.h>

#define FPGA_REG_DEV "/dev/mem"

#include <stdint.h>
#include "../../utils/utils.h"

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


#ifndef RF_ONE_CHANNEL_NUM
#define RF_ONE_CHANNEL_NUM 1            /* 一个通道射频数 */
#endif

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
    int32_t  temperature;    //射频温度
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
    RF_REG          *rfReg[RF_ONE_CHANNEL_NUM];
    AUDIO_REG		*audioReg;
	BROAD_BAND_REG  *broad_band;
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
#define GET_BROAD_AGC_THRESH(reg) 			(reg->broad_band->agc_thresh)
/*SET*/
#define SET_BROAD_SIGNAL_CARRIER(reg,v) 	(reg->broad_band->signal_carrier=v)
#define SET_BROAD_ENABLE(reg,v) 			(reg->broad_band->enable=v)
#define SET_BROAD_BAND(reg,v) 				(reg->broad_band->band=v)
#define SET_BROAD_FIR_COEFF(reg,v) 			(reg->broad_band->fir_coeff=v)
/*fft smooth */
#define SET_FFT_SMOOTH_TYPE(reg,v) 			(reg->broad_band->fft_smooth_type=v)
#define SET_FFT_MEAN_TIME(reg,v) 			(reg->broad_band->fft_mean_time=v)
#define SET_FFT_CALIB(reg,v) 				(reg->broad_band->fft_calibration=v)
#define SET_FFT_FFT_LEN(reg,v) 				(reg->broad_band->fft_lenth=v)

/*****narrow band*****/
/*GET*/
#define GET_NARROW_SIGNAL_VAL(reg,id) 		(reg->narrow_band[id]->sigal_val)
/*SET*/
#define SET_NARROW_BAND(reg,id,v) 			(reg->narrow_band[id]->band=v)
#define SET_NARROW_FIR_COEFF(reg,id,v) 		(reg->narrow_band[id]->fir_coeff=v)
#define SET_NARROW_DECODE_TYPE(reg,id,v) 	(reg->narrow_band[id]->decode_type=v)
#define SET_NARROW_ENABLE(reg,id,v) 		(reg->narrow_band[id]->enable=v)
#define SET_NARROW_SIGNAL_CARRIER(reg,id,v) (reg->narrow_band[id]->signal_carrier=v)
#define SET_NARROW_NOISE_LEVEL(reg,id,v) 	(reg->narrow_band[id]->noise_level=v)

/* others */
#define SET_CURRENT_TIME(reg,v)			    (reg->signal->current_time=v)
#define SET_DATA_RESET(reg,v)			    (reg->signal->data_path_reset=v)
#define SET_CURRENT_COUNT(reg,v)		    (reg->signal->current_count=v)
#define SET_TRIG_TIME(reg,v)			    (reg->signal->trig_time=v)
#define SET_TRIG_COUNT(reg,v)			    (reg->signal->trig_count=v)

#define AUDIO_REG(reg)                      (reg->audioReg)


/* RF */
/*GET*/
/* 
@ch: rf control channel
@index: a channel may have multiple RF controls
*/
static inline int32_t _reg_get_rf_temperature(int ch, int index, FPGA_CONFIG_REG *reg)
{
    #define RF_TEMPERATURE_FACTOR 0.0625
    int16_t  rf_temperature = 0;

    if(index >= RF_ONE_CHANNEL_NUM)
        return -1;
    rf_temperature = reg->rfReg[index]->temperature;
    usleep(100);
    rf_temperature = reg->rfReg[index]->temperature;
    rf_temperature = rf_temperature * RF_TEMPERATURE_FACTOR;
    
    return rf_temperature;
}

static inline bool _reg_get_rf_ext_clk(int ch, int index, FPGA_CONFIG_REG *reg)
{
    int32_t  inout = 0;
    bool is_ext = false;

    if(index >= RF_ONE_CHANNEL_NUM)
        return -1;
    
    inout = reg->rfReg[index]->in_out_clk;
    usleep(100);
    inout = reg->rfReg[index]->in_out_clk;
    /*  1: out clock 0: in clock */
    is_ext = (((inout & 0x01) == 0) ? false : true);
    
    return inout;
}

static inline bool _reg_get_rf_lock_clk(int ch, int index, FPGA_CONFIG_REG *reg)
{
    int32_t  lock = 0;
    bool is_lock = false;

    if(index >= RF_ONE_CHANNEL_NUM)
        return -1;
    
    lock = reg->rfReg[index]->clk_lock;
    usleep(100);
    lock = reg->rfReg[index]->clk_lock;
    
    is_lock = (lock == 0 ? false : true);
    return is_lock;
}

/* RF */
/*SET*/
static inline void _reg_set_rf_frequency(int ch, int index, uint32_t freq_hz, FPGA_CONFIG_REG *reg)
{
    uint32_t freq_khz = freq_hz/1000;
    reg->rfReg[0]->freq_khz = freq_khz;
    usleep(300);
}

static inline void _reg_set_rf_bandwidth(int ch, int index, uint32_t bw_hz, FPGA_CONFIG_REG *reg)
{
    uint32_t mbw;
    int i, found = 0;
    uint8_t set_val = 0;
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
        if(mbw == _reg[i].bw_hz){
            set_val = _reg[i].reg_val;
            found ++;
        }
    }
    if(found == 0){
        printf("NOT found bandwidth %uHz in tables,use default[200Mhz]\n", mbw);
        set_val = 0x03; /* default 200MHz */
    }
    for(i = 0; i < RF_ONE_CHANNEL_NUM; i++){
        reg->rfReg[i]->mid_band = set_val;
        usleep(100);
        reg->rfReg[i]->mid_band = set_val;
    }
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
        for(i = 0; i < RF_ONE_CHANNEL_NUM; i++){
            reg->rfReg[i]->rf_mode = set_val;
            usleep(100);
            reg->rfReg[i]->rf_mode = set_val;
        }
}

static inline void _reg_set_rf_mgc_gain(int ch, int index, uint8_t gain, FPGA_CONFIG_REG *reg)
{
    int i;
    if(gain > 30)
        gain = 30;
    else if(gain < 0)
        gain = 0;
    for(i = 0; i < RF_ONE_CHANNEL_NUM; i++){
        reg->rfReg[i]->midband_minus = gain;
        usleep(100);
        reg->rfReg[i]->midband_minus = gain;
    }
}


static inline void _reg_set_rf_attenuation(int ch, int index, uint8_t atten, FPGA_CONFIG_REG *reg)
{
    int i;
    if(atten > 30)
        atten = 30;
    else if(atten < 0)
        atten = 0;
    for(i = 0; i < RF_ONE_CHANNEL_NUM; i++){
        reg->rfReg[i]->rf_minus = atten;
        usleep(100);
        reg->rfReg[i]->rf_minus = atten;
    }
}

static inline void _reg_set_rf_cali_source_attenuation(int ch, int index, uint8_t level, FPGA_CONFIG_REG *reg)
{
    /*  射频模块-90～-30dBm 功率可调，步进 1dB 
        接收机校正衰减有效范围为 寄存器0 dB～60dB
    */
    int reg_val, i;
    ch = ch;
    index = index;
    
    if(level > -30)
        level = -30;
    else if(level < -90)
        level = -90;
    level = -level;
    reg_val = level - 30;

    for(i = 0; i < RF_ONE_CHANNEL_NUM; i++){
        reg->rfReg[i]->revise_minus = reg_val;
        usleep(100);
        reg->rfReg[i]->revise_minus = reg_val;
    }
}

static inline void _reg_set_rf_direct_sample_ctrl(int ch, int index, uint8_t val, FPGA_CONFIG_REG *reg)
{
    uint8_t data;
    int i;
    
    ch = ch;
    index = index;
    data = (val == 0 ? 0 : 0x01);
    /* 0关闭直采，1开启直采 */
    for(i = 0; i < RF_ONE_CHANNEL_NUM; i++){
        reg->rfReg[i]->direct_control = data;
        usleep(100);
        reg->rfReg[i]->direct_control = data;
    }
}

static inline void _reg_set_rf_cali_source_choise(int ch, int index, uint8_t val, FPGA_CONFIG_REG *reg)
{
    int _reg, i;
    
    /* 0 为外部射频输入，1 为校正源输入 */
    _reg = (val == 0 ? 0 : 1);
    for(i = 0; i < RF_ONE_CHANNEL_NUM; i++){
        reg->rfReg[i]->input = _reg;
        usleep(100);
        reg->rfReg[i]->input = _reg;
    }
}


extern FPGA_CONFIG_REG *get_fpga_reg(void);
extern void fpga_io_init(void);
extern void fpga_io_close(void);
#endif

