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


struct gps_info {
    float utc_hms;		/*utc时间*/	
    int utc_ymd;		
    float latitude;		/*纬度*/
    float longitude;	/*经度*/
    char  ns;			/*南北指示*/
    char  ew;			/*东西指示*/
    float altitude;     /*海拔高度 米*/
    float horizontal_speed;  //水平速度，单位节
    float vertical_speed; //垂直速度 单位千米每小时
    int location_type;  //定位类型 0：无效定位 1：SPS模式 2：DGPS模式
    float true_north_angle; //真北方位角 
    float magnetic_angle;   //磁北方位角  
};
extern bool gps_location_is_valid(void);
extern int gps_get_latitude(void);
extern int gps_get_longitude(void);
extern int gps_get_altitude(void);
extern struct gps_info *gps_get_info(void);
extern int gps_init(void);

#endif
