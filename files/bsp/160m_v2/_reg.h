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
*  Rev 1.0   10 March 2019   bob
*  Initial revision.
******************************************************************************/
#ifndef _FPGA_REG_H_
#define _FPGA_REG_H_

#include <stdint.h>

#define FPGA_REG_DEV "/dev/mem"

#define FPGA_REG_BASE 0x43c00000
#define SYSTEM_CONFG_REG_OFFSET	0x0
#define SYSTEM_CONFG_REG_LENGTH 0x100
#define BROAD_BAND_REG_OFFSET 	0x100
#define BROAD_BAND_REG_LENGTH 	0x100
#define NARROW_BAND_REG_BASE 	0x43c01000
#define NARROW_BAND_REG_LENGTH 	0x100
#define NARROW_BAND_CHANNEL_MAX_NUM 0x10

typedef struct _SYSTEM_CONFG_REG_
{
	uint32_t version;
	uint32_t data_path_reset;
	uint32_t time_pulse;
	uint32_t reserve[0x1D];
	uint32_t time;
}SYSTEM_CONFG_REG;

typedef struct _BROAD_BAND_REG_
{
	uint32_t signal_carrier;
	uint32_t enable;
	uint32_t band;
	uint32_t fft_mean_time;
	uint32_t fft_smooth_type;
	uint32_t fft_calibration;
	uint32_t fft_nosise_thresh;
	uint32_t fft_lenth;
	uint32_t reserve;
	uint32_t agc_thresh;
	uint32_t reserves[0x4];
	uint32_t fir_coeff;
}BROAD_BAND_REG;

typedef struct _NARROW_BAND_REG_
{
	uint32_t signal_carrier;
	uint32_t enable;
	uint32_t band;
	uint32_t reserve[0x9];
	uint32_t decode_type;
	uint32_t fir_coeff;
	uint32_t noise_level;
}NARROW_BAND_REG;

typedef struct _FPGA_CONFIG_REG_
{
    SYSTEM_CONFG_REG *system;
    BROAD_BAND_REG	*broad_band;
    NARROW_BAND_REG *narrow_band[NARROW_BAND_CHANNEL_MAX_NUM];
}FPGA_CONFIG_REG;

/*****system*****/
/*GET*/
#define GET_SYS_FPGA_VER(reg)				(reg->system->version)
#define GET_SYS_FPGA_STATUS(reg)			0
#define GET_SYS_FPGA_BOARD_VI(reg)			0
/*SET*/
#define SET_SYS_RESET(reg,v) 				
#define SET_SYS_IF_CH(reg,v) 				
#define SET_SYS_SSD_MODE(reg,v) 			

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
#define GET_NARROW_SIGNAL_VAL(reg,id) 		0
/*SET*/
#define SET_NARROW_BAND(reg,id,v) 			(reg->narrow_band[id]->band=v)
#define SET_NARROW_FIR_COEFF(reg,id,v) 		(reg->narrow_band[id]->fir_coeff=v)
#define SET_NARROW_DECODE_TYPE(reg,id,v) 	(reg->narrow_band[id]->decode_type=v)
#define SET_NARROW_ENABLE(reg,id,v) 		(reg->narrow_band[id]->enable=v)
#define SET_NARROW_SIGNAL_CARRIER(reg,id,v) (reg->narrow_band[id]->signal_carrier=v)
#define SET_NARROW_NOISE_LEVEL(reg,id,v) 	(reg->narrow_band[id]->noise_level=v)

/* others */
#define SET_CURRENT_TIME(reg,v)			    (reg->system->time=v)
#define SET_DATA_RESET(reg,v)			    (reg->system->data_path_reset=v)
#define SET_CURRENT_COUNT(reg,v)		
#define SET_TRIG_TIME(reg,v)			
#define SET_TRIG_COUNT(reg,v)			
#define AUDIO_REG(reg)                      0


/*****rf*****/
/*GET*/
#define GET_RF_TEMPERATURE(reg)				0
#define GET_RF_CLK_LOCK(reg)				0
#define GET_RF_INOUT_CLK(reg)				0
/*SET*/
#define SET_RF_MID_FREQ(reg,v) 				
#define SET_RF_ATTENUATION(reg,v) 			
#define SET_RF_IF_ATTENUATION(reg,v) 		
#define SET_RF_MODE(reg,v) 					
#define SET_RF_BAND(reg,v) 					
#define SET_RF_CALIB_SOURCE_CHOISE(reg,v) 	
#define SET_RF_DIRECT_SAMPLE_CTRL(reg,v) 	
#define SET_RF_CALIB_SOURCE_ATTENUATION(reg,v) 
#define SET_RF_DIRECT_SAMPLE_ATTENUATION(reg,v)

extern FPGA_CONFIG_REG *get_fpga_reg(void);
extern void fpga_io_init(void);
extern void fpga_io_close(void);
#endif
