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

struct sem_st work_sem;

static int8_t executor_wait_kernel_deal(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        printf_note("clock_gettime failed\n");
        sem_wait(&work_sem.kernel_sysn);
    }else{
        ts.tv_sec += DEFAULT_FFT_TIMEOUT;
        if(sem_timedwait(&work_sem.kernel_sysn, &ts) == -1){
            printf_note("sem waited timeout\n");
        }
    }
    return 0;
}

void executor_work_mode_thread(void *arg)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    
    while(1)
    {
loop:   printf_info("######wait to deal work######\n");
        sem_wait(&work_sem.notify_deal);
        printf_info("receive notify, start to deal work\n");
        for(;;){
            switch(poal_config->work_mode)
            {
                case OAL_FIXED_FREQ_ANYS_MODE:
                {
                    uint32_t fft_size;
                    uint8_t ch = poal_config->enable.cid;
                    uint8_t sub_ch = poal_config->enable.sub_id;
                    printf_info("start fixed freq thread\n");
                    fft_size = poal_config->multi_freq_point_param[ch].points[sub_ch].fft_size;
                    if(poal_config->enable.psd_en){
                        io_set_enable_command(PSD_MODE_ENABLE, poal_config->enable.cid, fft_size);
                        executor_wait_kernel_deal();
                    }else{
                        io_set_enable_command(PSD_MODE_DISABLE, poal_config->enable.cid, fft_size);
                        io_set_enable_command(AUDIO_MODE_DISABLE, poal_config->enable.cid, fft_size);
                        goto loop;
                    }
                }
                    break;
                case OAL_FAST_SCAN_MODE:
                    printf_info("start fast scan thread\n");
                    goto loop;
                    break;
                case OAL_MULTI_ZONE_SCAN_MODE:
                    printf_info("start multi zone thread\n");
                    goto loop;
                    break;
                case OAL_MULTI_POINT_SCAN_MODE:
                    printf_info("start multi point thread\n");
                    goto loop;
                    break;
                default:
                    printf_err("not support work thread\n");
                    goto loop;
                    break;
            }
        }
    }
}

static int8_t executor_get_kernel_command(uint8_t type, void *data)
{

    
}


static int8_t executor_set_kernel_command(uint8_t type, void *data)
{
    
    switch(type)
     {
        case EX_CHANNEL_SELECT:
        {
            printf_debug("channel select: %d\n", *(uint8_t *)data);
            io_set_para_command(type, data);
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
        case EX_DEC_BW:
        {
            io_set_dq_param();
            break;
        }
        case EX_SMOOTH_TIME:
        {
            io_set_para_command(type, data);
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
            io_set_para_command(type, data);
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
        case EX_BANDWITH:
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
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    switch(type)
     {
        case EX_RF_MODE_CODE:
        {
            printf_info("set rf bw: ch: %d, rf_mode_code:%d\n", poal_config->cid, poal_config->rf_para[poal_config->cid].rf_mode_code);
            break;
        }
        case EX_RF_GAIN_MODE:
        {
            printf_info("set rf bw: ch: %d, gain_ctrl_method:%d\n", poal_config->cid, poal_config->rf_para[poal_config->cid].gain_ctrl_method);
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
        case EX_RF_MID_BW:
        {
            printf_info("set rf bw: ch: %d, bw:%d\n", poal_config->cid, poal_config->multi_freq_point_param[poal_config->cid].bandwidth);
            break;
        }
        case EX_RF_ANTENNA_SELECT:
        {
            break;
        }
        case EX_RF_ATTENUATION:
        {
            printf_info("set rf bw: ch: %d, attenuation:%d\n", poal_config->cid, poal_config->rf_para[poal_config->cid].attenuation);
            break;
        }
        default:
            printf_err("not support type[%d]\n", type);
     }
    return 0;
}


int8_t executor_set_command(exec_cmd cmd, uint8_t type,  void *data)
{
     struct poal_config *poal_config = &(config_get_config()->oal_config);

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
            /* notify thread to deal data */
            printf_debug("notify thread to deal data\n");
            sem_post(&work_sem.notify_deal);
            break;
        }
        case EX_WORK_MODE_CMD:
        {
            char *pbuf;
            printf_info("set work mode[%d]\n", type);
            poal_config->assamble_kernel_response_data(pbuf, type);
            io_set_work_mode_command((void *)pbuf);
            break;
        }
        case EX_NETWORK_CMD:
        {
            io_set_network_to_interfaces((void *)&poal_config->network);
            break;
        }
        default:
            printf_err("invalid set command[%d]\n", cmd);
     }
     return 0;
}

void executor_init(void)
{
    int ret;
    pthread_t work_id;
    io_init();
    /* set default network */
    executor_set_command(EX_NETWORK_CMD, 0, NULL);
    sem_init(&(work_sem.notify_deal), 0, 0);
    sem_init(&(work_sem.kernel_sysn), 0, 0);
    ret=pthread_create(&work_id,NULL,(void *)executor_work_mode_thread, NULL);
    if(ret!=0)
        perror("pthread cread work_id");
    pthread_detach(work_id);
}



