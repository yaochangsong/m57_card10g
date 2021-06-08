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
    float utc_hms;		/*utcʱ��*/	
    int utc_ymd;		
    float latitude;		/*γ��*/
    float longitude;	/*����*/
    char  ns;			/*�ϱ�ָʾ*/
    char  ew;			/*����ָʾ*/
    float altitude;     /*���θ߶� ��*/
    float horizontal_speed;  //ˮƽ�ٶȣ���λ��
    float vertical_speed; //��ֱ�ٶ� ��λǧ��ÿСʱ
    int location_type;  //��λ���� 0����Ч��λ 1��SPSģʽ 2��DGPSģʽ
    float true_north_angle; //�汱��λ�� 
    float magnetic_angle;   //�ű���λ��  
};
extern bool gps_location_is_valid(void);
extern int gps_get_latitude(void);
extern int gps_get_longitude(void);
extern int gps_get_altitude(void);
extern struct gps_info *gps_get_info(void);
extern int gps_init(void);

#endif
