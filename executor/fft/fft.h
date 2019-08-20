

#ifndef __FFT_H
#define __FFT_H
#include "config.h"


#define  W  (200)    //wav trans txt 数组大小
#define  MM_PI        (3.14159265358979323846264338327950288419716939937510)       // 圆周率atan(1)* 4
//#define  N  256
static unsigned int N=1024*1024;
#define  SIGNALNUM        (20000)//输入信号数
#define  THRESHOLD         (135)//128.999176/门限
#define  CORRECTIONSIGNAL  (300)


#define   SMOOTHPOINT        (128)//滑动平均滤波计算平均值时所取的点数
#define   TRANNUM     (1024)      //每次传1K的点
#define  FILTER_A      (0.01)
static unsigned int INTERVALNUM=10000;
static unsigned int SHIELDPOINTS=20000;
static unsigned int  INTERVALNUMTOW=2;
static unsigned int  firstfftlen=8192;


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
}fft_state;

extern fft_state fftstate;
typedef struct {
   int signalsnumber;//找到的信号个数
   int centfeqpoint[SIGNALNUM];  //上报中心频点，中心频点的集合 
   int bandwidth[SIGNALNUM];     //带宽
   float arvcentfreq[SIGNALNUM]; //中心频点功率值
   int  maximum_x;
}fft_result;

typedef enum _signalnum_flag{
    SIGNALNUM_NORMAL        = 0x01,
    SIGNALNUM_ABNORMAL      = 0x02,
}signalnum_flag;    

void fft_init(void);
void fft_exit(void);
void fft_iqdata_handle(int bd,short *data,int fftsize ,int datalen);
float *fft_get_data(uint32_t *len);
fft_result *fft_get_result(void);
void fft_fftw_calculate(short *iqdata,int32_t fftsize,int datalen,float *mozhi);



void xulitest(void);



#endif




