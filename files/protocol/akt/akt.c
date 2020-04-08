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

int akt_get_device_id(void)
{
    return 0;
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
        {
            uint8_t method_id;
            /* 解调参数 （带宽、解调方式）转换*/
            sig_cnt = pakt_config->decode_param[ch].freq_band_cnt; 
            printf_note("sig_cnt=%d\n", sig_cnt);
            poal_config->multi_freq_point_param[ch].freq_point_cnt = sig_cnt;
            for(i = 0; i < sig_cnt; i++){
                if((method_id = akt_decode_method_convert(pakt_config->decode_param[ch].sig_ch[i].decode_method_id)) == -1){
                    printf_warn("d_method error=%d\n", poal_config->multi_freq_point_param[ch].points[i].d_method);
                    return -1;
                }
                poal_config->multi_freq_point_param[ch].points[i].raw_d_method = pakt_config->decode_param[ch].sig_ch[i].decode_method_id;
                poal_config->multi_freq_point_param[ch].points[i].d_method = method_id;
                poal_config->multi_freq_point_param[ch].points[i].d_bandwith = pakt_config->decode_param[ch].sig_ch[i].bandwidth;
                poal_config->multi_freq_point_param[ch].points[i].center_freq = pakt_config->decode_param[ch].sig_ch[i].center_freq;
                /* 不在需要解调中心频率，除非窄带解调 */
                printf_info("ch:%d,subch:%d, d_bw:%u\n", ch, i,poal_config->multi_freq_point_param[ch].points[i].d_bandwith);
                printf_info("ch:%d,d_method:%u\n", ch, poal_config->multi_freq_point_param[ch].points[i].d_method);
                printf_info("ch:%d,center_freq:%llu\n", ch, poal_config->multi_freq_point_param[ch].points[i].center_freq);
            }
            break;
        }
       case SUB_SIGNAL_PARAM_CMD:
       {
            struct sub_channel_freq_para_st *sub_channel_array;
            int cid = 0;
            cid = pakt_config->sub_channel[ch].cid;
            sub_channel_array = &poal_config->sub_channel_para[cid];
            sub_channel_array->cid = cid ;
            poal_config->cid = cid;
            sub_channel_array->sub_ch[ch].index = ch;
            sub_channel_array->sub_ch[ch].center_freq = pakt_config->sub_channel[ch].freq;
            sub_channel_array->sub_ch[ch].d_method = akt_decode_method_convert(pakt_config->sub_channel[ch].decode_method_id);
            sub_channel_array->sub_ch[ch].d_bandwith = pakt_config->sub_channel[ch].bandwidth;
            printf_info("cid:%d, subch:%d\n", cid,ch);
            printf_info("subch:%d,d_method:%u, raw dmethod:%d\n", ch, sub_channel_array->sub_ch[ch].d_method, pakt_config->sub_channel[ch].decode_method_id);
            printf_info("center_freq:%llu, d_bandwith=%u\n", sub_channel_array->sub_ch[ch].center_freq, sub_channel_array->sub_ch[ch].d_bandwith);
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
            printf_info("sub_ch =%d, bit_en=%x, psd_en=%d, audio_en=%d,iq_en=%d\n",ch, poal_config->sub_ch_enable.bit_en, 
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
                    printf_note("residence_time:%u\n",point->residence_time);
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
                        point->points[i].raw_d_method = pakt_config->decode_param[ch].sig_ch[i].decode_method_id;
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
    printf_note("set bussiness code[0x%x]\n", header->code);
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
               //config_save_batch(EX_WORK_MODE_CMD, EX_FIXED_FREQ_ANYS_MODE, config_get_config());
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
            if(poal_config->work_mode == OAL_FIXED_FREQ_ANYS_MODE || poal_config->work_mode == OAL_MULTI_POINT_SCAN_MODE){
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
            STATION_INFO sta_info_para = {0};
            struct sockaddr_in client;
            struct sockaddr_in tcp_client;
            struct timespec ts;
            check_valid_channel(header->buf[0]);
            net_para.cid = ch;
            memcpy(&net_para, header->buf, sizeof(SNIFFER_DATA_REPORT_ST));
            /* Test */
        #if 1
            tcp_get_peer_addr_port(cl, &tcp_client);
            printf_note("tcp connection from: %s:%d\n", inet_ntoa(tcp_client.sin_addr), ntohs(tcp_client.sin_port));
            //net_para.port = ntohs(net_para.port);
            //net_para.ipaddr = ntohs(net_para.ipaddr);
            net_para.ipaddr =  tcp_client.sin_addr.s_addr;
            #ifdef SUPPORT_NET_WZ
            net_para.wz_ipaddr = ntohl(tcp_client.sin_addr.s_addr)+(1 << 8);
            net_para.wz_port = ntohs(net_para.port);
            #endif
        #else
             #ifdef SUPPORT_NET_WZ
            //net_para.wz_ipaddr = net_para.wz_ipaddr;
            //net_para.wz_port = net_para.port;  
            net_para.wz_ipaddr = ntohl(net_para.wz_ipaddr);
            net_para.wz_port = ntohs(net_para.wz_port);    /* debug test*/
            #endif
        #endif
            /* EndTest */
            client.sin_port = net_para.port;
            client.sin_addr.s_addr = net_para.ipaddr;//ntohl(net_para.sin_addr.s_addr);
            //printf_note("udp client ipstr=%s, port=%d, type=%d\n",
            //    inet_ntoa(client.sin_addr),  client.sin_port, net_para.type);
            /* UDP链表以大端模式存储客户端地址信息；内部以小端模式处理；注意转换 */
            udp_add_client_to_list(&client, ch);
            akt_add_udp_client(&net_para);
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
            printf_note("ch=%d, bandwidth:%u\n", ch, poal_config->rf_para[ch].mid_bw);
            executor_set_command(EX_RF_FREQ_CMD, EX_RF_MID_BW, ch, &poal_config->rf_para[ch].mid_bw);
            break;
        }
        case RF_ATTENUATION_CMD:
            check_valid_channel(header->buf[0]);
            poal_config->rf_para[ch].attenuation = header->buf[1];
            printf_info("RF_ATTENUATION_CMD:%d\n", poal_config->rf_para[ch].attenuation);
            executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, ch, &poal_config->rf_para[ch].attenuation);
            break;
        case RF_WORK_MODE_CMD:
            check_valid_channel(header->buf[0]);
            /*
                1: low distortion mode of operation
                2: Normal working mode
                3: Low noise mode of operation
            */
            poal_config->rf_para[ch].rf_mode_code = header->buf[1] + 1; /* need to convert */
            executor_set_command(EX_RF_FREQ_CMD, EX_RF_MODE_CODE, ch, &poal_config->rf_para[ch].rf_mode_code);
            break;
        case RF_GAIN_MODE_CMD:
            check_valid_channel(header->buf[0]);
            poal_config->rf_para[ch].gain_ctrl_method = header->buf[1];
            //executor_set_command(EX_RF_FREQ_CMD, EX_RF_GAIN_MODE, ch, &poal_config->rf_para[ch].gain_ctrl_method);
            break;
        case RF_AGC_CMD:
            check_valid_channel(header->buf[0]);
            poal_config->rf_para[ch].agc_ctrl_time = *((uint16_t *)(header->buf + 1));
            poal_config->rf_para[ch].agc_mid_freq_out_level = *((uint8_t *)(header->buf + 3));
            printf_info("agc_ctrl_time:%d, agc_mid_freq_out_level=%d\n", 
                        poal_config->rf_para[ch].agc_ctrl_time, poal_config->rf_para[ch].agc_mid_freq_out_level);
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
            //if(akt_convert_oal_config(ch, header->code) == -1){
            //    err_code = RET_CODE_PARAMTER_ERR;
            //    goto set_exit;
            //}
            executor_set_command(EX_MID_FREQ_CMD, EX_FFT_SIZE, ch, &pakt_config->fft[ch].fft_size);
            break;
        case SUB_SIGNAL_PARAM_CMD:
        {           
            struct sub_channel_freq_para_st *sub_channel_array;
            uint8_t sub_ch;
            sub_ch = *(uint16_t *)(header->buf+1);
            if(sub_ch >= 1)
                sub_ch-= 1;
            check_valid_channel(header->buf[0]);
            if(check_sub_channel(sub_ch)){
                err_code = RET_CODE_PARAMTER_ERR;
                goto set_exit;
            }
            memcpy(&(pakt_config->sub_channel[sub_ch]), header->buf, sizeof(SUB_SIGNAL_PARAM));
            sub_channel_array = &poal_config->sub_channel_para[ch];
            if(akt_convert_oal_config(sub_ch, header->code) == -1){
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
                poal_config->multi_freq_point_param[ch].points[0].center_freq); /* 频点工作频率 */
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
            uint8_t sub_ch, enable = 0;
            check_valid_channel(header->buf[0]);
            sub_ch = *(uint16_t *)(header->buf+1);
            if(sub_ch >= 1)
                sub_ch-= 1;
            if(check_sub_channel(sub_ch)){
                err_code = RET_CODE_PARAMTER_ERR;
                goto set_exit;
            }
            memcpy(&(pakt_config->sub_channel_enable[sub_ch]), header->buf, sizeof(SUB_SIGNAL_ENABLE_PARAM));
            if(akt_convert_oal_config(sub_ch, SUB_SIGNAL_OUTPUT_ENABLE_CMD) == -1){
                err_code = RET_CODE_PARAMTER_NOT_SET;
                goto set_exit;
            }
           // io_set_enable_command(IQ_MODE_DISABLE, ch, 0);
            printf_note("ch:%d, sub_ch=%d, au_en:%d,iq_en:%d, %d\n", ch,sub_ch, poal_config->sub_ch_enable.audio_en, poal_config->sub_ch_enable.iq_en);
            enable = (poal_config->sub_ch_enable.iq_en == 0 ? 0 : 1);
            
            #ifdef SUPPORT_NET_WZ
                printf_note("wz_threshold_bandwidth[%u],enable=%d\n", poal_config->ctrl_para.wz_threshold_bandwidth,enable);
                /* 判断解调带宽是否大于万兆传输阈值；大于等于则使用万兆传输（关闭千兆），否则使用千兆传输（关闭万兆） */
                struct sub_channel_freq_para_st *sub_channel_array;
                sub_channel_array = &poal_config->sub_channel_para[ch];
                printf_note("sub_channel_array->sub_ch[sub_ch].d_bandwith[%u]\n", sub_channel_array->sub_ch[sub_ch].d_bandwith);
                if(poal_config->ctrl_para.wz_threshold_bandwidth <=  sub_channel_array->sub_ch[sub_ch].d_bandwith){
                    io_set_1ge_net_onoff(0);/* 关闭IQ千兆传输 */
                    if(enable)
                        io_set_10ge_net_onoff(1); /* 开启IQ万兆传输 */
                    else
                        io_set_10ge_net_onoff(0); /* 关闭IQ万兆传输 */
                    
                   printf_note("d_bandwith[%u] >= threshold[%u], Data is't sent by the Gigabit, but by 10 Gigabit\n", 
                        sub_channel_array->sub_ch[sub_ch].d_bandwith, poal_config->ctrl_para.wz_threshold_bandwidth);
                }else{
                   printf_note("Data is sent by the Gigabit, NOT by 10 Gigabit\n");
                   io_set_1ge_net_onoff(1);/* 开启IQ千兆传输 */
                   io_set_10ge_net_onoff(0); /* 关闭IQ万兆传输 */
                }
            #endif
            /* 通道IQ使能 */
            if(enable){
                /* NOTE:The parameter must be a MAIN channel, not a subchannel */
                io_set_enable_command(IQ_MODE_ENABLE, -1, sub_ch, 0);
            }else{
                io_set_enable_command(IQ_MODE_DISABLE, -1,sub_ch, 0);
            }
            break;
        }
#ifdef SUPPORT_NET_WZ
        case SET_NET_WZ_THRESHOLD_CMD:
        {
            poal_config->ctrl_para.wz_threshold_bandwidth =  *(uint32_t *)(header->buf+1);
            printf_note("wz_threshold_bandwidth  %u\n", poal_config->ctrl_para.wz_threshold_bandwidth);
            break;
        }
#endif
        case SPCTRUM_PARAM_CMD:
        {
            uint64_t freq_hz;
            int i; 
            freq_hz = *(uint64_t *)(header->buf);
            printf_debug("spctrum freq hz=%lluHz\n", freq_hz);
            poal_config->ctrl_para.specturm_analysis_param.frequency_hz = freq_hz;
            break;
        }
        case SPCTRUM_ANALYSIS_CTRL_EN_CMD:
        {
            bool enable;
            enable =  (bool)header->buf[0];
            printf_note("Spctrum Analysis Ctrl  %s\n", enable == false ? "Enable" : "Disable");
            poal_config->enable.spec_analy_en = !enable;
            INTERNEL_ENABLE_BIT_SET(poal_config->enable.bit_en,poal_config->enable);
            printf_info("sub_ch bit_en=%x, spec_analy_en=%d\n", 
            poal_config->sub_ch_enable.bit_en, poal_config->enable.spec_analy_en);
            executor_set_enable_command(0);
            break;
        }
        case SYSTEM_TIME_SET_REQ:
            break;       
        case DEVICE_CALIBRATE_CMD:
        {
            CALIBRATION_SOURCE_ST cal_source;
            check_valid_channel(header->buf[0]);
            memcpy(&cal_source, header->buf, sizeof(CALIBRATION_SOURCE_ST));
            printf_note("RF calibrate: cid=%d, enable=%d, middle_freq_hz=%uhz, power=%d\n", 
                cal_source.cid, cal_source.enable, cal_source.middle_freq_hz, cal_source.power);
            executor_set_command(EX_RF_FREQ_CMD, EX_RF_CALIBRATE, ch, &cal_source);
            break;
        }
        /* disk cmd */
        case STORAGE_IQ_CMD:
        {
            int ret = 0;
            STORAGE_IQ_ST  sis;
            uint32_t old_bandwidth = 0;
            check_valid_channel(header->buf[0]);
            memcpy(&sis, header->buf, sizeof(STORAGE_IQ_ST));
            if(sis.cmd == 1){/* start add iq file */
                printf_note("Start add file:%s, bandwidth=%u\n", sis.filepath, sis.bandwidth);
                executor_set_command(EX_MID_FREQ_CMD, EX_BANDWITH, ch, &sis.bandwidth);
                #if defined(SUPPORT_XWFS)
                ret = xwfs_start_save_file(sis.filepath);
                #endif
            }else if(sis.cmd == 0){/* stop add iq file */
                printf_note("Stop add file:%s\n", sis.filepath);
                //ret = io_stop_save_file(sis.filepath);
                #if defined(SUPPORT_XWFS)
                ret = xwfs_stop_save_file(sis.filepath);
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
            check_valid_channel(header->buf[0]);
            memcpy(&bis, header->buf, sizeof(BACKTRACE_IQ_ST));
            if(bis.cmd == 1){/* start backtrace iq file */
                printf_note("Start backtrace file:%s, bandwidth=%u\n", bis.filepath, bis.bandwidth);
                executor_set_command(EX_MID_FREQ_CMD, EX_BANDWITH, ch, &bis.bandwidth);
                #if defined(SUPPORT_XWFS)
                ret = xwfs_start_backtrace(bis.filepath);
                #endif
            }else if(bis.cmd == 0){/* stop backtrace iq file */
                printf_note("Stop backtrace file:%s\n", bis.filepath);
                #if defined(SUPPORT_XWFS)
                ret = xwfs_stop_backtrace(bis.filepath);
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
            memcpy(filename, header->buf, FILE_PATH_MAX_LEN);
            #if defined(SUPPORT_XWFS)
            ret = xwfs_delete_file(filename);
            #endif
            printf_note("Delete file:%s, %d\n", filename, ret);
            if(ret != 0){
                err_code = akt_err_code_check(ret);
                goto set_exit;
            }
            break;
        }
        default:
          printf_err("not support class code[%x]\n",header->code);
          err_code = RET_CODE_PARAMTER_ERR;
          goto set_exit;
    }
set_exit:
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
    return err_code;

}

static int akt_execute_get_command(void)
{
    PDU_CFG_REQ_HEADER_ST *header;
    int err_code;
    header = &akt_header;

    err_code = RET_CODE_SUCCSESS;
    printf_note("get bussiness code[%x]\n", header->code);
    switch (header->code)
    {
        case DEVICE_SELF_CHECK_CMD:
        {
            DEVICE_SELF_CHECK_STATUS_RSP_ST self_check;
            uint64_t freq_hz;
            self_check.clk_status = 1;
            self_check.ad_status = 0;
            self_check.pfga_temperature = io_get_adc_temperature();
            memcpy(akt_get_response_data.payload_data, &self_check, sizeof(DEVICE_SELF_CHECK_STATUS_RSP_ST));
            akt_get_response_data.header.len = sizeof(DEVICE_SELF_CHECK_STATUS_RSP_ST);
            break;
        }
        case SPCTRUM_PARAM_CMD:
        {
        #ifdef SUPPORT_SPECTRUM_FFT_ANALYSIS
            #define AVG_FREQ_POINT  16  
            int i, datalen;
            FFT_SIGNAL_RESPINSE_ST *resp_result;
            struct spectrum_fft_result_st *fft_result;
            static uint64_t center_freq_buf[AVG_FREQ_POINT], cnt = 0;
            uint64_t avg_freq = 0;
            uint32_t result_num = 0;


            fft_result = (struct spectrum_fft_result_st *)spectrum_rw_fft_result(NULL, 0, 0, 0, NULL);
            if(fft_result == NULL){
                err_code = RET_CODE_INTERNAL_ERR;
                printf_info("error fft_result\n");
                return -1;
            }

            printf_warn("#########Find FFT Parameter[%d]:###############\n",fft_result->result_num);
            result_num = 1;
            if(fft_result->result_num > 1){
                fft_result->result_num = 1;
            }
            resp_result = (FFT_SIGNAL_RESPINSE_ST *)safe_malloc(sizeof(FFT_SIGNAL_RESPINSE_ST) + sizeof(FFT_SIGNAL_RESULT_ST)*result_num);
            resp_result->signal_num = fft_result->result_num;
            printf_debug("malloc size:%d\n",sizeof(FFT_SIGNAL_RESPINSE_ST) + sizeof(FFT_SIGNAL_RESULT_ST) * result_num);
            for(i = 0; i < resp_result->signal_num; i++){
                resp_result->signal_array[i].center_freq = fft_result->mid_freq_hz[i];
                resp_result->signal_array[i].bandwidth = fft_result->bw_hz[i];
                resp_result->signal_array[i].power_level = fft_result->level[i];
                
                center_freq_buf[cnt++] = resp_result->signal_array[i].center_freq;
                if(cnt >= AVG_FREQ_POINT){
                    for(int i = 0; i< AVG_FREQ_POINT; i++){
                        avg_freq +=  center_freq_buf[i];
                    }
                    printf_warn("After %d times filter: center freq:%llu\n", AVG_FREQ_POINT, avg_freq/AVG_FREQ_POINT);
                    cnt = 0;
                    avg_freq = 0;
                }
                
                printf_warn("[%d]center_freq=%lluHz, bandwidth=%llu, power_level=%f, peak:%d\n", i,
                    resp_result->signal_array[i].center_freq,
                    resp_result->signal_array[i].bandwidth,
                    resp_result->signal_array[i].power_level,
                    fft_result->peak_value);
            }
            /* not find signal */
            if(resp_result->signal_num == 0){
                 resp_result->signal_num = 1;
                 resp_result->signal_array[0].center_freq = fft_result->mid_freq_hz[0];
                 resp_result->signal_array[0].bandwidth = fft_result->bw_hz[0];
                 resp_result->signal_array[0].power_level = fft_result->level[0];
                 printf_warn("No Signal Found!!!\n");
                 printf_warn("center_freq=%lluHz, bandwidth=%llu, power_level=%f\n",
                    resp_result->signal_array[0].center_freq,
                    resp_result->signal_array[0].bandwidth,
                    resp_result->signal_array[0].power_level);
            }
            resp_result->temperature = io_get_ambient_temperature();
            resp_result->humidity = io_get_ambient_humidity();
            printf_warn("temperature:%0.2f℃, humidity:%0.2f%\n", resp_result->temperature, resp_result->humidity);
            datalen = sizeof(FFT_SIGNAL_RESPINSE_ST) + sizeof(FFT_SIGNAL_RESULT_ST)*result_num;
            memcpy(akt_get_response_data.payload_data, resp_result, datalen);
            akt_get_response_data.header.len = datalen;
            safe_free(resp_result);
        #else
            printf_warn("###########SPECTRUM ANALYSIS NOT SUPPORT#############\n");
        #endif
        }
        case SOFTWARE_VERSION_CMD:
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
            #endif
            printf_info("Get disk info: %d\n", ret);
            if(ret != 0){
                err_code = akt_err_code_check(ret);
                goto exit;
            }
            printf_debug("ret=%d, num=%d, speed=%uKB/s, capacity_bytes=%llu, used_bytes=%llu\n",
                ret, psi->disk_num, psi->read_write_speed_kbytesps, 
                psi->disk_capacity[0].disk_capacity_byte, psi->disk_capacity[0].disk_used_byte);
            memcpy(akt_get_response_data.payload_data, psi, st_size);
            akt_get_response_data.header.len = st_size;
            safe_free(psi);
            break;
        } 
        case SEARCH_FILE_STATUS_CMD:
        {
            int ret = 0;
            size_t f_bg_size = 0; 
            SEARCH_FILE_STATUS_RSP_ST fsp;
            char filename[FILE_PATH_MAX_LEN];
            
            memcpy(filename, header->buf, FILE_PATH_MAX_LEN);
            memset(&fsp, 0 ,sizeof(SEARCH_FILE_STATUS_RSP_ST));
            strcpy(fsp.filepath, filename);
            #if defined(SUPPORT_XWFS)
            ret = xwfs_get_file_size_by_name(filename, &f_bg_size, sizeof(ssize_t));//io_read_more_info_by_name(filename, &fsp, io_find_file_info);
            #endif
            printf_note("Find file:%s, fsize=%u ret =%d\n", fsp.filepath, f_bg_size, ret);
            if(ret != 0){
                fsp.status = 0;
                fsp.file_size = 0;
            }else{
                fsp.status = 1;
                fsp.file_size = (uint64_t)f_bg_size*8*1024*1024; /* data block group size is 8MByte */
            }
            printf_note("ret=%d, filepath=%s, file_size=[%u bg]%llu, status=%d\n",ret, fsp.filepath, f_bg_size, fsp.file_size, fsp.status);
            memcpy(akt_get_response_data.payload_data, &fsp, sizeof(SEARCH_FILE_STATUS_RSP_ST));
            akt_get_response_data.header.len = sizeof(SEARCH_FILE_STATUS_RSP_ST);
            break;
        }
        default:
            printf_debug("not support get commmand\n");
    }
exit:
    //io_set_refresh_keepalive_time(0);
    akt_get_response_data.header.operation = QUERY_CMD_RSP;
    return err_code;
}

static int akt_execute_net_command(void)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    
    DEVICE_NET_INFO_ST netinfo;
    struct in_addr ipdata;

    memcpy(netinfo.mac, poal_config->network.mac, sizeof(netinfo.mac));
    netinfo.ipaddr = htonl(poal_config->network.ipaddress);
    netinfo.gateway = htonl(poal_config->network.gateway);
    netinfo.mask = htonl(poal_config->network.netmask);
    netinfo.port = htons(poal_config->network.port);
    netinfo.status = 0;
    ipdata.s_addr = poal_config->network.ipaddress;
    printf_note("mac:%x%x%x%x%x%x, ipaddr=%x[%s], gateway=%x\n", netinfo.mac[0],netinfo.mac[1],netinfo.mac[2],netinfo.mac[3],netinfo.mac[4],netinfo.mac[5],
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
bool akt_execute_method(int *code, void *cl)
{
    PDU_CFG_REQ_HEADER_ST *header;
    int err_code;
    header = &akt_header;

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
            err_code = akt_execute_get_command();
            break;
        }
        case NET_CTRL_CMD:
        {
            if(header->code == HEART_BEAT_MSG_REQ){
                update_tcp_keepalive(cl);
                akt_get_response_data.header.len = 0;
            } else if(header->code == DISCOVER_LAN_DEV_PARAM){
                printf_note("discover ...\n");
                err_code = akt_execute_net_command();
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
        //header_len += 1; /*结构体struct response_set_data cid长度*/
        response_data = &akt_set_response_data;
        response_data->header.code = req_header->code;
    }else if(req_header->operation == NET_CTRL_CMD){
        /* hearbeat response code */
        if(req_header->code == HEART_BEAT_MSG_REQ){
            printf_info("response heartbeat code\n");
            response_data->header.operation = req_header->operation;
            response_data->header.code = HEART_BEAT_MSG_RSP;
        }else if(req_header->code == DISCOVER_LAN_DEV_PARAM){
            printf_note("response discover...\n");
            response_data->header.operation = req_header->operation;
            response_data->header.code = req_header->code;
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


uint8_t *akt_assamble_demodulation_data_extend_frame_header_data(uint32_t *len, void *config)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    static uint8_t param_buf[sizeof(DATUM_DEMODULATE_HEADER_ST)];
    DATUM_DEMODULATE_HEADER_ST *ext_hdr = NULL;
    static bool start_flag = false;
    uint32_t time_interval_ms;

    ext_hdr = (DATUM_DEMODULATE_HEADER_ST *)param_buf;
    
    memset(param_buf, 0, sizeof(DATUM_DEMODULATE_HEADER_ST));
    printf_debug("akt_assamble_data_extend_frame_header_data\n");
    ext_hdr->dev_id = akt_get_device_id();
    
    ext_hdr->work_mode =  poal_config->work_mode;
    ext_hdr->gain_mode = poal_config->rf_para[poal_config->cid].gain_ctrl_method;
    ext_hdr->gain = poal_config->rf_para[poal_config->cid].mgc_gain_value;

    time_interval_ms=(uint32_t)diff_time();
    if(start_flag == false){
        ext_hdr->duration = 100; /* first time, set default duration time:  100ms */
        start_flag = true;
    }else{
        ext_hdr->duration = (uint64_t)time_interval_ms;
    }
    printf_debug("ext_hdr->duration=%lu, time_interval_ms=%u\n", ext_hdr->duration,time_interval_ms);

    struct spm_run_parm *header_param;
    header_param = (struct spm_run_parm *)config;
    ext_hdr->cid = header_param->ch;
    ext_hdr->center_freq = header_param->m_freq;
    ext_hdr->bandwidth = header_param->bandwidth;
    ext_hdr->demodulate_type = header_param->d_method;
    ext_hdr->sample_rate = 0;
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
        printfd("%x ", *(param_buf+i));
    }
    printfd("\n");

    *len = sizeof(DATUM_DEMODULATE_HEADER_ST);
    return param_buf;
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
    static uint8_t param_buf[sizeof(DATUM_SPECTRUM_HEADER_ST)];
    DATUM_SPECTRUM_HEADER_ST *ext_hdr = NULL;
    static bool start_flag = false;
    uint32_t time_interval_ms;
    ext_hdr = (DATUM_SPECTRUM_HEADER_ST *)param_buf;
    
    memset(param_buf, 0, sizeof(DATUM_SPECTRUM_HEADER_ST));
    printf_debug("akt_assamble_data_extend_frame_header_data\n");
    ext_hdr->dev_id = akt_get_device_id();
    
    ext_hdr->work_mode =  poal_config->work_mode;
    ext_hdr->gain_mode = poal_config->rf_para[poal_config->cid].gain_ctrl_method;
    ext_hdr->gain = poal_config->rf_para[poal_config->cid].mgc_gain_value;

    time_interval_ms=(uint32_t)diff_time();
    if(start_flag == false){
        ext_hdr->duration = 100; /* first time, set default duration time:  100ms */
        start_flag = true;
    }else{
        ext_hdr->duration = (uint64_t)time_interval_ms;
    }
    printf_debug("ext_hdr->duration=%lu, time_interval_ms=%u\n", ext_hdr->duration,time_interval_ms);
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
    ext_hdr->sample_rate = 0;
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
        printfd("%x ", *(param_buf+i));
    }
    printfd("\n");

    *len = sizeof(DATUM_SPECTRUM_HEADER_ST);
    return param_buf;
}

/******************************************************************************
* FUNCTION:
*     akt_assamble_data_frame_header_data
*
* DESCRIPTION:
*     assamble data(FFT/IQ...) frame header
* PARAMETERS: 
*       @len: return total header len(data header+extend data header)
* RETURNS
******************************************************************************/
uint8_t * akt_assamble_data_frame_header_data(uint32_t *len, void *config)
{
    DATUM_PDU_HEADER_ST *package_header;
    static unsigned short seq_num[MAX_RADIO_CHANNEL_NUM] = {0};
    struct spm_run_parm *header_param;
    static uint8_t head_buf[sizeof(DATUM_PDU_HEADER_ST)+sizeof(DATUM_SPECTRUM_HEADER_ST)+sizeof(DATUM_DEMODULATE_HEADER_ST)];
    uint8_t *pextend;
    uint32_t extend_data_header_len;
    struct timeval tv;

    printf_debug("akt_assamble_data_frame_header_data. v3\n");
    header_param = (struct spm_run_parm *)config;
    printf_debug("header_param->ex_type:%d\n", header_param->ex_type);
    if(header_param->ex_type == SPECTRUM_DATUM)
        pextend = akt_assamble_data_extend_frame_header_data(&extend_data_header_len, config);
    else if(header_param->ex_type == DEMODULATE_DATUM)
        pextend = akt_assamble_demodulation_data_extend_frame_header_data(&extend_data_header_len, config);
    else{
        printf_err("extend type is not valid!\n");
        return NULL;
    }
    memset(head_buf, 0, sizeof(DATUM_PDU_HEADER_ST)+extend_data_header_len);
    memcpy(head_buf+ sizeof(DATUM_PDU_HEADER_ST), pextend, extend_data_header_len);
    
    package_header = (DATUM_PDU_HEADER_ST*)head_buf;
    package_header->syn_flag = AKT_START_FLAG;
    package_header->type = header_param->type;
    gettimeofday(&tv,NULL);
    package_header->toa = 0;//tv.tv_sec*1000 + tv.tv_usec/1000;
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
        printfd("%x ", *(head_buf + i));
    }
    printfd("\n");
    printfd("extend header len[%d]: \n", extend_data_header_len);
    for(int i = 0; i< extend_data_header_len; i++){
        printfd("%x ", *(head_buf + sizeof(DATUM_PDU_HEADER_ST)+i));
    }
    printfd("\n");
    return head_buf;
}




