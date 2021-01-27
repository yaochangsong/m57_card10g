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

#ifndef RF_DIVISION_FREQ_COUNT
#define RF_DIVISION_FREQ_COUNT  2    /* 射频频段数 */
#endif
#ifndef RF_DIVISION_FREQ_HZ
#define RF_DIVISION_FREQ_HZ  GHZ(18)    /* 射频频段划分 */
#endif
#ifndef RF_DIVISION_FREQ2_HZ
#define RF_DIVISION_FREQ2_HZ  GHZ(18)    /* 射频频段2划分 */
#endif

#ifndef MAX_SCAN_FREQ_HZ
#define MAX_SCAN_FREQ_HZ (40000000000)
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
	BROAD_BAND_REG  *broad_band[MAX_RADIO_CHANNEL_NUM];
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
//#define SET_SYS_SSD_MODE(reg,v) 			(reg->system->ssd_mode=v)

/*****broad band*****/
/*GET*/
#define GET_BROAD_AGC_THRESH(reg, ch) 			(reg->broad_band[ch]->agc_thresh)
/*SET*/
#define SET_BROAD_SIGNAL_CARRIER(reg,v, ch) 	(reg->broad_band[ch]->signal_carrier=v)
#define SET_BROAD_ENABLE(reg,v, ch) 			(reg->broad_band[ch]->enable=v)
#define SET_BROAD_BAND(reg,v, ch) 				(reg->broad_band[ch]->band=v)
#define SET_BROAD_FIR_COEFF(reg,v, ch) 			(reg->broad_band[ch]->fir_coeff=v)
/*fft smooth */
#define SET_FFT_SMOOTH_TYPE(reg,v, ch) 			(reg->broad_band[ch]->fft_smooth_type=v)
#define SET_FFT_MEAN_TIME(reg,v, ch) 			(reg->broad_band[ch]->fft_mean_time=v)
#define SET_FFT_CALIB(reg,v, ch) 				(reg->broad_band[ch]->fft_calibration=v)
#define SET_FFT_FFT_LEN(reg,v, ch) 				(reg->broad_band[ch]->fft_lenth=v)

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

#define AUDIO_REG(reg)                      (reg->audioReg)


static inline void _set_narrow_channel(FPGA_CONFIG_REG *reg, int ch, int subch, int enable)
{
    uint32_t _reg;
    if(ch >= 0)
        _reg = (enable &0x01) | ((ch &0x01) << 0x08);
    else
        _reg = enable &0x01;
    reg->narrow_band[subch]->enable = _reg;
}

/*
    @back:1 playback  0: normal
*/
static inline void _set_ssd_mode(FPGA_CONFIG_REG *reg, int ch,int back)
{
    uint32_t _reg = 0;

    if(ch >= 0)
        _reg = ((back &0x01) << ch);
    else
        _reg = back &0x01;

    reg->system->ssd_mode = _reg;
}

extern pthread_mutex_t rf_param_mutex[MAX_RF_NUM];


static inline void set_rf_safe(int ch, uint32_t *reg, uint32_t val)
{
    pthread_mutex_lock(&rf_param_mutex[ch]);
    *reg = val;
    usleep(40);
    pthread_mutex_unlock(&rf_param_mutex[ch]);
}

static inline uint32_t get_rf_safe(int ch, uint32_t *reg)
{
    pthread_mutex_lock(&rf_param_mutex[ch]);
    uint32_t val;
    val = *reg;
    usleep(40);
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
        if(rf_temp_reg > 1280 || rf_temp_reg < 480 || rf_temp_reg == 0){
            read_ok = false;
        }else{
            read_ok = true;
        }
        if(++tye_count > 4)
            time_out = true;
    }while(!read_ok && !time_out);
    printf_note("read ch=%d,rf_temperature = 0x%x, tye_count=%d, read_ok=%d\n",
                ch, rf_temp_reg,  tye_count, read_ok);
    
    if(read_ok == false)
        return -1;
    
    rf_temperature = rf_temp_reg * RF_TEMPERATURE_FACTOR;
    return rf_temperature;
}

/* 获取射频不同工作模式下放大倍数 ;
   通道1: 射频1常规模式放大倍数+射频2指定模式放大倍数
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
    else if(ch == 1){
        if(mid_freq < GHZ(18)){
            return config->channel[ch].rf_para.rf_mode.mag[index];
        }else{
            return (config->channel[ch].rf_para.rf_mode.mag[POAL_NORMAL] + config->channel[ch+1].rf_para.rf_mode.mag[index]);
        }
        
    }
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
/*
CH1:
18000M-40000M模块(1840G30):
18000M-32000M为倒谱关系 （包含18000M和32000M这两个点）
32000M~40000M均为正谱关系（包含40G这个点）

射频18000M-19000M: 中频为7880M（包括19000M这个点）
射频19000M-26500M: 中频为5820M（包括26500M这个点）
射频26500M-30500M: 中频为7880M（包括30500M这个点）
射频30500M-34000M: 中频为5820M（包括34000M这个点）
射频34000M-38600M: 中频为7880M（包括38600M这个点）
射频38600M-40000M: 中频为5820M（包括40000M这个点）
*/
    struct _param_freq_t{
        uint32_t start_rf_freq_mhz;
        uint32_t end_rf_freq_mhz;
        uint32_t mid_freq_mhz;
    };
    struct _param_freq_t _freqtable[] = {
            {18000,    19000, 7880},
            {19000,    26500, 5820},
            {26500,    30500, 7880},
            {30500,    34000, 5820},
            {34000,    38600, 7880},
            {38600,    40000, 5820},
    };
    //freq_hz = GHZ(18.5); //test
    uint32_t freq_10khz = freq_hz/10000, freq_set_val_mhz;
    uint32_t freq_mhz = freq_hz/1000000;
    int i, found = 0;
    int is_cepstrum[MAX_RADIO_CHANNEL_NUM] = {0, 0};
    static  int cepstrum_dup[MAX_RADIO_CHANNEL_NUM] = {-1, -1};
    static uint32_t freq_10khz_dup = 0; //only ch1

    if(ch >= MAX_RADIO_CHANNEL_NUM || freq_hz > GHZ(40))
        return;
    
    if(ch == 1){
        if(reg->rfReg[1] == NULL)
            return;
        if(freq_hz >= GHZ(18)){
            set_rf_safe(ch, &reg->rfReg[2]->freq_khz, freq_10khz);
            set_rf_safe(ch, &reg->rfReg[2]->freq_khz, freq_10khz);
            printf_info("rf2 ch=%d, freq=%"PRIu64" 10khz\n", ch, freq_10khz);
            for(i = 0; i < ARRAY_SIZE(_freqtable); i++){
                if((freq_hz > MHZ(_freqtable[i].start_rf_freq_mhz)) && (freq_hz <= MHZ(_freqtable[i].end_rf_freq_mhz))){
                    freq_set_val_mhz = _freqtable[i].mid_freq_mhz;
                    found ++;
                }
            }
            if(found == 0){
                printf_info("NOT found freq %uMHz in tables, use default 18000mhz \n", freq_set_val_mhz);
                freq_set_val_mhz = 18000; /* default 18GHz */
            }
            freq_10khz = freq_set_val_mhz * 100;
        }
        if(freq_10khz != freq_10khz_dup){
            set_rf_safe(ch, &reg->rfReg[1]->freq_khz, freq_10khz);
            set_rf_safe(ch, &reg->rfReg[1]->freq_khz, freq_10khz);
            printf_info("rf1 ch=%d, freq=%"PRIu64" 10khz\n", ch, freq_10khz);
        }
        freq_10khz_dup = freq_10khz;
    }
    else{
        if(reg->rfReg[0] == NULL)
            return;
        //set_rf_safe(ch, &reg->rfReg[0]->freq_khz, freq_10khz);
        set_rf_safe(ch, &reg->rfReg[0]->freq_khz, freq_10khz);
        set_rf_safe(ch, &reg->rfReg[0]->freq_khz, freq_10khz);
        printf_info("rf0 ch=%d, freq=%"PRIu64" 10khz\n", ch, freq_10khz);
    }
    
    if((freq_hz >= GHZ(13)) && (freq_hz <= GHZ(32))){
        //倒谱
        is_cepstrum[ch] = 1;
        if(is_cepstrum[ch] != cepstrum_dup[ch]){
            printf_info("reg_set_cepstrum 1\n");
            reg_set_cepstrum(ch, 1);
        }
    }else{
        is_cepstrum[ch] = 0;
        if(is_cepstrum[ch] != cepstrum_dup[ch]){
            printf_info("reg_set_cepstrum 0\n");
            reg_set_cepstrum(ch, 0);
        }
    }
    cepstrum_dup[ch] = is_cepstrum[ch];
    
    usleep(100);
}

static inline void _reg_set_rf_bandwidth(int ch, int index, uint32_t bw_hz, FPGA_CONFIG_REG *reg)
{
    uint32_t mbw;
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
    
    //set_rf_safe(ch, &reg->rfReg[ch]->mid_band, set_val);
    set_rf_safe(ch, &reg->rfReg[ch]->mid_band, set_val);
}

static inline void _reg_set_rf_mode_code(int ch, int index, uint8_t code, FPGA_CONFIG_REG *reg)
{
        int i, found = 0;
        uint8_t set_val = 0;
        uint64_t mid_freq;
        static int set_mode_dup[MAX_RADIO_CHANNEL_NUM] = {-1, -1};
        static int set_val_dup[MAX_RADIO_CHANNEL_NUM]= {-1, -1};
        int set_mode[MAX_RADIO_CHANNEL_NUM] = {0, 0};
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

        if(ch == 1){
            mid_freq = executor_get_mid_freq(ch);
            if(mid_freq >= GHZ(18)){
                set_mode[ch] = 0;
                if((set_mode_dup[ch] != set_mode[ch]) || (set_val_dup[ch] != set_val)){
                    set_rf_safe(ch, &reg->rfReg[ch]->rf_mode, 0x00);     //rf1 normal
                    set_rf_safe(ch, &reg->rfReg[ch+1]->rf_mode, set_val);
                    set_mode_dup[ch] = set_mode[ch];
                    set_val_dup[ch] = set_val;
                    printf_debug("rf mode set: ch:%d, rf_mode[1]=00, rf_mode[2]=%d\n", ch, set_val);
                }
            } else{
                set_mode[ch] = 1;
                if((set_mode_dup[ch] != set_mode[ch]) || (set_val_dup[ch] != set_val)){
                    set_rf_safe(ch, &reg->rfReg[ch]->rf_mode, set_val);
                    set_mode_dup[ch] = set_mode[ch];
                    set_val_dup[ch] = set_val;
                    printf_debug("rf mode set: ch:%d, rf_mode[1]=%d\n", ch, set_val);
                }
            }
        }else{
            if(set_val_dup[ch] != set_val){
                set_rf_safe(ch, &reg->rfReg[ch]->rf_mode, set_val);
                set_val_dup[ch] = set_val;
                printf_debug("rf mode set: ch:%d, rf_mode[0]=%d\n", ch, set_val);
            }
        }
        
}

static inline void _reg_set_rf_mgc_gain(int ch, int index, uint8_t gain, FPGA_CONFIG_REG *reg)
{
    uint64_t mid_freq;
    if(ch >= MAX_RADIO_CHANNEL_NUM)
        return;
    
    if(gain > 30)
        gain = 30;
    else if(gain < 0)
        gain = 0;
    
    if(reg->rfReg[ch] == NULL)
        return;

    //set_rf_safe(ch, &reg->rfReg[ch]->midband_minus, gain);
    if(ch == 1){
        mid_freq = executor_get_mid_freq(ch);
        if (mid_freq >= GHZ(18)) {
           set_rf_safe(ch, &reg->rfReg[ch]->midband_minus, 0);
           set_rf_safe(ch, &reg->rfReg[ch+1]->midband_minus, gain);
        } else {
            set_rf_safe(ch, &reg->rfReg[ch]->midband_minus, gain);
        }
    } else {
        set_rf_safe(ch, &reg->rfReg[ch]->midband_minus, gain);
    }
}


static inline void _reg_set_rf_attenuation(int ch, int index, uint8_t atten, FPGA_CONFIG_REG *reg)
{
    uint64_t mid_freq;
    if(ch >= MAX_RADIO_CHANNEL_NUM)
        return;
    if(atten > 30)
        atten = 30;
    else if(atten < 0)
        atten = 0;
    
    if(reg->rfReg[ch] == NULL)
        return;

    if(ch == 1){
        mid_freq = executor_get_mid_freq(ch);
        if (mid_freq >= GHZ(18)) {
            set_rf_safe(ch, &reg->rfReg[ch]->rf_minus, atten);
            set_rf_safe(ch, &reg->rfReg[ch+1]->rf_minus, 0);
        } else {
            set_rf_safe(ch, &reg->rfReg[ch]->rf_minus, atten);
        }
    } else {
        set_rf_safe(ch, &reg->rfReg[ch]->rf_minus, atten);
    }
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
    
    //set_rf_safe(ch, &reg->rfReg[ch]->revise_minus, reg_val);
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
    //set_rf_safe(ch, &reg->rfReg[ch]->direct_control, data);
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
    //set_rf_safe(ch, &reg->rfReg[ch]->input, _reg);
    set_rf_safe(ch, &reg->rfReg[ch]->input, _reg);
}

extern FPGA_CONFIG_REG *get_fpga_reg(void);
extern void fpga_io_init(void);
extern void fpga_io_close(void);
extern int64_t *get_division_point_array(int ch, int *array_len);
#endif
