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
    DC_MODE_WFM = 0x06,
    DC_MODE_ISB = 0x07,
    DC_MODE_PM = 0x08,
}_DECODE_ID;


extern int parse_json_client_net(int ch, char * body,const char *type);
extern int parse_json_net(const char * const body);
extern int parse_json_multi_band(const char * const body,uint8_t cid);
extern int parse_json_muti_point(const char * const body,uint8_t cid);
extern int parse_json_demodulation(const char * const body,uint8_t cid,uint8_t subid );
extern int parse_json_bddc(const char * const body,uint8_t ch);
extern int parse_json_if_multi_value(const char * const body, uint8_t cid);
extern int parse_json_rf_multi_value(const char * const body, uint8_t cid);
extern int parse_json_batch_delete(const char * const body);
extern char *assemble_json_file_list(void);
extern char *assemble_json_file_dir(const char * dirpath);
extern char *assemble_json_file_size(const char *path);
extern char *assemble_json_find_rec_file(const char *path, const char *filename);
extern char *assemble_json_data_response(int err_code, const char *message, const char * const data);
extern char *assemble_json_response(int err_code, const char *message);
extern char *assemble_json_find_file(const char *path, const char *filename);
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
extern char *assemble_json_selfcheck_info(void);
extern char *assemble_json_netlist_info(void);
extern char *assemble_json_statistics_all_info(void);
#endif

