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

static struct poal_config oal_config;
struct akt_protocal_param akt_config;


static int akt_free(void)
{
    PDU_CFG_REQ_HEADER_ST *header;
    header = &akt_header;
    if(header->pbuf != NULL){
        free(header->pbuf);
        header->pbuf = NULL;
    }
}

static int akt_convert_oal_config(uint8_t ch)
{
    struct akt_protocal_param *pakt_config = &akt_config;
    struct poal_config *poal_config = &oal_config;

    /*切换频点（频段）数*/
    uint8_t sig_cnt = 0;
    switch (poal_config->work_mode)
    {
        case OAL_FIXED_FREQ_ANYS_MODE:
        {
            sig_cnt= 0;
            /*主通道转换*/
            poal_config->multi_freq_point_param.cid = pakt_config->cid;
            /*(RF)中心频率转换*/
            poal_config->multi_freq_point_param.points[sig_cnt].center_freq = pakt_config->multi_freq_zone[ch].sig_ch[sig_cnt].center_freq;
            /* (RF)带宽转换 */
            poal_config->multi_freq_point_param.points[sig_cnt].bandwidth = pakt_config->multi_freq_zone[ch].sig_ch[sig_cnt].bandwidth;
            /* 解调带宽转换 */
            poal_config->multi_freq_point_param.points[sig_cnt].d_bandwith = pakt_config->decode_param[ch].sig_ch[sig_cnt].bandwidth;
            /* 不在需要解调中心频率，除非窄带解调 */
            /*解调方式转换*/
            poal_config->multi_freq_point_param.points[sig_cnt].d_method = pakt_config->decode_param[ch].sig_ch[sig_cnt].decode_method_id;
            /* fft size转换 */
            poal_config->multi_freq_point_param.points[sig_cnt].fft_size = pakt_config->fft[ch].fft_size;
             /* smooth */
            poal_config->multi_freq_point_param.smooth_time = pakt_config->smooth[ch].smooth;
        }
        case OAL_FAST_SCAN_MODE:
        {
            sig_cnt= 0;
            /*主通道转换*/
            poal_config->multi_freq_fregment_para.cid = pakt_config->cid;
            /*(RF)开始频率转换 = 中心频率 - 带宽/2 */
            poal_config->multi_freq_fregment_para.fregment[sig_cnt].start_freq = pakt_config->multi_freq_zone[ch].sig_ch[sig_cnt].center_freq - pakt_config->multi_freq_zone[ch].sig_ch[sig_cnt].bandwidth/2;
            /*(RF)结束频率转换 = 中心频率 + 带宽/2 */
            poal_config->multi_freq_fregment_para.fregment[sig_cnt].end_freq = pakt_config->multi_freq_zone[ch].sig_ch[sig_cnt].center_freq + pakt_config->multi_freq_zone[ch].sig_ch[sig_cnt].bandwidth/2;
            /* 步长 */
            poal_config->multi_freq_fregment_para.fregment[sig_cnt].step = pakt_config->multi_freq_zone[ch].sig_ch[sig_cnt].freq_step;
            /* fft size转换 */
            poal_config->multi_freq_fregment_para.fregment[sig_cnt].fft_size = pakt_config->fft[ch].fft_size;
            /* smooth */
            poal_config->multi_freq_fregment_para.smooth_time = pakt_config->smooth[ch].smooth;
            /*分辨率转换*/
            poal_config->multi_freq_fregment_para.fregment[sig_cnt].freq_resolution = pakt_config->multi_freq_zone[ch].sig_ch[sig_cnt].freq_resolution;
        }
        case OAL_MULTI_ZONE_SCAN_MODE:
        {

        }
        case OAL_MULTI_POINT_SCAN_MODE:
            break;
    }

}

static int akt_work_mode_set(struct akt_protocal_param *akt_config)
{
    struct poal_config *poal_config = &oal_config;
   
    if(akt_config->multi_freq_zone[akt_config->cid].resident_time== 0){
        if(akt_config->multi_freq_zone[akt_config->cid].freq_band_cnt == 1){
             /*scan mode*/
            poal_config->work_mode = OAL_FAST_SCAN_MODE;
        }else{
             /*multi freq zone mode*/
            poal_config->work_mode = OAL_MULTI_ZONE_SCAN_MODE;
        }
    }else{
        if(akt_config->multi_freq_zone[akt_config->cid].freq_band_cnt == 1){
            /*fixed freq*/
            poal_config->work_mode =OAL_FIXED_FREQ_ANYS_MODE;
        }else{
            /*multi freq point*/
            poal_config->work_mode =OAL_MULTI_POINT_SCAN_MODE;
        }
    }
    return 0;
}

static int akt_executor_set_command(void)
{
    struct poal_config *poal_config = &oal_config;
    
    if(poal_config->enable.psd_en){
        executor_set_command(EX_ENABLE_CMD, PSD_MODE_ENABLE, poal_config);
    }else{
        executor_set_command(EX_ENABLE_CMD, PSD_MODE_DISABLE, poal_config);
    }
    if(poal_config->enable.audio_en){
        executor_set_command(EX_ENABLE_CMD, AUDIO_MODE_ENABLE, poal_config);
    }else{
        executor_set_command(EX_ENABLE_CMD, AUDIO_MODE_DISABLE, poal_config);
    }
    if(poal_config->enable.iq_en){
        executor_set_command(EX_ENABLE_CMD, IQ_MODE_ENABLE, poal_config);
    }else{
        executor_set_command(EX_ENABLE_CMD, IQ_MODE_DISABLE, poal_config);
    }
    if(poal_config->enable.spec_analy_en){
        executor_set_command(EX_ENABLE_CMD, SPCTRUM_MODE_ANALYSIS_ENABLE, poal_config);
    }else{
        executor_set_command(EX_ENABLE_CMD, SPCTRUM_MODE_ANALYSIS_DISABLE, poal_config);
    }
    if(poal_config->enable.direction_en){
        executor_set_command(EX_ENABLE_CMD, DIRECTION_MODE_ENABLE, poal_config);
    }else{
        executor_set_command(EX_ENABLE_CMD, DIRECTION_MODE_ENABLE_DISABLE, poal_config);
    }
    return 0;
}


static int akt_execute_set_command(void)
{
    #define  convert_enable_mode(en, mask)  (en|mask == 0 ? 0 : 1)

    PDU_CFG_REQ_HEADER_ST *header;
    struct akt_protocal_param *pakt_config = &akt_config;
    struct poal_config *poal_config = &oal_config;
    int err_code;
    header = &akt_header;
    int ch;

    err_code = RET_CODE_SUCCSESS;
    printf_info("bussiness code[%d]", header->code);
    switch (header->code)
    {
        case OUTPUT_ENABLE_PARAM:
        {
            memcpy(&(pakt_config->enable), header->pbuf, sizeof(OUTPUT_ENABLE_PARAM_ST));
            if(check_radio_channel(pakt_config->enable.cid)){
                return -1;
            }
            /* Here we convert enable perameter */
            poal_config->enable.cid = pakt_config->enable.cid;
            poal_config->enable.sub_id = -1; /* not support in akt protocal */
            poal_config->enable.psd_en = convert_enable_mode(pakt_config->enable.output_en, SPECTRUM_MASK);
            poal_config->enable.audio_en = convert_enable_mode(pakt_config->enable.output_en, D_OUT_MASK);
            poal_config->enable.iq_en = convert_enable_mode(pakt_config->enable.output_en, IQ_OUT_MASK);

            if(pakt_config->enable.output_en){
                akt_convert_oal_config(pakt_config->enable.cid);
                akt_executor_set_command();
            }else{
                /*disable all output*/
                executor_set_command(EX_ENABLE_CMD, PSD_MODE_DISABLE, poal_config);
                executor_set_command(EX_ENABLE_CMD, AUDIO_MODE_DISABLE, poal_config);
                executor_set_command(EX_ENABLE_CMD, IQ_MODE_DISABLE, poal_config);
                executor_set_command(EX_ENABLE_CMD, SPCTRUM_MODE_ANALYSIS_DISABLE, poal_config);
                executor_set_command(EX_ENABLE_CMD, DIRECTION_MODE_ENABLE_DISABLE, poal_config);
            }    
            break;
        }
        case SAMPLE_CONTROL_FFT_CMD:
        {
            break;
        }
        case DIRECTION_FREQ_POINT_REQ_CMD:
        {
            pakt_config->cid = header->pbuf[0];
            if(check_radio_channel(pakt_config->cid)){
                return -1;
            }
            ch = pakt_config->cid;
            memcpy(&(pakt_config->fft[ch]), header->pbuf, sizeof(DIRECTION_FFT_PARAM));
            break;
        }
        case DIRECTION_MULTI_FREQ_ZONE_CMD:
        {
            
            pakt_config->cid = header->pbuf[0];
            if(check_radio_channel(pakt_config->cid)){
                return -1;
            }
            ch = pakt_config->cid;
            memcpy(&pakt_config->multi_freq_zone[ch], header->pbuf, sizeof(DIRECTION_MULTI_FREQ_ZONE_PARAM));
            akt_work_mode_set(pakt_config);
            break;
        }
        case MULTI_FREQ_DECODE_CMD:
        {
            pakt_config->cid = header->pbuf[0];
            if(check_radio_channel(pakt_config->cid)){
                return -1;
            }
            memcpy(&(pakt_config->decode_param[pakt_config->cid]), header->pbuf, sizeof(MULTI_FREQ_DECODE_PARAM));
            break;
        }
        case DIRECTION_SMOOTH_CMD:
        {
            pakt_config->cid = header->pbuf[0];
            if(check_radio_channel(pakt_config->cid)){
                return -1;
            }
            memcpy(&(pakt_config->smooth[pakt_config->cid]), header->pbuf, sizeof(DIRECTION_SMOOTH_PARAM));
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
        default:
          printf_err("error class code[%d]\n",header->code);
          err_code = RET_CODE_PARAMTER_ERR;
          goto exit;
    }

exit:
    return err_code;

}

static int akt_execute_get_command(void)
{

}

static int akt_execute_net_command(void)
{

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
static int akt_execute_method(void)
{
    PDU_CFG_REQ_HEADER_ST *header;
    int err_code;
    header = &akt_header;

    err_code = RET_CODE_SUCCSESS;
    printf_info("operation code[%d]", header->operation);
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
            printf_err("invalid method[%d]", header->operation);
            break;
        }
        default:
            printf_err("error method[%d]", header->operation);
            err_code = RET_CODE_PARAMTER_ERR;
            break;
    }
    return err_code;

    return 0;
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
static bool akt_parse_header(const uint8_t *data, int len, uint8_t **payload, int *err_code)
{
    uint8_t *val;
    PDU_CFG_REQ_HEADER_ST *header, *pdata;
    header = &akt_header;
    pdata = data;
    int header_len;

    header_len = sizeof(PDU_CFG_REQ_HEADER_ST) - sizeof(header->pbuf);

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
    printf_info("header.start_flag=%x\n", header->start_flag);
    printf_info("header.len=%x\n", header->len);
    printf_info("header.method_code=%x\n", header->operation);
    printf_info("header.code=%x\n", header->code);
    printf_info("header.usr_id=%llx\n", header->usr_id);
    printf_info("header.receiver_id=%x\n", header->receiver_id);
    printf_info("header.crc=%s, %x\n", header->crc);

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


static bool akt_parse_data(const uint8_t *payload)
{
    int i;
    PDU_CFG_REQ_HEADER_ST *header;
    header = &akt_header;
    
    printf_debug("payload:\n");
    for(i = 0; i< header->len; i++)
        printfd("%x ", payload[i]);
    printfd("\n");

    header->pbuf = calloc(1, header->len);
    if (!header->pbuf){
        printf_err("calloc failed\n");
        return false;
    }

    memcpy(header->pbuf, payload, header->len);

    return true;
}

/******************************************************************************
* FUNCTION:
*     akt_handle_request
*
* DESCRIPTION:
*     akt protocol receive data handle process
*     
* PARAMETERS
*     data:  receive data pointer
*       len:  reveive data length
*       code:  return error code
* RETURNS
*     false: handle data false 
*      ture: handle data successful
******************************************************************************/

bool akt_handle_request(char *data, int len, int *code)
{
    uint8_t *payload = NULL;
    
    printf_info("len[%d] %d\n", len, sizeof(struct xnrp_header));
    printf_info("Prepare to handle akt protocol data\n");
    if(akt_parse_header(data, len, &payload, code) == false){
        return false;
    }
    if(payload != NULL){
        if(akt_parse_data(payload) == false){
            *code = RET_CODE_INTERNAL_ERR;
            return false;
        }
    }

    if(akt_execute_method() != RET_CODE_SUCCSESS){
        akt_free();
        return false;
    }
    akt_free();
    return true;
}

/******************************************************************************
* FUNCTION:
*     akt_assamble_response_data
*
* DESCRIPTION:
*     akt protocol send data prepare: assamble package
*     
* PARAMETERS
*     buf:  send data pointer(not include header)
*    code: error code
* RETURNS
*     len: total data len 
******************************************************************************/

int akt_assamble_response_data(char *buf,          int err_code)
{
    int len = 0;
    printf_info("Prepare to assamble response akt data\n");
    return len;
}


