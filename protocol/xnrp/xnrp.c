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
#include "xnrp-xml.h"


struct xnrp_header xnrp_data;
struct xnrp_header xnrp_response_data;


static uint8_t *xnrp_strstr(const uint8_t *s1, const uint8_t *s2, int len)   
{  
    int n = 0;  
    if (*s2)   
    {   
        while (*s1 && len > n)   
        {   
            for (n = 0; *(s1 + n) == *(s2 + n); n ++)  
            {   
                if (!*(s2 + n + 1))   
                    return (uint8_t *)s1;   
            }   
            s1++;   
        }  
        return NULL;   
    }   
    else   
        return s1;   
} 


//static int xnrp_execute_set_command(void)
int xnrp_execute_set_command(void)

{
    printf_debug("xnrp_execute_set_command()\n");
    struct xnrp_header *header;
    //struct xnrp_header xnrp_data;
    //xnrp_data.class_code=CLASS_CODE_NET;
  //  xnrp_data.business_code=B_CODE_ALL_NET;
    int err_code;
    header = &xnrp_data;
    err_code = RET_CODE_SUCCSESS;
    
    switch (header->class_code)
    {
        case CLASS_CODE_REGISTER:
        {
            break;
        }
        case CLASS_CODE_NET:
        {
            switch (header->business_code)
            {
                case B_CODE_ALL_NET:
                {
                    xnrp_xml_parse_data(header->class_code, header->business_code, header->payload, header->payload_len);
                    config_save_batch(EX_NETWORK_CMD, NULL, config_get_config());
                    break;
                }
                default:
                    printf_err("invalid bussiness code[%d]", header->business_code);
            }
            break;
        }
        case CLASS_CODE_WORK_MODE:
        {
            switch (header->business_code)
            {
                case B_CODE_WK_MODE_MULTI_FRQ_POINT:
                {
                    xnrp_xml_parse_data(header->class_code, header->business_code, header->payload, header->payload_len);
                    config_save_batch(EX_WORK_MODE_CMD, EX_MULTI_POINT_SCAN_MODE, config_get_config());
                    break;
                }
                case B_CODE_WK_MODE_SUB_CH_DEC:
                    xnrp_xml_parse_data(header->class_code, header->business_code, header->payload, header->payload_len);
                    break;
                case B_CODE_WK_MODE_MULTI_FRQ_FREGMENT:
                    xnrp_xml_parse_data(header->class_code, header->business_code, header->payload, header->payload_len);
                    break;
                default:
                    printf_err("invalid bussiness code[%d]", header->business_code);
            }
            break;
        }
        case CLASS_CODE_MID_FRQ:
        {
            switch(header->business_code)
            {
                case B_CODE_MID_FRQ_DEC_METHOD:
                {
                    xnrp_xml_parse_data(header->class_code, header->business_code, header->payload, header->payload_len);
                    config_save_batch(EX_MID_FREQ_CMD, EX_DEC_BW, config_get_config());
                    break;

                }
                case B_CODE_MID_FRQ_MUTE_SW:
                {
                    xnrp_xml_parse_data(header->class_code, header->business_code, header->payload, header->payload_len);
                    config_save_batch(EX_MID_FREQ_CMD, EX_MUTE_SW, config_get_config());
                    break;


                }
                case B_CODE_MID_FRQ_MUTE_THRE:
                {
                    xnrp_xml_parse_data(header->class_code, header->business_code, header->payload, header->payload_len);
                    config_save_batch(EX_MID_FREQ_CMD, EX_MUTE_THRE, config_get_config());
                    break;

                }
                case B_CODE_MID_FRQ_AU_SAMPLE_RATE:
                {
                    xnrp_xml_parse_data(header->class_code, header->business_code, header->payload, header->payload_len);
                    break;
                }
                default :
                  printf_err("invalid bussiness code[%d]", header->business_code);

            }
            break;
        }
        case CLASS_CODE_RF:
        {
            switch(header->business_code)
            {
                case B_CODE_RF_FRQ_PARA:
                {
                    xnrp_xml_parse_data(header->class_code, header->business_code, header->payload, header->payload_len);
                    config_save_batch(EX_RF_FREQ_CMD, EX_RF_MODE_CODE, config_get_config());
                    config_save_batch(EX_RF_FREQ_CMD, EX_RF_GAIN_MODE, config_get_config());
                    config_save_batch(EX_RF_FREQ_CMD, EX_RF_AGC_CTRL_TIME, config_get_config());
                    config_save_batch(EX_RF_FREQ_CMD, EX_RF_AGC_OUTPUT_AMP, config_get_config());
                    config_save_batch(EX_RF_FREQ_CMD, EX_RF_MID_BW, config_get_config());
                    break;

                }
                case B_CODE_RF_FRQ_ANTENASELEC:
                {
                    xnrp_xml_parse_data(header->class_code, header->business_code, header->payload, header->payload_len);
                    //config_save_batch(EX_RF_FREQ_CMD, EX_MUTE_SW, config_get_config());//没有这个选项
                    break;


                }
                case B_CODE_RF_FRQ_OUTPUT_ATTENUATION:
                {
                    xnrp_xml_parse_data(header->class_code, header->business_code, header->payload, header->payload_len);
                    //config_save_batch(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, config_get_config());//xml没有
                    break;

                }
                case B_CODE_RF_FRQ_INTPUT_ATTENUATION:
                {
                    xnrp_xml_parse_data(header->class_code, header->business_code, header->payload, header->payload_len);
                    //config_save_batch(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, config_get_config());//xml没有
                    break;
                }
                case B_CODE_RF_FRQ_INTPUT_BANDWIDTH:
                {
                    xnrp_xml_parse_data(header->class_code, header->business_code, header->payload, header->payload_len);
                    config_save_batch(EX_RF_FREQ_CMD, EX_RF_MID_BW, config_get_config());
                    break;
                }

                default :
                  printf_err("invalid bussiness code[%d]", header->business_code);

            }
            break;
        }
        case CLASS_CODE_CONTROL:
        {
            switch(header->business_code)
            {
                case B_CODE_CONTROL_CONTR_DATA_OUTPUT_ENABLE:
                {
                    xnrp_xml_parse_data(header->class_code, header->business_code, header->payload, header->payload_len);
                    config_save_batch(EX_RF_FREQ_CMD, PSD_MODE_ENABLE, config_get_config());
                    config_save_batch(EX_RF_FREQ_CMD, AUDIO_MODE_ENABLE, config_get_config());
                    config_save_batch(EX_RF_FREQ_CMD, IQ_MODE_ENABLE, config_get_config());
                    config_save_batch(EX_RF_FREQ_CMD, SPCTRUM_MODE_ANALYSIS_ENABLE, config_get_config());
                    config_save_batch(EX_RF_FREQ_CMD, DIRECTION_MODE_ENABLE, config_get_config());
                    break;
                }
                case B_CODE_CONTROL_CONTR_CALIBRATION_CONTR:
                {
                    xnrp_xml_parse_data(header->class_code, header->business_code, header->payload, header->payload_len);
                    break;
                }
                case B_CODE_CONTROL_CONTR_LOCAL_REMOTE:
                {
                    xnrp_xml_parse_data(header->class_code, header->business_code, header->payload, header->payload_len);
                    break;
                }
                case B_CODE_CONTROL_CONTR_CHANNEL_POWER:
                {
                    xnrp_xml_parse_data(header->class_code, header->business_code, header->payload, header->payload_len);
                    break;
                }
                case B_CODE_CONTROL_CONTR_TIME_CONTR:
                {
                    xnrp_xml_parse_data(header->class_code, header->business_code, header->payload, header->payload_len);
                    break;
                }

            }
            break;
        }
        case CLASS_CODE_STATUS:
        {
            break;
        }
        case CLASS_CODE_JOURNAL:
        {
            break;
        }
        case CLASS_CODE_FILE:
        {
            break;
        }
        case CLASS_CODE_HEARTBEAT:
        {
            err_code = RET_CODE_PARAMTER_INVALID;
            break;
        }
        default:
            printf_err("error class code[%d]\n",header->class_code);
            err_code = RET_CODE_PARAMTER_ERR;
            break;
    }
    return err_code;
}

bool xnrp_execute_method(int *code)
{
    struct xnrp_header *header;
    int err_code;
    header = &xnrp_data;

    err_code = RET_CODE_SUCCSESS;
    switch (header->method_code)
    {
        case METHOD_SET_COMMAND:
        {
            err_code = xnrp_execute_set_command();
            break;
        }
        case METHOD_GET_COMMAND:
        {
            err_code = xnrp_xml_execute_get_command(header->class_code,header->business_code,header->payload,header->payload_len);
            break;
        }
        case METHOD_RESPONSE_COMMAND:
        case METHOD_REPORT_COMMAND:
        {
            err_code = RET_CODE_PARAMTER_INVALID;
            printf_err("invalid method[%d]", header->method_code);
            break;
        }
        default:
            printf_err("error method[%d]", header->method_code);
            err_code = RET_CODE_PARAMTER_ERR;
            break;
    }
    *code = err_code;
     printf_debug("error code[%d]\n", *code);
    if(err_code == RET_CODE_SUCCSESS)
        return true;
    else
        return false;

}


bool xnrp_parse_header(const uint8_t *data, int len, uint8_t **payload, int *err_code)
{

    printf_debug("=================================xnrp_parse_header\n");
    uint8_t *val;
    struct xnrp_header *header;
    header = &xnrp_data;
    int header_len;

    header_len = sizeof(struct xnrp_header) - sizeof(header->payload);

    if(len < header_len){
        printf_err("receive data len[%d < %d] is too short\n", len, header_len);
        *err_code = RET_CODE_FORMAT_ERR;
        return false;
    }
    printf_debug("parse_header[%c %c %c %c][%x,%x,%x,%x]\n", data[0], data[1], data[2], data[3],data[0], data[1], data[2], data[3]);
    val = xnrp_strstr(data, (uint8_t *)XNRP_HEADER_START, len);
    if (!val){
        printf_debug("parse_header error\n");
        *err_code = RET_CODE_FORMAT_ERR;
        return false;
    }

    memcpy(header, val, sizeof(struct xnrp_header) - sizeof(header->payload));
    printf_info("find header start=%c%c%c%c\n", header->start_flag[0], header->start_flag[1],
                                                header->start_flag[2], header->start_flag[3]);
    printf_info("header.version=%x\n", header->version);
    printf_info("header.method_code=%x\n", header->method_code);
    printf_info("header.error_code=%x\n", header->error_code);
    printf_info("header.class_code=%x\n", header->class_code);
    printf_info("header.business_code=%x\n", header->business_code);
    printf_info("header.time=%s, %x\n", ctime(&header->time_stamp), header->time_stamp);
    printf_info("header.msg_id=%x\n", header->msg_id);
    printf_info("header.check_sum=%x\n", header->check_sum);
    printf_info("header.payload_len=%x\n", header->payload_len);

    if(header_len + header->payload_len > len){
        *err_code = RET_CODE_FORMAT_ERR;
        printf_err("invalid payload len=%d\n", header->payload_len);
        return false;
    }
    if(header->payload_len > 0){
        *payload = data + header_len;
    }
    else{
        *payload = NULL;
    }
    
    return true;
}

bool xnrp_parse_data(const uint8_t *payload, int *code)
{
    uint8_t i;
    struct xnrp_header *header;
    header = &xnrp_data;
    
    if(payload == NULL){
        return true;
    }
    if(header->payload_len == 0){
        return true;
    }

    printf_debug("payload[%d]:\n", header->payload_len);
    for(i = 0; i< header->payload_len; i++)
        printfd("%x ", payload[i]);
    printfd("\n");

    if(header->payload_len > MAX_RECEIVE_DATA_LEN){
        *code = RET_CODE_PARAMTER_TOO_LONG;
        return false;
    }
/*    
    header->payload = calloc(1, header->payload_len);
    if (!header->payload){
        *code = RET_CODE_INTERNAL_ERR;
        printf_err("calloc failed\n");
        return false;
    }
*/    
    memcpy(header->payload, payload, header->payload_len);

    return true;
}

int xnrp_assamble_response_data(uint8_t **buf,          int err_code)
{
    struct xnrp_header *header, *response_header;
    header = &xnrp_data;
    response_header = &xnrp_response_data;
    int len = 0;
    static uint16_t msg_id_counter = 0;
    
    printf_info("Prepare to assamble response xnrp data\n");

    *buf = response_header;
    memcpy(response_header->start_flag, XNRP_HEADER_START, sizeof(response_header->start_flag)); 
    response_header->version = XNRP_HEADER_VERSION;
    response_header->method_code = METHOD_RESPONSE_COMMAND;
    response_header->error_code = err_code;
    response_header->class_code = header->class_code;
    response_header->business_code = header->business_code;
    memcpy(response_header->client_id, header->client_id, sizeof(response_header->client_id));
    memcpy(response_header->device_id, header->device_id, sizeof(response_header->device_id));
    response_header->time_stamp = time(NULL);
    response_header->msg_id = msg_id_counter++;
    response_header->check_sum = crc16_caculate((uint8_t *)response_header->payload, response_header->payload_len);

    len = sizeof(struct xnrp_header) - sizeof(response_header->payload) + response_header->payload_len;
   
    return len;
}


