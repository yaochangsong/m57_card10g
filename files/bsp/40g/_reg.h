#ifndef __REG_H__
#define __REG_H__

#include <stdint.h>

#define FPGA_REG_DEV "/dev/mem"

#include <stdint.h>

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
    RF_REG          *rfReg;
    RF_REG          *rfReg2;
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

/*****rf1*****/
/*GET*/
#define GET_RF_TEMPERATURE(reg)				(reg->rfReg->temperature)
#define GET_RF_CLK_LOCK(reg)				(reg->rfReg->clk_lock)
#define GET_RF_INOUT_CLK(reg)				(reg->rfReg->in_out_clk)
/*SET*/
#define SET_RF_MID_FREQ(reg,v) 				(reg->rfReg->freq_khz=v)
#define SET_RF_ATTENUATION(reg,v) 			(reg->rfReg->rf_minus=v)
#define SET_RF_IF_ATTENUATION(reg,v) 		(reg->rfReg->midband_minus=v)
#define SET_RF_MODE(reg,v) 					(reg->rfReg->rf_mode=v)
#define SET_RF_BAND(reg,v) 					(reg->rfReg->mid_band=v)
#define SET_RF_CALIB_SOURCE_CHOISE(reg,v) 	(reg->rfReg->input=v)
#define SET_RF_DIRECT_SAMPLE_CTRL(reg,v) 	(reg->rfReg->direct_control=v)
#define SET_RF_CALIB_SOURCE_ATTENUATION(reg,v) (reg->rfReg->revise_minus=v)
#define SET_RF_DIRECT_SAMPLE_ATTENUATION(reg,v)(reg->rfReg->direct_minus=v)

/*****rf2*****/
/*GET*/
#define GET_RF2_TEMPERATURE(reg)			(reg->rfReg2->temperature)
#define GET_RF2_CLK_LOCK(reg)				(reg->rfReg2->clk_lock)
#define GET_RF2_INOUT_CLK(reg)				(reg->rfReg2->in_out_clk)
/*SET*/
#define SET_RF2_MID_FREQ(reg,v) 			(reg->rfReg2->freq_khz=v)
#define SET_RF2_ATTENUATION(reg,v) 			(reg->rfReg2->rf_minus=v)
#define SET_RF2_IF_ATTENUATION(reg,v) 		(reg->rfReg2->midband_minus=v)
#define SET_RF2_MODE(reg,v) 				(reg->rfReg2->rf_mode=v)
#define SET_RF2_BAND(reg,v) 				(reg->rfReg2->mid_band=v)
#define SET_RF2_CALIB_SOURCE_CHOISE(reg,v) 	(reg->rfReg2->input=v)
#define SET_RF2_DIRECT_SAMPLE_CTRL(reg,v) 	(reg->rfReg2->direct_control=v)
#define SET_RF2_CALIB_SOURCE_ATTENUATION(reg,v) (reg->rfReg2->revise_minus=v)
#define SET_RF2_DIRECT_SAMPLE_ATTENUATION(reg,v)(reg->rfReg2->direct_minus=v)

extern FPGA_CONFIG_REG *get_fpga_reg(void);
extern void fpga_io_init(void);
extern void fpga_io_close(void);
#endif