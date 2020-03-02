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
extern int cmd_muti_point(struct uh_client *cl, void **arg);
extern int cmd_multi_band(struct uh_client *cl, void **arg);
extern int cmd_demodulation(struct uh_client *cl, void **arg);
extern int cmd_if_single_value_set(struct uh_client *cl, void **arg);
extern int cmd_rf_single_value_set(struct uh_client *cl, void **arg);
extern int cmd_if_multi_value_set(struct uh_client *cl, void **arg);
extern int cmd_rf_multi_value_set(struct uh_client *cl, void **arg);
extern int cmd_disk_cmd(struct uh_client *cl, void **arg);
extern int cmd_ch_enable_set(struct uh_client *cl, void **arg);
extern int cmd_subch_enable_set(struct uh_client *cl, void **arg);

#endif
