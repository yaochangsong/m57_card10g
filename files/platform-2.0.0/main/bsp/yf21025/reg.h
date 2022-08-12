#ifndef __REG_H__
#define __REG_H__

#include "config.h"

#define FPGA_REG_DEV "/dev/mem"

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
#define FPGA_DAC_BASE   		(FPGA_REG_BASE+0x7000)


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
	uint32_t reserve;
	uint32_t agc_thresh;
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
	uint32_t fft_win;
	uint32_t fir_coeff2;    //0x44
	uint32_t reserve3[0x7];
	uint32_t agc_mgc_gain;  //0x64
	uint32_t agc_time;      //50ms/bit
}NARROW_BAND_REG;

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

typedef struct _DAC_REG_
{
//数据控制寄存器
    uint32_t backplay_data_type;   //回放数据类型
    uint32_t backplay_mid_freq;     //实数回放中频
    uint32_t interpolation;         //插值寄存器
    uint32_t reserve;           
    uint32_t dac_power_calibration; //dac功率校准
    uint32_t reserve2[7];
//芯片控制寄存器
    uint32_t dac_inside_interpolation; //dac内部插值
    uint32_t dac_nco;               //dac NCO
    uint32_t dac_output_gain;       //DAC输出增益
}DAC_REG;

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
	DAC_REG         *dacReg;
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
#define SET_ADC_CLOCK_REF(reg, v)			(reg->adcReg->DDS_REF=v)

//dac
#define SET_DAC_INTERPOLATION(reg, v)			(reg->dacReg->interpolation=v)  //同 ddc 带宽系数
#define SET_DAC_OUTPUT_GAIN(reg, v)             (reg->dacReg->dac_output_gain=v)
#define SET_DAC_POWER_CALI(reg, v)              (reg->dacReg->dac_power_calibration=v)
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
#define SET_FFT_SMOOTH_THRESHOLD(reg,v, ch) 	(reg->broad_band[ch]->fft_smooth_threshold=v)
#define SET_FFT_MEAN_TIME(reg,v, ch) 			(reg->broad_band[ch]->fft_mean_time=v)
#define SET_FFT_CALIB(reg,v, ch) 				(reg->broad_band[ch]->fft_calibration=v)
#define SET_FFT_FFT_LEN(reg,v, ch) 				(reg->broad_band[ch]->fft_lenth=v)
#define SET_FFT_WINDOW_TYPE(reg,v, ch) 			(reg->broad_band[ch]->fft_win=v)

/* BIQ */
#define SET_BIQ_SIGNAL_CARRIER(reg,v, ch) 		(reg->broad_band[1]->signal_carrier=v)
#define SET_BIQ_BAND(reg,v, ch) 				(reg->broad_band[1]->band=v)
#define SET_BIQ_ENABLE(reg,v, ch) 				(reg->broad_band[1]->enable=v)


/*****narrow band*****/
/*GET*/
#define GET_NARROW_SIGNAL_VAL(reg,id) 		(reg->narrow_band[id]->sigal_val)
/*SET*/
#define SET_NARROW_BAND(reg,id,v) 			(reg->narrow_band[id]->band=v)
#define SET_NARROW_FIR_COEFF(reg,id,v) 		(reg->narrow_band[id]->fir_coeff=v)
#define SET_NARROW_FIR_COEFF2(reg,id,v) 	(reg->narrow_band[id]->fir_coeff2=v)
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


extern FPGA_CONFIG_REG *get_fpga_reg(void);
extern void fpga_io_init(void);
extern void fpga_io_close(void);
#endif

