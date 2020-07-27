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
*  Rev 1.0   07 March 2020   yaochangsong
*  Initial revision.
******************************************************************************/
#ifndef _GPS_COM_H
#define _GPS_COM_H

#if 0
extern int gps_parse_txt(char *str, size_t nbyte);
extern uint32_t gps_get_utc_hms(void);
#endif

extern int gps_parse_recv_msg(char *str, size_t nbyte);
extern uint32_t gps_get_utc_hms(void);
extern int gps_get_date_cmdstring(char *cmdbuf);
extern uint32_t gps_get_format_date(void);
extern bool gps_location_is_valid(void);

#endif
