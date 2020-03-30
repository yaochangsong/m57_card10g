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
*  Rev 1.0   30 march 2020   yaochangsong
*  Initial revision.
******************************************************************************/
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <getopt.h>

#include "config.h"
#include "data_frame.h"

uint8_t * resetful_assamble_frame_data(uint32_t *len, void *args)
{
    #define  SYN_HEADER  0x78776a6b /* 'x','w','k','j'[ASCII]=>0x78,0x77,0x6b,0x6a*/
    struct data_frame_header *pfh;
    uint8_t *ptr = NULL;
    static uint32_t _t_counter = 0;
    struct spm_run_parm *pargs;
    
    pargs = (struct spm_run_parm *)args;
    ptr = calloc(1, sizeof(struct data_frame_header));
    if(ptr == NULL){
        return NULL;
    }
    pfh = (struct data_frame_header *)ptr;
    /* 组装数据帧头 */
    pfh->sysn = SYN_HEADER;
    pfh->timestamp = time(NULL);
    pfh->time_counter = _t_counter++;
    pfh->ex_frame_type = pargs->ex_type;
    /* 组装频谱扩展帧头 */
    if(pfh->ex_frame_type == DFH_EX_TYPE_PSD){
        struct data_ex_frame_psd_head *pexh;
        pfh->ex_frame_header_len = sizeof(struct data_ex_frame_psd_head);
        ptr=(uint8_t*)realloc(ptr, sizeof(struct data_ex_frame_psd_head));
        pexh = (struct data_ex_frame_psd_head *)(ptr + sizeof(struct data_frame_header));
        pexh->ch = pargs->ch;
        pexh->mode = pargs->mode;
        pexh->gain_mode = pargs->gain_mode;
        pexh->gain_value = pargs->gain_value;
        pexh->start_freq_hz = pargs->s_freq;
        pexh->end_freq_hz = pargs->e_freq;
        pexh->mid_freq_hz = pargs->m_freq;
        pexh->freq_resolution = pargs->freq_resolution;
        pexh->sn = pargs->fft_sn;
        pexh->fft_len = pargs->fft_size;
        pexh->data_type = pargs->type;
        pexh->data_len = pargs->data_len;
        printfn("-----------------------------assamble psd head----------------------------------------\n");
        printfn("ch=[%d], mode[%d], gain_mode[%d],gain_value[%d],start_freq_hz[%llu]\n" , 
                    pexh->ch,pexh->mode, pexh->gain_mode, pexh->gain_value, pexh->start_freq_hz);
        printfn("end_freq_hz[%llu], mid_freq_hz[%llu], freq_resolution[%f],sn[%u],fft[%u]\n" , 
                    pexh->end_freq_hz, pexh->mid_freq_hz, pexh->freq_resolution,pexh->sn,pexh->fft_len);
        printfn("data_type[%d], data_len[%d]\n", pexh->data_type, pexh->data_len);
        printfn("----------------------------------------------------------------------------------------\n");
    } else if(pfh->ex_frame_type == DFH_EX_TYPE_DEMO){
        pfh->ex_frame_header_len = sizeof(struct data_ex_frame_demodulation_head);
    }else{
        printf_warn("Undefined ex frame type:%d\n", pfh->ex_frame_type);
        safe_free(ptr);
        return NULL;
    }
    
    return ptr;
}


