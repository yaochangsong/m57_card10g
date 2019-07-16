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

bool akt_assamble_kernel_header_response_data(char *pbuf, work_mode wmode);


int akt_get_device_id(void)
{
    return 0;
}

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
                poal_config->multi_freq_point_param[ch].points[i].d_method = pakt_config->decode_param[ch].sig_ch[i].decode_method_id;
                poal_config->multi_freq_point_param[ch].points[i].d_bandwith = pakt_config->decode_param[ch].sig_ch[i].bandwidth;
                poal_config->multi_freq_point_param[ch].points[i].center_freq = pakt_config->decode_param[ch].sig_ch[i].center_freq;
                /* 不在需要解调中心频率，除非窄带解调 */
                printf_info("ch:%d,subch:%d, d_bw:%u\n", ch, i,poal_config->multi_freq_point_param[ch].points[i].d_bandwith);
                printf_info("ch:%d,d_method:%u\n", ch, poal_config->multi_freq_point_param[ch].points[i].d_method);
                printf_info("ch:%d,center_freq:%u\n", ch, poal_config->multi_freq_point_param[ch].points[i].center_freq);
            }
            break;
       case OUTPUT_ENABLE_PARAM:
                /* 使能位转换 */
            poal_config->enable.cid = pakt_config->enable.cid;
            poal_config->enable.sub_id = -1; /* not support in akt protocal */
            poal_config->enable.psd_en = convert_enable_mode(pakt_config->enable.output_en, SPECTRUM_MASK);
            poal_config->enable.audio_en = convert_enable_mode(pakt_config->enable.output_en, D_OUT_MASK);
            poal_config->enable.iq_en = convert_enable_mode(pakt_config->enable.output_en, IQ_OUT_MASK);
           // poal_config->enable.bit_en = pakt_config->enable.output_en;
            INTERNEL_ENABLE_BIT_SET(poal_config->enable.bit_en,poal_config->enable);
            printf_info("bit_en=%x,psd_en=%d, audio_en=%d,iq_en=%d\n", poal_config->enable.bit_en, 
                        poal_config->enable.psd_en,poal_config->enable.audio_en,poal_config->enable.iq_en);
            printf_info("work_mode=%d\n", poal_config->work_mode);
        case DIRECTION_MULTI_FREQ_ZONE_CMD: /* 多频段扫描参数 */
        {
            switch (poal_config->work_mode)
            {
                case OAL_FIXED_FREQ_ANYS_MODE:
                {
                    sig_cnt= 0;
                    /*主通道转换*/
                    poal_config->multi_freq_point_param[ch].cid = pakt_config->cid;
                    /*(RF)中心频率转换*/
                    poal_config->multi_freq_point_param[ch].points[sig_cnt].center_freq = pakt_config->multi_freq_zone[ch].sig_ch[sig_cnt].center_freq;
                    /* (RF)带宽转换 */
                    poal_config->multi_freq_point_param[ch].points[sig_cnt].bandwidth = pakt_config->multi_freq_zone[ch].sig_ch[sig_cnt].bandwidth;
                    /* 解调带宽转换 */
                   // poal_config->multi_freq_point_param[ch].points[sig_cnt].d_bandwith = pakt_config->decode_param[ch].sig_ch[sig_cnt].bandwidth;
                    /* 不在需要解调中心频率，除非窄带解调 */
                    /*解调方式转换*/
                   // poal_config->multi_freq_point_param[ch].points[sig_cnt].d_method = pakt_config->decode_param[ch].sig_ch[sig_cnt].decode_method_id;
                    /* fft size转换 */
                    poal_config->multi_freq_point_param[ch].points[sig_cnt].fft_size = pakt_config->fft[ch].fft_size;
                     /* smooth */
                    poal_config->multi_freq_point_param[ch].smooth_time = pakt_config->smooth[ch].smooth;
                    printf_info("----------------------------fixed freq:---------------------------------\n");
                    printf_info("ch:%d, sig_cnt:%d,center_freq:%u\n", ch,sig_cnt,poal_config->multi_freq_point_param[ch].points[sig_cnt].center_freq);
                    printf_info("bandwidth:%u\n",poal_config->multi_freq_point_param[ch].points[sig_cnt].bandwidth);
                    printf_info("fft_size:%u\n",poal_config->multi_freq_point_param[ch].points[sig_cnt].fft_size);
                    printf_info("smooth_time:%u\n",poal_config->multi_freq_point_param[ch].smooth_time);
                    break;
                }
                case OAL_FAST_SCAN_MODE:
                {
                    sig_cnt= 0;
                    /*主通道转换*/
                    poal_config->multi_freq_fregment_para[ch].cid = pakt_config->cid;
                    /*(RF)开始频率转换 = 中心频率 - 带宽/2 */
                    poal_config->multi_freq_fregment_para[ch].fregment[sig_cnt].start_freq = pakt_config->multi_freq_zone[ch].sig_ch[sig_cnt].center_freq - pakt_config->multi_freq_zone[ch].sig_ch[sig_cnt].bandwidth/2;
                    /*(RF)结束频率转换 = 中心频率 + 带宽/2 */
                    poal_config->multi_freq_fregment_para[ch].fregment[sig_cnt].end_freq = pakt_config->multi_freq_zone[ch].sig_ch[sig_cnt].center_freq + pakt_config->multi_freq_zone[ch].sig_ch[sig_cnt].bandwidth/2;
                    /* 步长 */
                    poal_config->multi_freq_fregment_para[ch].fregment[sig_cnt].step = pakt_config->multi_freq_zone[ch].sig_ch[sig_cnt].freq_step;
                    /* fft size转换 */
                    poal_config->multi_freq_fregment_para[ch].fregment[sig_cnt].fft_size = pakt_config->fft[ch].fft_size;
                    /* smooth */
                    poal_config->multi_freq_fregment_para[ch].smooth_time = pakt_config->smooth[ch].smooth;
                    /*分辨率转换*/
                    poal_config->multi_freq_fregment_para[ch].fregment[sig_cnt].freq_resolution = pakt_config->multi_freq_zone[ch].sig_ch[sig_cnt].freq_resolution;
                    break;
                }
                case OAL_MULTI_ZONE_SCAN_MODE:
                {
                    break;
                }
                case OAL_MULTI_POINT_SCAN_MODE:
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

static int akt_executor_set_enable_command(uint8_t ch)
{
    struct akt_protocal_param *pakt_config = &akt_config;
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    printf_debug("bit_en[%x]\n", poal_config->enable.bit_en);
    if(poal_config->enable.bit_en == 0){
        printf_info("all Work disabled, waite thread stop...\n");
    }else{
        printf_debug("akt_assamble work_mode[%d]\n", poal_config->work_mode);
        printf_debug("bit_en=%x,psd_en=%d, audio_en=%d,iq_en=%d\n", poal_config->enable.bit_en, 
            poal_config->enable.psd_en,poal_config->enable.audio_en,poal_config->enable.iq_en);
        poal_config->assamble_kernel_response_data = akt_assamble_kernel_header_response_data;
        switch (poal_config->work_mode)
        {
            case OAL_FIXED_FREQ_ANYS_MODE:
            {
                executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, &poal_config->rf_para[ch].attenuation);
                executor_set_command(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, &poal_config->rf_para[ch].mgc_gain_value);
                executor_set_command(EX_RF_FREQ_CMD, EX_RF_MID_FREQ, &poal_config->rf_para[ch].mid_freq);
                executor_set_command(EX_RF_FREQ_CMD, EX_RF_MID_BW, &poal_config->rf_para[ch].mid_bw);
                executor_set_command(EX_WORK_MODE_CMD, EX_FIXED_FREQ_ANYS_MODE, NULL);
                executor_set_command(EX_MID_FREQ_CMD, EX_CHANNEL_SELECT, &poal_config->enable.cid);
                executor_set_command(EX_MID_FREQ_CMD, EX_SMOOTH_TIME, &poal_config->multi_freq_point_param[ch].smooth_time);
                executor_set_command(EX_MID_FREQ_CMD, EX_FFT_SIZE, &poal_config->multi_freq_point_param[ch].points[poal_config->enable.sub_id].fft_size);
                executor_set_command(EX_MID_FREQ_CMD, EX_BANDWITH, &poal_config->multi_freq_point_param[ch].points[poal_config->enable.sub_id].bandwidth);
                executor_set_command(EX_MID_FREQ_CMD, EX_DEC_METHOD, ch);
                break;
            }
            case OAL_FAST_SCAN_MODE:
            {
                executor_set_command(EX_WORK_MODE_CMD, EX_FAST_SCAN_MODE, NULL);
                break;
            }
            case OAL_MULTI_ZONE_SCAN_MODE:
            {
                executor_set_command(EX_WORK_MODE_CMD, EX_MULTI_ZONE_SCAN_MODE, NULL);
                break;
            }
            case OAL_MULTI_POINT_SCAN_MODE:
                executor_set_command(EX_WORK_MODE_CMD, EX_MULTI_POINT_SCAN_MODE, NULL);
                break;
            default:
                return -1;
        }
        executor_set_command(EX_ENABLE_CMD, 0, NULL);
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
    printf_info("set bussiness code[%x]\n", header->code);
    switch (header->code)
    {
        case OUTPUT_ENABLE_PARAM:
        {
            printf_debug("enable[cid:%x en:%x]\n", header->buf[0], header->buf[1]);
            memcpy(&(pakt_config->enable), header->buf, sizeof(OUTPUT_ENABLE_PARAM_ST));
            ch = poal_config->cid = pakt_config->enable.cid;
            if(check_radio_channel(pakt_config->enable.cid)){
                err_code = RET_CODE_PARAMTER_ERR;
                goto set_exit;
            }
            if(pakt_config->enable.output_en){
                if(akt_convert_oal_config(ch, OUTPUT_ENABLE_PARAM) == -1){
                    err_code = RET_CODE_PARAMTER_NOT_SET;
                    goto set_exit;
                }
            }   
            akt_executor_set_enable_command(ch);
            break;
        }
        case SAMPLE_CONTROL_FFT_CMD:
        {
            break;
        }
        case DIRECTION_FREQ_POINT_REQ_CMD:
        {
            pakt_config->cid = header->buf[0];
            if(check_radio_channel(pakt_config->cid)){
                err_code = RET_CODE_PARAMTER_ERR;
                goto set_exit;
            }
            ch = poal_config->cid = pakt_config->cid;
            memcpy(&(pakt_config->fft[ch]), header->buf, sizeof(DIRECTION_FFT_PARAM));
            break;
        }
        case DIRECTION_MULTI_FREQ_ZONE_CMD:
        {
            
            pakt_config->cid = header->buf[0];
            if(check_radio_channel(pakt_config->cid)){
                err_code = RET_CODE_PARAMTER_ERR;
                goto set_exit;
            }
            ch = poal_config->cid = pakt_config->cid;
            printf_debug("cid:%d\n", ch);
            memcpy(&pakt_config->multi_freq_zone[ch], header->buf, sizeof(DIRECTION_MULTI_FREQ_ZONE_PARAM));
            akt_work_mode_set(pakt_config);
            if(akt_convert_oal_config(ch, DIRECTION_MULTI_FREQ_ZONE_CMD) == -1){
                err_code = RET_CODE_PARAMTER_NOT_SET;
                goto set_exit;
            }
            //config_save_batch(EX_WORK_MODE_CMD, EX_MULTI_ZONE_SCAN_MODE, poal_config);
            break;
        }
        case MULTI_FREQ_DECODE_CMD:
        {
            pakt_config->cid = header->buf[0];
            if(check_radio_channel(pakt_config->cid)){
                err_code = RET_CODE_PARAMTER_ERR;
                goto set_exit;
            }
            ch = poal_config->cid = pakt_config->cid;
            memcpy(&(pakt_config->decode_param[pakt_config->cid]), header->buf, sizeof(MULTI_FREQ_DECODE_PARAM));
            if(akt_convert_oal_config(ch, MULTI_FREQ_DECODE_CMD) == -1){
                err_code = RET_CODE_PARAMTER_NOT_SET;
                goto set_exit;
            }
            config_save_batch(EX_WORK_MODE_CMD, EX_DEC_METHOD, poal_config);
            break;
        }
        case DIRECTION_SMOOTH_CMD:
        {
            pakt_config->cid = header->buf[0];
            if(check_radio_channel(pakt_config->cid)){
                err_code = RET_CODE_PARAMTER_ERR;
                goto set_exit;
            }
            ch = poal_config->cid = pakt_config->cid;
            memcpy(&(pakt_config->smooth[pakt_config->cid]), header->buf, sizeof(DIRECTION_SMOOTH_PARAM));
            poal_config->multi_freq_point_param[ch].smooth_time = pakt_config->smooth[ch].smooth;
            executor_set_command(EX_MID_FREQ_CMD, EX_SMOOTH_TIME, &poal_config->multi_freq_point_param[ch].smooth_time);
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
        case SUB_SIGNAL_PARAM_CMD:
        {
          break;
        }
        case SUB_SIGNAL_OUTPUT_ENABLE_CMD:
        {
          break;
        }
        case MID_FREQ_ATTENUATION_CMD:
        {
            break;
        }
        case DECODE_METHOD_CMD:
        {
            break;
        }
        case SNIFFER_DATA_REPORT_PARAM:
        {
            SNIFFER_DATA_REPORT_ST net_para;

            net_para.cid = header->buf[0];
            poal_config->cid = net_para.cid;
            printf_debug("net_para.cid=%d\n", net_para.cid);
            if(check_radio_channel(net_para.cid)){
                err_code = RET_CODE_PARAMTER_ERR;
                goto set_exit;
            }
            ch = poal_config->cid;
            memcpy(&net_para, header->buf, sizeof(SNIFFER_DATA_REPORT_ST));
            
            struct in_addr ipdata;
            char *ipstr=NULL;
            ipdata.s_addr = ntohl(net_para.ipaddr);
            ipstr= inet_ntoa(ipdata);
            printf_debug("ipstr=%s  ipaddr=%x, port=%d, type=%d\n", ipstr,  ipdata.s_addr, htons(net_para.port), net_para.type);
            break;
        }
        case AUDIO_SAMPLE_RATE:
        {
            poal_config->cid = header->buf[0];
            if(check_radio_channel(poal_config->cid)){
                err_code = RET_CODE_PARAMTER_ERR;
                goto set_exit;
            }
            ch = poal_config->cid;
            poal_config->multi_freq_point_param[ch].audio_sample_rate = *((uint32_t *)(header->buf+1));
            executor_set_command(EX_MID_FREQ_CMD, EX_AUDIO_SAMPLE_RATE, &ch);
        }
        case MID_FREQ_BANDWIDTH_CMD:
        {
            poal_config->cid = header->buf[0];
            if(check_radio_channel(poal_config->cid)){
                err_code = RET_CODE_PARAMTER_ERR;
                goto set_exit;
            }
            ch = poal_config->cid;
            poal_config->multi_freq_point_param[ch].bandwidth = *((uint32_t *)(header->buf+1));
            executor_set_command(EX_MID_FREQ_CMD, EX_BANDWITH, &ch);
            executor_set_command(EX_RF_FREQ_CMD, EX_RF_MID_BW, &ch);
            break;
        }
        case RF_ATTENUATION_CMD:
            poal_config->cid = header->buf[0];
            if(check_radio_channel(poal_config->cid)){
                err_code = RET_CODE_PARAMTER_ERR;
                goto set_exit;
            }
            ch = poal_config->cid;
            poal_config->rf_para[ch].attenuation = header->buf[1];
            executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, &ch);
            break;
        case RF_WORK_MODE_CMD:
            poal_config->cid = header->buf[0];
            if(check_radio_channel(poal_config->cid)){
                err_code = RET_CODE_PARAMTER_ERR;
                goto set_exit;
            }
            ch = poal_config->cid;
            poal_config->rf_para[ch].rf_mode_code = header->buf[1];
            executor_set_command(EX_RF_FREQ_CMD, EX_RF_MODE_CODE, &ch);
            break;
        case RF_GAIN_MODE_CMD:
             poal_config->cid = header->buf[0];
            if(check_radio_channel(poal_config->cid)){
                err_code = RET_CODE_PARAMTER_ERR;
                goto set_exit;
            }
            ch = poal_config->cid;
            poal_config->rf_para[ch].gain_ctrl_method = header->buf[1];
            executor_set_command(EX_RF_FREQ_CMD, EX_RF_GAIN_MODE, &ch);
            break;
        default:
          printf_err("not support class code[%x]\n",header->code);
          err_code = RET_CODE_PARAMTER_ERR;
          goto set_exit;
    }
set_exit:
    memcpy(akt_set_response_data.payload_data, &err_code, sizeof(err_code));
    akt_set_response_data.header.len = sizeof(err_code);
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
            err_code = akt_execute_net_command();
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

    req_header = &akt_header;
    response_data = &akt_get_response_data;
    
    header_len = sizeof(PDU_CFG_RSP_HEADER_ST);
    if(req_header->operation == SET_CMD_REQ){
        header_len += 1; /*结构体struct response_set_data cid长度*/
        response_data = &akt_set_response_data;
    }
    
    *buf = response_data;

    response_data->header.start_flag = AKT_START_FLAG; 
    response_data->header.code = req_header->code;
    memcpy(response_data->header.usr_id, req_header->usr_id, sizeof(req_header->usr_id));
    response_data->header.receiver_id = 0;
    response_data->header.crc = crc16_caculate((uint8_t *)response_data->payload_data, response_data->header.len);
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

bool akt_assamble_kernel_header_response_data(char *pbuf, work_mode wmode)
{
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    DATUM_SPECTRUM_HEADER_ST *ext_hdr = NULL;
    static char param_buf[sizeof(DATUM_PDU_HEADER_ST)+sizeof(DATUM_SPECTRUM_HEADER_ST)];
    
    ext_hdr = (DATUM_SPECTRUM_HEADER_ST*)(&param_buf + sizeof(DATUM_PDU_HEADER_ST));
    ext_hdr->dev_id = akt_get_device_id();
    ext_hdr->cid = poal_config->cid;
    ext_hdr->work_mode = poal_config->work_mode;
    ext_hdr->gain_mode = poal_config->rf_para[poal_config->cid].gain_ctrl_method;
    ext_hdr->gain = poal_config->rf_para[poal_config->cid].mgc_gain_value;
    ext_hdr->duration = 0;   
    ext_hdr->datum_type = 0;
    printf_debug("assamble kernel header[mode: %d]\n", wmode);
    switch (wmode)
    {
        case EX_FIXED_FREQ_ANYS_MODE:
        {
            uint8_t point_cnt;
            point_cnt = 0;
            /* start freq = center freq - bindwith/2*/
            ext_hdr->start_freq = poal_config->multi_freq_point_param[ext_hdr->cid].points[point_cnt].center_freq - poal_config->multi_freq_point_param[ext_hdr->cid].points[point_cnt].bandwidth/2;
            ext_hdr->cutoff_freq = poal_config->multi_freq_point_param[ext_hdr->cid].points[point_cnt].center_freq + poal_config->multi_freq_point_param[ext_hdr->cid].points[point_cnt].bandwidth/2;
            ext_hdr->center_freq = poal_config->multi_freq_point_param[ext_hdr->cid].points[point_cnt].center_freq;
            //ext_hdr->bandwith = poal_config->multi_freq_point_param[ext_hdr->cid].points[point_cnt].bandwidth;
            ext_hdr->bandwith = poal_config->multi_freq_point_param[ext_hdr->cid].bandwidth;
            ext_hdr->sample_rate = poal_config->multi_freq_point_param[ext_hdr->cid].audio_sample_rate;
            ext_hdr->fft_len = poal_config->multi_freq_point_param[ext_hdr->cid].points[point_cnt].fft_size;
            ext_hdr->freq_resolution = (ext_hdr->bandwith* BAND_FACTOR)/ext_hdr->fft_len;
            ext_hdr->datum_total = 1;
            ext_hdr->sn = 0;
            break;
        }
        case EX_FAST_SCAN_MODE:
        {
            break;
        }
        case EX_MULTI_ZONE_SCAN_MODE:
        {
            break;
        }
        case EX_MULTI_POINT_SCAN_MODE:
            break;
    }
    pbuf = &param_buf;
    return true;
}



