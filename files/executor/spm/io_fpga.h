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

extern FPGA_CONFIG_REG *get_fpga_reg(void);

#endif
