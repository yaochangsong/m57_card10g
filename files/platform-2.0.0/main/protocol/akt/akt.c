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
bool akt_send_resp_discovery(void *client, const void *pdata, int len);

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

#ifdef CONFIG_SPM_FFT_SCAN_SEGMNET
static int scan_segment_param_convert(uint8_t ch)
{
    uint8_t finish = 0;
    uint32_t i = 0, j = 0, k = 0;
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    struct multi_freq_fregment_para_st *fregment;
    struct freq_fregment_para_st  array_fregment[MAX_SIG_CHANNLE];
    if (poal_config->channel[ch].work_mode != OAL_FAST_SCAN_MODE && poal_config->channel[ch].work_mode != OAL_MULTI_ZONE_SCAN_MODE) {
        return 0;
    }
    int array_len = 0;
    int64_t *division_point_array =  get_division_point_array(ch, &array_len);
    if (!division_point_array || array_len == 0) {
        printf("don't need to divise\n");
        return 0;
    }
    fregment = &poal_config->channel[ch].multi_freq_fregment_para;
    for (i = 0; i < fregment->freq_segment_cnt; i++) {
        finish = 0;  //
        for (k = 0; k < array_len; k++) {
            if (fregment->fregment[i].end_freq <= division_point_array[k]) {
                memcpy(&array_fregment[j], &fregment->fregment[i], sizeof(struct freq_fregment_para_st));
                j++;
                finish = 1;
                break;
            } else if (fregment->fregment[i].start_freq >= division_point_array[k]) {
                continue;
            } else { //if(fregment->fregment[i].start_freq < division_point_array[k] && fregment->fregment[i].end_freq > division_point_array[k]) {
                 memcpy(&array_fregment[j], &fregment->fregment[i], sizeof(struct freq_fregment_para_st));
                 array_fregment[j].end_freq = division_point_array[k];
                 j++;
                 fregment->fregment[i].start_freq = division_point_array[k];  //note
                 continue;
            }
        }
        if (!finish) {
            memcpy(&array_fregment[j], &fregment->fregment[i], sizeof(struct freq_fregment_para_st));
            j++;
        }
    }
    if (j > fregment->freq_segment_cnt) {
        fregment->freq_segment_cnt = j;
        #if 0
        if(fregment->freq_segment_cnt > 1) {
            poal_config->channel[ch].work_mode = OAL_MULTI_ZONE_SCAN_MODE;
        }
        #endif
        memcpy(&fregment->fregment[0], &array_fregment[0], sizeof(struct freq_fregment_para_st)*MAX_SIG_CHANNLE);
    }

    printf_debug("--------------after division------------------\n");
    for (i = 0; i < fregment->freq_segment_cnt; i++) {
        printf_debug("i:%d, start:%"PRIu64", end:%"PRIu64"\n", i, fregment->fregment[i].start_freq, fregment->fregment[i].end_freq);
    }
    return 0;
}
#endif
int get_origin_start_end_freq(uint8_t ch, uint64_t start_in, uint64_t end_in, uint64_t *start_out, uint64_t *end_out)
{
    int i = 0;
    uint8_t found = 0;
    uint64_t start_freq, end_freq;
    struct akt_protocal_param *pakt_config = &akt_config;
    DIRECTION_MULTI_FREQ_ZONE_PARAM *muti_zone = &pakt_config->multi_freq_zone[ch];
    for (i = 0; i < muti_zone->freq_band_cnt; i++) {
        start_freq = muti_zone->sig_ch[i].center_freq - muti_zone->sig_ch[i].bandwidth/2;
        end_freq = muti_zone->sig_ch[i].center_freq + muti_zone->sig_ch[i].bandwidth/2;
        if (start_in >= start_freq && end_in <= end_freq) {
            *start_out = start_freq;
            *end_out = end_freq;
            found = 1;
            break;
        }
    }
    if (found) {
        return 0;
    } else {
        printf_err("get origin freq fail, it shouldn't be here\n");
        return -1;
    }
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
                printf_info("ch:%d,center_freq:%"PRIu64"\n", ch, poal_config->channel[ch].multi_freq_point_param.points[i].center_freq);
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
            printf_info("subch:%d,d_method:%d,%d raw dmethod:%d\n", ch, sub_channel_array->sub_ch[subch].raw_d_method, sub_channel_array->sub_ch[subch].d_method, sub_channel_array->sub_ch[subch].raw_d_method);
            printf_info("center_freq:%"PRIu64", d_bandwith=%u\n", sub_channel_array->sub_ch[subch].center_freq, sub_channel_array->sub_ch[subch].d_bandwith);
       }
            break;
       case OUTPUT_ENABLE_PARAM:
                /* 使能位转换 */
            poal_config->channel[ch].enable.cid = pakt_config->enable.cid;
            poal_config->channel[ch].enable.sub_id = -1;
            poal_config->channel[ch].enable.map.bit.fft = convert_enable_mode(pakt_config->enable.output_en, SPECTRUM_MASK);
            poal_config->channel[ch].enable.map.bit.audio = convert_enable_mode(pakt_config->enable.output_en, D_OUT_MASK);
            poal_config->channel[ch].enable.map.bit.iq = convert_enable_mode(pakt_config->enable.output_en, IQ_OUT_MASK);
            printf_note("work_mode=%d\n", poal_config->channel[ch].work_mode);
            break;
        case SUB_SIGNAL_OUTPUT_ENABLE_CMD:
            poal_config->channel[ch].sub_channel_para.sub_ch_enable[subch].cid = pakt_config->sub_channel_enable[subch].cid;
            poal_config->channel[ch].sub_channel_para.sub_ch_enable[subch].sub_id = pakt_config->sub_channel_enable[subch].signal_ch;
            poal_config->channel[ch].sub_channel_para.sub_ch_enable[subch].map.bit.fft = convert_enable_mode(pakt_config->sub_channel_enable[subch].en, SPECTRUM_MASK);
            poal_config->channel[ch].sub_channel_para.sub_ch_enable[subch].map.bit.audio = convert_enable_mode(pakt_config->sub_channel_enable[subch].en, D_OUT_MASK);
            poal_config->channel[ch].sub_channel_para.sub_ch_enable[subch].map.bit.iq = convert_enable_mode(pakt_config->sub_channel_enable[subch].en, IQ_OUT_MASK);
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
                        printf_info("freq_resolution:%u\n",point->points[sig_cnt].freq_resolution);
                    }
                     /* smooth */
                    point->smooth_time = pakt_config->smooth[ch].smooth;
                    printf_note("raw_d_method:%d, d_method:%u\n",point->points[sig_cnt].raw_d_method, point->points[sig_cnt].d_method);
                    printf_note("residence_time:%ums\n",point->residence_time);
                    printf_note("freq_point_cnt:%u\n",point->freq_point_cnt);
                    printf_note("ch:%d, sig_cnt:%d,center_freq:%"PRIu64"\n", ch,sig_cnt,point->points[sig_cnt].center_freq);
                    printf_note("bandwidth:%"PRIu64"\n",point->points[sig_cnt].bandwidth);
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
                    printf_info("start_freq:%"PRIu64"\n", fregment->fregment[sig_cnt].start_freq);
                    printf_info("end_freq:%"PRIu64"\n", fregment->fregment[sig_cnt].end_freq);
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
                    bw = poal_config->channel[ch].rf_para.mid_bw;
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
                        printf_info("[%d]start_freq:%"PRIu64"\n",i, fregment->fregment[i].start_freq);
                        printf_info("[%d]end_freq:%"PRIu64"\n",i, fregment->fregment[i].end_freq);
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
                            printf_info("[%d]resolution:%u\n",i, point->points[i].freq_resolution);
                        }
                        printf_info("[%d]center_freq:%"PRIu64"\n",i, point->points[i].center_freq);
                        printf_info("[%d]bandwidth:%"PRIu64"\n",i, point->points[i].bandwidth);
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
        printf_err("s_freq[%"PRIu64"] is big than e_freq[%"PRIu64"]\n", cal_source->e_freq, cal_source->s_freq);
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
        printf_note("[%d]center_freq:%"PRIu64", bandwidth:%"PRIu64",fft_size:%u\n",i, point->points[i].center_freq, point->points[i].bandwidth, point->points[i].fft_size);
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
            #ifdef CONFIG_SPM_FFT_SCAN_SEGMNET
            scan_segment_param_convert(ch);
            #endif
            int32_t enable = 0;
            if(poal_config->channel[ch].enable.map.bit.fft || poal_config->channel[ch].enable.map.bit.audio){
                enable = 1;
            }
            executor_set_command(EX_FFT_ENABLE_CMD, -1, ch, &enable);
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
            struct net_tcp_client * client = (struct net_tcp_client *)cl;
           
            memcpy(&netinfo, payload, sizeof(DEVICE_NET_INFO_ST));
            if(get_ifa_name_by_ip(client->get_serv_addr(client), ifname) != 0){
                err_code = RET_CODE_INTERNAL_ERR;
                goto set_exit;
            }
            config_set_network(ifname, netinfo.ipaddr, netinfo.mask, netinfo.gateway);
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
            net_data_add_client(&client, ch, NET_DATA_TYPE_FFT);

            /*分端口*/
            if (payload_len == sizeof(SNIFFER_DATA_REPORT_ST) && net_para.iq_port != 0) {
                client.sin_port = net_para.iq_port;
            } else {
                client.sin_port = net_para.port;
            }
            net_data_add_client(&client, ch, NET_DATA_TYPE_NIQ);

            if (payload_len == sizeof(SNIFFER_DATA_REPORT_ST) && net_para.audio_port != 0) {
                client.sin_port = net_para.audio_port;
            } else {
                client.sin_port = net_para.port;
            }
            net_data_add_client(&client, ch, NET_DATA_TYPE_AUDIO);
            
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
            printf_note("oal ch=%d,%d,sub_ch=%d,%d, freq=%"PRIu64", method_id=%d, bandwidth=%u\n", ch,
                        sub_channel_array->cid, sub_ch,sub_channel_array->sub_ch[sub_ch].index,
                        sub_channel_array->sub_ch[sub_ch].center_freq, sub_channel_array->sub_ch[sub_ch].d_method, 
                        sub_channel_array->sub_ch[sub_ch].d_bandwith);
            /* 解调中心频率需要工作中心频率计算 */
            executor_set_command(EX_MID_FREQ_CMD, EX_SUB_CH_MID_FREQ, sub_ch,
                &sub_channel_array->sub_ch[sub_ch].center_freq,/* 解调频率*/
                poal_config->channel[ch].multi_freq_point_param.points[0].center_freq, ch); /* 频点工作频率 */
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
            int32_t sub_ch, enable = 0;
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
            printf_note("ch:%d, sub_ch=%d, au_en:%d,iq_en:%d\n", ch,sub_ch, poal_config->channel[ch].sub_channel_para.sub_ch_enable[sub_ch].map.bit.audio, poal_config->channel[ch].sub_channel_para.sub_ch_enable[sub_ch].map.bit.iq);
            enable = (poal_config->channel[ch].sub_channel_para.sub_ch_enable[sub_ch].map.bit.iq == 0 ? 0 : 1);
            printf_note("ch:%d, sub_ch=%d, au_en:%d,iq_en:%d, enable=%d\n", ch,sub_ch, poal_config->channel[ch].sub_channel_para.sub_ch_enable[sub_ch].map.bit.audio, poal_config->channel[ch].sub_channel_para.sub_ch_enable[sub_ch].map.bit.iq, enable);
            executor_set_command(EX_NIQ_ENABLE_CMD, -1, ch, &enable, sub_ch);
            break;
        }
#ifdef CONFIG_NET_10G
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
            config_set_network(CONFIG_NET_10G_IFNAME, netinfo.ipaddr, netinfo.mask, netinfo.gateway);
            break;
        }
#endif
        case SPCTRUM_PARAM_CMD:
        {
            uint64_t freq_hz;
            int i; 
            freq_hz = *(uint64_t *)(payload);
            printf_debug("spctrum freq hz=%"PRIu64"Hz\n", freq_hz);
            poal_config->ctrl_para.specturm_analysis_param.frequency_hz = freq_hz;
            break;
        }
        case SPCTRUM_ANALYSIS_CTRL_EN_CMD:
        {
            bool enable;
            ch = 0;
            enable =  (bool)payload[0];
            printf_note("Spctrum Analysis Ctrl  %s\n", enable == false ? "Enable" : "Disable");
            poal_config->channel[ch].enable.map.bit.analy = !enable;
            printf_note("ch=%d, spec_analy_en=%d\n",  ch, poal_config->channel[ch].enable.map.bit.analy);
            break;
        }
        case SYSTEM_TIME_SET_REQ:
        {
            struct _time_param {
                uint64_t utc_time;
                uint32_t time_zone;
            }__attribute__ ((packed)) _time;
            time_t tval;
            char buffer[64];
            memcpy(&_time, payload, sizeof(_time));
            printf_note("set utc time:%"PRIu64", time zone:%u\n", _time.utc_time, _time.time_zone); 
            snprintf(buffer, sizeof(buffer) - 1, "GMT-%u",  _time.time_zone);
            buffer[sizeof(buffer) - 1] = 0;
            setenv("TZ", buffer, 1);
            tzset();
            safe_system("date -R");
            
            if (_time.utc_time > 0) {
                tval = _time.utc_time;
                stime(&tval);
            } else {
                err_code = RET_CODE_PARAMTER_ERR;
            }
        }
            break;       
        case DEVICE_CALIBRATE_CMD:
        {
            CALIBRATION_SOURCE_ST_V2 cal_source;
            check_valid_channel(payload[0]);
            memcpy(&cal_source, payload, sizeof(CALIBRATION_SOURCE_ST_V2));
            printf_note("RF calibrate: cid=%d, enable=%d, middle_freq_hz=%"PRIu64"hz, power=%d, s_freq=%"PRIu64", e_freq=%"PRIu64", r_time_ms=%u,step=%"PRIu64"\n", 
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
#if defined(CONFIG_FS_XW)
                ret = xwfs_start_save_file(sis.filepath);
 #elif defined(CONFIG_FS)
                struct fs_context *fs_ctx;
                struct fs_save_sample_args sample_args;
                fs_ctx = get_fs_ctx();
                if(fs_ctx == NULL){
                    printf_warn("NOT FOUND DISK!!\n");
                    break;
                }
                sample_args.bindwidth = sis.bandwidth;
                sample_args.sample_size = sis.filesize;
                sample_args.sample_time = sis.caputure_time_len;
                fs_ctx->ops->fs_start_save_file(ch, sis.filepath, &sample_args, client->fsctx);
#endif
            }else if(sis.cmd == 0){/* stop add iq file */
                printf_note("Stop add file:%s\n", sis.filepath);
#if defined(CONFIG_FS_XW)
                ret = xwfs_stop_save_file(sis.filepath);
 #elif defined(CONFIG_FS)
                struct fs_context *fs_ctx;
                fs_ctx = get_fs_ctx();
                if(fs_ctx == NULL){
                    printf_warn("NOT FOUND DISK!!\n");
                    break;
                }
                fs_ctx->ops->fs_stop_save_file(ch, sis.filepath, client->fsctx);
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
#if defined(CONFIG_FS_XW)
                ret = xwfs_start_backtrace(bis.filepath);
#elif defined(CONFIG_FS)
                struct fs_context *fs_ctx;
                fs_ctx = get_fs_ctx();
                if(fs_ctx == NULL){
                    printf_warn("NOT FOUND DISK!!\n");
                    break;
                }
                fs_ctx->ops->fs_start_read_raw_file(ch, bis.filepath, client->fsctx);
#endif
            }else if(bis.cmd == 0){/* stop backtrace iq file */
                printf_note("Stop backtrace file:%s\n", bis.filepath);
#if defined(CONFIG_FS_XW)
                ret = xwfs_stop_backtrace(bis.filepath);
#elif defined(CONFIG_FS)
                struct fs_context *fs_ctx;
                fs_ctx = get_fs_ctx();
                if(fs_ctx == NULL){
                    printf_warn("NOT FOUND DISK!!\n");
                    break;
                }
                fs_ctx->ops->fs_stop_read_raw_file(ch, bis.filepath, client->fsctx);
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
            uint8_t free_threshold_G = payload[0];
            uint64_t free_threshold_byte = free_threshold_G * 1024 * 1024 * 1024;
            printf_note("set free threshold %d G\n", free_threshold_G);
            config_set_disk_alert_threshold(free_threshold_byte);
            break;
        }
        case STORAGE_DELETE_FILE_CMD:
        {
            int ret = 0;
            char filename[FILE_PATH_MAX_LEN];
            memcpy(filename, payload, FILE_PATH_MAX_LEN);
#if defined(CONFIG_FS_XW)
            ret = xwfs_delete_file(filename);
#elif defined(CONFIG_FS)
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
			#if defined(CONFIG_FS_XW)
            file_disk_format(NULL, NULL);
           	#elif defined(CONFIG_FS)
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
            printf_note("set split file threshold:%"PRIu64" Bytes, %"PRIu64" MB\n", para.split_threshold, (para.split_threshold / 1024 / 1024));
            break;
        }
        case LOW_NOISE_CMD:
        {
            struct _low_noise_st{
                uint64_t s_freq_hz;
                uint64_t e_freq_hz;
            }__attribute__ ((packed));
            struct _low_noise_st noise;
            memcpy(&noise, payload, sizeof(noise));
            printf_info("low noise para: start freq:%lu hz, end freq:%lu hz\n", noise.s_freq_hz, noise.e_freq_hz);
            executor_set_command(EX_RF_FREQ_CMD,  EX_RF_LOW_NOISE, ch, &noise.e_freq_hz);
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
                strncpy((char *)self_check.system_power_on_time, time_str, sizeof(self_check.system_power_on_time));
            }
            printf_note("power on time:%s\n", self_check.system_power_on_time);
            self_check.ch_num = MAX_RF_NUM;
            for(int i = 0; i< MAX_RF_NUM; i++){
                executor_get_command(EX_RF_FREQ_CMD, EX_RF_STATUS_TEMPERAT, i,  &self_check.t_s[i].rf_temperature);
                if(self_check.t_s[i].rf_temperature < 0){
                    /* 通过判断获取的温度值是否在有效范围内来确定 */
                    self_check.t_s[i].ch_status = 1; //0:正常 1：异常
                }else{
                    self_check.t_s[i].ch_status = 0;
                }
                printf_note("rf ch:%d, ext_clk:%d, ad_status=%d,pfga_temperature=%d,ch_num=%d, rf_temperature=%d, ch_status=%d\n", i,
                    self_check.ext_clk, self_check.ad_status, self_check.pfga_temperature, self_check.ch_num,
                    self_check.t_s[i].rf_temperature, self_check.t_s[i].ch_status);
            }
            self_check.irig_b_status = 0;  //0:正常 1：异常
            #if defined(CONFIG_DEVICE_GPS)
            self_check.gps_status = (gps_location_is_valid() == true ? 0 : 1);
            #else
            self_check.gps_status = 0;
            #endif
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
        #ifdef CONFIG_SPM_FFT_SIGNAL_ANALYSIS
            FFT_SIGNAL_RESPINSE_ST *resp_result;
            struct spectrum_analysis_result_vec **r;
            ssize_t number;
            size_t payload_len, i;
            
            number = spm_analysis_get_info(&r);
            payload_len = sizeof(FFT_SIGNAL_RESPINSE_ST) + sizeof(FFT_SIGNAL_RESULT_ST)*number;
            resp_result = safe_malloc(payload_len);
            printf_warn("#########Find SPM ANALYSIS Parameter[%ld]:###############\n",number);
            for(i = 0; i < number; i++){
                resp_result->signal_array[i].center_freq = r[i]->mid_freq_hz;
                resp_result->signal_array[i].bandwidth = r[i]->bw_hz;
                resp_result->signal_array[i].power_level = r[i]->level;
                printf_warn("[%ld] middle_freq=%"PRIu64"Hz, bw=%"PRIu64"Hz, level=%f\n", i,
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
            sprintf((char *)info.btime,"%s-%s",get_build_time(), __TIME__);
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
            int i, st_size = 0;
            bool ret = false;
            st_size = sizeof(STORAGE_STATUS_RSP_ST)+sizeof(STORAGE_DISK_INFO_ST)*SSD_DISK_NUM;
            psi = (STORAGE_STATUS_RSP_ST *)safe_malloc(st_size);
            if(psi == NULL){
                printf_err("malloc error!\n");
                err_code = RET_CODE_INTERNAL_ERR;
                break;
            }
            #if defined(CONFIG_FS_XW)
            xwfs_get_disk_info(psi);
            #elif defined(CONFIG_FS)
            struct fs_context *fs_ctx;
            struct statfs diskInfo;
            fs_ctx = get_fs_ctx_ex();
            ret = fs_ctx->ops->fs_disk_info(&diskInfo);
            if(ret){
            psi->disk_num = 1;
            psi->read_write_speed_kbytesps = 0;  //按照写速度换算约等于1.8G
            psi->disk_capacity[0].disk_state = fs_ctx->ops->fs_get_err_code();
            psi->disk_capacity[0].disk_capacity_byte = diskInfo.f_bsize * diskInfo.f_blocks;
            psi->disk_capacity[0].disk_used_byte = diskInfo.f_bsize * (diskInfo.f_blocks - diskInfo.f_bfree);
            } else{
                psi->disk_num = 0;
                psi->read_write_speed_kbytesps = 0;
                psi->disk_capacity[0].disk_state = fs_ctx->ops->fs_get_err_code();
                psi->disk_capacity[0].disk_capacity_byte = 0;
                psi->disk_capacity[0].disk_used_byte = 0;
            }
		    #endif
            printf_note("ret=%d, num=%d, speed=%uKB/s, capacity_bytes=%"PRIu64", used_bytes=%"PRIu64"\n",
                ret, psi->disk_num, psi->read_write_speed_kbytesps, 
                psi->disk_capacity[0].disk_capacity_byte, psi->disk_capacity[0].disk_used_byte);
            client->response.response_length = st_size;
            client->response.data = calloc(1, client->response.response_length);
            if(client->response.data == NULL){
                printf_err("calloc err!");
                safe_free(psi);
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
            strcpy((char *)fsp.filepath, filename);
            #if defined(CONFIG_FS_XW)
            ret = xwfs_get_file_size_by_name(filename, &f_bg_size, sizeof(ssize_t));
            if(ret != 0){
                fsp.status = 0;
                fsp.file_size = 0;
            }else{
                fsp.status = 1;
                fsp.file_size = (uint64_t)f_bg_size*8*1024*1024; /* data block group size is 8MByte */
            }
            #elif defined(CONFIG_FS)
            struct fs_context *fs_ctx;
            fs_ctx = get_fs_ctx();
            if(fs_ctx != NULL && fs_ctx->ops->fs_get_filesize){
                ret = fs_ctx->ops->fs_get_filesize(filename, &f_bg_size);
            }
            if(ret != 0){
                fsp.status = 0;
                fsp.file_size = 0;
            }else{
                fsp.status = 1;
                fsp.file_size = (uint64_t)f_bg_size;
            }
            #endif
            printf_note("Find file:%s, fsize=%lu ret =%d\n", fsp.filepath, f_bg_size, ret);
            
            printf_note("ret=%d, filepath=%s, file_size=[%lu bg]%"PRIu64", status=%d\n",ret, fsp.filepath, f_bg_size, fsp.file_size, fsp.status);
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
        #if defined(CONFIG_DEVICE_RS485)
        #if defined(CONFIG_DEVICE_RS485_EC)
            elec_compass1_com_get_angle(&angle);
            #endif
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

        case DEVICE_LOCATION_REQ_CMD:
        {
            #define UNSUPPORT_GPS_DATA (400000000)
            struct gps_data {
                int32_t longitude;
                int32_t latitude;
                int32_t altitude;
            }__attribute__ ((packed));
            
            struct gps_data _gps_data;
            
            #if defined(CONFIG_DEVICE_GPS)
            _gps_data.longitude = gps_get_longitude();
            _gps_data.latitude  = gps_get_latitude();
            _gps_data.altitude  = gps_get_altitude();
            #else
            _gps_data.longitude = UNSUPPORT_GPS_DATA;
            _gps_data.latitude  = UNSUPPORT_GPS_DATA;
            _gps_data.altitude  = 0;
            #endif

            client->response.response_length = sizeof(_gps_data);
            client->response.data = calloc(1, client->response.response_length);
            if(client->response.data == NULL){
                printf_err("calloc err!");
                break;
            }
            memcpy(client->response.data, &_gps_data, sizeof(_gps_data));
            break;
        }
        
        case FILE_LIST_CMD:
        {
            FILE_LIST_INFO *file_list = NULL;
            int32_t payload_len = 0;
        #if defined(CONFIG_FS)
            cJSON *value = NULL;
            cJSON *node = NULL;
            cJSON *array = cJSON_CreateArray();
            struct fs_context *fs_ctx;
            fs_ctx = get_fs_ctx();
            if(fs_ctx == NULL) {
                printf_err("get fd ctx failed!");
                break;
            }
            fs_ctx->ops->fs_list(NULL, fs_file_list, array);
            payload_len = sizeof(FILE_LIST_INFO) + sizeof(SINGLE_FILE_INFO) * cJSON_GetArraySize(array);
            file_list = (FILE_LIST_INFO *)safe_malloc(payload_len);
            file_list->file_num = cJSON_GetArraySize(array);
            for (int i = 0; i < cJSON_GetArraySize(array); i++) {
                node = cJSON_GetArrayItem(array, i);            
                value = cJSON_GetObjectItem(node, "size");
                if(cJSON_IsNumber(value)){
                    file_list->files[i].file_size = value->valuedouble;
                }
                value = cJSON_GetObjectItem(node, "createTime");
                if(cJSON_IsNumber(value)){
                    file_list->files[i].file_mtime = value->valuedouble;
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

    int index = config_get_if_nametoindex(cl->ifname);
    if(index == -1){
        return -1;
    }
    memcpy(netinfo.mac, poal_config->network[index].addr.mac, sizeof(netinfo.mac));
    netinfo.ipaddr = htonl(poal_config->network[index].addr.ipaddress);
    netinfo.gateway = htonl(poal_config->network[index].addr.gateway);
    netinfo.mask = htonl(poal_config->network[index].addr.netmask);
    netinfo.port = htons(poal_config->network[index].port);
        netinfo.status = 0;
    ipdata.s_addr = poal_config->network[index].addr.ipaddress;
    printf_note("ifname:%s,mac:%x%x%x%x%x%x, ipaddr=%x[%s], gateway=%x\n", cl->ifname, netinfo.mac[0],netinfo.mac[1],netinfo.mac[2],netinfo.mac[3],netinfo.mac[4],netinfo.mac[5],
                                                        netinfo.ipaddr, inet_ntoa(ipdata), netinfo.gateway);
    memcpy(&cl->discover_peer_addr, &addr, sizeof(addr));
    akt_send_resp_discovery(client, (void *)&netinfo, sizeof(DEVICE_NET_INFO_ST));

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

bool akt_send_resp_discovery(void *client, const void *pdata, int len)
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
    rsp_header = (PDU_CFG_RSP_HEADER_ST *)ptr_rsp;
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

void akt_send(const void *data, int len, int code)
{
    uint8_t *psend;
    int send_len, offset = 0;
    send_len = sizeof(struct response_get_data);
    
    psend = calloc(1, send_len);
    if(psend == NULL){
        printf_err("psend err!\n");
        return;
    }
    if ((send_len = akt_assamble_send_data(psend, data, len, code)) == -1){
        printf_err("assamble err!\n");
        goto exit;
    }
    tcp_active_send_all_client(psend, send_len);

exit:
    safe_free(psend);
}


void akt_send_alert(int code)
{
    struct disk_alarm_report alarm;
    alarm.ch = 0;
    alarm.code = (uint8_t)code;
    printf_note("send alarm: code[%d]\n", code);
    akt_send(&alarm, sizeof(alarm), REPORT_DISK_ALARM);
}

#ifdef CONFIG_FS
void akt_send_file_status(void *data, size_t data_len, void *args)
{
    struct fs_node *fns = data;
    struct fs_notifier *fner = args;
    struct fs_sampler *fsa = fner->args;
    struct fs_status _fss;
    if(fns == NULL || fns->filename == NULL)
        return;

    memset(&_fss, 0, sizeof(_fss));
    strncpy(_fss.path, fns->filename, sizeof(_fss.path));
    _fss.ch = fns->ch;
    _fss.filesize = fns->nbyte;
    _fss.duration_time = fns->duration_time;
    _fss.middle_freq = executor_get_mid_freq(fns->ch);
    _fss.bindwidth = fsa->args.bindwidth;
    _fss.sample_rate = io_get_raw_sample_rate(fns->ch, _fss.middle_freq, _fss.bindwidth);
    _fss.file_threshold = config_get_split_file_threshold();
    _fss.state = fns->status;
    printf_note("ch=%d, path=%s, filesize=%"PRIu64"[0x%lx], time=%"PRIu64", middle_freq=%"PRIu64", bindwidth=%"PRIu64", sample_rate=%"PRIu64", state:%d\n", 
        _fss.ch, _fss.path, _fss.filesize,_fss.filesize,  _fss.duration_time, _fss.middle_freq, _fss.bindwidth, _fss.sample_rate, _fss.state);
    akt_send(&_fss, sizeof(_fss), FILE_STATUS_NOTIFY);
}
#endif


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
    uint8_t *ptr;
    int ch = poal_config->cid;

    ext_hdr = calloc(1, sizeof(DATUM_SPECTRUM_HEADER_ST));
    if(ext_hdr == NULL){
        return NULL;
    }
    ptr = (uint8_t *)ext_hdr;
    printf_debug("akt_assamble_data_extend_frame_header_data\n");
    ext_hdr->dev_id = akt_get_device_id();
    ext_hdr->duration = 0;
    struct spm_run_parm *header_param;
    header_param = (struct spm_run_parm *)config;
    ext_hdr->cid = header_param->ch;
    ext_hdr->work_mode =  header_param->mode;
    ext_hdr->gain_mode = header_param->gain_mode;
    ext_hdr->gain = header_param->gain_value;
    ext_hdr->center_freq = header_param->sub_ch_para.m_freq_hz;
    ext_hdr->bandwidth = header_param->sub_ch_para.bandwidth_hz;
    ext_hdr->demodulate_type = header_param->sub_ch_para.d_method;
    ext_hdr->sample_rate = header_param->sub_ch_para.sample_rate;
    ext_hdr->frag_total_num = 1;
    ext_hdr->frag_cur_num = 0;
    ext_hdr->frag_data_len = (int16_t)(header_param->data_len);
#if 1
    printfd("-----------------------------assamble_spectrum_header-----------------------------------\n");
    printfd("dev_id[%d], cid[%d], work_mode[%d], gain_mode[%d]\n", 
        ext_hdr->dev_id, ext_hdr->cid, ext_hdr->work_mode, ext_hdr->gain_mode);
    printfd("d_method[%d], m_freq[%"PRIu64"], bandwidth[%u]\n", 
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
    uint8_t *ptr;
    int ch = poal_config->cid;

    ext_hdr = calloc(1, sizeof(DATUM_SPECTRUM_HEADER_ST));
    if(ext_hdr == NULL){
        return NULL;
    }
    ptr = (uint8_t *)ext_hdr;
    printf_debug("akt_assamble_data_extend_frame_header_data\n");
    ext_hdr->dev_id = akt_get_device_id();

    ext_hdr->duration = 0;
    ext_hdr->datum_type = 1;

    struct spm_run_parm *header_param;
    header_param = (struct spm_run_parm *)config;
    ext_hdr->cid = header_param->ch;
    ext_hdr->work_mode =  header_param->mode;
    ext_hdr->gain_mode = header_param->gain_mode;
    ext_hdr->gain = header_param->gain_value;
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
    ext_hdr->frag_data_len = (int16_t)(header_param->data_len);
    

#if 1
    printfd("-----------------------------assamble_spectrum_header-----------------------------------\n");
    printfd("dev_id[%d], cid[%d], work_mode[%d], gain_mode[%d]\n", 
        ext_hdr->dev_id, ext_hdr->cid, ext_hdr->work_mode, ext_hdr->gain_mode);
    printfd("gain[%d], duration[%"PRIu64"], start_freq[%"PRIu64"], cutoff_freq[%"PRIu64"], center_freq[%"PRIu64"]\n",
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
    printfd("toa=[%"PRIu64"], seqnum[%d], ex_len[%d],data_len[%d]\n",  package_header->toa, package_header->seqnum, package_header->ex_len, package_header->data_len);
    printfd("----------------------------------------------------------------------------------------\n");
    *len = sizeof(DATUM_PDU_HEADER_ST) + extend_data_header_len;
    printfd("Header len[%zd]: \n", sizeof(DATUM_PDU_HEADER_ST));
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




