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
#include "config.h"

static int8_t executor_set_kernel_command(uint8_t type, void *data)
{
    switch(type)
     {
        case EX_CHANNEL_SELECT:
        {
            break;
        }
        case EX_MUTE_SW:
        {
            break;
        }
        case EX_MUTE_THRE:
        {
            break;
        }
        case EX_MID_FREQ:
        {
            break;
        }
        case EX_DEC_METHOD:
        {
            break;
        }
        case EX_DEC_BW:
        {
            break;
        }
        case EX_SMOOTH_TIME:
        {
            break;
        }
        case EX_RESIDENCE_TIME:
        {
            break;
        }
        case EX_RESIDENCE_POLICY:
        {
            break;
        }
        case EX_AUDIO_SAMPLE_RATE:
        {
            break;
        }
        case EX_FFT_SIZE:
        {
            break;
        }
        case EX_FRAME_DROP_CNT:
        {
            break;
        }
        default:
            printf_err("not support type[%d]\n", type);
     }
    return 0;
}


static int8_t executor_set_rf_command(uint8_t type, void *data)
{
    switch(type)
     {
        case EX_RF_MODE_CODE:
        {
            break;
        }
        case EX_RF_GAIN_MODE:
        {
            break;
        }
        case EX_RF_MGC_GAIN:
        {
            break;
        }
        case EX_RF_AGC_CTRL_TIME:
        {
            break;
        }
        case EX_RF_AGC_OUTPUT_AMP:
        {
            break;
        }
        case EX_RF_MID_FREQ:
        {
            break;
        }
        case EX_RF_ANTENNA_SELECT:
        {
            break;
        }
        case EX_RF_ATTENUATION:
        {
            break;
        }
        default:
            printf_err("not support type[%d]\n", type);
     }
    return 0;
}

static int8_t executor_set_mode_command(uint8_t type, void *data)
{
    struct poal_config *poal_config;
    poal_config = (struct poal_config *)data;
    
    switch(type)
    {
        case PSD_MODE_ENABLE:
            io_set_command(EX_CHANNEL_SELECT, EX_MID_FREQ_CMD, poal_config->enable.cid);
            
            break;
        case AUDIO_MODE_ENABLE:
        case IQ_MODE_ENABLE:
        case SPCTRUM_MODE_ANALYSIS_ENABLE:
        case DIRECTION_MODE_ENABLE:
            
        case PSD_MODE_DISABLE:
        case AUDIO_MODE_DISABLE:
        case IQ_MODE_DISABLE:
        case SPCTRUM_MODE_ANALYSIS_DISABLE:
        case DIRECTION_MODE_ENABLE_DISABLE:
        default:
            printf_err("not support type[%d]\n", type);
    }
    return 0;
}

int8_t executor_set_command(exec_cmd cmd, uint8_t type,  void *data)
{
     printf_debug("cmd[%d], type[%d]\n", cmd, type);

     switch(cmd)
     {
        case EX_MID_FREQ_CMD:
        {
            executor_set_kernel_command(type, data);
            break;
        }
        case EX_RF_FREQ_CMD:
        {
            executor_set_rf_command(type, data);
            break;
        }
        case EX_ENABLE_CMD:
        {
            executor_set_mode_command(type, data);
            break;
        }
        default:
            printf_err("invalid set command[%d]\n", cmd);
     }
     return 0;
}




