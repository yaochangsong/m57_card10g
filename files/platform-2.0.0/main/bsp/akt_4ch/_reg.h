#ifndef __REG_H__
#define __REG_H__

#include <stdint.h>
#include <math.h>
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

#define FPGA_Z7_REG_BASE           0x43c00000   //Z7 寄存器基地址
#define FPGA_V7_REG_BASE           0x43c10000   //v7 寄存器基地址

//v7 寄存器
#define FPGA_SYSETM_BASE        (FPGA_V7_REG_BASE+0x1000)
#define FPGA_ADC_BASE           (FPGA_V7_REG_BASE+0x2000)
#define FPGA_DDC_BASE           (FPGA_V7_REG_BASE+0x3000)
//#define FPGA_NDDC_BASE          (FPGA_V7_REG_BASE+0x3400)
#define FPGA_FFT_BASE           (FPGA_V7_REG_BASE+0x4000)
#define FPGA_NDDC_BASE          (FPGA_V7_REG_BASE+0x5000)

//Z7 寄存器
#define FPGA_AUDIO_BASE         (FPGA_Z7_REG_BASE+0x0c00)
#define CONFG_AUDIO_OFFSET       0xc00


#define SYSTEM_CONFG_4K_LENGTH   0x1000


#define SYSTEM_REG_LENGTH        0x100
#define ADC_REG_LENGTH           0x100
#define AUDIO_REG_LENGTH         0x100
#define DDC_REG_LENGTH           0x100
#define DDC_CHANNEL_MAX_NUM      4

#define FFT_REG_LENGTH           0x100
#include "platform.h"

//---------------------------------------------------- new fpga  reg start
//系统寄存器
typedef struct _SYSTEM_CONFG_REG_
{
    uint32_t version;
    uint32_t system_reset;
    uint32_t generate_time;
    uint32_t rsv;
    uint32_t test_reg;
    uint32_t temp;

}SYSTEM_CONFG_REG;
//adc寄存器
typedef struct _ADC_REG_
{   
    uint32_t ext_clk_sel;
    uint32_t re_config_en;
    uint32_t wr_ad9517_reg;
    uint32_t rsv;
    uint32_t ad9467_0_wr;
    uint32_t ad9467_1_wr;
    uint32_t ad9467_2_wr;
    uint32_t rsv2;
    uint32_t ad9517_ld;
    uint32_t audio_sample;
}ADC_REG;
//宽带寄存器
typedef struct _BROAD_BAND_REG_
{
    uint32_t signal_carrier;
    uint32_t enable;
    uint32_t band;
    uint32_t fft_mean_time;
    uint32_t fft_smooth_type;
    uint32_t fft_calibration;
    uint32_t fft_noise_thresh;
    uint32_t fft_lenth;
    uint32_t reserve;
    uint32_t agc_thresh;
}BROAD_BAND_REG;

//窄带寄存器
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


//fft fifo寄存器
typedef struct _FFT_REG_
{
    uint32_t rsv[16];
    uint32_t Chn1_threshold;
    uint32_t Chn2_threshold;
    uint32_t Chn3_threshold;
    uint32_t Chn4_threshold;
}FFT_REG;


/*
通道1滤波 baseaddr:0x10
通道1音量 baseaddr:0x80   

iic通道选择：baseaddr:0xd0
通道2-8：向后依次偏移4bytes
*/
typedef struct _AUDIO_REG_
{
    uint32_t rsv[4];        //0x00
    uint32_t filter_fc[8];  //0x10+0x20
    uint32_t rsv2[8];      //0x30+0x20
    uint32_t com_rate[8];   //0x50+0x20
    uint32_t rsv3[4];       //0x70
    uint32_t volume[8];     //0x80+0x20
    uint32_t rsv4[12];      //0xa0+0x30
    uint32_t iic_ch_en;     //0xd0 bit[7:0] -ch0-ch7
    uint32_t rsv5[3];
    uint32_t fft_check[4];   //0xe0 fft点数校验（4ch）
}AUDIO_REG;



//窄带DDC模块不具备输出FFT功能，宽带DDC模块具备IQ和FFT输出功能
typedef struct _FPGA_CONFIG_REG_
{
    void                *z7base;
    SYSTEM_CONFG_REG    *system;
    AUDIO_REG           *audioReg;
    ADC_REG             *adcReg;
    BROAD_BAND_REG      *bddc[MAX_RADIO_CHANNEL_NUM];  //宽带ddc
    NARROW_BAND_REG     *nddc[MAX_RADIO_CHANNEL_NUM];  //窄带ddc
    FFT_REG             *fft;
}FPGA_CONFIG_REG;

/*GET*/
#define GET_SYS_FPGA_VER(reg)				(reg->system->version)
#define GET_SYS_FPGA_TEMP(reg)				(reg->system->temp)
#define GET_SYS_GENERATE_TIME(reg)			(reg->system->generate_time)

/*SET*/
#define SET_SYS_RESET(reg,v) 				(reg->system->system_reset=v)


/*SET*/
#define SET_SELECT_CLOCK(reg,v)             (reg->adcReg->ext_clk_sel=v)  //rw 1:外时钟 0：内时钟
#define SET_RECONFIG_CLOCK_EN(reg, v)       (reg->adcReg->re_config_en=v) //w 写1完成切换
//audio
#define SET_AUDIO_SAMPLE_RATE(reg, v)       (reg->adcReg->audio_sample=v)
#define SET_AUDIO_COM_RATE(reg, v, ch)      (reg->audioReg->com_rate[ch]=v)
#define SET_AUDIO_FILTER_FC(reg, v, ch)      (reg->audioReg->filter_fc[ch]=v)

/*GET*/
#define GET_CLOCK(reg)                      (reg->adcReg->ext_clk_sel)

/*GET*/
#define GET_V7_STATUS(reg)                  ((reg->adcReg->ad9517_ld&0x02)?1:0)  //1:正常工作 0：复位状态
#define GET_INTER_CLOCK_LOCK(reg)           ((reg->adcReg->ad9517_ld&0x01)?1:0)  //仅限内时钟模式 0：失锁 1：锁定

//宽带
/*SET*/
#define SET_BROAD_SIGNAL_CARRIER(reg,v, ch) 	(reg->bddc[ch]->signal_carrier=v)
#define SET_BROAD_ENABLE(reg,v, ch) 			(reg->bddc[ch]->enable=v)
#define SET_BROAD_BAND(reg,v, ch) 				(reg->bddc[ch]->band=v)

#define SET_FFT_SMOOTH_TYPE(reg,v, ch) 			(reg->bddc[ch]->fft_smooth_type=v)
#define SET_FFT_SMOOTH_THRESHOLD(reg,v, ch) 	
#define SET_FFT_WINDOW_TYPE(reg,v, ch) 			(v=v)

#define SET_FFT_MEAN_TIME(reg,v, ch) 			(reg->bddc[ch]->fft_mean_time=v)
#define SET_FFT_CALIB(reg,v, ch) 				(reg->bddc[ch]->fft_calibration=v)
#define SET_FFT_FFT_LEN(reg,v, ch) 				(reg->bddc[ch]->fft_lenth=v)
#define SET_FFT_FFT_CHECK(reg,v, ch) 		    (reg->audioReg->fft_check[ch]=v)

/* BIQ */
#define SET_BIQ_SIGNAL_CARRIER(reg,v, ch) 		
#define SET_BIQ_BAND(reg,v, ch) 				
#define SET_BIQ_ENABLE(reg,v, ch) 				

/*GET*/
#define GET_BROAD_AGC_THRESH(reg, ch) 			(reg->bddc[ch]->agc_thresh)

//窄带
/*GET*/
#define GET_NARROW_SIGNAL_VAL(reg,ch) 		(reg->nddc[ch]->sigal_val)
/*SET*/
#define SET_NARROW_BAND(reg,ch,v) 			(reg->nddc[ch]->band=v)
#define SET_NARROW_FIR_COEFF(reg,ch,v) 		(reg->nddc[ch]->fir_coeff=v)
#define SET_NARROW_DECODE_TYPE(reg,ch,v) 	(reg->nddc[ch]->decode_type=v)
//#define SET_NARROW_ENABLE(reg,ch,v) 		(reg->nddc[ch]->enable=v)
#define SET_NARROW_SIGNAL_CARRIER(reg,ch,v) (reg->nddc[ch]->signal_carrier=v)
#define SET_NARROW_NOISE_LEVEL(reg,ch,v) 	(reg->nddc[ch]->noise_level=v)


//other 
#define GET_SYS_FPGA_STATUS(reg)			(0)
#define GET_SYS_FPGA_BOARD_VI(reg)			(0)
#define GET_CURRENT_COUNT(reg)			    (0)
#define GET_CURRENT_TIME(reg)			    (0)

#define SET_SYS_IF_CH(reg,v) 				
#define SET_CURRENT_TIME(reg,v)			    
#define SET_DATA_RESET(reg,v)			    
#define SET_CURRENT_COUNT(reg,v)		    
#define SET_TRIG_TIME(reg,v)			    
#define SET_TRIG_COUNT(reg,v)			    


static inline uint64_t _get_channel_middle_freq(int ch, uint64_t rang_freq, void *args)
{
    return *(uint64_t *)args;
}

static inline int _get_channel_by_mfreq(uint64_t rang_freq, void *args)
{
    return *(int *)args;
}

static inline void _set_narrow_audio_gain_mode(FPGA_CONFIG_REG *reg, int ch, int subch, int mode)
{

}

static inline void _set_narrow_audio_gain(FPGA_CONFIG_REG *reg, int ch, int subch, float gain)
{

}

static inline void _set_fft_channel(FPGA_CONFIG_REG *reg, int ch, void *args)
{
}
static inline void _set_biq_channel(FPGA_CONFIG_REG *reg, int ch, void *args)
{

}
static inline bool _get_adc_status(FPGA_CONFIG_REG *reg)
{
    return true;
}

static inline void _set_narrow_audio_sample_rate(FPGA_CONFIG_REG *reg, int ch, int subch, int rate)
{

}

static inline int get_adc_status_code(bool is_ok)
{
    if(is_ok)
        return 5;
    else
        return 4;
}

static inline int get_rf_status_code(bool is_ok)
{
    if(is_ok)
        return 5;
    else
        return 4;
}

static inline int get_gps_disk_code(bool is_ok,  void *args)
{
    args = args;
    if(is_ok)
        return 5;
    else
        return 4;
}

static inline void _set_audio_iic_en(FPGA_CONFIG_REG *reg, int ch)
{
    reg->audioReg->iic_ch_en = 1 << ch;
}

/*
音量设置 :
val:0-100
寄存器bit[4:0]: 0-31
*/

static inline void _set_audio_volume(FPGA_CONFIG_REG *reg, int ch, int val)
{
    uint8_t volume = 0;
    if (val > 100) {
        val = 100;
    }
    volume = val / 3;
    if (volume > 31) {
        volume = 31;
    }
    volume += 32; //增益6db
    printf_note("set audio ch%d, volume:%d\n", ch, volume);
    reg->audioReg->iic_ch_en = 1 << ch;
    usleep(100);
    reg->audioReg->volume[ch] = volume;
}

static inline void _set_narrow_channel(FPGA_CONFIG_REG *reg, int ch, int subch, int enable)
{
    uint32_t _reg;
    _reg = enable & 0x01;
    reg->nddc[subch]->enable = _reg;
}

static inline void _set_niq_channel(FPGA_CONFIG_REG *reg, int ch, int subch, void *args, int enable)
{
    uint32_t _reg;
    _reg = enable & 0x01;
    reg->nddc[subch]->enable = _reg;
}

static inline void _set_fifo_channel_threshold(FPGA_CONFIG_REG *reg, int ch, uint32_t fftsize)
{
    uint32_t _reg;
    _reg = fftsize / 4 + 2;
    printf_note("ch%d set fft fifo threshold:%u\n", ch, _reg);
    for (int i=0; i<MAX_RADIO_CHANNEL_NUM; i++) {
        if (i == ch) {
            printf_note("[ch%d]set fifo:%u\n",i, _reg);
            if (i == 0) {
                reg->fft->Chn1_threshold = _reg;
            } else if (i == 1) {
                reg->fft->Chn2_threshold = _reg;
            }else if (i == 2) {
                reg->fft->Chn3_threshold = _reg;
            }else if (i == 3) {
                reg->fft->Chn4_threshold = _reg;
            }else{
                break;
            }
            break;
        }
    }
}

/*
    @back:1 playback  0: normal
*/
static inline void _set_ssd_mode(FPGA_CONFIG_REG *reg, int ch,int back)
{
    //do nothing
}

extern FPGA_CONFIG_REG *get_fpga_reg(void);
extern void fpga_io_init(void);
extern void fpga_io_close(void);
#endif

