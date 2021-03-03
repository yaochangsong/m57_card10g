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
*  Rev 1.0   29 Feb 2020   yaochangsong
*  Initial revision.
******************************************************************************/
#ifndef _PARSE_JSON_H
#define _PARSE_JSON_H

typedef enum _XW_DECODE_ID{
    DC_MODE_IQ = 0x00,
    DC_MODE_AM = 0x01,
    DC_MODE_FM = 0x02,
    DC_MODE_CW = 0x03,
    DC_MODE_LSB = 0x04,
    DC_MODE_USB = 0x05,
}_DECODE_ID;


extern int parse_json_client_net(int ch, const char * const body, char *type);
extern int parse_json_net(const char * const body);
extern int parse_json_multi_band(const char * const body,uint8_t cid);
extern int parse_json_muti_point(const char * const body,uint8_t cid);
extern int parse_json_demodulation(const char * const body,uint8_t cid,uint8_t subid );
extern int parse_json_file_backtrace(const char * const body, uint8_t ch,  uint8_t enable, char  *filename);
extern int parse_json_file_store(const char * const body, uint8_t ch,  uint8_t enable, char  *filename);
extern int parse_json_if_multi_value(const char * const body, uint8_t cid);
extern int parse_json_rf_multi_value(const char * const body, uint8_t cid);
extern char *assemble_json_file_list(void);
extern char *assemble_json_data_response(int err_code, const char *message, const char * const data);
extern char *assemble_json_response(int err_code, const char *message);
extern char *assemble_json_find_file(char *filename);
extern char *assemble_json_softversion(void);
extern char *assemble_json_fpag_info(void);
extern char *assemble_json_temp_info(void);
extern char *assemble_json_gps_info(void);
extern char *assemble_json_clock_info(void);
extern char *assemble_json_board_info(void);
extern char *assemble_json_net_info(void);
extern char *assemble_json_rf_info(void);
extern char *assemble_json_disk_info(void);
extern char *assemble_json_all_info(void);

#endif

