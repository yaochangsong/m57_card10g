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
#include "../../bsp/io.h"
#include "../../fs/fs.h"


uint8_t * xw_assamble_frame_data(uint32_t *len, void *args)
{
    #define  SYN_HEADER  0x78776b6a /* 'x','w','k','j'[ASCII]=>0x78,0x77,0x6b,0x6a*/
    struct data_frame_header *pfh;
    uint8_t *ptr = NULL;
    static uint32_t _t_counter = 0;
    struct spm_run_parm *pargs;
    uint32_t header_len = 0;
    struct poal_config *config = &(config_get_config()->oal_config);
    pargs = (struct spm_run_parm *)args;
    ptr = calloc(1, sizeof(struct data_frame_header));
    if(ptr == NULL){
        return NULL;
    }
    pfh = (struct data_frame_header *)ptr;
    /* 组装数据帧头 */
    pfh->sysn = SYN_HEADER;
    pfh->timestamp = io_get_fpga_sys_time();//time(NULL);
    pfh->time_ns = io_get_fpga_sys_ns_time();
    pfh->version = XW_PROTOCAL_VERSION;
    pfh->ex_frame_type = pargs->ex_type;
    header_len = sizeof(struct data_frame_header);
    /* 组装频谱扩展帧头 */
    if(pfh->ex_frame_type == DFH_EX_TYPE_PSD){
        struct data_ex_frame_psd_head *pexh;
        pfh->ex_frame_header_len = sizeof(struct data_ex_frame_psd_head);
        ptr=(uint8_t*)realloc(ptr, sizeof(struct data_ex_frame_psd_head)+header_len);
        if(ptr == NULL){
            printf_warn("realloc err\n");
            return NULL;
        }
            
        pexh = (struct data_ex_frame_psd_head *)(ptr + sizeof(struct data_frame_header));
        pexh->ch = pargs->ch;
        pexh->mode = pargs->mode;
        pexh->gain_mode = pargs->gain_mode;
        for(int i = 0; i < MAX_RADIO_CHANNEL_NUM; i++){
            pexh->mgc_gain[i] = config->channel[i].rf_para.mgc_gain_value;
            pexh->rf_gain[i] = config->channel[i].rf_para.attenuation;
        }
        pexh->start_freq_hz = pargs->s_freq;
        pexh->end_freq_hz = pargs->e_freq;
        pexh->mid_freq_hz = pargs->m_freq;
        pexh->bandwidth = pargs->bandwidth;
        pexh->freq_resolution = pargs->freq_resolution;
        pexh->sn = pargs->fft_sn;
        pexh->fft_len = pargs->fft_size;
        pexh->data_type = pargs->type;
        pexh->data_len = pargs->data_len;
        header_len += sizeof(struct data_ex_frame_psd_head);
        printfd("-----------------------------assamble psd head----------------------------------------\n");
        printfd("ch=[%d], mode[%d], gain_mode[%d],start_freq_hz[%"PRIu64"]\n" , 
                    pexh->ch,pexh->mode, pexh->gain_mode, pexh->start_freq_hz);
        printfd("end_freq_hz[%"PRIu64"], mid_freq_hz[%"PRIu64"], freq_resolution[%u],sn[%u],fft[%u]\n" , 
                    pexh->end_freq_hz, pexh->mid_freq_hz, pexh->freq_resolution,pexh->sn,pexh->fft_len);
        printfd("data_type[%d], data_len[%u]\n", pexh->data_type, pexh->data_len);
        printfd("----------------------------------------------------------------------------------------\n");
    } else if(pfh->ex_frame_type == DFH_EX_TYPE_DEMO){
        struct data_ex_frame_demodulation_head *pdh;
        pfh->ex_frame_header_len = sizeof(struct data_ex_frame_demodulation_head);
        ptr=(uint8_t*)realloc(ptr, sizeof(struct data_ex_frame_demodulation_head)+header_len);
        pdh = (struct data_ex_frame_demodulation_head *)(ptr + sizeof(struct data_frame_header));
        pdh->ch = pargs->ch;
        pdh->mode = pargs->mode;
        pdh->gain_mode = pargs->gain_mode;
        for(int i = 0; i < MAX_RADIO_CHANNEL_NUM; i++){
            pdh->mgc_gain[i] = config->channel[i].rf_para.mgc_gain_value;
            pdh->rf_gain[i] = config->channel[i].rf_para.attenuation;
        }
        pdh->mid_freq_hz = pargs->ddc_m_freq;//pargs->m_freq;
        pdh->bandwidth = pargs->ddc_bandwidth;//pargs->bandwidth;
        pdh->sn = pargs->fft_sn;
        pdh->demodulate_type = pargs->d_method;
        pdh->data_type = pargs->type;
        pdh->data_len = pargs->data_len;
        header_len += sizeof(struct data_ex_frame_demodulation_head);
        printfd("-----------------------------assamble demodulation head----------------------------------------\n");
        printfd("ch=[%d], mode[%d], gain_mode[%d],mid_freq_hz[%"PRIu64"]\n" , pdh->ch,pdh->mode, pdh->gain_mode, pdh->mid_freq_hz);
        printfd("bandwidth[%u], sn[%u],demodulate_type[%d]\n" , pdh->bandwidth, pdh->sn,pdh->demodulate_type);
        printfd("data_type[%d], data_len[%d]\n", pdh->data_type, pdh->data_len);
        printfd("----------------------------------------------------------------------------------------\n");
    }else{
        printf_warn("Undefined ex frame type:%d\n", pfh->ex_frame_type);
        safe_free(ptr);
        return NULL;
    }
    *len = header_len;
    printfd("-----------------------------assamble  head---------------------------------------------\n");
    printfd("header_len=%u", header_len);
    printfd("syn=0x%x, timestamp=0x%x, version=%d, ex_frame_header_len=%d\n", pfh->sysn, pfh->timestamp, pfh->version, pfh->ex_frame_header_len);
    printfd("----------------------------------------------------------------------------------------\n");
    
    return ptr;
}

void xw_send_cmd_data(void *client, enum xw_cmd_code cmd_code, void *data)
{
    struct uh_client *cl = client;
    char url[PATH_MAX];
        
    switch(cmd_code){
        case CMD_CODE_BACKTRACE_PROGRESS:
        {
#if defined(CONFIG_FS_PROGRESS)
            struct push_arg *args = data;
            ///file/backtrace/progress/@ch?path=/xxx/path/file.dat&progress=x
            int progress = args->progress.val;
            size_t offset = args->progress.offset;
            int ch = args->ch;
            char *filename = args->filename;
            snprintf(url, PATH_MAX -1, "/file/backtrace/progress/%d?path=%s&progress=%d&offset=%lu", ch, filename, progress, offset);
            printf_note("url: %s\n", url);
#endif
        }
            break;
        case CMD_CODE_DISK_ALERT_FULL:
            break;
            
        default:
            break;
    }
    if(cl && cl->send_str2client)
        cl->send_str2client(cl, url, NULL); /* GET */
}


