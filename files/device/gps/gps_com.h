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


extern bool gps_location_is_valid(void);
extern int gps_get_latitude(void);
extern int gps_get_longitude(void);
extern int gps_get_altitude(void);

extern int gps_init(void);

#endif
