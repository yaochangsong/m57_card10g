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
#include <math.h>

#define UART_HANDER_TIMROUT 1000   //ms
#define FPGA_TIME_INTERVAL (1000 * 360)
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
extern void io_set_fpga_sys_time(uint32_t time);
extern int uart_init_dev(char *name, uint32_t baudrate);


enum GGA_PARA_INDEX{
    GGA_ID = 1,
    GGA_UTC_HMS,
    GGA_LATITUDE,
    GGA_NS_STA,
    GGA_LONGITUDE,
    GGA_EW_STA,
    GGA_DATA_STA,
    GGA_SATELLITE,
    GGA_PRECISION,
    GGA_AUTITUDE,     //10
    GGA_AUTI_UNIT,
};

enum RMC_PARA_INDEX{
    RMC_ID = 1,
    RMC_UTC_HMS,
    RMC_DATA_STA,
    RMC_LATITUDE,
    RMC_NS_STA,
    RMC_LONGITUDE,
    RMC_EW_STA,
    RMC_SPEED_GROUND,
    RMC_ANGLE_GROUND,
    RMC_UTC_YMD,        //10
    RMC_ANGLE_MAGNETIC,
};
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
    float   altitude;     /*海拔高度*/
};

struct uloop_timeout *gps_timer = NULL;
struct uloop_timeout *fpga_timer = NULL;

static struct gps_info g_gps_info;
static bool gps_status = false;         /*定位状态*/



int gps_parse_recv_msg_rmc(char *str, size_t nbyte)
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

    if ((ptr1 = strstr(str, GPS_HEADER1)) == NULL && 
        (ptr1 = strstr(str, GPS_HEADER2)) == NULL ) {
        return -1;
    }

    memset(&gps_info, 0, sizeof(struct gps_info));
    ptr2 = strstr(ptr1, GPS_SEPARATOR);
    
    while (ptr2 != NULL) {
    
        n = ptr2 - ptr1;
        memset(para_buf, 0, sizeof(para_buf));
        strncpy(para_buf, ptr1, n);
        param_num++;
        switch (param_num) {
        
            case RMC_UTC_HMS:
            {
                gps_info.utc_hms = atof(para_buf);
                break;
            }
            case RMC_DATA_STA:
            {
                if (NULL == strstr(para_buf, "A")) {
                    gps_status = false;
                    printf_debug("invalid gps data:%s\n", para_buf);
                    return -1;
                } else {
                    gps_status = true;
                }
                break;
            }
            case RMC_LATITUDE:
            {
                gps_info.latitude= atof(para_buf);
                break;
            }
            case RMC_NS_STA:
            {
                if (strstr(para_buf, "N")) {
                    gps_info.ns = 'N';
                } else if (strstr(para_buf, "S")) {
                    gps_info.ns = 'S';
                }
                break;
            }
            case RMC_LONGITUDE:
            {
                gps_info.longitude= atof(para_buf);
                break;
            }
            case RMC_EW_STA:
            {
                if (strstr(para_buf, "E")) {
                    gps_info.ew = 'E';
                } else if (strstr(para_buf, "W")) {
                    gps_info.ew = 'W';
                }
                break;
            }
            case RMC_UTC_YMD:
            {
                gps_info.utc_ymd= atoi(para_buf);
                break;
            }
        }

        ptr1 = ptr2 + 1;
        ptr2 = strstr(ptr1, GPS_SEPARATOR);
        if (param_num >= RMC_UTC_YMD) {
            break;
        }
    }

    if (param_num < RMC_UTC_YMD) {
        printf_info("gps para n=%d, failed!\n", param_num);
        return -1;
    }

    memcpy(&g_gps_info, &gps_info, sizeof(gps_info));
    printf_info("latitude:[%c]%.4f, longitude:[%c]%.4f, utc_time:%d %.3f\n", 
        g_gps_info.ns,g_gps_info.latitude, g_gps_info.ew, g_gps_info.longitude, g_gps_info.utc_ymd, g_gps_info.utc_hms);
    return 0;
}


int gps_parse_recv_msg_gga(char *str, size_t nbyte)
{
    #define GPS_HEADER1      "$GNGGA"
    #define GPS_HEADER2      "$GPGGA"
    #define GPS_SEPARATOR    ","

    char *ptr1 = NULL, *ptr2 = NULL;
    char para_buf[24];
    int param_num = 0, n = 0;
    struct gps_info gps_info;
    
    if(str == NULL || nbyte == 0)
        return -1;

    if ((ptr1 = strstr(str, GPS_HEADER1)) == NULL && 
        (ptr1 = strstr(str, GPS_HEADER2)) == NULL ) {
        return -1;
    }

    memset(&gps_info, 0, sizeof(struct gps_info));
    ptr2 = strstr(ptr1, GPS_SEPARATOR);
    
    while (ptr2 != NULL) {
    
        n = ptr2 - ptr1;
        memset(para_buf, 0, sizeof(para_buf));
        strncpy(para_buf, ptr1, n);
        param_num++;
        switch (param_num) {

            case GGA_DATA_STA:
            {
                if (atoi(para_buf) <= 0 ) {  // 0:无效 1：SPS 2：DGPS
                    gps_status = false;
                    printf_debug("invalid gps data:%s\n", para_buf);
                    return -1;
                } else {
                    gps_status = true;
                }
                break;
            }

            case GGA_AUTITUDE:
            {
                gps_info.altitude = atof(para_buf);
                break;
            }
            
        }

        ptr1 = ptr2 + 1;
        ptr2 = strstr(ptr1, GPS_SEPARATOR);
        if (param_num >= GGA_AUTITUDE) {
            break;
        }
    }

    if (param_num < GGA_AUTITUDE) {
        printf_info("gps para n=%d, failed!\n", param_num);
        return -1;
    }

    g_gps_info.altitude = gps_info.altitude;
    printf_info("altitude:%.1f m\n", g_gps_info.altitude);
    return 0;
}


int gps_get_date_cmdstring(char *cmdbuf)
{
    char tmp_date[128] = {0};
    uint32_t i_hms;
    uint8_t y, mon, d, h, m, s;
    char buf[256];
    
    if (NULL == cmdbuf) {
        printf_note("para buf empty!");
        return -1;
    }

    FILE *fp = popen("date -R | cut -d + -f 2", "r");
    if (fp) {
        memset(buf, 0, sizeof(buf));
        fgets(buf, sizeof(buf)-1, fp);
        buf[strlen(buf)-1] = '\0';
        printf_note("time zone:%s\n", buf);
        if (!strstr(buf, "0800")) {  //如果不是东八区，则设置为东八区
            printf_note("now,set time zone east 8\n");
            setenv("TZ", "UTC-08:00", 1);
        }
        fclose(fp);
    } else {
        printf_warn("get time zone failed!\n");
    }
    
    i_hms = (uint32_t)g_gps_info.utc_hms;
    h = (i_hms / 10000) % 10000 + 8;     //utc
    m = (i_hms / 100) % 100;
    s = i_hms % 100;

    d = (g_gps_info.utc_ymd / 10000) % 10000;
    mon = (g_gps_info.utc_ymd / 100) % 100;
    y = g_gps_info.utc_ymd % 100;
    
    memset(tmp_date, 0, sizeof(tmp_date));
    sprintf(tmp_date, "date -s '20%02d-%02d-%02d %02d:%02d:%02d'", y, mon, d, h, m, s);
    memcpy(cmdbuf, tmp_date, strlen(tmp_date));
    return 0;
}

uint32_t gps_get_utc_hms(void)
{
    uint32_t i_hms;
    uint8_t h, m, s;
    i_hms = (uint32_t)g_gps_info.utc_hms;
    h = (i_hms / 10000) % 10000;   //utc
    m = (i_hms / 100) % 100;
    s = i_hms % 100;
    i_hms = s | (m << 8) | (h << 16);  
    return (uint32_t)i_hms;
}

uint16_t gps_format_days_to_fpga(uint32_t year, uint8_t month, uint8_t day)
{
    int i = 0;
    uint16_t days = 0;
    bool is_leap_year = false;
    
    if ((year % 4 == 0 && year % 100 != 0) || 
        (year % 400 == 0)) {
        is_leap_year = true;
    }

    for (i = 1; i <= 12; i++) {
        if (i < month) {
            switch (i) {
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
        } else {
            days += day;
            break;
        }
    }

    return days;
}
 
uint32_t gps_get_format_date(void)
{
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
    days = gps_format_days_to_fpga((uint32_t)y+2000, M, d);
    //printf_note("format fpga date:y(%d),m(%d),d(%d), days(%d)\n",y,M,d,days);
    
    date_to_fpga = (s & 0x3f) | (m & 0x3f) << 6 | (h & 0x1f) << 12 | (days & 0x1ff) << 17 | (y & 0x3f) << 26;
    //printf_note("date_to_fpga:0x%08x\n",date_to_fpga);
    return date_to_fpga;
}

uint32_t syscall_get_format_date(void)
{
    uint32_t i_hms,i_ymd, date_to_fpga;
    int h, m, s, d, M, y;
    uint16_t days;
    char *ptr = NULL;
    char buf[256];
    
    FILE *stream = popen("date '+%Y-%m-%d-%H:%M:%S'", "r");
    if (stream) {
        memset(buf, 0, sizeof(buf));
        fgets(buf, sizeof(buf)-1, stream);
        //printf_note("syscall date:%s\n", buf);
        y = atoi(buf);
        ptr = strstr(buf, "-");
        M = atoi(ptr+1);
        ptr = strstr(ptr+1, "-");
        d = atoi(ptr+1);

        ptr = strstr(ptr+1, "-");
        h = atoi(ptr+1);
        ptr = strstr(ptr+1, ":");
        m = atoi(ptr+1);
        ptr = strstr(ptr+1, ":");
        s = atoi(ptr+1);
        printf_note("y-m-s h:m:s:%04d-%02d-%02d %02d:%02d:%02d\n", y,M,d,h,m,s);
        fclose(stream);
    } else {
        printf_err("syscall failed!\n");
        return 0;
    }

    days = gps_format_days_to_fpga(y, M, d);
    //printf_note("format fpga date:y(%d),m(%d),d(%d), days(%d)\n",y,M,d,days);
    
    date_to_fpga = (s & 0x3f) | (m & 0x3f) << 6 | (h & 0x1f) << 12 | (days & 0x1ff) << 17 | (y & 0x3f) << 26;
    //printf_note("date_to_fpga:0x%08x\n",date_to_fpga);
    return date_to_fpga;
}


int gps_get_latitude(void)
{
    int32_t latitude;
    float flatitude;
    flatitude = g_gps_info.latitude;
    latitude = (int32_t)(((int32_t)(flatitude/100) + fmod(flatitude,100)/60)*1000000);
    if (g_gps_info.ns == 'S') {
        latitude *= -1;   //北纬为正，南纬为负
    }
    return latitude;
}

int gps_get_longitude(void)
{
    int32_t longitude;
    float flongitude;
    flongitude = g_gps_info.longitude;
    longitude = (int32_t)(((int32_t)(flongitude/100) + fmod(flongitude,100)/60)*1000000);
    if (g_gps_info.ew == 'W') {
        longitude *= -1;   //东经为正，西经为负
    }
    return longitude;
}

int gps_get_altitude(void)
{
    int32_t altitude;
    float faltitude;
    faltitude = g_gps_info.altitude;
    altitude = (int32_t)(faltitude * 1000);
    return altitude;
}


bool gps_location_is_valid(void)
{
    return gps_status;
}



void gps_timer_task_cb(struct uloop_timeout *t)
{
    #define TIME_INTERVAL_SEC   (300)
    char buf[SERIAL_BUF_LEN];
    char cmdbuf[128];
    int32_t  nread; 
    static uint8_t timed_flag = 0;   //授时完成标志
    static int tick = 0;
    memset(buf,0,SERIAL_BUF_LEN);
    tick++;
    struct uart_info *gps_uart = get_uart_info_by_index(UART_INFO_GPS);
    if (!gps_uart)
        return;
    int fd = gps_uart->sfd;
    nread = read(fd, buf, SERIAL_BUF_LEN);
    if(nread > 0) {
        if (0 == gps_parse_recv_msg_rmc(buf, nread)) {
            if(timed_flag == 0) {
                io_set_fpga_sys_time(gps_get_format_date());  //fpga
                memset(cmdbuf, 0, sizeof(cmdbuf));
                gps_get_date_cmdstring(cmdbuf);
                printf_note("set sys time:%s\n", cmdbuf);
                safe_system(cmdbuf);
                safe_system("hwclock -w");
                timed_flag = 1;
            } else if (tick >= TIME_INTERVAL_SEC) {
                memset(cmdbuf, 0, sizeof(cmdbuf));
                gps_get_date_cmdstring(cmdbuf);
                printf_note("set sys time:%s\n", cmdbuf);
                safe_system(cmdbuf);
                safe_system("hwclock -w");
                tick = 0;
            }
        } else {
            if((nread < 0) && (EAGAIN != errno) && (EINTR != errno)) {
                printf_err("gps fd err!\r\n");
                close(fd);
                usleep(100);
                gps_uart->sfd = uart_init_dev(gps_uart->devname, gps_uart->baudrate);
            }
        }
        gps_parse_recv_msg_gga(buf, nread);
    } 
    
    tcflush(fd, TCIOFLUSH);
    uloop_timeout_set(t, UART_HANDER_TIMROUT);
}

int gps_task_init(void)
{
    if (!gps_timer) {
        gps_timer = (struct uloop_timeout *)malloc(sizeof(struct uloop_timeout));
        if (!gps_timer) {
            printf_err("create gps task failed!\n");
            return -1;
        }
    }

    gps_timer->cb = gps_timer_task_cb;
    gps_timer->pending = false;
    uloop_timeout_set(gps_timer, UART_HANDER_TIMROUT);
    printf_note("create gps task ok!\n");
    return 0;
}


//将系统时间设置到fpga
void fpga_timer_task_cb(struct uloop_timeout *t)
{
    uint32_t date = syscall_get_format_date();
    if (date > 0) {
        io_set_fpga_sys_time(date);
    }
    uloop_timeout_set(t, FPGA_TIME_INTERVAL);
}

int time_fpga_task_init(void)
{
    if (!fpga_timer) {
        fpga_timer = (struct uloop_timeout *)malloc(sizeof(struct uloop_timeout));
        if (!fpga_timer) {
            printf_err("create fpga timer task failed!\n");
            return -1;
        }
    }

    fpga_timer->cb = fpga_timer_task_cb;
    fpga_timer->pending = false;
    uloop_timeout_set(fpga_timer, FPGA_TIME_INTERVAL);
    printf_note("create fpga task ok!\n");
    return 0;
}

int gps_init(void)
{
    memset(&g_gps_info, 0, sizeof(struct gps_info));
    g_gps_info.latitude = 400000000;
    g_gps_info.longitude = 400000000;
    gps_task_init();
    time_fpga_task_init();
	return 0;
}


