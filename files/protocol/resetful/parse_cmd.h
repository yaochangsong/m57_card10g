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
*  Rev 1.0   16 Feb 2020   yaochangsong
*  Initial revision.
******************************************************************************/

#ifndef _CMD_HTTP_RESTFUL_
#define _CMD_HTTP_RESTFUL_

#define RESP_CODE_OK                     0
#define RESP_CODE_UNKNOWN_OPS_MODE      -1
#define RESP_CODE_PARSE_ERR             -2
#define RESP_CODE_INTERNAL_ERR          -3
#define RESP_CODE_INVALID_PARAM         -4
#define RESP_CODE_CHANNEL_ERR           -5
#define RESP_CODE_BUSY                  -6
#define RESP_CODE_FILE_NOT_EXIST        -7
#define RESP_CODE_FILE_ALREADY_EXIST    -8
#define RESP_CODE_DISK_FORMAT_ERR       -9
#define RESP_CODE_DISK_DETECTED_ERR     -10
#define RESP_CODE_PATH_PARAM_ERR        -11
#define RESP_CODE_EXECMD_ERR            -12


extern int cmd_muti_point(struct uh_client *cl, void **arg, void **content);
extern int cmd_multi_band(struct uh_client *cl, void **arg, void **content);
extern int cmd_demodulation(struct uh_client *cl, void **arg, void **content);
extern int cmd_if_single_value_set(struct uh_client *cl, void **arg, void **content);
extern int cmd_rf_single_value_set(struct uh_client *cl, void **arg, void **content);
extern int cmd_if_multi_value_set(struct uh_client *cl, void **arg, void **content);
extern int cmd_rf_multi_value_set(struct uh_client *cl, void **arg, void **content);
extern int cmd_disk_cmd(struct uh_client *cl, void **arg, void **content);
extern int cmd_ch_enable_set(struct uh_client *cl, void **arg, void **content);
extern int cmd_subch_enable_set(struct uh_client *cl, void **arg, void **content);
extern int cmd_file_store(struct uh_client *cl, void **arg, void **content);
extern int cmd_file_download(struct uh_client *cl, void **arg, void **content);
extern int cmd_file_delete(struct uh_client *cl, void **arg, void **content);
extern int cmd_file_backtrace(struct uh_client *cl, void **arg, void **content);
extern int cmd_file_list(struct uh_client *cl, void **arg, void **content);
extern int cmd_file_find(struct uh_client *cl, void **arg, void **content);
extern int cmd_disk_format(struct uh_client *cl, void **arg, void **content);
extern int cmd_ping(struct uh_client *cl, void **arg, void **content);
extern int cmd_netset(struct uh_client *cl, void **arg, void **content);
extern int cmd_net_client(struct uh_client *cl, void **arg, void **content);
extern int cmd_net_client_type(struct uh_client *cl, void **arg, void **content);
extern int cmd_get_softversion(struct uh_client *cl, void **arg, void **content);
extern int cmd_get_fpga_info(struct uh_client *cl, void **arg, void **content);
extern int cmd_get_all_info(struct uh_client *cl, void **arg, void **content);
extern int cmd_bddc(struct uh_client *cl, void **arg, void **content);

#endif
