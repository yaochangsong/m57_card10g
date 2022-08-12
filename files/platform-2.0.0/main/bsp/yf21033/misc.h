/******************************************************************************
*  Copyright 2020, Showay Technology Dev Co.,Ltd.
*  ---------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Showay Technology Dev Co.,Ltd. (C) 2019
******************************************************************************/
/*****************************************************************************     
*  Rev 1.0   8 Dec 2020    yaochangsong
*  Initial revision.
******************************************************************************/
#ifndef _CTRL_H
#define _CTRL_H

#define get_rf_status_code(is_ok)   (is_ok == true ? 5 : 4)
#define get_adc_status_code(is_ok)  (is_ok == true ? 5 : 4)
#define get_gps_status_code(is_ok)  (is_ok == true ? 7 : 4)
#define get_disk_code(is_ok, args)  (is_ok == true ? 7 : 4)

extern const struct misc_ops * misc_create_ctx(void);


#endif
