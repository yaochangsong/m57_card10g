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
*  Rev 2.0   2020-05-15 Friday  wangzhiqiang
*  修改：
*    修改gps解析	
******************************************************************************/
#include "config.h"

/*
    $GNGGA:GPGGA(时间、定位质量)
    $GPGGA,<1>,<2>,<3>,<4>,<5>,<6>,<7>,<8>,<9>,M,<10>,M,<11>,<12>xx
    $GNGGA,060633.000,3119.3559,N,12135.9948,E,1,22,0.6,51.2,M,0.0,M,,*4A
    <1> UTC时间，格式为hhmmss.sss；
    <2> 纬度，格式为ddmm.mmmm(第一位是零也将传送)；
    <3> 纬度半球，N或S(北纬或南纬)
    <4> 经度，格式为dddmm.mmmm(第一位零也将传送)；
    <5> 经度半球，E或W(东经或西经)
    <6> GPS状态， 0初始化， 1单点定位， 2码差分， 3无效PPS， 4固定解， 5浮点解， 6正在估算 7，人工输入固定值， 8模拟模式， 9WAAS差分
    <7> 使用卫星数量，从00到12(第一个零也将传送)
    <8> HDOP-水平精度因子，0.5到99.9，一般认为HDOP越小，质量越好。
    <9> 海拔高度，-9999.9到9999.9米
    M 指单位米
    <10> 大地水准面高度异常差值，-9999.9到9999.9米
    M 指单位米
    <11> 差分GPS数据期限(RTCM SC-104)，最后设立RTCM传送的秒数量，如不是差分定位则为空
    <12> 差分参考基站标号，从0000到1023(首位0也将传送)。
    * 语句结束标志符
    xx 从$开始到之间的所有ASCII码的异或校验
*/

#if 0
struct gps_gngga_info{
    float utc_hms;
    float latitude;
    float longitude;
    int status;
    int num_satellites_used;
    float hdop;
    float altitude;
    float ghad;
    float duration;
    int station;
    char  ns;
    char  ew;
};


static struct gps_gngga_info g_gps_info;

int gps_parse_txt(char *str, size_t nbyte)
{
    #define GPS_HEADER      "$GNGGA"
    #define GPS_SEPARATOR   ","
    char *str_cpy = NULL,*saveptr = NULL, *cur_ptr=NULL;
    char* str_end;
    int param_num = 0, ret = 0;
    struct gps_gngga_info gps_info;
    
    if(str == NULL || nbyte == 0)
        return -1;
    str_cpy = strdup(str);
    printf_note("GPS text: %s\n", str_cpy);
    memset(&gps_info, 0, sizeof(struct gps_gngga_info));
    
    cur_ptr = strtok_r( str_cpy, GPS_SEPARATOR, &saveptr );
    printf_note("header: %s\n", cur_ptr);
    if(memcmp(GPS_HEADER, cur_ptr, sizeof(GPS_HEADER) != 0)){
        ret = -1;
        goto exit;
    }
    while (cur_ptr != NULL){
        cur_ptr = strtok_r( NULL, GPS_SEPARATOR, &saveptr );
        param_num ++;
        switch(param_num){
            case 1:
            {
                gps_info.utc_hms = strtof(cur_ptr, NULL);
                printf_note("utc hms: %f\n", gps_info.utc_hms);
                break;
            }
            case 2:
            {
                gps_info.latitude= strtof(cur_ptr, NULL);
                printf_note("latitude: %.4f\n", gps_info.latitude);
                break;
            }
            case 4:
            {
                gps_info.longitude= strtof(cur_ptr, NULL);
                printf_note("longitude: %.4f\n", gps_info.longitude);
                break;
            }
            case 6:
            {
                gps_info.status = (int) strtol(cur_ptr, &str_end, 10);
                printf_note("status: %d\n", gps_info.status);
                break;
            }
            case 7:
            {
                gps_info.num_satellites_used = (int) strtol(cur_ptr, &str_end, 10);
                printf_note("num_satellites_used: %d\n", gps_info.num_satellites_used);
                break;
            }
            case 9:
            {
                gps_info.altitude = strtof(cur_ptr, NULL);
                printf_note("altitude: %.4f\n", gps_info.altitude);
                break;
            }
        }
        
    }
    memcpy(&g_gps_info, &gps_info, sizeof(gps_info));
exit:
    safe_free(str_cpy);
    return ret;
}

uint32_t gps_get_utc_hms(void)
{
    /* get time for fpga */
    uint32_t i_hms;
    uint8_t h,m,s;
    i_hms = (uint32_t)g_gps_info.utc_hms;
    h = (i_hms/10000)%10000;
    m = (i_hms /100)%100;
    s = i_hms %100;
    i_hms = s|(m<<8)|(h<<16);
    return (uint32_t)i_hms;
}

int gps_get_latitude(void)
{
    int32_t latitude;
    float flatitude;
    flatitude = g_gps_info.latitude;
    latitude = (int32_t)(((int32_t)(flatitude/100) + fmod(flatitude,100)/60)*1000000);
    return latitude;
}

int gps_get_longitude(void)
{
    int32_t longitude;
    float flongitude;
    flongitude = g_gps_info.longitude;
    longitude = (int32_t)(((int32_t)(flongitude/100) + fmod(flongitude,100)/60)*1000000);
    return longitude;
}

int gps_init(void)
{
    memset(&g_gps_info, 0, sizeof(struct gps_gngga_info));
}
#endif


/*	
	GPS解析：GGA语句和RMC语句
*/


struct gps_info {
	float utc_hms;		/*utc时间*/	
	int utc_ymd;		
    float latitude;		/*纬度*/
    float longitude;	/*经度*/
	char  ns;			/*南北指示*/
    char  ew;			/*东西指示*/
};

static struct gps_info g_gps_info;
static bool gps_status = false;  /*定位状态*/


int gps_parse_recv_msg(char *str, size_t nbyte)
{
    #define GPS_HEADER1      "$GNRMC"
	#define GPS_HEADER2      "$GPRMC"
    #define GPS_SEPARATOR    ","
	
    char *ptr1 = NULL, *ptr2 = NULL;
	char para_buf[24];
    int param_num = 0, n = 0;
    struct gps_info gps_info;
    
    if(str == NULL || nbyte == 0)
        return -1;
	
	if( (ptr1 = strstr(str, GPS_HEADER1)) == NULL && (ptr1 = strstr(str, GPS_HEADER2)) == NULL )
	{
		return -1;
	}
	
    memset(&gps_info, 0, sizeof(struct gps_info));
	ptr2 = strstr(ptr1, GPS_SEPARATOR);	
    while (ptr2 != NULL)
	{
        n = ptr2 - ptr1;
		memset(para_buf, 0, sizeof(para_buf));
		strncpy(para_buf, ptr1, n);
		param_num++;
		switch(param_num)
		{
            case 2:
            {
                gps_info.utc_hms = atof(para_buf);
                break;
            }
			case 3:
			{
				if(NULL == strstr(para_buf, "A"))
				{
				    gps_status = false;
					printf_debug("invalid gps data:%s\n", para_buf);
					return -1;
				}
				else
				{
				    gps_status = true;
				}
				break;
			}
			case 4:
			{
				gps_info.latitude= atof(para_buf);
				break;
			}
			case 5:
			{
				if(strstr(para_buf, "N"))
				{
					gps_info.ns = 'N';
				}
				else if(strstr(para_buf, "S"))
				{
					gps_info.ns = 'S';
				}
				break;
			}
			case 6:
			{
				gps_info.longitude= atof(para_buf);
				break;
			}
			case 7:
			{
				if(strstr(para_buf, "E"))
				{
					gps_info.ew = 'E';
				}
				else if(strstr(para_buf, "W"))
				{
					gps_info.ew = 'W';
				}
				break;
			}
			case 10:
			{
				gps_info.utc_ymd= atoi(para_buf);
				break;
			}
        }
	
		ptr1 = ptr2 + 1;
		ptr2 = strstr(ptr1, GPS_SEPARATOR);	
		
        if(param_num >= 10)
		{
			break;
		}
    }
	
	if(param_num < 10)
	{
		printf_note("gps para n=%d, failed!\n", param_num);
		return -1;
	}

    memcpy(&g_gps_info, &gps_info, sizeof(gps_info));
    printf_note("latitude:[%c]%.4f, longitude:[%c]%.4f, utc_time:%d %.3f\n", g_gps_info.ns,g_gps_info.latitude, 
														g_gps_info.ew, g_gps_info.longitude, g_gps_info.utc_ymd, g_gps_info.utc_hms);
    return 0;
}

int gps_get_date_cmdstring(char *cmdbuf)
{
	char tmp_date[128] = {0};
	uint32_t i_hms;
    uint8_t y, mon, d, h, m, s;
   	
	if(NULL == cmdbuf)
	{
		printf_note("para buf empty!");
		return -1;
	}
	
	i_hms = (uint32_t)g_gps_info.utc_hms;
    h = (i_hms / 10000) % 10000 + 8;     //utc
    m = (i_hms / 100) % 100;
    s = i_hms % 100;

    d = (g_gps_info.utc_ymd / 10000) % 10000;
    mon = (g_gps_info.utc_ymd / 100) % 100;
    y = g_gps_info.utc_ymd % 100;
    
	memset(tmp_date, 0, sizeof(tmp_date));
	sprintf(tmp_date, "date -s '20%02d-%02d-%02d %02d:%02d:%02d'> /dev/null &", y, mon, d, h, m, s);
	
	memcpy(cmdbuf, tmp_date, strlen(tmp_date));
	return 0;
}

uint32_t gps_get_utc_hms(void)
{
    /* get time for fpga */
    uint32_t i_hms;
    uint8_t h, m, s;
    i_hms = (uint32_t)g_gps_info.utc_hms;
    h = (i_hms / 10000) % 10000 + 8;   //utc
    m = (i_hms / 100) % 100;
    s = i_hms % 100;
    i_hms = s | (m << 8) | (h << 16);  
    return (uint32_t)i_hms;
}

uint16_t gps_format_days_to_fpga(uint8_t year, uint8_t month, uint8_t day)
{
    int i = 0;
    uint16_t days = 0;
    uint32_t real_year = year + 2000;
    bool is_leap_year = false;
    if((real_year % 4 == 0 && real_year % 100 != 0) || (real_year % 400 == 0))
    {
        is_leap_year = true;
    }

    for(i = 1; i <= 12; i++)
    {
        if(i < month)
        {
            switch(i)
            {
                case 1:
                case 3:
                case 5:
                case 7:
                case 8:
                case 10:
                case 12:
                    days += 31;
                    break;
                case 2:
                    if(is_leap_year)
                       days += 29;
                    else
                       days += 28;
                    break;
                case 4:
                case 6:
                case 9:
                case 11:
                   days += 30;
                    break;
            }
        }
        else
        {
            days += day;
            break;
        }
    }

    return days;
}

uint32_t gps_get_format_date(void)
{
    /* get time for fpga */
    uint32_t i_hms,i_ymd, date_to_fpga;
    uint8_t h, m, s, d, M, y;
    uint16_t days;
    i_hms = (uint32_t)g_gps_info.utc_hms;
    i_ymd = (uint32_t)g_gps_info.utc_ymd;
    h = (i_hms / 10000) % 10000 + 8;   //utc
    m = (i_hms / 100) % 100;
    s = i_hms % 100;
    
    d = (i_ymd / 10000) % 10000;
    M = (i_ymd / 100) % 100;
    y = i_ymd % 100;
    days = gps_format_days_to_fpga(y, M, d);
    printf_note("format fpga date:y(%d),m(%d),d(%d), days(%d)\n",y,M,d,days);
    
    date_to_fpga = (s & 0x3f) | (m & 0x3f) << 6 | (h & 0x3f) << 12 | (days & 0x1ff) << 18 | (y & 0x1f) << 27;
    printf_note("date_to_fpga:0x%08x\n",date_to_fpga);
    return date_to_fpga;
}


int gps_get_latitude(void)
{
    int32_t latitude;
    float flatitude;
    flatitude = g_gps_info.latitude;
    latitude = (int32_t)(((int32_t)(flatitude/100) + fmod(flatitude,100)/60)*1000000);  
    return latitude;
}

int gps_get_longitude(void)
{
    int32_t longitude;
    float flongitude;
    flongitude = g_gps_info.longitude;
    longitude = (int32_t)(((int32_t)(flongitude/100) + fmod(flongitude,100)/60)*1000000);
    return longitude;
}

bool gps_location_is_valid(void)
{
    return gps_status;
}

int gps_init(void)
{
    memset(&g_gps_info, 0, sizeof(struct gps_info));
}


