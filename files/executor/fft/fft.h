


#ifndef __FFT_H
#define __FFT_H
#include "config.h"


#define  W  (200)    //wav trans txt 数组大小
#define  MM_PI        (3.14159265358979323846264338327950288419716939937510)       // 圆周率atan(1)* 4
//#define  N  256
static unsigned int N=16*1024*1024;
#define  SIGNALNUM        (20000)//输入信号数
#define  THRESHOLD         (8)//门限
#define  CORRECTIONSIGNAL  (300)

#define BOTTOM_CORRECT  562//没有信号时候的底噪校正值
#define   SMOOTHPOINT_BOTTOM        (64)//滑动平均滤波计算平均值时所取的点数
#define   TRANNUM     (1024)      //每次传1K的点
#define  FILTER_A      (0.01)
static unsigned int INTERVALNUM=10000;
static unsigned int SHIELDPOINTS=20000;
static unsigned int  INTERVALNUMTOW=2;
static unsigned int  firstfftlen=8*1024;
static unsigned int  narrowbandlen=128*1024;
static uint32_t  SMOOTHPOINT=32;






typedef struct {
    int cenfrepointnum;
    int y[SIGNALNUM];
    int z[SIGNALNUM];
    int centfeqpoint[SIGNALNUM];      //上报中心频点，中心频点的集合
    // int maxcentfeqpoint;
    float Threshold;
    int Centerpoint;//取出的唯一一个中心频率点
    float Bottomnoise;       
    float maxcentfrequency;
    int interval;	
    int bandwidth[SIGNALNUM];      //带宽
    float arvcentfreq[SIGNALNUM];  //中心频点功率值
    int firstpoint[SIGNALNUM];
    int endpoint[SIGNALNUM];
     int  maximum_x;
     int x_right[SIGNALNUM];
     int x_left[SIGNALNUM];
}fft_state;

extern fft_state fftstate;
typedef struct {
   int signalsnumber;//找到的信号个数
   int centfeqpoint[SIGNALNUM];  //上报中心频点，中心频点的集合 
   int bandwidth[SIGNALNUM];     //带宽
   float arvcentfreq[SIGNALNUM]; //中心频点功率值
   int  maximum_x;
   float Bottomnoise; 
}fft_result;

typedef enum _signalnum_flag{
    SIGNALNUM_NORMAL        = 0x01,
    SIGNALNUM_ABNORMAL      = 0x02,
}signalnum_flag;    

void fft_init(void);
void fft_exit(void);
float *fft_get_data(uint32_t *len);
fft_result *fft_get_result(void);
void fft_fftw_calculate(short *iqdata,int32_t fftsize,int datalen,float *mozhi);
void fft_fftw_calculate_hann(short *iqdata,int32_t fftsize,int datalen,float *mozhi);
void fft_iqdata_handle(int bd,short *data,int fftsize, int datalen,uint32_t midpoint,uint32_t bigbw,uint32_t littlebw);
/********************************************************************************

函数功能：获取找到的信号的信息
输入：
        int threshold    门限
        float *fuzzydata    粗检频谱数据
        uint32_t fuzzylen    粗检频谱数据长度
        float *bigdata    细检频谱数据
        uint32_t biglen   细检频谱数据长度
        uint32_t midpointhz    距离带宽中心的距离（单位：hz）
        uint32_t signal_bw   要检测信号的带宽
        uint32_t total_bw    总带宽
        uint32_t multiple     细检原始fftsize除以粗检原始fftsize
        
返回值：
      -1 异常
      0 正常

*********************************************************************************/

int  fft_fftdata_handle(int threshold,float *fuzzydata,uint32_t fuzzylen,float *bigdata,uint32_t biglen,
                             uint32_t midpointhz,uint32_t signal_bw,uint32_t total_bw,uint32_t multiple);
/********************************************************************************

函数功能：修改平滑次数
输入：
    uint32_t smoothcount :想要设置的平滑次数的值；

        
返回值：
     无
*********************************************************************************/


void fft_set_smoothcount(uint32_t smoothcount);



/********************************************************************************

函数功能：当没有信号的求底噪值
输入：
      float *data：去边带后的fft数据长度
      uint32 datalen；去边带后的fft数据长度
      
返回值：
      计算后的底噪值

*********************************************************************************/

float fft_get_bottom(float *data,uint32_t datalen);

void fft_fftw_calculate_hann_addsmooth(short *iqdata,int32_t fftsize,int datalen,float *smoothdata);
extern void fft_fftw_calculate_hann_addsmooth_ex(short *iqdata,int32_t fftsize,int datalen,float *smoothdata);

#endif










