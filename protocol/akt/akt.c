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
*  Rev 1.0   06 July 2019   yaochangsong
*  Initial revision.
******************************************************************************/
#include "config.h"

PDU_CFG_REQ_HEADER_ST akt_header;

struct response_get_data akt_get_response_data;
struct response_set_data akt_set_response_data;

struct akt_protocal_param akt_config;

bool akt_assamble_kernel_header_response_data(char *pbuf, work_mode wmode, void *config);


int akt_get_device_id(void)
{
    return 0;
}

int8_t akt_decode_method_convert(uint8_t method)
{
    uint8_t d_method;
    
    if(method == DQ_MODE_AM){
        d_method = IO_DQ_MODE_AM;
    }else if(method == DQ_MODE_FM || method == DQ_MODE_WFM) {
        d_method = IO_DQ_MODE_FM;
    }else if(method == DQ_MODE_LSB || method == DQ_MODE_USB) {
        d_method = IO_DQ_MODE_LSB;
    }else if(method == DQ_MODE_CW) {
        d_method = IO_DQ_MODE_CW;
    }else if(method == DQ_MODE_IQ) {
        d_method = IO_DQ_MODE_IQ;
    }else{
        printf_err("decode method not support:%d\n",method);
        return -1;
    }
    return d_method;
}

/******************************************************************************
* FUNCTION:
*     akt_convert_oal_config
*
* DESCRIPTION:
*     akt protocol need to convert config data structure
* PARAMETERS
*     ch:  channel
*    cmd:  
* RETURNS
*       0: ok
*      -1:
******************************************************************************/

static int akt_convert_oal_config(uint8_t ch, uint8_t cmd)
{
    #define  convert_enable_mode(en, mask)   ((en&mask) == 0 ? 0 : 1)
    
    struct akt_protocal_param *pakt_config = &akt_config;
    struct poal_config *poal_config = &(config_get_config()->oal_config);

    /*切换频点（频段）数*/
    uint8_t sig_cnt = 0,i;
    
    switch (cmd)
    {
        case MULTI_FREQ_DECODE_CMD: /* 多频点解调参数 */
            /* 解调参数 （带宽、解调方式）转换*/
            sig_cnt = pakt_config->decode_param[ch].freq_band_cnt; 
            printf_info("sig_cnt=%d\n", sig_cnt);
            poal_config->multi_freq_point_param[ch].freq_point_cnt = sig_cnt;
            for(i = 0; i < sig_cnt; i++){
                poal_config->multi_freq_point_param[ch].points[i].d_method = akt_decode_method_convert(pakt_config->decode_param[ch].sig_ch[i].decode_method_id);
                if(poal_config->multi_freq_point_param[ch].points[i].d_method == -1){
                    return -1;
                }
                poal_config->multi_freq_point_param[ch].points[i].d_bandwith = pakt_config->decode_param[ch].sig_ch[i].bandwidth;
                poal_config->multi_freq_point_param[ch].points[i].center_freq = pakt_config->decode_param[ch].sig_ch[i].center_freq;
                /* 不在需要解调中心频率，除非窄带解调 */
                printf_info("ch:%d,subch:%d, d_bw:%u\n", ch, i,poal_config->multi_freq_point_param[ch].points[i].d_bandwith);
                printf_info("ch:%d,d_method:%u\n", ch, poal_config->multi_freq_point_param[ch].points[i].d_method);
                printf_info("ch:%d,center_freq:%u\n", ch, poal_config->multi_freq_point_param[ch].points[i].center_freq);
            }
            break;
       case SUB_SIGNAL_PARAM_CMD:
       {
            struct sub_channel_freq_para_st *sub_channel_array;
            sub_channel_array = &poal_config->sub_channel_para[poal_config->cid];
            sub_channel_array->cid = poal_config->cid;
            sub_channel_array->sub_ch[ch].index = pakt_config->sub_cid;
            sub_channel_array->sub_ch[ch].center_freq = pakt_config->sub_channel[ch].freq;
            sub_channel_array->sub_ch[ch].d_method = akt_decode_method_convert(pakt_config->sub_channel[ch].decode_method_id);
            sub_channel_array->sub_ch[ch].d_bandwith = pakt_config->sub_channel[ch].bandwidth;
       }
            break;
       case OUTPUT_ENABLE_PARAM:
                /* 使能位转换 */
            poal_config->enable.cid = pakt_config->enable.cid;
            poal_config->enable.sub_id = -1;
            poal_config->enable.psd_en = convert_enable_mode(pakt_config->enable.output_en, SPECTRUM_MASK);
            poal_config->enable.audio_en = convert_enable_mode(pakt_config->enable.output_en, D_OUT_MASK);
            poal_config->enable.iq_en = convert_enable_mode(pakt_config->enable.output_en, IQ_OUT_MASK);
           // poal_config->enable.bit_en = pakt_config->enable.output_en;
            INTERNEL_ENABLE_BIT_SET(poal_config->enable.bit_en,poal_config->enable);
            printf_info("bit_en=%x,psd_en=%d, audio_en=%d,iq_en=%d\n", poal_config->enable.bit_en, 
                        poal_config->enable.psd_en,poal_config->enable.audio_en,poal_config->enable.iq_en);
            printf_info("work_mode=%d\n", poal_config->work_mode);
            break;
        case SUB_SIGNAL_OUTPUT_ENABLE_CMD:
            poal_config->sub_ch_enable.cid = pakt_config->sub_channel_enable[ch].cid;
            poal_config->sub_ch_enable.sub_id = pakt_config->sub_channel_enable[ch].signal_ch;
            poal_config->sub_ch_enable.psd_en = convert_enable_mode(pakt_config->sub_channel_enable[ch].en, SPECTRUM_MASK);
            poal_config->sub_ch_enable.audio_en = convert_enable_mode(pakt_config->sub_channel_enable[ch].en, D_OUT_MASK);
            poal_config->sub_ch_enable.iq_en = convert_enable_mode(pakt_config->sub_channel_enable[ch].en, IQ_OUT_MASK);
            INTERNEL_ENABLE_BIT_SET(poal_config->sub_ch_enable.bit_en,poal_config->sub_ch_enable);
            printf_info("sub_ch bit_en=%x, sub_ch psd_en=%d, sub_ch audio_en=%d,sub_ch iq_en=%d\n", poal_config->sub_ch_enable.bit_en, 
            poal_config->sub_ch_enable.psd_en,poal_config->sub_ch_enable.audio_en,poal_config->sub_ch_enable.iq_en);
            break;
        case DIRECTION_MULTI_FREQ_ZONE_CMD: /* 多频段扫描参数 */
        {
            switch (poal_config->work_mode)
            {
                case OAL_FIXED_FREQ_ANYS_MODE:
                {
                    struct multi_freq_point_para_st *point;
                    point = &poal_config->multi_freq_point_param[ch];
                    sig_cnt= 0;
                    /*主通道转换*/
                    point->cid = ch;
                    /*(RF)中心频率转换*/
                    point->points[sig_cnt].center_freq = pakt_config->multi_freq_zone[ch].sig_ch[sig_cnt].center_freq;
                    /* (RF)带宽转换 */
                    point->points[sig_cnt].bandwidth = pakt_config->multi_freq_zone[ch].sig_ch[sig_cnt].bandwidth;
                    /* 驻留时间 */
                    point->residence_time = pakt_config->multi_freq_zone[ch].resident_time;
                    /* 频点数 */
                    point->freq_point_cnt = pakt_config->multi_freq_zone[ch].freq_band_cnt;
                    /*带宽*/
                    if(point->points[sig_cnt].bandwidth == 0){
                        point->points[sig_cnt].bandwidth = BAND_WITH_20M;
                    }
                    point->points[sig_cnt].fft_size = pakt_config->fft[ch].fft_size;
                    if(point->points[sig_cnt].fft_size > 0){
                        point->points[sig_cnt].freq_resolution = ((float)point->points[sig_cnt].bandwidth/(float)point->points[sig_cnt].fft_size)*BAND_FACTOR;
                        printf_info("freq_resolution:%f\n",point->points[sig_cnt].freq_resolution);
                    }
                     /* smooth */
                    point->smooth_time = pakt_config->smooth[ch].smooth;
                    printf_info("residence_time:%u\n",point->residence_time);
                    printf_info("freq_point_cnt:%u\n",point->freq_point_cnt);
                    printf_info("ch:%d, sig_cnt:%d,center_freq:%u\n", ch,sig_cnt,point->points[sig_cnt].center_freq);
                    printf_info("bandwidth:%u\n",point->points[sig_cnt].bandwidth);
                    printf_info("fft_size:%u\n",point->points[sig_cnt].fft_size);
                    printf_info("smooth_time:%u\n",point->smooth_time);
                    break;
                }
                case OAL_FAST_SCAN_MODE:
                {
                    struct multi_freq_fregment_para_st *fregment;
                    uint32_t bw;
                    fregment = &poal_config->multi_freq_fregment_para[ch];
                    if(poal_config->rf_para[ch].mid_bw == 0){
                        poal_config->rf_para[ch].mid_bw = BAND_WITH_20M;
                    }
                    bw = poal_config->rf_para[ch].mid_bw;
                    sig_cnt= 0;
                    /*主通道转换*/
                    fregment->cid = ch;
                    /*(RF)开始频率转换 = 中心频率 - 带宽/2 */
                    fregment->fregment[sig_cnt].start_freq = pakt_config->multi_freq_zone[ch].sig_ch[sig_cnt].center_freq - pakt_config->multi_freq_zone[ch].sig_ch[sig_cnt].bandwidth/2;
                    /*(RF)结束频率转换 = 中心频率 + 带宽/2 */
                    fregment->fregment[sig_cnt].end_freq = pakt_config->multi_freq_zone[ch].sig_ch[sig_cnt].center_freq + pakt_config->multi_freq_zone[ch].sig_ch[sig_cnt].bandwidth/2;
                    /* 步长 */
                    fregment->fregment[sig_cnt].step = pakt_config->multi_freq_zone[ch].sig_ch[sig_cnt].freq_step;
                    /* fft size转换 */
                    fregment->fregment[sig_cnt].fft_size = pakt_config->fft[ch].fft_size;
                    /*扫描频段转换*/
                    fregment->freq_segment_cnt = pakt_config->multi_freq_zone[ch].freq_band_cnt;
                    /* smooth */
                    fregment->smooth_time = pakt_config->smooth[ch].smooth;
                    /*分辨率转换*/
                    if(fregment->fregment[sig_cnt].fft_size > 0){
                        fregment->fregment[sig_cnt].freq_resolution = ((float)bw/(float)fregment->fregment[sig_cnt].fft_size)*BAND_FACTOR;
                        printf_info("freq_resolution:%f\n",fregment->fregment[sig_cnt].freq_resolution);
                    }
                    printf_info("ch:%d, bw=%u\n", ch, bw);
                    printf_info("start_freq:%d\n", fregment->fregment[sig_cnt].start_freq);
                    printf_info("end_freq:%d\n", fregment->fregment[sig_cnt].end_freq);
                    printf_info("step:%d\n", fregment->fregment[sig_cnt].step);
                    printf_info("fft_size:%d\n", fregment->fregment[sig_cnt].fft_size);
                    printf_info("freq_resolution:%f\n", fregment->fregment[sig_cnt].freq_resolution);
                    printf_info("smooth_time:%d\n", fregment->smooth_time);
                    break;
                }
                case OAL_MULTI_ZONE_SCAN_MODE:
                {
                    struct multi_freq_fregment_para_st *fregment;
                    uint32_t bw;
                    bw = &poal_config->rf_para[ch].mid_bw;
                    if(poal_config->rf_para[ch].mid_bw == 0){
                        poal_config->rf_para[ch].mid_bw = BAND_WITH_20M;
                    }
                    fregment = &poal_config->multi_freq_fregment_para[ch];
                    /*主通道转换*/
                    fregment->cid = ch;
                    /*扫描频段转换*/
                    fregment->freq_segment_cnt = pakt_config->multi_freq_zone[ch].freq_band_cnt;
                     /* smooth */
                    fregment->smooth_time = pakt_config->smooth[ch].smooth;
                    printf_info("ch:%d, sig_cnt:%d,freq_segment_cnt:%u\n", ch,sig_cnt,fregment->freq_segment_cnt);
                    printf_info("smooth_time:%u\n", fregment->smooth_time);
                    for(i = 0; i < poal_config->multi_freq_fregment_para[ch].freq_segment_cnt; i++){
                        fregment->fregment[i].start_freq = pakt_config->multi_freq_zone[ch].sig_ch[i].center_freq - pakt_config->multi_freq_zone[ch].sig_ch[i].bandwidth/2;
                        fregment->fregment[i].end_freq = pakt_config->multi_freq_zone[ch].sig_ch[i].center_freq + pakt_config->multi_freq_zone[ch].sig_ch[i].bandwidth/2;
                        fregment->fregment[i].step = pakt_config->multi_freq_zone[ch].sig_ch[i].freq_step;
                        fregment->fregment[i].fft_size = pakt_config->fft[ch].fft_size;
                        if(fregment->fregment[i].fft_size > 0){
                            fregment->fregment[i].freq_resolution = ((float)bw/(float)fregment->fregment[i].fft_size)*BAND_FACTOR;
                            printf_info("[%d]resolution:%f\n",i, fregment->fregment[i].freq_resolution);
                        }
                        printf_info("[%d]start_freq:%u\n",i, fregment->fregment[i].start_freq);
                        printf_info("[%d]end_freq:%u\n",i, fregment->fregment[i].end_freq);
                        printf_info("[%d]fft_size:%u\n",i, fregment->fregment[i].fft_size);
                        printf_info("[%d]step:%u\n",i, fregment->fregment[i].step);
                    }
                    break;
                }
                case OAL_MULTI_POINT_SCAN_MODE:
                {
                    struct multi_freq_point_para_st *point;
                    point = &poal_config->multi_freq_point_param[ch];
                    /*主通道转换*/
                    point->cid = ch;
                    /* 频点数 */
                    point->freq_point_cnt = pakt_config->multi_freq_zone[ch].freq_band_cnt;
                    /* 驻留时间 */
                    point->residence_time = pakt_config->multi_freq_zone[ch].resident_time;
                    point->smooth_time = pakt_config->smooth[ch].smooth;
                    printf_info("[ch:%d]freq_point_cnt:%u\n",ch, point->freq_point_cnt);
                    printf_info("residence_time:%u\n",point->residence_time);
                    printf_info("smooth_time:%u\n",point->smooth_time);
                    for(i = 0; i < point->freq_point_cnt; i++){
                        point->points[i].center_freq = pakt_config->multi_freq_zone[ch].sig_ch[i].center_freq;
                        point->points[i].bandwidth = pakt_config->multi_freq_zone[ch].sig_ch[i].bandwidth;
                        point->points[i].fft_size = pakt_config->fft[ch].fft_size;
                        point->points[i].d_method = akt_decode_method_convert(pakt_config->decode_param[ch].sig_ch[i].decode_method_id);
                        point->points[i].d_bandwith = pakt_config->decode_param[ch].sig_ch[i].bandwidth;
                        if(point->points[i].fft_size > 0){
                            point->points[i].freq_resolution = BAND_FACTOR*(float)point->points[i].d_bandwith/(float)point->points[i].fft_size;
                            printf_info("[%d]resolution:%f\n",i, point->points[i].freq_resolution);
                        }
                        printf_info("[%d]center_freq:%u\n",i, point->points[i].center_freq);
                        printf_info("[%d]bandwidth:%u\n",i, point->points[i].bandwidth);
                        printf_info("[%d]fft_size:%u\n",i, point->points[i].fft_size);
                        printf_info("[%d]d_method:%u\n",i, point->points[i].d_method);
                        printf_info("[%d]d_bandwith:%u\n",i, point->points[i].d_bandwith);
                    }
                }
                    break;
                default:
                    printf_err("work mode not set:%d\n", poal_config->work_mode);
                    return -1;
              }
            break;
        }
      default: 
          printf_err("not support cmd:%x\n", cmd);
          return -1;
    }
    return 0;
}

static int akt_work_mode_set(struct akt_protocal_param *akt_config)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
   printf_info("resident_time:%d, freq_band_cnt:%d\n", akt_config->multi_freq_zone[akt_config->cid].resident_time,
    akt_config->multi_freq_zone[akt_config->cid].freq_band_cnt);
    if(akt_config->multi_freq_zone[akt_config->cid].resident_time== 0){
        if(akt_config->multi_freq_zone[akt_config->cid].freq_band_cnt == 1){
             /*scan mode*/
            printf_note("Fast Scan Mode\n");
            poal_config->work_mode = OAL_FAST_SCAN_MODE;
        }else{
             /*multi freq zone mode*/
            printf_note("Multi  Freq Zone Mode\n");
            poal_config->work_mode = OAL_MULTI_ZONE_SCAN_MODE;
        }
    }else{
        if(akt_config->multi_freq_zone[akt_config->cid].freq_band_cnt == 1){
            /*fixed freq*/
            printf_note("Fixed Freq  Mode\n");
            poal_config->work_mode =OAL_FIXED_FREQ_ANYS_MODE;
        }else{
            /*multi freq point*/
            printf_note("Multi Freq  Point Mode\n");
            poal_config->work_mode =OAL_MULTI_POINT_SCAN_MODE;
        }
    }
    return 0;
}


static int akt_execute_set_command(void)
{
    PDU_CFG_REQ_HEADER_ST *header;
    struct akt_protocal_param *pakt_config = &akt_config;
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    int err_code;
    header = &akt_header;
    int ch = -1;

    err_code = RET_CODE_SUCCSESS;
    if(config_get_control_mode() == CTRL_MODE_LOCAL){
        err_code = RET_CODE_LOCAL_CTRL_MODE;
        goto set_exit;
    }
    printf_info("set bussiness code[%x]\n", header->code);
    switch (header->code)
    {
        case OUTPUT_ENABLE_PARAM:
        {
            printf_info("enable[cid:%x en:%x]\n", header->buf[0], header->buf[1]);
            check_valid_channel(header->buf[0]);
            memcpy(&(pakt_config->enable), header->buf, sizeof(OUTPUT_ENABLE_PARAM_ST));
            if(check_radio_channel(pakt_config->enable.cid)){
                err_code = RET_CODE_PARAMTER_ERR;
                goto set_exit;
            }
            if(pakt_config->enable.output_en){
               akt_convert_oal_config(ch, DIRECTION_MULTI_FREQ_ZONE_CMD);
               poal_config->assamble_kernel_response_data = akt_assamble_kernel_header_response_data;
               config_save_batch(EX_WORK_MODE_CMD, EX_FIXED_FREQ_ANYS_MODE, config_get_config());
            }   
            if(akt_convert_oal_config(ch, OUTPUT_ENABLE_PARAM) == -1){
                err_code = RET_CODE_PARAMTER_NOT_SET;
                goto set_exit;
            }
            executor_set_enable_command(ch);
            break;
        }
        case DIRECTION_FREQ_POINT_REQ_CMD:
        {
            check_valid_channel(header->buf[0]);
            memcpy(&(pakt_config->fft[ch]), header->buf, sizeof(DIRECTION_FFT_PARAM));
            break;
        }
        case DIRECTION_MULTI_FREQ_ZONE_CMD:
        {
            
            check_valid_channel(header->buf[0]);
            memcpy(&pakt_config->multi_freq_zone[ch], header->buf, sizeof(DIRECTION_MULTI_FREQ_ZONE_PARAM));
            printf_info("resident_time:%d\n", pakt_config->multi_freq_zone[ch].resident_time);
            akt_work_mode_set(pakt_config);
            if(akt_convert_oal_config(ch, DIRECTION_MULTI_FREQ_ZONE_CMD) == -1){
                err_code = RET_CODE_PARAMTER_NOT_SET;
                goto set_exit;
            }
            /*need to enable out, take effect*/
            break;
        }
        case MULTI_FREQ_DECODE_CMD:
        {
            check_valid_channel(header->buf[0]);
            memcpy(&(pakt_config->decode_param[pakt_config->cid]), header->buf, sizeof(MULTI_FREQ_DECODE_PARAM));
            if(akt_convert_oal_config(ch, MULTI_FREQ_DECODE_CMD) == -1){
                err_code = RET_CODE_PARAMTER_NOT_SET;
                goto set_exit;
            }
            break;
        }
        case DIRECTION_SMOOTH_CMD:
        {
            check_valid_channel(header->buf[0]);
            memcpy(&(pakt_config->smooth[pakt_config->cid]), header->buf, sizeof(DIRECTION_SMOOTH_PARAM));
            if(poal_config->work_mode == OAL_FAST_SCAN_MODE || poal_config->work_mode == OAL_MULTI_POINT_SCAN_MODE){
                poal_config->multi_freq_point_param[ch].smooth_time = pakt_config->smooth[ch].smooth;
            }else if(poal_config->work_mode == OAL_FAST_SCAN_MODE || poal_config->work_mode == OAL_MULTI_ZONE_SCAN_MODE){
                poal_config->multi_freq_fregment_para[ch].smooth_time = pakt_config->smooth[ch].smooth;
            }else {
                printf_warn("Work Mode is not set!!\n");
                 err_code = RET_CODE_INVALID_MODULE;
                goto set_exit;
            }
            executor_set_command(EX_MID_FREQ_CMD, EX_SMOOTH_TIME, ch, &pakt_config->smooth[ch].smooth);
            break;
        }
        case RCV_NET_PARAM:
        {
          break;
        }
        case RCV_RF_PARAM:
        {
          break;
        }
        case RCV_STA_PARAM:
        {
          break;
        }
        case SNIFFER_DATA_REPORT_PARAM:
        {
            SNIFFER_DATA_REPORT_ST net_para;
            check_valid_channel(header->buf[0]);
            net_para.cid = ch;
            memcpy(&net_para, header->buf, sizeof(SNIFFER_DATA_REPORT_ST));
            
            struct in_addr ipdata;
            char *ipstr=NULL;
            ipdata.s_addr = net_para.ipaddr;
            ipstr= inet_ntoa(ipdata);
            printf_info("ipstr=%s  ipaddr=%x, port=%d, type=%d\n", ipstr,  ipdata.s_addr, ntohs(net_para.port), net_para.type);
            break;
        }
        case AUDIO_SAMPLE_RATE:
        {
            uint32_t rate;
            check_valid_channel(header->buf[0]);
            poal_config->multi_freq_point_param[ch].audio_sample_rate = *((float *)(header->buf+1));
            rate = (uint32_t)poal_config->multi_freq_point_param[ch].audio_sample_rate;
            executor_set_command(EX_MID_FREQ_CMD, EX_AUDIO_SAMPLE_RATE, ch, &rate);
        }
        case MID_FREQ_BANDWIDTH_CMD:
        {
            check_valid_channel(header->buf[0]);
            memcpy(&(pakt_config->mid_freq_bandwidth[ch]), header->buf, sizeof(DIRECTION_MID_FREQ_BANDWIDTH_PARAM));
            poal_config->rf_para[ch].mid_bw = *((uint32_t *)(header->buf+1));
            printf_info("bandwidth:%u\n", poal_config->rf_para[ch].mid_bw);
            executor_set_command(EX_RF_FREQ_CMD, EX_RF_MID_BW, ch, &poal_config->rf_para[ch].mid_bw);
            break;
        }
        case RF_ATTENUATION_CMD:
            check_valid_channel(header->buf[0]);
            poal_config->rf_para[ch].attenuation = header->buf[1];
            executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, ch, &poal_config->rf_para[ch].attenuation);
            break;
        case RF_WORK_MODE_CMD:
            check_valid_channel(header->buf[0]);
            poal_config->rf_para[ch].rf_mode_code = header->buf[1];
            executor_set_command(EX_RF_FREQ_CMD, EX_RF_MODE_CODE, ch, &poal_config->rf_para[ch].rf_mode_code);
            break;
        case RF_GAIN_MODE_CMD:
            check_valid_channel(header->buf[0]);
            poal_config->rf_para[ch].gain_ctrl_method = header->buf[1];
            executor_set_command(EX_RF_FREQ_CMD, EX_RF_GAIN_MODE, ch, &poal_config->rf_para[ch].gain_ctrl_method);
            break;
        case MID_FREQ_ATTENUATION_CMD:
            check_valid_channel(header->buf[0]);
            poal_config->rf_para[ch].mgc_gain_value = header->buf[1];
            executor_set_command(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, ch, &poal_config->rf_para[ch].mgc_gain_value);
            break;
        case QUIET_NOISE_SWITCH_CMD:
            break;
        case SAMPLE_CONTROL_FFT_CMD:
            check_valid_channel(header->buf[0]);
            memcpy(&(pakt_config->fft[ch]), header->buf, sizeof(DIRECTION_FFT_PARAM));
            if(akt_convert_oal_config(ch, header->code) == -1){
                err_code = RET_CODE_PARAMTER_ERR;
                goto set_exit;
            }
            executor_set_command(EX_MID_FREQ_CMD, EX_FFT_SIZE, ch, &pakt_config->fft[ch].fft_size);
            break;
        case SUB_SIGNAL_PARAM_CMD:
        {           
            struct io_decode_param_st decode_param;
            uint8_t sub_ch;
            sub_ch = *(uint16_t *)(header->buf+1);
            check_valid_channel(header->buf[0]);
            if(check_sub_channel(sub_ch)){
                err_code = RET_CODE_PARAMTER_ERR;
                goto set_exit;
            }
            memcpy(&(pakt_config->sub_channel[sub_ch]), header->buf, sizeof(SUB_SIGNAL_PARAM));
            if(akt_convert_oal_config(sub_ch, header->code) == -1){
                err_code = RET_CODE_PARAMTER_ERR;
                goto set_exit;
            }
            decode_param.cid = ch;
            //decode_param.sub_ch = sub_ch;
            decode_param.d_bandwidth = pakt_config->sub_channel[sub_ch].bandwidth;
            decode_param.d_method = akt_decode_method_convert(pakt_config->sub_channel[sub_ch].decode_method_id);
            decode_param.center_freq = pakt_config->sub_channel[sub_ch].freq;
            printf_info("ch:%d, sub_ch=%d, d_bandwidth:%llu,d_method:%d, center_freq:%llu", ch, sub_ch, decode_param.d_bandwidth, decode_param.d_method, decode_param.center_freq);
            executor_set_command(EX_MID_FREQ_CMD, EX_DEC_BW, ch, &decode_param);
            io_set_enable_command(AUDIO_MODE_DISABLE, ch, 0);
            
            if(poal_config->sub_ch_enable.audio_en || poal_config->sub_ch_enable.iq_en)
                io_set_enable_command(AUDIO_MODE_ENABLE, ch, 32768);
            break;
        }
        case SUB_SIGNAL_OUTPUT_ENABLE_CMD:
        {
            uint8_t sub_ch;
            check_valid_channel(header->buf[0]);
            sub_ch = *(uint16_t *)(header->buf+1);
            if(check_sub_channel(sub_ch)){
                err_code = RET_CODE_PARAMTER_ERR;
                goto set_exit;
            }
            memcpy(&(pakt_config->sub_channel_enable[sub_ch]), header->buf, sizeof(SUB_SIGNAL_ENABLE_PARAM));
            if(akt_convert_oal_config(sub_ch, SUB_SIGNAL_OUTPUT_ENABLE_CMD) == -1){
                err_code = RET_CODE_PARAMTER_NOT_SET;
                goto set_exit;
            }
            io_set_enable_command(AUDIO_MODE_DISABLE, ch, 0);
            if(poal_config->sub_ch_enable.audio_en || poal_config->sub_ch_enable.iq_en)
                io_set_enable_command(AUDIO_MODE_ENABLE, ch, 32768);
            break;
        }
        default:
          printf_err("not support class code[%x]\n",header->code);
          err_code = RET_CODE_PARAMTER_ERR;
          goto set_exit;
    }
set_exit:
    memcpy(akt_set_response_data.payload_data, &err_code, sizeof(err_code));
    akt_set_response_data.header.len = sizeof(err_code)+1;
    akt_set_response_data.header.operation = SET_CMD_RSP;
    if(ch != -1){
        akt_set_response_data.cid = (uint8_t)ch;
    }else{
        akt_set_response_data.cid = 0;
    }
    printf_debug("set cid=%d\n", akt_set_response_data.cid);
    return err_code;

}

static int akt_execute_get_command(void)
{
    PDU_CFG_REQ_HEADER_ST *header;
    int err_code;
    header = &akt_header;

    err_code = RET_CODE_SUCCSESS;
    printf_info("get bussiness code[%x]\n", header->code);
    switch (header->code)
    {
        case DEVICE_SELF_CHECK_CMD:
        {
            DEVICE_SELF_CHECK_STATUS_RSP_ST self_check;
            self_check.clk_status = 1;
            self_check.ad_status = 0;
            self_check.pfga_temperature = io_get_adc_temperature();
            printf_debug("pfga_temperature=%d\n", self_check.pfga_temperature);
            memcpy(akt_get_response_data.payload_data, &self_check, sizeof(DEVICE_SELF_CHECK_STATUS_RSP_ST));
            akt_get_response_data.header.len = sizeof(DEVICE_SELF_CHECK_STATUS_RSP_ST);
            break;
        }
        case SOFTWARE_VERSION_CMD:
            break;
        default:
            printf_debug("not sppoort get commmand\n");
    }
    akt_get_response_data.header.operation = QUERY_CMD_RSP;
    return 0;
}

static int akt_execute_net_command(void)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    
    DEVICE_NET_INFO_ST netinfo;
    struct in_addr ipdata;

    memcpy(netinfo.mac, poal_config->network.mac, sizeof(netinfo.mac));
    netinfo.ipaddr = poal_config->network.ipaddress;
    netinfo.gateway = poal_config->network.gateway;
    netinfo.mask = poal_config->network.netmask;
    netinfo.port = poal_config->network.port;
    netinfo.status = 1;
    ipdata.s_addr = netinfo.ipaddr;
    printf_debug("mac:%x%x%x%x%x%x, ipaddr=%x[%s], gateway=%x\n", netinfo.mac[0],netinfo.mac[1],netinfo.mac[2],netinfo.mac[3],netinfo.mac[4],netinfo.mac[5],
                                                            netinfo.ipaddr, inet_ntoa(ipdata), netinfo.gateway);
    memcpy(akt_get_response_data.payload_data, &netinfo, sizeof(DEVICE_NET_INFO_ST));
    akt_get_response_data.header.len = sizeof(DEVICE_NET_INFO_ST);
//    akt_get_response_data.header.operation = QUERY_CMD_RSP;
    return 0;
}


/******************************************************************************
* FUNCTION:
*     akt_execute_method
*
* DESCRIPTION:
*     akt protocol : Parse execute method
*     
* PARAMETERS
*     none
* RETURNS
*     err_code: error code
******************************************************************************/
bool akt_execute_method(int *code)
{
    PDU_CFG_REQ_HEADER_ST *header;
    int err_code;
    header = &akt_header;

    err_code = RET_CODE_SUCCSESS;
    printf_debug("operation code[%x]\n", header->operation);
    switch (header->operation)
    {
        case SET_CMD_REQ:
        {
            err_code = akt_execute_set_command();
            break;
        }
        case QUERY_CMD_REQ:
        {
            err_code = akt_execute_get_command();
            break;
        }
        case NET_CTRL_CMD:
        {
            if(header->code == HEART_BEAT_MSG_REQ){
                akt_get_response_data.header.len = 0;
            } else if(header->code == DISCOVER_LAN_DEV_PARAM){
                printf_info("discove ...\n");
            }else{
                err_code = akt_execute_net_command();
            }
            
            break;
        }
        case SET_CMD_RSP:
        case QUERY_CMD_RSP:
        {
            err_code = RET_CODE_PARAMTER_INVALID;
            printf_err("invalid method[%d]\n", header->operation);
            break;
        }
        default:
            printf_err("error method[%d]\n", header->operation);
            err_code = RET_CODE_PARAMTER_ERR;
            break;
    }
    *code = err_code;
    if(err_code == RET_CODE_SUCCSESS)
        return true;
    else{
        printf_warn("error code[%d]\n", *code);
        return false;
    }
}

/******************************************************************************
* FUNCTION:
*     akt_parse_header
*
* DESCRIPTION:
*     akt protocol : Parse Receiving Data Header
*     
* PARAMETERS
*     @data:   total receive data pointer
*       @len:  total receive data length
*   @payload:  payload data
*  @err_code:  error code,  return
* RETURNS
*     false: handle data false 
*      ture: handle data successful
******************************************************************************/
bool akt_parse_header(const uint8_t *data, int len, uint8_t **payload, int *err_code)
{
    uint8_t *val;
    PDU_CFG_REQ_HEADER_ST *header, *pdata;
    header = &akt_header;
    pdata = data;
    int header_len;

    int i;
    printf_debug("receive data:\n");
    for(i = 0; i< len; i++)
        printfd("%02x ", data[i]);
    printfd("\n");

    header_len = sizeof(PDU_CFG_REQ_HEADER_ST) - sizeof(header->buf);

    if(len < header_len){
        printf_err("receive data len[%d < %d] is too short\n", len, header_len);
        *err_code = RET_CODE_FORMAT_ERR;
        return false;
    }

    printf_debug("parse_header[%x,%x]\n", data[0], data[1]);
    if(pdata->start_flag != AKT_START_FLAG){
        printf_debug("parse_header error\n");
        *err_code = RET_CODE_FORMAT_ERR;
        return false;
    }
    memcpy(header, pdata, sizeof(PDU_CFG_REQ_HEADER_ST));
    printf_debug("header.start_flag=%x\n", header->start_flag);
    printf_debug("header.len=%x\n", header->len);
    printf_info("header.operation_code=%x\n", header->operation);
    printf_info("header.code=%x\n", header->code);
    printf_debug("header.usr_id:");
    for(i = 0; i< sizeof(header->usr_id); i++){
        printfd("%x", header->usr_id[i]);
    }
    printfd("\n");
    printf_debug("header.receiver_id=%x\n", header->receiver_id);
    printf_debug("header.crc=%x\n", header->crc);

    if(header_len + header->len > len){
        *err_code = RET_CODE_FORMAT_ERR;
        printf_err("invalid payload len=%d\n", header->len);
        return false;
    }
    if(header->len > 0){
        *payload = data + header_len;
    }
    else{
        *payload = NULL;
    }
    
    return true;
}


bool akt_parse_data(const uint8_t *payload, int *code)
{
    int i;
    PDU_CFG_REQ_HEADER_ST *header;
    header = &akt_header;
    
    printf_info("payload:\n");
    for(i = 0; i< header->len; i++)
        printfi("%02x ", payload[i]);
    printfi("\n");
    
    if(header->len > MAX_RECEIVE_DATA_LEN){
        *code = RET_CODE_PARAMTER_TOO_LONG;
        return false;
    }
/*
    header->pbuf = calloc(1, header->len);
    if (!header->pbuf){
        printf_err("calloc failed\n");
        *code = RET_CODE_INTERNAL_ERR;
        return false;
    }
*/    
    memcpy(header->buf, payload, header->len);

    return true;
}


/******************************************************************************
* FUNCTION:
*     akt_assamble_response_data
*
* DESCRIPTION:
*     akt protocol send data prepare: assamble package
*     注：  协议中，设置命令返回和查询命令返回头格式不一致（fuck!), 需要做判断
*     
* PARAMETERS
*     buf:  send data pointer(not include header)
*    code: error code
* RETURNS
*     len: total data len 
******************************************************************************/

int akt_assamble_response_data(uint8_t **buf, int err_code)
{
    int len = 0;
    printf_debug("Prepare to assamble response akt data\n");

    PDU_CFG_REQ_HEADER_ST *req_header;
    struct response_get_data *response_data;
    int header_len=0;
    uint8_t response_code;

    req_header = &akt_header;
    response_data = &akt_get_response_data;
    
    header_len = sizeof(PDU_CFG_RSP_HEADER_ST);
    if(req_header->operation == SET_CMD_REQ){
        header_len += 1; /*结构体struct response_set_data cid长度*/
        response_data = &akt_set_response_data;
        response_data->header.code = req_header->code;
    }else if(req_header->operation == NET_CTRL_CMD){
        /* hearbeat response code */
        if(req_header->code == HEART_BEAT_MSG_REQ){
            printf_info("response heartbeat code\n");
            req_header->operation = req_header->operation;
            response_data->header.code = HEART_BEAT_MSG_RSP;
        }
    }else{
        response_data->header.code = req_header->code;
    }
    *buf = response_data;

    response_data->header.start_flag = AKT_START_FLAG; 
    memcpy(response_data->header.usr_id, req_header->usr_id, sizeof(req_header->usr_id));
    response_data->header.receiver_id = 0;
    response_data->header.crc = htons(crc16_caculate((uint8_t *)response_data->payload_data, response_data->header.len));
    printf_debug("crc:%04x\n", response_data->header.crc);
    response_data->end_flag = AKT_END_FLAG;
    
    int i;
    for(i = 0 ;i< response_data->header.len; i++){
        printfd("%02x ", response_data->payload_data[i]);
    }
    printfd("\n");

    printf_debug("headerlen=%d, data len:%d\n", header_len, response_data->header.len);
    len = header_len + response_data->header.len;
    memcpy(*buf+len, (uint8_t *)&response_data->end_flag, sizeof(response_data->end_flag));
    len +=  sizeof(response_data->end_flag);
    
    return len;
}

int8_t  akt_assamble_send_active_data(uint8_t *send_buf, uint8_t *payload, uint32_t payload_len)
{
    struct response_get_data *response_data;
    int len = 0;
    int header_len=0;

    if(send_buf == NULL || payload == NULL || payload_len == 0){
        return -1;
    }
    header_len = sizeof(PDU_CFG_RSP_HEADER_ST);
    printf_info("header_len:%d\n", header_len);
    response_data = (struct response_get_data *)send_buf;
    response_data->header.start_flag = AKT_START_FLAG; 
    response_data->header.receiver_id = 0;
    memcpy(response_data->payload_data, payload, payload_len);
    memset(response_data->header.usr_id, 0, sizeof(response_data->header.usr_id));
    response_data->header.len = payload_len;
    response_data->header.crc = crc16_caculate((uint8_t *)response_data->payload_data, payload_len);
    response_data->header.operation = SET_CMD_RSP;
    response_data->header.code = NOTIFY_SIGNALl_STATUS;
    response_data->end_flag = AKT_END_FLAG;
    len = header_len + response_data->header.len;
    memcpy(send_buf+len, (uint8_t *)&response_data->end_flag, sizeof(response_data->end_flag));
    len +=  sizeof(response_data->end_flag);
    return len;
}


/******************************************************************************
* FUNCTION:
*     akt_assamble_error_response_data
*
* DESCRIPTION:
*     错误时，组装返回错误包
*     注：协议中，只有设置命令有错误返回！
* PARAMETERS
*     buf:  send data pointer(not include header)
*    code: error code
* RETURNS
*     len: total data len 
******************************************************************************/
int akt_assamble_error_response_data(uint8_t **buf, int err_code)
{
    int len = 0;
    printf_info("Prepare to assamble error response akt data\n");

    PDU_CFG_REQ_HEADER_ST *req_header;
    struct response_set_data *response_data;

    req_header = &akt_header;
    response_data = &akt_set_response_data;
    *buf = response_data;

    response_data->header.start_flag = AKT_START_FLAG; 
    //response_data->header.operation =  req_header->operation;
    response_data->header.code = req_header->code;
    memcpy(response_data->header.usr_id, req_header->usr_id, sizeof(req_header->usr_id));
    response_data->header.receiver_id = 0;
    memcpy(response_data->payload_data, &err_code, sizeof(err_code));
    response_data->header.len = sizeof(err_code);
    response_data->header.crc = crc16_caculate((uint8_t *)response_data->payload_data, response_data->header.len);
    printf_info("crc:%x\n", response_data->header.crc);
    response_data->end_flag = AKT_END_FLAG;
    len = sizeof(PDU_CFG_RSP_HEADER_ST) + response_data->header.len;
    memcpy(*buf+len, (uint8_t *)&response_data->end_flag, sizeof(response_data->end_flag));
    len +=  sizeof(response_data->end_flag);
    return len;
}

bool akt_assamble_kernel_header_response_data(char *pbuf, work_mode wmode, void *config)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    DATUM_SPECTRUM_HEADER_ST *ext_hdr = NULL;
    static char param_buf[sizeof(DATUM_PDU_HEADER_ST)+sizeof(DATUM_SPECTRUM_HEADER_ST)];
    
    ext_hdr = (DATUM_SPECTRUM_HEADER_ST*)(&param_buf + sizeof(DATUM_PDU_HEADER_ST));
    ext_hdr->dev_id = akt_get_device_id();
    
    ext_hdr->work_mode = wmode;
    ext_hdr->gain_mode = poal_config->rf_para[poal_config->cid].gain_ctrl_method;
    ext_hdr->gain = poal_config->rf_para[poal_config->cid].mgc_gain_value;
    ext_hdr->duration = 0;   
    ext_hdr->datum_type = 0;
    printf_info("assamble kernel header[mode: %d]\n", wmode);
    switch (wmode)
    {  
        case EX_FIXED_FREQ_ANYS_MODE:
        case EX_FAST_SCAN_MODE:
        case EX_MULTI_ZONE_SCAN_MODE:
        case EX_MULTI_POINT_SCAN_MODE:
        {
            struct kernel_header_param *header_param;
            header_param = (struct kernel_header_param *)config;
            ext_hdr->cid = header_param->ch;
            ext_hdr->center_freq = header_param->m_freq;
            ext_hdr->sn = header_param->fft_sn;
            ext_hdr->datum_total = header_param->total_fft;
            ext_hdr->bandwidth = header_param->bandwidth;
            ext_hdr->start_freq = header_param->s_freq;
            ext_hdr->cutoff_freq = header_param->e_freq;
            ext_hdr->sample_rate = 0;
            ext_hdr->fft_len = header_param->fft_size;
            ext_hdr->freq_resolution = header_param->freq_resolution;
            break;
        }
        default:
            printf_err("Not support Work Mode\n");
    }
    pbuf = &param_buf;
    return true;
}



