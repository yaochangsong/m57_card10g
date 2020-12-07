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
#include "../../bsp/io.h"
#include "../../executor/spm/spm.h"

struct akt_protocal_param akt_config;
bool akt_send_resp_discovery(void *client, const char *pdata, int len);

int akt_get_device_id(void)
{
    return 0;
}

static inline bool hxstr_to_int(char *str, int *ivalue, bool(*_check)(int))
{
    char *end;
    int value;
    
    if(str == NULL || ivalue == NULL)
        return false;
    
    value = (int) strtol(str, &end, 16);
    if (str == end){
        return false;
    }
    *ivalue = value;
    if(*_check == NULL){
         printf_note("null func\n");
         return true;
    }
       
    return ((*_check)(value));
}

int8_t akt_decode_method_convert(uint8_t method)
{
    uint8_t d_method;
    
    if(method == DQ_MODE_AM){
        d_method = IO_DQ_MODE_AM;
    }else if(method == DQ_MODE_FM) {
        d_method = IO_DQ_MODE_FM;
    }else if(method == DQ_MODE_LSB || method == DQ_MODE_USB) {
        d_method = IO_DQ_MODE_LSB;
    }else if(method == DQ_MODE_CW) {
        d_method = IO_DQ_MODE_CW;
    }else if(method == DQ_MODE_IQ) {
        d_method = IO_DQ_MODE_IQ;
    }else{
        printf_err("decode method not support:%d, use iq\n",method);
        return IO_DQ_MODE_IQ;
    }
    printf_note("method convert:%d ===> %d\n",method, d_method);
    return d_method;
}




uint32_t fftsize_check(uint32_t fft_size)
{
    #define _DEFAULT_FFT_SIZE 2048
    uint32_t t_fftsize[] = {64,128,256,512,1024,2048,4096,8192,16384,32768,65536};
    int i;
    for(i=0; i< ARRAY_SIZE(t_fftsize); i++){
        if(t_fftsize[i] == fft_size)
            return fft_size;
    }
    printf_err("fft size[%u] ERROR! use default size:%u\n", fft_size, _DEFAULT_FFT_SIZE);
    return _DEFAULT_FFT_SIZE;
}

#ifdef SUPPORT_SPECTRUM_SCAN_SEGMENT
static int scan_segment_param_convert(uint8_t ch)
{
    uint32_t i = 0, j = 0;
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    struct multi_freq_fregment_para_st *fregment;
    struct freq_fregment_para_st  array_fregment[MAX_SIG_CHANNLE];
    if (poal_config->channel[ch].work_mode != OAL_FAST_SCAN_MODE && poal_config->channel[ch].work_mode != OAL_MULTI_ZONE_SCAN_MODE) {
        return 0;
    }
    fregment = &poal_config->channel[ch].multi_freq_fregment_para;
    for (i = 0; i < fregment->freq_segment_cnt; i++) {
        if (fregment->fregment[i].start_freq < RF_DIVISION_FREQ_HZ && fregment->fregment[i].end_freq > RF_DIVISION_FREQ_HZ) {
             memcpy(&array_fregment[j], &fregment->fregment[i], sizeof(struct freq_fregment_para_st));
             array_fregment[j].end_freq = RF_DIVISION_FREQ_HZ;
             j++;
             memcpy(&array_fregment[j], &fregment->fregment[i], sizeof(struct freq_fregment_para_st));
             array_fregment[j].start_freq = RF_DIVISION_FREQ_HZ;
             j++;
        } else {
            memcpy(&array_fregment[j], &fregment->fregment[i], sizeof(struct freq_fregment_para_st));
            j++;
        }
    }
    if (j > fregment->freq_segment_cnt) {
        fregment->freq_segment_cnt = j;
        if(fregment->freq_segment_cnt > 1) {
            poal_config->channel[ch].work_mode = OAL_MULTI_ZONE_SCAN_MODE;
        }
        memcpy(&fregment->fregment[0], &array_fregment[0], sizeof(struct freq_fregment_para_st)*MAX_SIG_CHANNLE);
    }

    return 0;
}
#endif
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

static int akt_convert_oal_config(uint8_t ch, int8_t subch,uint8_t cmd)
{
    #define  convert_enable_mode(en, mask)   ((en&mask) == 0 ? 0 : 1)
    
    struct akt_protocal_param *pakt_config = &akt_config;
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    

    /*切换频点（频段）数*/
    uint8_t sig_cnt = 0,i;
    
    switch (cmd)
    {
        case MULTI_FREQ_DECODE_CMD: /* 多频点解调参数 */
        {
            uint8_t method_id;
            /* 解调参数 （带宽、解调方式）转换*/
            sig_cnt = pakt_config->decode_param[ch].freq_band_cnt; 
            printf_note("sig_cnt=%d\n", sig_cnt);
            poal_config->channel[ch].multi_freq_point_param.freq_point_cnt = sig_cnt;
            for(i = 0; i < sig_cnt; i++){
                if((method_id = akt_decode_method_convert(pakt_config->decode_param[ch].sig_ch[i].decode_method_id)) == -1){
                    printf_warn("d_method error=%d\n", poal_config->channel[ch].multi_freq_point_param.points[i].d_method);
                    return -1;
                }
                poal_config->channel[ch].multi_freq_point_param.points[i].raw_d_method = pakt_config->decode_param[ch].sig_ch[i].decode_method_id;
                poal_config->channel[ch].multi_freq_point_param.points[i].d_method = method_id;
                poal_config->channel[ch].multi_freq_point_param.points[i].d_bandwith = pakt_config->decode_param[ch].sig_ch[i].bandwidth;
                poal_config->channel[ch].multi_freq_point_param.points[i].center_freq = pakt_config->decode_param[ch].sig_ch[i].center_freq;
                poal_config->channel[ch].multi_freq_point_param.points[i].noise_thrh = pakt_config->decode_param[ch].sig_ch[i].quiet_noise_threshold;
                poal_config->channel[ch].multi_freq_point_param.points[i].noise_en = pakt_config->decode_param[ch].sig_ch[i].quiet_noise_switch;
                

                /* 不在需要解调中心频率，除非窄带解调 */
                printf_info("ch:%d,subch:%d, d_bw:%u\n", ch, i,poal_config->channel[ch].multi_freq_point_param.points[i].d_bandwith);
                printf_info("ch:%d,d_method:%u\n", ch, poal_config->channel[ch].multi_freq_point_param.points[i].d_method);
                printf_info("ch:%d,center_freq:%llu\n", ch, poal_config->channel[ch].multi_freq_point_param.points[i].center_freq);
                printf_info("ch:%d,noise_thrh:%d\n", ch, poal_config->channel[ch].multi_freq_point_param.points[i].noise_thrh);
            }
            break;
        }
       case SUB_SIGNAL_PARAM_CMD:
       {
            struct sub_channel_freq_para_st *sub_channel_array;
            //int cid = 0;
            //cid = pakt_config->sub_channel[ch].cid;
            sub_channel_array = &poal_config->channel[ch].sub_channel_para;
            sub_channel_array->cid = ch ;
            poal_config->cid = ch;
            sub_channel_array->sub_ch[subch].index = ch;
            sub_channel_array->sub_ch[subch].center_freq = pakt_config->sub_channel[subch].freq;
            sub_channel_array->sub_ch[subch].raw_d_method = pakt_config->sub_channel[subch].decode_method_id;
            sub_channel_array->sub_ch[subch].d_method = akt_decode_method_convert(pakt_config->sub_channel[subch].decode_method_id);
            sub_channel_array->sub_ch[subch].d_bandwith = pakt_config->sub_channel[subch].bandwidth;
            printf_info("cid:%d, subch:%d\n", subch,ch);
            printf_info("subch:%d,d_method:%u, raw dmethod:%d\n", ch, sub_channel_array->sub_ch[subch].raw_d_method, sub_channel_array->sub_ch[subch].d_method, sub_channel_array->sub_ch[subch].raw_d_method);
            printf_info("center_freq:%llu, d_bandwith=%u\n", sub_channel_array->sub_ch[subch].center_freq, sub_channel_array->sub_ch[subch].d_bandwith);
       }
            break;
       case OUTPUT_ENABLE_PARAM:
                /* 使能位转换 */
            poal_config->channel[ch].enable.cid = pakt_config->enable.cid;
            poal_config->channel[ch].enable.sub_id = -1;
            poal_config->channel[ch].enable.psd_en = convert_enable_mode(pakt_config->enable.output_en, SPECTRUM_MASK);
            poal_config->channel[ch].enable.audio_en = convert_enable_mode(pakt_config->enable.output_en, D_OUT_MASK);
            poal_config->channel[ch].enable.iq_en = convert_enable_mode(pakt_config->enable.output_en, IQ_OUT_MASK);
           // poal_config->channel[ch].enable.bit_en = pakt_config->enable.output_en;
            INTERNEL_ENABLE_BIT_SET(poal_config->channel[ch].enable.bit_en,poal_config->channel[ch].enable);
            printf_note("bit_en=%x,psd_en=%d, audio_en=%d,iq_en=%d\n", poal_config->channel[ch].enable.bit_en, 
                        poal_config->channel[ch].enable.psd_en,poal_config->channel[ch].enable.audio_en,poal_config->channel[ch].enable.iq_en);
            printf_note("work_mode=%d\n", poal_config->channel[ch].work_mode);
            break;
        case SUB_SIGNAL_OUTPUT_ENABLE_CMD:
            poal_config->channel[ch].sub_channel_para.sub_ch_enable[subch].cid = pakt_config->sub_channel_enable[subch].cid;
            poal_config->channel[ch].sub_channel_para.sub_ch_enable[subch].sub_id = pakt_config->sub_channel_enable[subch].signal_ch;
            poal_config->channel[ch].sub_channel_para.sub_ch_enable[subch].psd_en = convert_enable_mode(pakt_config->sub_channel_enable[subch].en, SPECTRUM_MASK);
            poal_config->channel[ch].sub_channel_para.sub_ch_enable[subch].audio_en = convert_enable_mode(pakt_config->sub_channel_enable[subch].en, D_OUT_MASK);
            poal_config->channel[ch].sub_channel_para.sub_ch_enable[subch].iq_en = convert_enable_mode(pakt_config->sub_channel_enable[subch].en, IQ_OUT_MASK);
            INTERNEL_ENABLE_BIT_SET(poal_config->channel[ch].sub_channel_para.sub_ch_enable[subch].bit_en,poal_config->channel[ch].sub_channel_para.sub_ch_enable[subch]);
            printf_debug("pakt_config->sub_channel_enable[%d].en = %x\n",subch, pakt_config->sub_channel_enable[subch].en);
            printf_note("enable subch=%d, sub_ch bit_en=%x, sub_ch psd_en=%d, sub_ch audio_en=%d,sub_ch iq_en=%d\n", ch, poal_config->channel[ch].sub_channel_para.sub_ch_enable[subch].bit_en, 
            poal_config->channel[ch].sub_channel_para.sub_ch_enable[subch].psd_en,poal_config->channel[ch].sub_channel_para.sub_ch_enable[subch].audio_en,poal_config->channel[ch].sub_channel_para.sub_ch_enable[subch].iq_en);
            break;
        case SAMPLE_CONTROL_FFT_CMD:
        {
            int i;
            struct multi_freq_point_para_st *point;
            point = &poal_config->channel[ch].multi_freq_point_param;
            for(i = 0; i < MAX_SIG_CHANNLE; i++){
                point->points[i].fft_size = fftsize_check(pakt_config->fft[ch].fft_size);
            }
        }
            break;
        case DIRECTION_MULTI_FREQ_ZONE_CMD: /* 多频段扫描参数 */
        {
            switch (poal_config->channel[ch].work_mode)
            {
                case OAL_FIXED_FREQ_ANYS_MODE:
                {
                    struct multi_freq_point_para_st *point;
                    point = &poal_config->channel[ch].multi_freq_point_param;
                    sig_cnt= 0;
                    /*主通道转换*/
                    point->cid = ch;
                    /*(RF)中心频率转换*/
                    point->points[sig_cnt].center_freq = pakt_config->multi_freq_zone[ch].sig_ch[sig_cnt].center_freq;
                    /* (RF)带宽转换 */
                    point->points[sig_cnt].bandwidth = pakt_config->multi_freq_zone[ch].sig_ch[sig_cnt].bandwidth;
                    /* 驻留时间 */
                    point->residence_time = pakt_config->multi_freq_zone[ch].resident_time*1000;
                    /* 频点数 */
                    point->freq_point_cnt = pakt_config->multi_freq_zone[ch].freq_band_cnt;
                    /*带宽*/
                    if(point->points[sig_cnt].bandwidth == 0){
                        point->points[sig_cnt].bandwidth = BAND_WITH_20M;
                    }
                    point->points[sig_cnt].raw_d_method = pakt_config->decode_param[ch].sig_ch[sig_cnt].decode_method_id;
                    point->points[sig_cnt].d_method = akt_decode_method_convert(point->points[sig_cnt].raw_d_method);
                    point->points[sig_cnt].fft_size = fftsize_check(pakt_config->fft[ch].fft_size);
                    if(point->points[sig_cnt].fft_size > 0){
                        point->points[sig_cnt].freq_resolution = ((float)point->points[sig_cnt].bandwidth/(float)point->points[sig_cnt].fft_size)*BAND_FACTOR;
                        printf_info("freq_resolution:%f\n",point->points[sig_cnt].freq_resolution);
                    }
                     /* smooth */
                    point->smooth_time = pakt_config->smooth[ch].smooth;
                    printf_note("raw_d_method:%d, d_method:%u\n",point->points[sig_cnt].raw_d_method, point->points[sig_cnt].d_method);
                    printf_note("residence_time:%ums\n",point->residence_time);
                    printf_note("freq_point_cnt:%u\n",point->freq_point_cnt);
                    printf_note("ch:%d, sig_cnt:%d,center_freq:%u\n", ch,sig_cnt,point->points[sig_cnt].center_freq);
                    printf_note("bandwidth:%u\n",point->points[sig_cnt].bandwidth);
                    printf_note("fft_size:%u\n",point->points[sig_cnt].fft_size);
                    printf_note("smooth_time:%u\n",point->smooth_time);
                    break;
                }
                case OAL_FAST_SCAN_MODE:
                {
                    struct multi_freq_fregment_para_st *fregment;
                    uint32_t bw;
                    fregment = &poal_config->channel[ch].multi_freq_fregment_para;
                    if(poal_config->channel[ch].rf_para.mid_bw == 0){
                        poal_config->channel[ch].rf_para.mid_bw = BAND_WITH_20M;
                    }
                    bw = poal_config->channel[ch].rf_para.mid_bw;
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
                    fregment->fregment[sig_cnt].fft_size = fftsize_check(pakt_config->fft[ch].fft_size);
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
                    printf_info("start_freq:%llu\n", fregment->fregment[sig_cnt].start_freq);
                    printf_info("end_freq:%llu\n", fregment->fregment[sig_cnt].end_freq);
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
                    bw = &poal_config->channel[ch].rf_para.mid_bw;
                    if(poal_config->channel[ch].rf_para.mid_bw == 0){
                        poal_config->channel[ch].rf_para.mid_bw = BAND_WITH_20M;
                    }
                    fregment = &poal_config->channel[ch].multi_freq_fregment_para;
                    /*主通道转换*/
                    fregment->cid = ch;
                    /*扫描频段转换*/
                    fregment->freq_segment_cnt = pakt_config->multi_freq_zone[ch].freq_band_cnt;
                     /* smooth */
                    fregment->smooth_time = pakt_config->smooth[ch].smooth;
                    printf_info("ch:%d, sig_cnt:%d,freq_segment_cnt:%u\n", ch,sig_cnt,fregment->freq_segment_cnt);
                    printf_info("smooth_time:%u\n", fregment->smooth_time);
                    for(i = 0; i < poal_config->channel[ch].multi_freq_fregment_para.freq_segment_cnt; i++){
                        fregment->fregment[i].start_freq = pakt_config->multi_freq_zone[ch].sig_ch[i].center_freq - pakt_config->multi_freq_zone[ch].sig_ch[i].bandwidth/2;
                        fregment->fregment[i].end_freq = pakt_config->multi_freq_zone[ch].sig_ch[i].center_freq + pakt_config->multi_freq_zone[ch].sig_ch[i].bandwidth/2;
                        fregment->fregment[i].step = pakt_config->multi_freq_zone[ch].sig_ch[i].freq_step;
                        fregment->fregment[i].fft_size = fftsize_check(pakt_config->fft[ch].fft_size);
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
                    point = &poal_config->channel[ch].multi_freq_point_param;
                    /*主通道转换*/
                    point->cid = ch;
                    /* 频点数 */
                    point->freq_point_cnt = pakt_config->multi_freq_zone[ch].freq_band_cnt;
                    /* 驻留时间 */
                    point->residence_time = pakt_config->multi_freq_zone[ch].resident_time*1000;
                    point->smooth_time = pakt_config->smooth[ch].smooth;
                    printf_info("[ch:%d]freq_point_cnt:%u\n",ch, point->freq_point_cnt);
                    printf_info("residence_time:%uMs\n",point->residence_time);
                    printf_info("smooth_time:%u\n",point->smooth_time);
                    for(i = 0; i < point->freq_point_cnt; i++){
                        point->points[i].center_freq = pakt_config->multi_freq_zone[ch].sig_ch[i].center_freq;
                        point->points[i].bandwidth = pakt_config->multi_freq_zone[ch].sig_ch[i].bandwidth;
                        point->points[i].fft_size =fftsize_check(pakt_config->fft[ch].fft_size);
                        point->points[i].d_method = akt_decode_method_convert(pakt_config->decode_param[ch].sig_ch[i].decode_method_id);
                        point->points[i].raw_d_method = pakt_config->decode_param[ch].sig_ch[i].decode_method_id;
                        point->points[i].d_bandwith = pakt_config->decode_param[ch].sig_ch[i].bandwidth;
                        if(point->points[i].fft_size > 0){
                            point->points[i].freq_resolution = BAND_FACTOR*(float)point->points[i].d_bandwith/(float)point->points[i].fft_size;
                            printf_info("[%d]resolution:%f\n",i, point->points[i].freq_resolution);
                        }
                        printf_info("[%d]center_freq:%u\n",i, point->points[i].center_freq);
                        printf_info("[%d]bandwidth:%u\n",i, point->points[i].bandwidth);
                        printf_info("[%d]fft_size:%u\n",i, point->points[i].fft_size);
                        printf_info("[%d]raw_d_method:%d,d_method:%u\n",i,point->points[i].raw_d_method, point->points[i].d_method);
                        printf_info("[%d]d_bandwith:%u\n",i, point->points[i].d_bandwith);
                    }
                }
                    break;
                default:
                    printf_err("work mode not set:%d\n", poal_config->channel[ch].work_mode);
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

static int akt_cali_source_param_convert(void *args)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    CALIBRATION_SOURCE_ST_V2 *cal_source;
    int ch;
    struct multi_freq_point_para_st *point;
    int i, count = 0;
    cal_source = args;
    if(cal_source->e_freq < cal_source->s_freq){
        printf_err("s_freq[%llu] is big than e_freq[%llu]\n", cal_source->e_freq, cal_source->s_freq);
        return -1;
    }
        
    count = (cal_source->e_freq - cal_source->s_freq)/cal_source->step;
    ch = cal_source->cid;
    point = &poal_config->channel[ch].multi_freq_point_param;
    /*主通道转换*/
    point->cid = ch;
    /* 频点数 */
    if(count >= MAX_SIG_CHANNLE){
        printf_err("freq count is bigger: %d than %d\n", count, MAX_SIG_CHANNLE);
        return -1;
    }
    point->freq_point_cnt = count;
    /* 驻留时间:毫秒 */
    point->residence_time = cal_source->r_time_ms;
    printf_note("[ch:%d]freq_point_cnt:%u\n",ch, point->freq_point_cnt);
    printf_note("residence_time:%uMSec.\n",point->residence_time);

    for(i = 0; i < point->freq_point_cnt; i++){
        //point->points[i].center_freq = cal_source->middle_freq_hz + i * cal_source->step;
        point->points[i].center_freq = cal_source->s_freq + i * cal_source->step + cal_source->step / 2;
        point->points[i].bandwidth = cal_source->step;
        if(point->points[i].fft_size == 0){
            printf_note("FFT size is 0, set default 2048\n");
            point->points[i].fft_size =fftsize_check(2048);
        }
        printf_note("[%d]center_freq:%llu, bandwidth:%llu,fft_size:%u\n",i, point->points[i].center_freq, point->points[i].bandwidth, point->points[i].fft_size);
    }
     if ((cal_source->e_freq - cal_source->s_freq) % cal_source->step != 0){
        uint64_t left_freq = cal_source->e_freq - cal_source->s_freq - cal_source->step * i;
        point->points[i].center_freq = cal_source->s_freq + cal_source->step * i + left_freq / 2;
        point->points[i].bandwidth = left_freq;
        if(point->points[i].fft_size == 0){
            printf_note("FFT size is 0, set default 2048\n");
            point->points[i].fft_size =fftsize_check(2048);
        }
        point->freq_point_cnt += 1;
    }
    poal_config->channel[ch].work_mode =OAL_MULTI_POINT_SCAN_MODE;
    return 0;
}
static int akt_work_mode_set(struct akt_protocal_param *akt_config)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    int ch = poal_config->cid;
   printf_info("resident_time:%d, freq_band_cnt:%d\n", akt_config->multi_freq_zone[akt_config->cid].resident_time,
    akt_config->multi_freq_zone[akt_config->cid].freq_band_cnt);
    if(akt_config->multi_freq_zone[akt_config->cid].resident_time== 0){
        if(akt_config->multi_freq_zone[akt_config->cid].freq_band_cnt == 1){
             /*scan mode*/
            printf_note("Fast Scan Mode\n");
            poal_config->channel[ch].work_mode = OAL_FAST_SCAN_MODE;
        }else{
             /*multi freq zone mode*/
            printf_note("Multi  Freq Zone Mode\n");
            poal_config->channel[ch].work_mode = OAL_MULTI_ZONE_SCAN_MODE;
        }
    }else{
        if(akt_config->multi_freq_zone[akt_config->cid].freq_band_cnt == 1){
            /*fixed freq*/
            printf_note("Fixed Freq  Mode\n");
            poal_config->channel[ch].work_mode =OAL_FIXED_FREQ_ANYS_MODE;
        }else{
            /*multi freq point*/
            printf_note("Multi Freq  Point Mode\n");
            poal_config->channel[ch].work_mode =OAL_MULTI_POINT_SCAN_MODE;
        }
    }
    return 0;
}

static inline int akt_err_code_check(int ret)
{
    int err_code = RET_CODE_INTERNAL_ERR;
    if(ret == 0){
        return RET_CODE_SUCCSESS;
    }
    if(ret == -EBUSY){
        printf_warn("Disk is busy now\n");
        err_code = RET_CODE_DEVICE_BUSY;
    }else if(ret == -ENOMEM || ret == EFAULT){
        printf_warn("No memory or Bad address:%d\n", ret);
        err_code = RET_CODE_INTERNAL_ERR;
    }else if(ret == -EEXIST){
        printf_warn("File exists\n");
        err_code = RET_CODE_FILE_EXISTS;
    }else if(ret == -ENOENT){
        printf_warn("No such file\n");
        err_code = RET_CODE_FILE_NOT_FOUND;
    }
    return err_code;
}


int akt_add_udp_client(void *cl_info)
{
    #define UDP_CLIENT_NUM 8
    struct net_udp_client *cl = NULL;
    struct net_udp_client *cl_list, *list_tmp;
    struct net_udp_server *srv = get_udp_server();
    int index = 0;
    struct udp_client_info ucli[UDP_CLIENT_NUM];
    SNIFFER_DATA_REPORT_ST *ci = (SNIFFER_DATA_REPORT_ST *)cl_info;
    memset(ucli, 0, sizeof(struct udp_client_info)*UDP_CLIENT_NUM);
#ifdef SUPPORT_NET_WZ
    printf_note("cid=%x, ipaddr=%x, port=%d, type=%d,wz_ipaddr=%x, wz_port=%d\n",
        ci->cid, ci->ipaddr, ci->port,ci->type, ci->wz_ipaddr, ci->wz_port);
#endif
    list_for_each_entry_safe(cl_list, list_tmp, &srv->clients, list){
        ucli[index].cid = cl_list->ch;
        ucli[index].ipaddr = ntohl(cl_list->peer_addr.sin_addr.s_addr);
        ucli[index].port = ntohs(cl_list->peer_addr.sin_port);
#ifdef SUPPORT_NET_WZ
        ucli[index].wz_ipaddr = ci->wz_ipaddr;
        ucli[index].wz_port = ci->wz_port;
        printf_note("akt kernel add client index=%d, cid=%d, [ip:%x][port:%d][10g_ipaddr=0x%x][10g_port=%d], online\n", 
                        index, ucli[index].cid, ucli[index].ipaddr, ucli[index].port, ucli[index].wz_ipaddr, ucli[index].wz_port);
#endif
        index ++;
        if(index >= UDP_CLIENT_NUM){
            break;
        }
    }
    io_set_udp_client_info(ucli);
    return 0;
}


static int akt_execute_set_command(void *cl)
{
    PDU_CFG_REQ_HEADER_ST_EX *header;
    
    struct akt_protocal_param *pakt_config = &akt_config;
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    struct net_tcp_client *client = cl;
    int err_code;
    header = (PDU_CFG_REQ_HEADER_ST_EX *)client->request.header;
    char *payload = (char *)client->dispatch.body;
    int payload_len = client->request.content_length;
    int ch = -1;

    err_code = RET_CODE_SUCCSESS;
    if(config_get_control_mode() == CTRL_MODE_LOCAL){
        err_code = RET_CODE_LOCAL_CTRL_MODE;
        goto set_exit;
    }
    printf_note("===>set bussiness code[0x%x]\n", header->code);
    switch (header->code)
    {
        case OUTPUT_ENABLE_PARAM:
        {
            printf_note("enable[cid:%x en:%x]\n", payload[0], payload[1]);
            check_valid_channel(payload[0]);
            memcpy(&(pakt_config->enable), payload, sizeof(OUTPUT_ENABLE_PARAM_ST));
            if(check_radio_channel(pakt_config->enable.cid)){
                err_code = RET_CODE_PARAMTER_ERR;
                goto set_exit;
            }
            if(pakt_config->enable.output_en && is_rf_calibration_source_enable() == false){
               akt_convert_oal_config(ch,-1, DIRECTION_MULTI_FREQ_ZONE_CMD);
               //config_save_batch(EX_WORK_MODE_CMD, EX_FIXED_FREQ_ANYS_MODE, config_get_config());
            }   
            if(akt_convert_oal_config(ch, -1, OUTPUT_ENABLE_PARAM) == -1){
                err_code = RET_CODE_PARAMTER_NOT_SET;
                goto set_exit;
            }
            executor_set_enable_command(ch);
            break;
        }
        case QUIET_NOISE_THRESHOLD_CMD:
        {
            check_valid_channel(payload[0]);
            poal_config->channel[ch].multi_freq_point_param.points[0].noise_thrh = payload[1];
            break;
        }
        case QUIET_NOISE_SWITCH_CMD:
        {
            check_valid_channel(payload[0]);
            poal_config->channel[ch].multi_freq_point_param.points[0].noise_en = payload[1];
            break;
        }
        case DIRECTION_FREQ_POINT_REQ_CMD:
        {
            check_valid_channel(payload[0]);
            memcpy(&(pakt_config->fft[ch]), payload, sizeof(DIRECTION_FFT_PARAM));
            break;
        }
        case DIRECTION_MULTI_FREQ_ZONE_CMD:
        {
            /* 校准源模式下不启动该参数 */
            if(is_rf_calibration_source_enable() == true){
                break;
            }
            check_valid_channel(payload[0]);
            memcpy(&pakt_config->multi_freq_zone[ch], payload, payload_len);
            printf_note("resident_time:%d\n", pakt_config->multi_freq_zone[ch].resident_time);
            akt_work_mode_set(pakt_config);
            if(akt_convert_oal_config(ch, -1, DIRECTION_MULTI_FREQ_ZONE_CMD) == -1){
                err_code = RET_CODE_PARAMTER_NOT_SET;
                goto set_exit;
            }
            #ifdef SUPPORT_SPECTRUM_SCAN_SEGMENT
            scan_segment_param_convert(ch);
            #endif
            /*need to enable out, take effect*/
            break;
        }
        case MULTI_FREQ_DECODE_CMD:
        {
            check_valid_channel(payload[0]);
            memcpy(&(pakt_config->decode_param[pakt_config->cid]), payload, sizeof(MULTI_FREQ_DECODE_PARAM));
            if(akt_convert_oal_config(ch, -1, MULTI_FREQ_DECODE_CMD) == -1){
                err_code = RET_CODE_PARAMTER_NOT_SET;
                goto set_exit;
            }
            break;
        }
        case DIRECTION_SMOOTH_CMD:
        {
            check_valid_channel(payload[0]);
            memcpy(&(pakt_config->smooth[pakt_config->cid]), payload, sizeof(DIRECTION_SMOOTH_PARAM));
            poal_config->channel[ch].multi_freq_point_param.smooth_time = pakt_config->smooth[ch].smooth;
            poal_config->channel[ch].multi_freq_fregment_para.smooth_time = pakt_config->smooth[ch].smooth;
            executor_set_command(EX_MID_FREQ_CMD, EX_SMOOTH_TIME, ch, &pakt_config->smooth[ch].smooth);
            break;
        }
        case RCV_NET_PARAM:
        {
            DEVICE_NET_INFO_ST netinfo;
            char ifname[32];
            static uint16_t port_1g = 0, port_10g = 0;
            struct net_tcp_client * client = (struct net_tcp_client *)cl;
            bool restart = false;
           
            memcpy(&netinfo, payload, sizeof(DEVICE_NET_INFO_ST));
            if(get_ifa_name_by_ip(client->get_serv_addr(client), ifname) != 0){
                err_code = RET_CODE_INTERNAL_ERR;
                goto set_exit;
            }
            #ifdef SUPPORT_NET_WZ
            if(!strcmp(ifname, NETWORK_10G_EHTHERNET_POINT)){
                poal_config->network_10g.ipaddress = netinfo.ipaddr;
                poal_config->network_10g.netmask =netinfo.mask;
                poal_config->network_10g.gateway = netinfo.gateway;
                port_10g = poal_config->network_10g.port;
                poal_config->network_10g.port = ntohs(netinfo.port);
                if(port_10g != poal_config->network_10g.port){
                    restart = true;
                }
                printf_note("set 10G net ipaddr=0x%x, mask=0x%x, gateway=0x%x, port=%d\n", 
                    poal_config->network_10g.ipaddress, poal_config->network_10g.netmask, poal_config->network_10g.gateway, poal_config->network_10g.port);
            } else
            #endif
            if(!strcmp(ifname, NETWORK_EHTHERNET_POINT)){
                poal_config->network.ipaddress = netinfo.ipaddr;
                poal_config->network.netmask =netinfo.mask;
                poal_config->network.gateway = netinfo.gateway;
                port_1g = poal_config->network.port ;
                poal_config->network.port = ntohs(netinfo.port);
                if(port_1g != poal_config->network.port){
                    restart = true;
                }
                printf_note("set 1G net ipaddr=0x%x, mask=0x%x, gateway=0x%x, port=%d\n", 
                    poal_config->network.ipaddress, poal_config->network.netmask, poal_config->network.gateway, poal_config->network.port);
            }
            
            config_save_all();
            executor_set_command(EX_NETWORK_CMD, EX_NETWORK, 0, NULL);
            if(restart)
                io_restart_app();
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
            struct sockaddr_in client;
            struct sockaddr_in tcp_client;
            struct timespec ts;
            char ifname[32];
            struct net_tcp_client * _cl = (struct net_tcp_client *)cl;

            check_valid_channel(payload[0]);
            net_para.cid = ch;
            if(get_ifa_name_by_ip(_cl->get_serv_addr(_cl), ifname) != 0){
                err_code = RET_CODE_INTERNAL_ERR;
                goto set_exit;
            }
            memcpy(&net_para, payload, sizeof(SNIFFER_DATA_REPORT_ST));
            /* EndTest */
            client.sin_port = net_para.port;
            client.sin_addr.s_addr = net_para.ipaddr;//ntohl(net_para.sin_addr.s_addr);
            printf_note("[%s]udp client ipstr=%s, port=%d, type=%d\n",ifname, 
                inet_ntoa(client.sin_addr),  client.sin_port, net_para.type);
            /* UDP链表以大端模式存储客户端地址信息；内部以小端模式处理；注意转换 */
            udp_add_client_to_list(&client, ch);
            //akt_add_udp_client(&net_para);
            break;
        }
        case AUDIO_SAMPLE_RATE:
        {
            uint32_t rate;
            check_valid_channel(payload[0]);
            poal_config->channel[ch].multi_freq_point_param.audio_sample_rate = *((float *)(payload+1));
            rate = (uint32_t)poal_config->channel[ch].multi_freq_point_param.audio_sample_rate;
            executor_set_command(EX_MID_FREQ_CMD, EX_AUDIO_SAMPLE_RATE, ch, &rate);
            break;
        }
        case MID_FREQ_BANDWIDTH_CMD:
        {
            check_valid_channel(payload[0]);
            memcpy(&(pakt_config->mid_freq_bandwidth[ch]), payload, sizeof(DIRECTION_MID_FREQ_BANDWIDTH_PARAM));
            poal_config->channel[ch].rf_para.mid_bw = *((uint32_t *)(payload+1));
            printf_warn("ch=%d, bandwidth:%u\n", ch, poal_config->channel[ch].rf_para.mid_bw);
            executor_set_command(EX_RF_FREQ_CMD, EX_RF_MID_BW, ch, NULL);
            break;
        }
        case RF_ATTENUATION_CMD:
            check_valid_channel(payload[0]);
            if(poal_config->channel[ch].rf_para.gain_ctrl_method != POAL_AGC_MODE){
            poal_config->channel[ch].rf_para.attenuation = payload[1];
            printf_note("=>RF_ATTENUATION_CMD:%d\n", poal_config->channel[ch].rf_para.attenuation);
            executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, ch, &poal_config->channel[ch].rf_para.attenuation);
            }
            break;
        case RF_WORK_MODE_CMD:
            check_valid_channel(payload[0]);
            /*
                0: low distortion mode of operation
                1: Normal working mode
                2: Low noise mode of operation
            */
            poal_config->channel[ch].rf_para.rf_mode_code = payload[1];
            printf_note("=>rf_mode_code = %d\n", poal_config->channel[ch].rf_para.rf_mode_code);
            executor_set_command(EX_RF_FREQ_CMD, EX_RF_MODE_CODE, ch, &poal_config->channel[ch].rf_para.rf_mode_code);
            break;
        case RF_GAIN_MODE_CMD:
            check_valid_channel(payload[0]);
            poal_config->channel[ch].rf_para.gain_ctrl_method = payload[1];
            printf_note("gain_ctrl_method=%d\n", poal_config->channel[ch].rf_para.gain_ctrl_method);
            //executor_set_command(EX_RF_FREQ_CMD, EX_RF_GAIN_MODE, ch, &poal_config->channel[ch].rf_para.gain_ctrl_method);
            break;
        case RF_AGC_CMD:
            check_valid_channel(payload[0]);
            poal_config->channel[ch].rf_para.agc_ctrl_time = *((uint16_t *)(payload + 1));
            poal_config->channel[ch].rf_para.agc_mid_freq_out_level = *((uint8_t *)(payload + 3));
            printf_note("agc_ctrl_time:%d, agc_mid_freq_out_level=%d\n", 
                        poal_config->channel[ch].rf_para.agc_ctrl_time, poal_config->channel[ch].rf_para.agc_mid_freq_out_level);
            break;
        case MID_FREQ_ATTENUATION_CMD:
            check_valid_channel(payload[0]);
            if(poal_config->channel[ch].rf_para.gain_ctrl_method != POAL_AGC_MODE){
            poal_config->channel[ch].rf_para.mgc_gain_value = payload[1];
            printf_note("=>mgc_gain_value:%d\n", poal_config->channel[ch].rf_para.mgc_gain_value);
            executor_set_command(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, ch, &poal_config->channel[ch].rf_para.mgc_gain_value);
            }
            break;
        case SAMPLE_CONTROL_FFT_CMD:
            check_valid_channel(payload[0]);
            memcpy(&(pakt_config->fft[ch]), payload, sizeof(DIRECTION_FFT_PARAM));
            if(akt_convert_oal_config(ch, -1, SAMPLE_CONTROL_FFT_CMD) == -1){
                err_code = RET_CODE_PARAMTER_ERR;
                goto set_exit;
            }
            executor_set_command(EX_MID_FREQ_CMD, EX_FFT_SIZE, ch, &pakt_config->fft[ch].fft_size);
            break;
        case SUB_SIGNAL_PARAM_CMD:
        {           
            struct sub_channel_freq_para_st *sub_channel_array;
            uint8_t sub_ch;
            sub_ch = *(uint16_t *)(payload+1);
            if(sub_ch >= 1)
                sub_ch-= 1;
            check_valid_channel(payload[0]);
            if(check_sub_channel(sub_ch)){
                err_code = RET_CODE_PARAMTER_ERR;
                goto set_exit;
            }
            memcpy(&(pakt_config->sub_channel[sub_ch]), payload, sizeof(SUB_SIGNAL_PARAM));
            sub_channel_array = &poal_config->channel[ch].sub_channel_para;
            if(akt_convert_oal_config(ch, sub_ch, SUB_SIGNAL_PARAM_CMD) == -1){
                err_code = RET_CODE_PARAMTER_ERR;
                goto set_exit;
            }
            printf_note("oal ch=%d,%d,sub_ch=%d,%d, freq=%llu, method_id=%d, bandwidth=%u\n", ch,
                        sub_channel_array->cid, sub_ch,sub_channel_array->sub_ch[sub_ch].index,
                        sub_channel_array->sub_ch[sub_ch].center_freq, sub_channel_array->sub_ch[sub_ch].d_method, 
                        sub_channel_array->sub_ch[sub_ch].d_bandwith);
            /* 解调中心频率需要工作中心频率计算 */
            executor_set_command(EX_MID_FREQ_CMD, EX_SUB_CH_MID_FREQ, sub_ch,
                &sub_channel_array->sub_ch[sub_ch].center_freq,/* 解调频率*/
                poal_config->channel[ch].multi_freq_point_param.points[0].center_freq); /* 频点工作频率 */
            /* 解调带宽, 不同解调方式，带宽系数表不一样*/
            executor_set_command(EX_MID_FREQ_CMD, EX_SUB_CH_DEC_BW, sub_ch, 
                &sub_channel_array->sub_ch[sub_ch].d_bandwith,
                sub_channel_array->sub_ch[sub_ch].d_method);
            /* 子通道解调方式 */
            executor_set_command(EX_MID_FREQ_CMD, EX_SUB_CH_DEC_METHOD, sub_ch, 
                &sub_channel_array->sub_ch[sub_ch].d_method);
            break;
        }
        case SUB_SIGNAL_OUTPUT_ENABLE_CMD:
        {
            uint8_t sub_ch, enable = 0, i;
            bool net_10g_threshold_on = false, net_1g_threshold_on = false;
            check_valid_channel(payload[0]);
            sub_ch = *(uint16_t *)(payload+1);
            if(sub_ch >= 1)
                sub_ch-= 1;
            if(check_sub_channel(sub_ch)){
                err_code = RET_CODE_PARAMTER_ERR;
                goto set_exit;
            }
            memcpy(&(pakt_config->sub_channel_enable[sub_ch]), payload, sizeof(SUB_SIGNAL_ENABLE_PARAM));
            if(akt_convert_oal_config(ch, sub_ch, SUB_SIGNAL_OUTPUT_ENABLE_CMD) == -1){
                err_code = RET_CODE_PARAMTER_NOT_SET;
                goto set_exit;
            }
           // io_set_enable_command(IQ_MODE_DISABLE, ch, 0);
            printf_note("ch:%d, sub_ch=%d, au_en:%d,iq_en:%d\n", ch,sub_ch, poal_config->channel[ch].sub_channel_para.sub_ch_enable[sub_ch].audio_en, poal_config->channel[ch].sub_channel_para.sub_ch_enable[sub_ch].iq_en);
            enable = (poal_config->channel[ch].sub_channel_para.sub_ch_enable[sub_ch].iq_en == 0 ? 0 : 1);
            printf_info("ch:%d, sub_ch=%d, au_en:%d,iq_en:%d, enable=%d\n", ch,sub_ch, poal_config->channel[ch].sub_channel_para.sub_ch_enable[sub_ch].audio_en, poal_config->channel[ch].sub_channel_para.sub_ch_enable[sub_ch].iq_en, enable);

            /* 通道IQ使能 */
            if(enable){
                struct net_tcp_client *s_cl;
                /* NOTE:The parameter must be a MAIN channel, not a subchannel */
                io_set_enable_command(IQ_MODE_ENABLE, ch, sub_ch, 0);
            }else{
                io_set_enable_command(IQ_MODE_DISABLE, ch,sub_ch, 0);
            }
            break;
        }
#ifdef SUPPORT_NET_WZ
        case SET_NET_WZ_THRESHOLD_CMD:
        {
            poal_config->ctrl_para.wz_threshold_bandwidth =  *(uint32_t *)(payload+1);
            printf_note("wz_threshold_bandwidth  %u\n", poal_config->ctrl_para.wz_threshold_bandwidth);

            break;
        }
        case SET_NET_WZ_NETWORK_CMD:
        {   
            DEVICE_NET_INFO_ST netinfo;
            memcpy(&netinfo, payload, sizeof(DEVICE_NET_INFO_ST));
            poal_config->network_10g.ipaddress = netinfo.ipaddr;
            poal_config->network_10g.netmask =netinfo.mask;
            poal_config->network_10g.gateway = netinfo.gateway;
            poal_config->network_10g.port = ntohs(netinfo.port);
            
            printf_note("ipaddr=0x%x, mask=0x%x, gateway=0x%x, port=%d\n", 
                poal_config->network_10g.ipaddress, poal_config->network_10g.netmask, poal_config->network_10g.gateway, poal_config->network_10g.port);
            config_save_all();
            executor_set_command(EX_NETWORK_CMD, EX_NETWORK_10G, 0, NULL);
            break;
        }
#endif
        case SPCTRUM_PARAM_CMD:
        {
            uint64_t freq_hz;
            int i; 
            freq_hz = *(uint64_t *)(payload);
            printf_debug("spctrum freq hz=%lluHz\n", freq_hz);
            poal_config->ctrl_para.specturm_analysis_param.frequency_hz = freq_hz;
            break;
        }
        case SPCTRUM_ANALYSIS_CTRL_EN_CMD:
        {
            bool enable;
            ch = 0;
            enable =  (bool)payload[0];
            printf_note("Spctrum Analysis Ctrl  %s\n", enable == false ? "Enable" : "Disable");
            poal_config->channel[ch].enable.spec_analy_en = !enable;
            INTERNEL_ENABLE_BIT_SET(poal_config->channel[ch].enable.bit_en,poal_config->channel[ch].enable);
            printf_note("ch=%d, spec_analy_en=%d\n",  ch, poal_config->channel[ch].enable.spec_analy_en);
            //executor_set_enable_command(0);
            break;
        }
        case SYSTEM_TIME_SET_REQ:
            break;       
        case DEVICE_CALIBRATE_CMD:
        {
            CALIBRATION_SOURCE_ST_V2 cal_source;
            check_valid_channel(payload[0]);
            memcpy(&cal_source, payload, sizeof(CALIBRATION_SOURCE_ST_V2));
            printf_note("RF calibrate: cid=%d, enable=%d, middle_freq_hz=%uhz, power=%d, s_freq=%llu, e_freq=%llu, r_time_ms=%u,step=%llu\n", 
                cal_source.cid, cal_source.enable, cal_source.middle_freq_hz, cal_source.power, cal_source.s_freq, cal_source.e_freq, cal_source.r_time_ms, cal_source.step);
            //executor_set_command(EX_RF_FREQ_CMD, EX_RF_CALIBRATE, ch, &cal_source);
            if(cal_source.enable){
                if(akt_cali_source_param_convert(&cal_source) != 0){
                    err_code = RET_CODE_PARAMTER_ERR;
                    goto set_exit;
                }
                io_set_rf_calibration_source_enable(ch, 1);
                usleep(1000);
                io_set_rf_calibration_source_level(cal_source.power);
            }
            else{
                io_set_rf_calibration_source_enable(ch, 0);
            }
            break;
        }
        case FREQ_RESIDENT_MODE:
            check_valid_channel(payload[0]);
            poal_config->ctrl_para.residency.policy[ch] = (int32_t)*(int16_t *)(payload+1);
            printf_note("Residency Policy, ch=%d, policy=%d\n", ch, poal_config->ctrl_para.residency.policy[ch]);
            break;
        case AUDIO_VOLUME_CMD:
            check_valid_channel(payload[0]);
            for (int i = 0; i < MAX_SIG_CHANNLE; i++) {
                poal_config->channel[ch].multi_freq_point_param.points[i].audio_volume = payload[1];
            }
            executor_set_command(EX_MID_FREQ_CMD, EX_AUDIO_VOL_CTRL, CONFIG_AUDIO_CHANNEL,&payload[1]);
            printf_note("audio_volume=%d\n", poal_config->channel[ch].multi_freq_point_param.points[0].audio_volume);
            break;
        /* disk cmd */
        case STORAGE_IQ_CMD:
        {
            int ret = 0;
            STORAGE_IQ_ST  sis;
            uint32_t old_bandwidth = 0;
            check_valid_channel(payload[0]);
            memcpy(&sis, payload, sizeof(STORAGE_IQ_ST));
            if(sis.cmd == 1){/* start add iq file */
                printf_note("ch:%d, Start add file:%s, bandwidth=%u\n", ch, sis.filepath, sis.bandwidth);
                executor_set_command(EX_MID_FREQ_CMD, EX_BANDWITH, ch, &sis.bandwidth);
#if defined(SUPPORT_XWFS)
                ret = xwfs_start_save_file(sis.filepath);
 #elif defined(SUPPORT_FS)
                struct fs_context *fs_ctx;
                fs_ctx = get_fs_ctx();
                if(fs_ctx == NULL){
                    printf_warn("NOT FOUND DISK!!\n");
                    break;
                }
                fs_ctx->ops->fs_start_save_file(ch, sis.filepath);
#endif
            }else if(sis.cmd == 0){/* stop add iq file */
                printf_note("Stop add file:%s\n", sis.filepath);
                //ret = io_stop_save_file(sis.filepath);
#if defined(SUPPORT_XWFS)
                ret = xwfs_stop_save_file(sis.filepath);
 #elif defined(SUPPORT_FS)
                struct fs_context *fs_ctx;
                fs_ctx = get_fs_ctx();
                if(fs_ctx == NULL){
                    printf_warn("NOT FOUND DISK!!\n");
                    break;
                }
                fs_ctx->ops->fs_stop_save_file(ch, sis.filepath);
#endif
            }else{
                printf_err("error cmd\n");
                err_code = RET_CODE_PARAMTER_ERR;
            }
            if(ret != 0){
                err_code = akt_err_code_check(ret);
                goto set_exit;
            }
            break;
        }
        case BACKTRACE_IQ_CMD:
        {
            BACKTRACE_IQ_ST bis;
            int ret = 0;
            uint32_t max_bandwidth = 0;
            check_valid_channel(payload[0]);
            memcpy(&bis, payload, sizeof(BACKTRACE_IQ_ST));
            if(bis.cmd == 1){/* start backtrace iq file */
                printf_note("Start backtrace file:%s, bandwidth=%u\n", bis.filepath, bis.bandwidth);
                executor_set_command(EX_MID_FREQ_CMD, EX_BANDWITH, ch, &bis.bandwidth);
#if defined(SUPPORT_XWFS)
                ret = xwfs_start_backtrace(bis.filepath);
#elif defined(SUPPORT_FS)
                struct fs_context *fs_ctx;
                fs_ctx = get_fs_ctx();
                if(fs_ctx == NULL){
                    printf_warn("NOT FOUND DISK!!\n");
                    break;
                }
                fs_ctx->ops->fs_start_read_raw_file(ch, bis.filepath);
#endif
            }else if(bis.cmd == 0){/* stop backtrace iq file */
                printf_note("Stop backtrace file:%s\n", bis.filepath);
#if defined(SUPPORT_XWFS)
                ret = xwfs_stop_backtrace(bis.filepath);
#elif defined(SUPPORT_FS)
                struct fs_context *fs_ctx;
                fs_ctx = get_fs_ctx();
                if(fs_ctx == NULL){
                    printf_warn("NOT FOUND DISK!!\n");
                    break;
                }
                fs_ctx->ops->fs_stop_read_raw_file(ch, bis.filepath);
#endif
            }else{
                printf_err("error cmd\n");
                err_code = RET_CODE_PARAMTER_ERR;
            }
            if(ret != 0){
                err_code = akt_err_code_check(ret);
                goto set_exit;
            }
            break;
        }
        case STORAGE_DELETE_POLICY_CMD:
        {

            break;
        }
        case STORAGE_DELETE_FILE_CMD:
        {
            int ret = 0;
            char filename[FILE_PATH_MAX_LEN];
            memcpy(filename, payload, FILE_PATH_MAX_LEN);
#if defined(SUPPORT_XWFS)
            ret = xwfs_delete_file(filename);
#elif defined(SUPPORT_FS)
            struct fs_context *fs_ctx;
            fs_ctx = get_fs_ctx();
            if(fs_ctx == NULL){
                goto set_exit;
            }
            fs_ctx->ops->fs_delete(filename);
#endif
            printf_note("Delete file:%s, %d\n", filename, ret);
            if(ret != 0){
                err_code = akt_err_code_check(ret);
                goto set_exit;
            }
            break;
        }
        case DISK_FORMAT_CMD:
        {
			#if defined(SUPPORT_XWFS)
            file_disk_format(NULL, NULL);
           	#elif defined(SUPPORT_FS)
           	int ret;
            struct fs_context *fs_ctx;
			pthread_t work_id;
            fs_ctx = get_fs_ctx_ex();
            //需要增加协议返回当前格式化的状态
            ret=pthread_create(&work_id,NULL,(void *)fs_ctx->ops->fs_format, NULL);
    		if(ret!=0)
        		perror("pthread cread format disk");
    		pthread_detach(work_id);
            //ret = fs_ctx->ops->fs_format();
            //if(ret)
            //	printf_err("format failed.\n");
            #endif
            break;
        }
        case DEVICE_REBOOT_CMD:
        {
            check_valid_channel(payload[0]);
            printf_note("remote reboot!\n");
            safe_system("reboot -f");
            break;
        }
        case FILE_STROE_SIZE_CMD:
        {
            struct _file_store{
                uint8_t ch;
                uint64_t split_threshold;
            }__attribute__ ((packed));
            struct _file_store para;
            memcpy(&para, payload, sizeof(para));
            check_valid_channel(para.ch);
            config_set_split_file_threshold(para.split_threshold);
            printf_note("set split file threshold:%llu Bytes, %llu MB\n", para.split_threshold, (para.split_threshold / 1024 / 1024));
            break;
        }
        default:
          printf_err("not support class code[0x%x]\n",header->code);
          err_code = RET_CODE_PARAMTER_ERR;
          goto set_exit;
    }
set_exit:
    client->response.ch = (ch >= 0 ? ch : 0);
#if 0
    memcpy(akt_set_response_data.payload_data, &err_code, sizeof(err_code));
    akt_set_response_data.header.operation = SET_CMD_RSP;
    if(ch != -1){
        akt_set_response_data.cid = (uint8_t)ch;
        akt_set_response_data.header.len = sizeof(err_code)+1;  /* data+ch */
    }else{
        akt_set_response_data.header.len = sizeof(err_code)+1;  /* data+ch */
        akt_set_response_data.cid = 0;
    }
    printf_debug("set cid=%d\n", akt_set_response_data.cid);
#endif
    return err_code;

}

static void _find_file_list(char *filename, struct stat *stats, size_t *size)
{
    cJSON* item = NULL;
    if(stats == NULL || size == NULL)
        return;
    *size = stats->st_size;
}

static int akt_execute_get_command(void *cl)
{
    PDU_CFG_REQ_HEADER_ST *header;
    int err_code;
    struct net_tcp_client *client = cl;
    header = (PDU_CFG_REQ_HEADER_ST *)client->request.header;

    err_code = RET_CODE_SUCCSESS;
    printf_note("====>get bussiness code[%x]\n", header->code);
    switch (header->code)
    {
        case DEVICE_SELF_CHECK_CMD:
        {
            uint8_t lock_ok=0,external_clk=0;
            char *time_str= NULL;
            DEVICE_SELF_CHECK_STATUS_RSP_ST self_check;
            struct arg_s{
                uint32_t temp;
                float vol;
                float current;
                uint32_t resources;
            };
            struct arg_s fpga_status;
            io_get_fpga_status(&fpga_status);
            lock_ok = (io_get_inout_clock_status(&external_clk) == true ? 1 : 0);
            printf_debug("lock_ok=%d, external_clk=%d\n",lock_ok, external_clk);
            self_check.ext_clk = (external_clk == 0 ? 0 :1);
            self_check.ad_status = (io_get_adc_status(NULL) == true ? 0 : 1);
            self_check.pfga_temperature = fpga_status.temp;
            bzero(self_check.system_power_on_time, sizeof(self_check.system_power_on_time));
            if((time_str = get_proc_boot_time()) != NULL){
                strncpy(self_check.system_power_on_time, time_str, sizeof(self_check.system_power_on_time));
            }
            printf_note("power on time:%s\n", self_check.system_power_on_time);
            self_check.ch_num = MAX_RADIO_CHANNEL_NUM;
            executor_get_command(EX_RF_FREQ_CMD, EX_RF_STATUS_TEMPERAT, 0,  &self_check.t_s[0].rf_temperature);
            self_check.t_s[0].ch_status = (self_check.t_s[0].rf_temperature > 200 || 
                                           self_check.t_s[0].rf_temperature < -100||
                                           self_check.t_s[0].rf_temperature == 0) ? 1 : 0;  //可以通过判断获取的温度值是否在有效范围内来确定
            printf_note("ext_clk:%d, ad_status=%d,pfga_temperature=%d,ch_num=%d, rf_temperature=%d, ch_status=%d\n", 
                self_check.ext_clk, self_check.ad_status, self_check.pfga_temperature, self_check.ch_num,
                self_check.t_s[0].rf_temperature, self_check.t_s[0].ch_status);

            self_check.irig_b_status = 0;  //0:正常 1：异常
            self_check.gps_status = (gps_location_is_valid() == true ? 0 : 1);
            client->response.response_length = sizeof(DEVICE_SELF_CHECK_STATUS_RSP_ST);
            client->response.data = calloc(1, client->response.response_length);
            if(client->response.data == NULL){
                printf_err("calloc err!");
                break;
            }
            memcpy(client->response.data, &self_check, client->response.response_length);
                
            //memcpy(akt_get_response_data.payload_data, &self_check, sizeof(DEVICE_SELF_CHECK_STATUS_RSP_ST));
            //akt_get_response_data.header.len = sizeof(DEVICE_SELF_CHECK_STATUS_RSP_ST);
            break;
        }
        case SPCTRUM_PARAM_CMD:
        {
        #ifdef SUPPORT_SPECTRUM_ANALYSIS
            FFT_SIGNAL_RESPINSE_ST *resp_result;
            struct spectrum_analysis_result_vec **r;
            ssize_t number;
            size_t payload_len, i;
            
            number = spm_analysis_get_info(&r);
            payload_len = sizeof(FFT_SIGNAL_RESPINSE_ST) + sizeof(FFT_SIGNAL_RESULT_ST)*number;
            resp_result = safe_malloc(payload_len);
            printf_warn("#########Find SPM ANALYSIS Parameter[%d]:###############\n",number);
            for(i = 0; i < number; i++){
                resp_result->signal_array[i].center_freq = r[i]->mid_freq_hz;
                resp_result->signal_array[i].bandwidth = r[i]->bw_hz;
                resp_result->signal_array[i].power_level = r[i]->level;
                printf_warn("[%d] middle_freq=%lluHz, bw=%uHz, level=%f\n", i,
                    resp_result->signal_array[i].center_freq, resp_result->signal_array[i].bandwidth, resp_result->signal_array[i].power_level);
            }
            resp_result->signal_num = number;
            resp_result->temperature = io_get_ambient_temperature();
            resp_result->humidity = io_get_ambient_humidity();
            if(number > 0){
                for(i = 0; i < number; i++)
                    safe_free(r[i]);
                safe_free(r);
            }
            printf_warn("temperature:%0.2f℃, humidity:%0.2f%\n", resp_result->temperature, resp_result->humidity);
            client->response.response_length = payload_len;
            client->response.data = calloc(1, client->response.response_length);
            if(client->response.data == NULL){
                safe_free(resp_result);
                printf_err("calloc err!");
                break;
            }
            memcpy(client->response.data, resp_result, client->response.response_length);
            safe_free(resp_result);
        #else
            printf_warn("###########SPECTRUM ANALYSIS NOT SUPPORT#############\n");
        #endif
            break;
        }
        case SOFTWARE_VERSION_CMD:
        {
            struct poal_config *poal_config = &(config_get_config()->oal_config);
            struct _soft_info{
                uint8_t num;
                uint8_t name[16];
                uint8_t btime[20];
                uint8_t ver;
            }__attribute__ ((packed));
            
            struct _soft_info info;
             memset(&info, 0, sizeof(info));
            info.num = 1;
            printf_note("device sn=%s\n", poal_config->status_para.device_sn);
            memcpy(info.name, poal_config->status_para.device_sn, sizeof(info.name));
            sprintf(info.btime,"%s-%s",get_build_time(), __TIME__);
            printf_note("compile time:%s\n", info.btime);
            info.ver = 0x10;
            
            client->response.response_length = sizeof(info);
            client->response.data = calloc(1, client->response.response_length);
            if(client->response.data == NULL){
                printf_err("calloc err!");
                break;
            }
            memcpy(client->response.data, &info, client->response.response_length);

            //memcpy(akt_get_response_data.payload_data, &info, sizeof(info));
            //akt_get_response_data.header.len = sizeof(info);
        }
            break;
        /* disk cmd */
        case STORAGE_STATUS_CMD:
        {
            #define SSD_DISK_NUM 1
            STORAGE_STATUS_RSP_ST *psi;
            int i, ret = 0, st_size = 0;
            st_size = sizeof(STORAGE_STATUS_RSP_ST)+sizeof(STORAGE_DISK_INFO_ST)*SSD_DISK_NUM;
            psi = (STORAGE_STATUS_RSP_ST *)safe_malloc(st_size);
            if(psi == NULL){
                printf_err("malloc error!\n");
                ret = -ENOMEM;
                break;
            }
            #if defined(SUPPORT_XWFS)
            ret = xwfs_get_disk_info(psi);
            #elif defined(SUPPORT_FS)
            struct fs_context *fs_ctx;
            struct statfs diskInfo;
            fs_ctx = get_fs_ctx_ex();
            ret = fs_ctx->ops->fs_disk_info(&diskInfo);
            printf_debug("Get disk info: %d\n", ret);
            psi->disk_num = 1;
            psi->read_write_speed_kbytesps = 0;  //按照写速度换算约等于1.8G
            psi->disk_capacity[0].disk_state = fs_ctx->ops->fs_get_err_code();
            psi->disk_capacity[0].disk_capacity_byte = diskInfo.f_bsize * diskInfo.f_blocks;
            psi->disk_capacity[0].disk_used_byte = diskInfo.f_bsize * (diskInfo.f_blocks - diskInfo.f_bfree);
		    #endif
            printf_note("ret=%d, num=%d, speed=%uKB/s, capacity_bytes=%llu, used_bytes=%llu\n",
                ret, psi->disk_num, psi->read_write_speed_kbytesps, 
                psi->disk_capacity[0].disk_capacity_byte, psi->disk_capacity[0].disk_used_byte);
            client->response.response_length = st_size;
            client->response.data = calloc(1, client->response.response_length);
            if(client->response.data == NULL){
                printf_err("calloc err!");
                break;
            }
            memcpy(client->response.data, psi, client->response.response_length);
            //memcpy(akt_get_response_data.payload_data, psi, st_size);
            //akt_get_response_data.header.len = st_size;
            safe_free(psi);
            break;
        } 
        case SEARCH_FILE_STATUS_CMD:
        {
            int ret = 0;
            size_t f_bg_size = 0; 
            SEARCH_FILE_STATUS_RSP_ST fsp;
            char filename[FILE_PATH_MAX_LEN];
            
            memcpy(filename, client->dispatch.body, FILE_PATH_MAX_LEN);
            filename[sizeof(filename) - 1] = 0;
            memset(&fsp, 0 ,sizeof(SEARCH_FILE_STATUS_RSP_ST));
            strcpy(fsp.filepath, filename);
            #if defined(SUPPORT_XWFS)
            ret = xwfs_get_file_size_by_name(filename, &f_bg_size, sizeof(ssize_t));//io_read_more_info_by_name(filename, &fsp, io_find_file_info);
            if(ret != 0){
                fsp.status = 0;
                fsp.file_size = 0;
            }else{
                fsp.status = 1;
                fsp.file_size = (uint64_t)f_bg_size*8*1024*1024; /* data block group size is 8MByte */
            }
            #elif defined(SUPPORT_FS)
            struct fs_context *fs_ctx;
            fs_ctx = get_fs_ctx();
            if(fs_ctx != NULL){
                fs_ctx->ops->fs_find(filename, _find_file_list, &f_bg_size);
            }
            if(ret != 0){
                fsp.status = 0;
                fsp.file_size = 0;
            }else{
                fsp.status = 1;
                fsp.file_size = (uint64_t)f_bg_size;
            }
            #endif
            printf_note("Find file:%s, fsize=%u ret =%d\n", fsp.filepath, f_bg_size, ret);
            
            printf_note("ret=%d, filepath=%s, file_size=[%u bg]%llu, status=%d\n",ret, fsp.filepath, f_bg_size, fsp.file_size, fsp.status);
            client->response.response_length = sizeof(SEARCH_FILE_STATUS_RSP_ST);
            client->response.data = calloc(1, client->response.response_length);
            if(client->response.data == NULL){
                printf_err("calloc err!");
                break;
            }
            memcpy(client->response.data, &fsp, client->response.response_length);
            break;
        }

        case COMPASS_REQ_CMD:
        {
            float angle = -1.0f;
        #if defined(SUPPORT_RS485_EC)
            elec_compass1_com_get_angle(&angle);
        #endif
            client->response.response_length = sizeof(float);
            client->response.data = calloc(1, client->response.response_length);
            if(client->response.data == NULL){
                printf_err("calloc err!");
                break;
            }
            memcpy(client->response.data, &angle, client->response.response_length);
            break;
        }

        case DEVICE_MODEL_CMD:
        {
            struct poal_config *poal_config = &(config_get_config()->oal_config);
            struct _device_model{
                int8_t type[10];
            }__attribute__ ((packed));
            struct _device_model model;
            memset(&model, 0, sizeof(model));
            memcpy(model.type, poal_config->status_para.device_sn, sizeof(model.type));
            printf_debug("device mode:%s\n", model.type);
            client->response.response_length = sizeof(model);
            client->response.data = calloc(1, client->response.response_length);
            if(client->response.data == NULL){
                printf_err("calloc err!");
                break;
            }
            memcpy(client->response.data, &model, client->response.response_length);
            break;
        }

        case FILE_LIST_CMD:
        {
            FILE_LIST_INFO *file_list = NULL;
            int32_t payload_len = 0;
        #if defined(SUPPORT_FS)
            cJSON *value = NULL;
            cJSON *node = NULL;
            cJSON *array = cJSON_CreateArray();
            struct fs_context *fs_ctx;
            fs_ctx = get_fs_ctx();
            if(fs_ctx == NULL) {
                printf_err("get fd ctx failed!");
                break;
            }
            fs_ctx->ops->fs_dir(NULL, fs_file_list, array);
            payload_len = sizeof(FILE_LIST_INFO) + sizeof(SINGLE_FILE_INFO) * cJSON_GetArraySize(array);
            file_list = (FILE_LIST_INFO *)safe_malloc(payload_len);
            file_list->file_num = cJSON_GetArraySize(array);
            for (int i = 0; i < cJSON_GetArraySize(array); i++) {
                node = cJSON_GetArrayItem(array, i);            
                value = cJSON_GetObjectItem(node, "size");
                if(cJSON_IsNumber(value)){
                    file_list->files[i].file_size = value->valueint;
                }
                value = cJSON_GetObjectItem(node, "createTime");
                if(cJSON_IsNumber(value)){
                    file_list->files[i].file_mtime = value->valueint;
                }
                value = cJSON_GetObjectItem(node, "filename");
                if(cJSON_IsString(value)){
                    memcpy(file_list->files[i].file_path, value->valuestring, strlen(value->valuestring));
                }
            }

            client->response.response_length = payload_len;
            client->response.data = calloc(1, client->response.response_length);
            if(client->response.data == NULL){
                safe_free(file_list);
                printf_err("calloc err!");
                break;
            }
            memcpy(client->response.data, file_list, client->response.response_length);
            safe_free(file_list);
         #endif
            break;
        }
        default:
            printf_debug("not support get commmand\n");
    }
exit:
    return err_code;
}

static int akt_execute_discovery_command(void *client, const char *buf, int len)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    
    DEVICE_NET_INFO_ST netinfo;
    struct in_addr ipdata;
    struct net_udp_client *cl = NULL;
    struct sockaddr_in addr;
    struct discover_net{
        uint32_t ipaddr;
        uint16_t port;
    };
    /* parse ipaddress&port */
    struct discover_net dis_net;
    memcpy(&dis_net, buf, sizeof(struct discover_net));
    printf_note("ipaddr = 0x%x, port=0x%x[%d]\n", dis_net.ipaddr, dis_net.port,  dis_net.port);
    addr.sin_port = dis_net.port;
    addr.sin_addr.s_addr = dis_net.ipaddr;
    addr.sin_family = AF_INET;   /* fixedb bug: Address family not supported by protocol */
    
    cl = (struct net_udp_client *)client;
#ifdef  SUPPORT_NET_WZ
    if(cl->ifname && !strcmp(cl->ifname, NETWORK_10G_EHTHERNET_POINT)){
        memcpy(netinfo.mac, poal_config->network_10g.mac, sizeof(netinfo.mac));
        netinfo.ipaddr = htonl(poal_config->network_10g.ipaddress);
        netinfo.gateway = htonl(poal_config->network_10g.gateway);
        netinfo.mask = htonl(poal_config->network_10g.netmask);
        netinfo.port = htons(poal_config->network_10g.port);
        netinfo.status = 0;
        ipdata.s_addr = poal_config->network_10g.ipaddress;
    }else
#endif
    {
        memcpy(netinfo.mac, poal_config->network.mac, sizeof(netinfo.mac));
        netinfo.ipaddr = htonl(poal_config->network.ipaddress);
        netinfo.gateway = htonl(poal_config->network.gateway);
        netinfo.mask = htonl(poal_config->network.netmask);
        netinfo.port = htons(poal_config->network.port);
        netinfo.status = 0;
        ipdata.s_addr = poal_config->network.ipaddress;
    }
    printf_note("ifname:%s,mac:%x%x%x%x%x%x, ipaddr=%x[%s], gateway=%x\n", cl->ifname, netinfo.mac[0],netinfo.mac[1],netinfo.mac[2],netinfo.mac[3],netinfo.mac[4],netinfo.mac[5],
                                                        netinfo.ipaddr, inet_ntoa(ipdata), netinfo.gateway);
    memcpy(&cl->discover_peer_addr, &addr, sizeof(addr));
    akt_send_resp_discovery(client, &netinfo, sizeof(DEVICE_NET_INFO_ST));

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
bool akt_execute_method(void *cl, int *code)
{
    PDU_CFG_REQ_HEADER_ST_EX *header;
    int err_code;
    struct net_tcp_client *client = cl;
    header = (PDU_CFG_REQ_HEADER_ST_EX *)client->request.header;

    err_code = RET_CODE_SUCCSESS;
    printf_note("operation code[%x]\n", header->operation);
    switch (header->operation)
    {
        case SET_CMD_REQ:
        {
            update_tcp_keepalive(cl);
            err_code = akt_execute_set_command(cl);
            break;
        }
        case QUERY_CMD_REQ:
        {
            update_tcp_keepalive(cl);
            err_code = akt_execute_get_command(cl);
            break;
        }
        case NET_CTRL_CMD:
        {
            if(header->code == HEART_BEAT_MSG_REQ){
                update_tcp_keepalive(cl);
            //    akt_get_response_data.header.len = 0;
            }// else if(header->code == DISCOVER_LAN_DEV_PARAM){
               // printf_note("discover ...\n");
               // err_code = akt_execute_net_command(cl);
            //}else{
               // err_code = akt_execute_net_command(cl);
           // }
            
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

/* UDP */
bool akt_parse_discovery(void *client, const char *buf, int len)
{
    #define _DISCOVERY_CODE 0xaa
    #define _DISCOVERY_BUSINESS_CODE 0xff
    #define _DISCOVERY_PAYLOAD_LEN 6    /* ip(4)+port(2) */
    
    struct net_udp_client *cl = client;
    PDU_CFG_REQ_HEADER_ST_EX header;
    int hlen = 0 , i;
    char *payload = NULL;
    int payload_len = 0;
    hlen = sizeof(PDU_CFG_REQ_HEADER_ST_EX);
    
    if(len < (hlen+_DISCOVERY_PAYLOAD_LEN) || buf == NULL)
        return false;
    
    memcpy(&header, buf, hlen);
    
    if(header.start_flag != AKT_START_FLAG){
        return false;
    }
    if(header.operation != _DISCOVERY_CODE || header.code != _DISCOVERY_BUSINESS_CODE){
        printf_err("err header.operation_code=%x\n", header.operation);
        printf_err("err header.code=%x\n", header.code);
        return false;
    }
    printf_debug("header.data len=%x\n", header.len);
    printf_debug("header.operation_code=%x\n", header.operation);
    printf_debug("header.code=%x\n", header.code);
    printf_debug("header.usr_id:");
    for(i = 0; i< sizeof(header.usr_id); i++){
        printfd("%x", header.usr_id[i]);
    }
    printfd("\n");
    printf_debug("header.receiver_id=%x\n", header.receiver_id);
    printf_debug("header.crc=%x\n", header.crc);

    if(header.len != _DISCOVERY_PAYLOAD_LEN){
        printf_debug("err header.len=%d\n", header.len);
        return false;
    }

    printfd("Discovery: ");
    for(i = 0; i< len; i++)
        printfd("%02x ", buf[i]);
    printfd("\n");
    payload_len = header.len;
    payload = buf + hlen;
    akt_execute_discovery_command(cl, payload, payload_len);

    return true;
}

bool akt_send_resp_discovery(void *client, const char *pdata, int len)
{
    #define _DISCOVERY_CODE 0xaa
    #define _DISCOVERY_BUSINESS_CODE 0xff
    uint16_t end_flag = AKT_END_FLAG;
    PDU_CFG_RSP_HEADER_ST rsp_header;
    int header_len=0, offset = 0;
    struct net_udp_client *cl = client;
    char send_buf[sizeof(rsp_header) + len + sizeof(end_flag)];

    header_len = sizeof(rsp_header);
    memset(&rsp_header, 0, header_len);
    rsp_header.start_flag = AKT_START_FLAG;
    rsp_header.operation = _DISCOVERY_CODE;
    rsp_header.code  = _DISCOVERY_BUSINESS_CODE;
    rsp_header.len = len;

    memcpy(send_buf, &rsp_header, header_len);
    offset+=header_len;
    memcpy(send_buf+offset, pdata, len);
    offset+=len;
    memcpy(send_buf+offset, &end_flag, sizeof(end_flag));
    offset+=sizeof(end_flag);

    printfd("Discovery resp: ");
    for(int i = 0; i< len; i++)
        printfd("%02x ", send_buf[i]);
    printfd("\n");

    int n;
    n = sendto(cl->srv->fd.fd, send_buf, offset, 0, (struct sockaddr *)&cl->discover_peer_addr, sizeof(struct sockaddr));
    if(n < 0){
        printf_err("n=%d, errno=%d[%s]\n", n, errno, strerror(errno));
    }
    return true;
}



bool akt_parse_header_v2(void *client, const char *buf, int len, int *head_len, int *code)
{
    struct net_tcp_client *cl = client;
    PDU_CFG_REQ_HEADER_ST_EX *header;
    int hlen = 0 , i;
    hlen = sizeof(PDU_CFG_REQ_HEADER_ST_EX);
    
    header = calloc(1, hlen);
    if (!header) {
        printf_err("calloc\n");
        exit(1);
    }
    if(len < hlen){
        *head_len = len;
        *code = RET_CODE_HEADER_ERR;
        printf_err("header err\n");
        goto exit;
    }
    memcpy(header, buf, hlen);
    
    for(i = 0; i< len; i++)
        printfd("%02x ", buf[i]);
    printfd("\n");
    
    if(header->start_flag != AKT_START_FLAG){
        printf_err("parse_header error\n");
        *head_len = hlen;
        *code = RET_CODE_FORMAT_ERR;
        goto exit;
    }
    printf_debug("header.data len=%x\n", header->len);
    printf_debug("header.operation_code=%x\n", header->operation);
    printf_debug("header.code=%x\n", header->code);
    printf_debug("header.usr_id:");
    for(i = 0; i< sizeof(header->usr_id); i++){
        printfd("%x", header->usr_id[i]);
    }
    printfd("\n");
    printf_debug("header.receiver_id=%x\n", header->receiver_id);
    printf_debug("header.crc=%x\n", header->crc);
    *head_len = hlen;
    *code = RET_CODE_SUCCSESS;

exit:
    cl->request.header = header;
    cl->request.content_length = header->len; 
    
    return (*code == RET_CODE_SUCCSESS ? true : false);
}


int  akt_parse_end(void *client, char *buf, int len)
{
    struct net_tcp_client *cl = client;
    uint16_t end_flag = AKT_END_FLAG;
    int ret_len;
    
    if(cl == NULL || buf == NULL || len == 0)
        goto exit;
    
    if(len < sizeof(end_flag)){
        goto exit;
    }
    
    if(*(uint16_t *)buf == end_flag){
         return sizeof(end_flag);
    }
    
exit:
    ret_len = min(sizeof(end_flag), len);
    //cl->srv->send_error(cl, RET_CODE_HEADER_ERR, NULL);
    printf_note("ret_len=%d\n", ret_len);
    return ret_len;
}



void akt_send_resp(void *client, int code, void *args)
{
    struct net_tcp_client *cl = client;
    PDU_CFG_REQ_HEADER_ST_EX *req_header;
    PDU_CFG_RSP_HEADER_ST *rsp_header;
    char *psend;
    char *ptr_rsp;
    uint16_t end_flag = AKT_END_FLAG;
    int rsp_len;
    printf_debug("AKT Send Rsp...[%d]\n", code);
    rsp_len = sizeof(PDU_CFG_RSP_HEADER_ST);
    ptr_rsp = calloc(1, rsp_len);
    if(ptr_rsp == NULL){
        printf_err("calloc err!\n");
        return;
    }
    rsp_header = ptr_rsp;
    req_header = cl->request.header;
    printf_debug("req_header->operation=%d\n", req_header->operation);
    if(req_header->operation == SET_CMD_REQ){
        rsp_header->operation = SET_CMD_RSP;
        rsp_header->code = req_header->code;
        cl->response.data = calloc(1, sizeof(code));
        if(cl->response.data == NULL)
            goto exit;
        memcpy(cl->response.data, &code, sizeof(code));
        cl->response.response_length = sizeof(uint32_t);
        
        /* 设置反馈命令需要反馈通道号 */
        rsp_len++;
        rsp_header = realloc(rsp_header, rsp_len);
        if(rsp_header == NULL){
            printf_err("realloc err!\n");
            goto exit;
        }
        ptr_rsp[rsp_len - 1] = cl->response.ch;
        /* 设置反馈命令数据长度要加上通达号长度（1个字节） */
        rsp_header->len = cl->response.response_length + 1;
    }
    else if(req_header->operation == QUERY_CMD_REQ){
        rsp_header->operation = QUERY_CMD_RSP;
        rsp_header->code = req_header->code;
        rsp_header->len = cl->response.response_length;
    }
    else if(req_header->operation == NET_CTRL_CMD){
        if(req_header->code == HEART_BEAT_MSG_REQ){
            rsp_header->operation = req_header->operation;
            rsp_header->code  = HEART_BEAT_MSG_RSP;
            rsp_header->len = 0;
        }
        else if(req_header->code == DISCOVER_LAN_DEV_PARAM){
            rsp_header->operation = req_header->operation;
            rsp_header->code  = req_header->code;
            rsp_header->len = cl->response.response_length;
        }
    }
    rsp_header->start_flag = AKT_START_FLAG;
    rsp_header->receiver_id = 0;

   // rsp_header->crc = htons(crc16_caculate((uint8_t *)cl->response.data, cl->response.response_length));
    rsp_header->crc = crc16_caculate((uint8_t *)cl->response.data, cl->response.response_length);
    printf_debug("crc=0x%x", rsp_header->crc);
    memcpy(rsp_header->usr_id, req_header->usr_id, sizeof(req_header->usr_id));

#if 0
    int i;
    printfn("Send Response: \n");
    for(i = 0; i< rsp_len; i++){
        printfn("%02x ", ptr_rsp[i]);
    }
    char *data = (char *)cl->response.data;
    for(i = 0; i< cl->response.response_length; i++){
        printfn("%02x ", data[i]);
    }
    data = (char *)&end_flag;
    for(i = 0; i< sizeof(end_flag); i++){
        printfn("%02x ", data[i]);
    }
    printfn("\n");
#endif

    size_t send_len, offset = 0;
    send_len = rsp_len+cl->response.response_length+sizeof(end_flag);
    psend = calloc(1, send_len);
    if(psend == NULL){
        printf_err("psend err!\n");
        goto exit;
    }
    
    memcpy(psend, rsp_header, rsp_len);
    offset += rsp_len;
    if(cl->response.response_length > 0){
        memcpy(psend+offset, cl->response.data, cl->response.response_length);
        offset += cl->response.response_length;
    }
    
    memcpy(psend+offset, &end_flag, sizeof(end_flag));
    int i;
    for(i = 0; i< send_len; i++){
        printfd("%02x ", psend[i]);
    }
    printfd("\n");
    cl->send(cl, psend, send_len);
exit:
    safe_free(rsp_header);
}

int  akt_assamble_send_data(uint8_t *send_buf, uint8_t *payload, uint32_t payload_len, int code)
{
    struct response_get_data *response_data;
    int len = 0;
    int header_len=0;

    if(send_buf == NULL || payload_len == 0){
        return -1;
    }
    header_len = sizeof(PDU_CFG_RSP_HEADER_ST);
    printf_info("header_len:%d\n", header_len);
    response_data = (struct response_get_data *)send_buf;
    response_data->header.start_flag = AKT_START_FLAG; 
    response_data->header.receiver_id = 0;
    if(payload_len > sizeof(response_data->payload_data)){
        payload_len = sizeof(response_data->payload_data);
    }
    if(payload != NULL)
        memcpy(response_data->payload_data, payload, payload_len);
    memset(response_data->header.usr_id, 0, sizeof(response_data->header.usr_id));
    response_data->header.len = payload_len;
    response_data->header.crc = crc16_caculate((uint8_t *)response_data->payload_data, payload_len);
    response_data->header.operation = SET_CMD_RSP;
    response_data->header.code = code;//NOTIFY_SIGNALl_STATUS;
    response_data->end_flag = AKT_END_FLAG;
    len = header_len + response_data->header.len;
    memcpy(send_buf+len, (uint8_t *)&response_data->end_flag, sizeof(response_data->end_flag));
    len +=  sizeof(response_data->end_flag);
    return len;
}

void akt_send(void *client, const void *data, int len, int code)
{
    char *psend;
    size_t send_len, offset = 0;
    send_len = sizeof(struct response_get_data);
    struct net_tcp_client *cl = client;
    
    psend = calloc(1, send_len);
    if(psend == NULL){
        printf_err("psend err!\n");
        return;
    }
    if(akt_assamble_send_data(psend, data, len, code) != 0){
        goto exit;
    }
    cl->send(cl, psend, send_len);

exit:
    safe_free(psend);
}


void akt_send_alert(void *client, int code)
{
    akt_send(client, NULL, 0, code);
}



int  akt_assamble_send_active_data(uint8_t *send_buf, uint8_t *payload, uint32_t payload_len)
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


uint8_t *akt_assamble_demodulation_header_data(uint32_t *len, void *config)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    DATUM_DEMODULATE_HEADER_ST *ext_hdr = NULL;
    char *ptr;
    int ch = poal_config->cid;

    ext_hdr = calloc(1, sizeof(DATUM_SPECTRUM_HEADER_ST));
    if(ext_hdr == NULL){
        return NULL;
    }
    ptr = (char *)ext_hdr;
    printf_debug("akt_assamble_data_extend_frame_header_data\n");
    ext_hdr->dev_id = akt_get_device_id();
    
    ext_hdr->work_mode =  poal_config->channel[ch].work_mode;
    ext_hdr->gain_mode = poal_config->channel[ch].rf_para.gain_ctrl_method;
    ext_hdr->gain = poal_config->channel[ch].rf_para.mgc_gain_value;

    ext_hdr->duration = 0;
    struct spm_run_parm *header_param;
    header_param = (struct spm_run_parm *)config;
    ext_hdr->cid = header_param->ch;
    ext_hdr->center_freq = header_param->m_freq;
    ext_hdr->bandwidth = header_param->bandwidth;
    ext_hdr->demodulate_type = header_param->d_method;
    ext_hdr->sample_rate = header_param->sample_rate;
    ext_hdr->frag_total_num = 1;
    ext_hdr->frag_cur_num = 0;
    ext_hdr->frag_data_len = (int16_t)(header_param->data_len);
#if 1
    printfd("-----------------------------assamble_spectrum_header-----------------------------------\n");
    printfd("dev_id[%d], cid[%d], work_mode[%d], gain_mode[%d]\n", 
        ext_hdr->dev_id, ext_hdr->cid, ext_hdr->work_mode, ext_hdr->gain_mode);
    printfd("d_method[%d], m_freq[%llu], bandwidth[%u]\n", 
        ext_hdr->demodulate_type, header_param->m_freq, header_param->bandwidth);
    printfd("----------------------------------------------------------------------------------------\n");
#endif
    printfd("DEMODULATE Extend frame header:\n");
    for(int i = 0; i< sizeof(DATUM_DEMODULATE_HEADER_ST); i++){
        printfd("%x ", *(ptr+i));
    }
    printfd("\n");

    *len = sizeof(DATUM_DEMODULATE_HEADER_ST);
    return ptr;
}



/******************************************************************************
* FUNCTION:
*     akt_assamble_data_extend_frame_header_data
*
* DESCRIPTION:
*     assamble data(FFT/IQ...) extend frame header
* PARAMETERS
* RETURNS
******************************************************************************/
uint8_t *akt_assamble_data_extend_frame_header_data(uint32_t *len, void *config)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    //static uint8_t param_buf[sizeof(DATUM_SPECTRUM_HEADER_ST)];
    DATUM_SPECTRUM_HEADER_ST *ext_hdr = NULL;
    char *ptr;
    int ch = poal_config->cid;

    ext_hdr = calloc(1, sizeof(DATUM_SPECTRUM_HEADER_ST));
    if(ext_hdr == NULL){
        return NULL;
    }
    ptr = (char *)ext_hdr;
    printf_debug("akt_assamble_data_extend_frame_header_data\n");
    ext_hdr->dev_id = akt_get_device_id();
    ext_hdr->work_mode =  poal_config->channel[ch].work_mode;
    ext_hdr->gain_mode = poal_config->channel[ch].rf_para.gain_ctrl_method;
    ext_hdr->gain = poal_config->channel[ch].rf_para.mgc_gain_value;
    ext_hdr->duration = 0;
    ext_hdr->datum_type = 1;

    struct spm_run_parm *header_param;
    header_param = (struct spm_run_parm *)config;
    ext_hdr->cid = header_param->ch;
    ext_hdr->center_freq = header_param->m_freq;
    ext_hdr->sn = header_param->fft_sn;
    ext_hdr->datum_total = header_param->total_fft;
    ext_hdr->bandwidth = header_param->bandwidth;
    ext_hdr->start_freq = header_param->s_freq;
    ext_hdr->cutoff_freq = header_param->e_freq;
    ext_hdr->sample_rate = header_param->sample_rate;;
    ext_hdr->fft_len = header_param->fft_size;
    ext_hdr->freq_resolution = header_param->freq_resolution;
    ext_hdr->frag_total_num = 1;
    ext_hdr->frag_cur_num = 0;
    ext_hdr->frag_data_len = (int16_t)((float)(header_param->data_len)/BAND_FACTOR);
    

#if 1
    printfd("-----------------------------assamble_spectrum_header-----------------------------------\n");
    printfd("dev_id[%d], cid[%d], work_mode[%d], gain_mode[%d]\n", 
        ext_hdr->dev_id, ext_hdr->cid, ext_hdr->work_mode, ext_hdr->gain_mode);
    printfd("gain[%d], duration[%llu], start_freq[%llu], cutoff_freq[%llu], center_freq[%llu]\n",
         ext_hdr->gain, ext_hdr->duration, ext_hdr->start_freq, ext_hdr->cutoff_freq, ext_hdr->center_freq);
    printfd("bandwidth[%u], sample_rate[%u], freq_resolution[%f], fft_len[%u], datum_total=%d\n",
        ext_hdr->bandwidth,ext_hdr->sample_rate,ext_hdr->freq_resolution,ext_hdr->fft_len,ext_hdr->datum_total);
    printfd("sn[%d], datum_type[%d],frag_data_len[%d]\n", 
        ext_hdr->sn,ext_hdr->datum_type,ext_hdr->frag_data_len);
    printfd("----------------------------------------------------------------------------------------\n");
#endif
    //memcpy(param_buf, ex_buf, sizeof(DATUM_SPECTRUM_HEADER_ST));
    printfd("Extend frame header:\n");
    for(int i = 0; i< sizeof(DATUM_SPECTRUM_HEADER_ST); i++){
        printfd("%x ", *(ptr+i));
    }
    printfd("\n");

    *len = sizeof(DATUM_SPECTRUM_HEADER_ST);
    return ptr;
}

/******************************************************************************
* FUNCTION:
*     akt_assamble_data_frame_header_data
*
* DESCRIPTION:
*     assamble data(FFT/IQ...) frame header
* PARAMETERS: 
*       @len: return total header len(data header+extend data header)
* RETURNS Need to Free
******************************************************************************/
void *akt_assamble_data_frame_header_data(uint32_t *len, void *config)
{
    DATUM_PDU_HEADER_ST *package_header;
    static unsigned short seq_num[MAX_RADIO_CHANNEL_NUM] = {0};
    struct spm_run_parm *header_param;
    uint8_t *pextend;
    uint32_t extend_data_header_len;
    struct timeval tv;
    uint8_t *ptr = NULL;

    printf_debug("akt_assamble_data_frame_header_data. v3\n");
    header_param = (struct spm_run_parm *)config;
    printf_debug("header_param->ex_type:%d\n", header_param->ex_type);
    if(header_param->ex_type == SPECTRUM_DATUM)
        pextend = akt_assamble_data_extend_frame_header_data(&extend_data_header_len, config);
    else if(header_param->ex_type == DEMODULATE_DATUM){
        pextend = akt_assamble_demodulation_header_data(&extend_data_header_len,  config);
    }else{
        printf_err("extend type is not valid!\n");
        return NULL;
    }
    ptr = calloc(1, sizeof(DATUM_PDU_HEADER_ST) + extend_data_header_len);
    if(ptr == NULL){
        safe_free(pextend);
        return NULL;
    }
    memcpy(ptr+ sizeof(DATUM_PDU_HEADER_ST), pextend, extend_data_header_len);
    
    package_header = (DATUM_PDU_HEADER_ST*)ptr;
    package_header->syn_flag = AKT_START_FLAG;
    package_header->type = header_param->type;
    gettimeofday(&tv,NULL);
    package_header->toa = tv.tv_sec*1000000 + tv.tv_usec;
    package_header->seqnum = seq_num[header_param->ch]++;
    package_header->virtual_ch = 0;
    memset(package_header->reserve, 0, sizeof(package_header->reserve));
    package_header->ex_type = header_param->ex_type;
    package_header->ex_len = extend_data_header_len;
    package_header->data_len = header_param->data_len;
    package_header->crc = 0;
    printfd("-----------------------------assamble_pdu_header----------------------------------------\n");
    printfd("toa=[%llu], seqnum[%d], ex_len[%d],data_len[%d]\n",  package_header->toa, package_header->seqnum, package_header->ex_len, package_header->data_len);
    printfd("----------------------------------------------------------------------------------------\n");
    *len = sizeof(DATUM_PDU_HEADER_ST) + extend_data_header_len;
    printfd("Header len[%d]: \n", sizeof(DATUM_PDU_HEADER_ST));
    for(int i = 0; i< sizeof(DATUM_PDU_HEADER_ST); i++){
        printfd("%x ", *(ptr + i));
    }
    printfd("\n");
    printfd("extend header len[%d]: \n", extend_data_header_len);
    for(int i = 0; i< extend_data_header_len; i++){
        printfd("%x ", *(ptr + sizeof(DATUM_PDU_HEADER_ST)+i));
    }
    printfd("\n");
    safe_free(pextend);
    return ptr;
}




