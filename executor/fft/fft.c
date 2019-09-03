#include "config.h"
#include <stdlib.h>
#include "fft.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <math.h>




#define SAFE_FREE(p)  do{     \
 			if(p != NULL){    \
				free(p);       \
				p=NULL;       \
			}                 \
		}while(0)

typedef enum Mode
{
	UPLOADCENTERFREQUENCY=0,
	UPLOADBOTTOMNOISE=1
}Mode;


typedef enum Datatype
{
	IQDATA=0,
	FFTDATA=1
}Datatype;


typedef double        mathDouble;

/*定义复数类型*/
typedef struct _ReDefcomplex
{
	float    Real;
	float    Imag;
}complexType;

typedef struct _ReDefTranData
{
	int   centfeqpoint;
	int     bandwidth;
	float   arvcentfreq;
}ComplexDataType;

struct fft_data{

    float *mozhi;
    //fftwf_complex *x;
   // fftwf_complex *y;
    float *smoothdata;
    short *wavdata;
   // float *hann;
	
};
struct fft_data fftdata;

/*struct fft_state{
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
     int bandwidth[SIGNALNUM];      ////
     float arvcentfreq[SIGNALNUM];
};
struct fft_state fftstate;*/



fft_state fftstate;
fft_result fftresult;



/*static int cenfrepointnum=0;
static int y[SIGNALNUM];
static int z[SIGNALNUM];
static float centfrequenty[SIGNALNUM];     // 上报中心频率
static int bandwidth[SIGNALNUM];			//上报带宽
static float arvcentfreq[SIGNALNUM];
static int   centfeqpoint[SIGNALNUM];      //上报中心频点
//int maxcentfeqpoint;
static float Threshold=0;
static int Centerpoint=0;
static float Bottomnoise=0;
static float maxcentfrequency=0;
static int interval=0;*/
/*
*函数名：is_power_of_two
*说明：判定一个数是否是2的N次幂
*输入：
*输出：2的N次幂返回true
*/
bool is_power_of_two(int  num)
{
	if (num < 2)
		return false;
	if (num == 2)
		return true;

	return (num&(num - 1)) == 0;
}


/*
* 函数名：Cfft
* 说  明：FFT Algorithm
* === Inputs ===
* inVec:   complex numbers
* vecLen : nodes of FFT. @N should be power of 2, that is 2^(*)
* === Output ===
* outVec: 独立输出,不影响输入
* 其他：函数返回值false表示非法参数
*/
//bool Cfft(complexType  const inVec[], int  const vecLen, complexType  outVec[])           //fft

bool Cfft(complexType  *inVec, int  const vecLen, complexType  *outVec) 
{
	if ((vecLen <= 0) || (NULL == inVec) || (NULL == outVec))
		return false;
	if (!is_power_of_two(vecLen))
		return false;

	//步骤1 初始化因子数组Weights
	complexType  *Weights = (complexType *)malloc(sizeof(complexType) * vecLen); //权重数组
																				 // 计算权重序列
	double fixed_factor = (-2 * MM_PI) / vecLen;
	for (int i = 0; i < vecLen / 2; i++) {
		double angle = i * fixed_factor;
		Weights[i].Real = cos(angle);
		Weights[i].Imag = sin(angle);
	}
	for (int i = vecLen / 2; i < vecLen; i++) {
		Weights[i].Real = -(Weights[i - vecLen / 2].Real);
		Weights[i].Imag = -(Weights[i - vecLen / 2].Imag);
	}

	//步骤2:目的数组清零,源数组做change变址运算后拷贝目的数组
	memset(outVec, 0, vecLen * sizeof(complexType));

	//计算倒序位码
	int r = (int)(log(vecLen) / log(2));//2的多少次幂
	int index = 0;
	for (int i = 0; i < vecLen; i++) {
		index = 0;
		for (int m = r - 1; m >= 0; m--) {
			index += (1 && (i & (1 << m))) << (r - m - 1);
		}
		outVec[i].Real = inVec[index].Real;
		outVec[i].Imag = inVec[index].Imag;
	}

	/*for (int i = 0; i < vecLen; ++i)
	{
	printf("%lf %lf\n", outVec[i].Real, outVec[i].Imag);
	}*/

	// 计算快速傅里叶变换对（变地址后端outVec蝶形运算）
	uint64_t i = 0, j = 0, k = 0, m = 0;
	complexType up, down, product;
	int size_x = vecLen;
	for (i = 0; i< log(size_x) / log(2); i++) /*一级蝶形运算 stage */
	{
		m = 1 << i;
		for (j = 0; j<size_x; j += 2 * m)     /*一组蝶形运算 group,每组group的蝶形因子乘数不同*/
		{
			for (k = 0; k<m; k++)             /*一个蝶形运算 每个group内的蝶形运算的蝶形因子乘数成规律变化*/
			{
				//优化，减少函数调用
				//complexMultiply(outVec[j + k + l], Weights[size_x*k / 2 / l], &product);
				product.Real = outVec[j + k + m].Real *  Weights[size_x*k / 2 / m].Real - outVec[j + k + m].Imag *  Weights[size_x*k / 2 / m].Imag;
				product.Imag = outVec[j + k + m].Imag *  Weights[size_x*k / 2 / m].Real + outVec[j + k + m].Real *  Weights[size_x*k / 2 / m].Imag;
				//complexAdd(outVec[j + k], product, &up);
				up.Real = outVec[j + k].Real + product.Real;
				up.Imag = outVec[j + k].Imag + product.Imag;
				//complexSubtract(outVec[j + k], product, &down);
				down.Real = outVec[j + k].Real - product.Real;
				down.Imag = outVec[j + k].Imag - product.Imag;
				outVec[j + k] = up;
				outVec[j + k + m] = down;
			}
		}
	}

	//步骤3: 释放所占空间
	free(Weights);

	return true;
}
void  Rfftshift(float *in, int dim)
{
	//in is even
	if ((dim & 1) == 0) {
		int pivot = dim / 2;
		for (int pstart = 0; pstart<pivot; ++pstart) {//swap two elements
			float temp = in[pstart + pivot];
			in[pstart + pivot] = in[pstart];
			in[pstart] = temp;
		}
	}
	else { //in is odd
		int pivot = dim / 2;
		float pivotElement = in[pivot];
		for (int pstart = 0; pstart<pivot; ++pstart) {//swap two elements
			float temp = in[pstart + pivot + 1];
			in[pstart + pivot] = in[pstart];
			in[pstart] = temp; //将pivot+1对应的元素与pstart对应元素交换
		}
		in[dim - 1] = pivotElement;
	}
}
/*函数名：CfftAbs
*说明：对复数类型complexType求模运算
*输入：待复数类型数组和长度
*输出：abs求幅度
*其它：空间复杂度O(n),时间复杂度O(n)
*/
//void  CfftAbs(complexType x[], int N, double *amplitude) // 求模运算计算的是20*log10（Abs）
void  CfftAbs(complexType *x, int n, float *amplitude) // 求模运算
{
	for (int i = 0; i < n; i++)
	{
		//amplitude[i] = 20.0 * log10( qSqrt( g.real() * g.real() + g.imag() * g.imag() ) );
		//实际幅值 计算*2/N (直流分量其实不需要*2)
		//if(i !=0)
		amplitude[i] = 10.0 * log10(x[i].Real*x[i].Real + x[i].Imag*x[i].Imag)+CORRECTIONSIGNAL;   
	}
}

/*函数名：Coutput
*说明：输出傅里叶变换的结果,方便测试
*输入：待打印数组和长度
*输出：
*/
void  Coutput(const complexType x[], int size_x)         //输出傅里叶变换的结果
{
	int i;
	printf("The result are as follows：\n");
	for (i = 0; i<size_x; i++)
	{
		printfd("%4d  %8.8lf ", i, x[i].Real);
		if (x[i].Imag >= 0.0001)printfd(" +%8.488fj\n", x[i].Imag);
		else if (fabs(x[i].Imag)<0.0001) printfd("\n");
		else printfd("  %8.8fj\n", x[i].Imag);
	}
}
/*函数名：Routput
*说明：输出傅里叶变换的结果,方便测试
*输入：待打印数组和长度
*输出：
*/
void  Routput(const mathDouble x[], int size_x)         //输出傅里叶变换的结果
{
	int i;
	printf_debug("The result are as follows：\n");
	for (i = 0; i<size_x; i++)
	{
		printfd("%4d  %8.8lf\n", i, x[i]);
	}
}



//void writefileArr(const char* filename, double arr[], int rank) //写文件
void writefileArr(const char* filename, float *arr, int rank) 

{
	FILE *fp;
	if ((fp = fopen(filename, "w")) == NULL)
	{
		printf_warn("file open error");
	}
	for (int i = 0; i<rank; ++i)
	{
		fprintf(fp, "%lf\n", arr[i]);
	}
	fclose(fp);


}
void writefileArrint(const char* filename, int *arr, int rank) 

{
	FILE *fp;
	if ((fp = fopen(filename, "w")) == NULL)
	{
		printf_warn("file open error");
	}
	for (int i = 0; i<rank; ++i)
	{
		fprintf(fp, "%d\n", arr[i]);
	}
	fclose(fp);


}

void Verification(const char* filename, float* num, int rank)       //从文件中，读入原始信号
{
	FILE *fp;
	if ((fp = fopen(filename, "rb")) == NULL)
	{
		printf_warn("file not exist\n");
	}
	for (int i = 0; i<rank; i++)
	{
		fscanf(fp, "%f", &num[i]);
	}
	fclose(fp);
}
void Verificationfloat(const char* filename, short* num, int rank)       //从文件中，读入原始信号
{
	FILE *fp;
	if ((fp = fopen(filename, "r")) == NULL)
	{
		printf_warn("file not exist\n");
        return ;
	}
	for (int i = 0; i<rank; i++)
	{
		 fscanf(fp, "%hd", &num[i]);
	}
	fclose(fp);
}

pthread_mutex_t fft_data_mutex = PTHREAD_MUTEX_INITIALIZER;


void fft_fftw_calculate(short *iqdata,int32_t fftsize,int datalen,float *mozhi)
{
    int i;
    fftwf_plan p;
    short* wavdatafp;
    short* wandateimage;
    pthread_mutex_lock(&fft_data_mutex);
    fftwf_complex *din = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fftsize);
    fftwf_complex *out = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fftsize);
    float *hann=(float*)malloc(sizeof(float)*fftsize);
    memset(hann,0,sizeof(float)*fftsize );
    if(din==NULL||out==NULL||hann==NULL)
    {
        printf_note("fft_fftw_calculate Memory allocation failed!!\n");

    }
  //  hannwindow(fftsize,hann);
    //writefileArr("hann.txt",hann,fftsize);
    
    wavdatafp=iqdata;
    wandateimage=iqdata+1;
    int j=0;
    int num=0;
    for(i=0; i<datalen; i+=2)
    {
        din[j++][0] = (*wavdatafp);
        din[num++][1] = (*wandateimage);
        wavdatafp+=2;
        wandateimage+=2;
    }
    /*for(i=0; i<fftsize; i++)
    {
        din[i][0] = (din[i][0])*(hann[i]);
        din[i][1] = (din[i][1])*(hann[i]);
    }*/
    double costtime;
    clock_t start,end;
    start=clock();
    p = fftwf_plan_dft_1d(fftsize,din, out, FFTW_FORWARD,FFTW_ESTIMATE);
    fftwf_execute(p);
    
    end=clock();
    costtime=(double)(end-start)/CLOCKS_PER_SEC;
    printf_debug("======costtime=%.20lf\n",costtime);
    CfftAbs(out,fftsize,mozhi);
    Rfftshift(mozhi,fftsize);
    fftwf_destroy_plan(p);
   // fftwf_cleanup();
    if(din!=NULL) fftwf_free(din);
    if(out!=NULL) fftwf_free(out);
    free(hann);
    pthread_mutex_unlock(&fft_data_mutex);
    return 0;
}

void fft_fftw_calculate_hann(short *iqdata,int32_t fftsize,int datalen,float *mozhi)
{
    int i;
    fftwf_plan p;
    short* wavdatafp;
    short* wandateimage;
    pthread_mutex_lock(&fft_data_mutex);
    fftwf_complex *din = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fftsize);
    fftwf_complex *out = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fftsize);
    float *hann=(float*)malloc(sizeof(float)*fftsize);
    memset(hann,0,sizeof(float)*fftsize );
    if(din==NULL||out==NULL||hann==NULL)
    {
        printf_note("fft_fftw_calculate Memory allocation failed!!\n");

    }
     hannwindow(fftsize,hann);
    //writefileArr("hann.txt",hann,fftsize);

    wavdatafp=iqdata;
    wandateimage=iqdata+1;
    int j=0;
    int num=0;
    for(i=0; i<datalen; i+=2)
    {
        din[j++][0] = (*wavdatafp);
        din[num++][1] = (*wandateimage);
        wavdatafp+=2;
        wandateimage+=2;
    }
    for(i=0; i<fftsize; i++)
    {
        din[i][0] = (din[i][0])*(hann[i]);
        din[i][1] = (din[i][1])*(hann[i]);
    }
    double costtime;
    clock_t start,end;
    start=clock();
    p = fftwf_plan_dft_1d(fftsize,din, out, FFTW_FORWARD,FFTW_ESTIMATE);
    fftwf_execute(p);

    end=clock();
    costtime=(double)(end-start)/CLOCKS_PER_SEC;
    printf_debug("======costtime=%.20lf\n",costtime);
    CfftAbs(out,fftsize,mozhi);
    Rfftshift(mozhi,fftsize);
    fftwf_destroy_plan(p);
   // fftwf_cleanup();
    if(din!=NULL) fftwf_free(din);
    if(out!=NULL) fftwf_free(out);
    free(hann);
    pthread_mutex_unlock(&fft_data_mutex);
    return 0;
}

void smooth(float* fftdata,int fftdatanum,float *smoothdata)    //fft后数据的平滑处， 返回平滑滤波之后的值
{
    float Sum1=0;
    for(int j=0;j<fftdatanum;j++)
    {
        if(j<SMOOTHPOINT/2)
        {
            for(int k=0;k<SMOOTHPOINT;k++)
            {
                Sum1+=fftdata[j+k];
            }
            smoothdata[j]=Sum1/SMOOTHPOINT;
        }
        else
            if(j<fftdatanum -SMOOTHPOINT/2)
            {
                for(int k=0;k<SMOOTHPOINT/2;k++)
                {
                    Sum1+=(fftdata[j+k]+fftdata[j-k]);
                }
                smoothdata[j]=Sum1/SMOOTHPOINT;
            }
            else
            {
                for(int k=0;k<fftdatanum-j;k++)
                {
                    Sum1+=fftdata[j+k];
                }
                for(int k=0;k<(SMOOTHPOINT-fftdatanum+j);k++)
                {
                    Sum1+=fftdata[j-k];
                }
                smoothdata[j]=Sum1/SMOOTHPOINT;
            }
        Sum1=0;
    }
}
void smooth2(float* fftdata,int fftdatanum,float *smoothdata)    //fft后数据的平滑处， 返回平滑滤波之后的值
{
    float Sum1=0;
    for(int j=0;j<fftdatanum;j++)
    {
        if(j<SMOOTHPOINT/2)
        {
            for(int k=0;k<SMOOTHPOINT;k++)
            {
                Sum1+=fftdata[j+k];
            }
            smoothdata[j]=Sum1/SMOOTHPOINT;
        }
        else
            if(j<fftdatanum -SMOOTHPOINT/2)
            {
                for(int k=0;k<((SMOOTHPOINT/2)+1);k++)
                {
                    if(k==0)
                    {
                        Sum1=fftdata[j];

                    }else{

                         Sum1+=(fftdata[j+k]+fftdata[j-k]);

                    }

                }
                smoothdata[j]=Sum1/(SMOOTHPOINT+1);
            }
            else
            {
                for(int k=0;k<fftdatanum-j;k++)
                {
                    Sum1+=fftdata[j+k];
                }
                for(int k=0;k<(SMOOTHPOINT-fftdatanum+j);k++)
                {
                    Sum1+=fftdata[j-k];
                }
                smoothdata[j]=Sum1/SMOOTHPOINT;
            }
        Sum1=0;
    }
}


void findcomplexcentfrequencypoint(float *data,float* maxaverage,float* minaverage,int datalen)  //找到信号中的最大值和最小值，求平均得到平均最大值和最小值，并选择计算寻找中心频点的方式
{
	*maxaverage=data[0];
	*minaverage=data[0];	

	for(int i=0;i<datalen;i++)
	{
		if(*maxaverage<data[i])
		{
			*maxaverage=data[i];
		}
		else if(*minaverage>data[i])
		{
			*minaverage=data[i];
		}
	}

}
signalnum_flag  findCentfreqpoint(float *data, int pointnum,int * centfeqpoint,float *Threshold ,int * cenfrepointnum,int *y,int *z,float *maxvalue)//大于阈值的第一个 z小于阈值的第一个数 ,计算中心频点
{
	printf_debug("findCentfreqpoint \n");
	int *h;	
	h=centfeqpoint;
		
	int p[SIGNALNUM]={0};
	int q[SIGNALNUM]={0};
	int j =0,x=0;
	//int centpointnum1;
	int flag=0;
    signalnum_flag signalflg;
	
	printf_debug("data[0]=%lf   data[1]=%lf\n,",data[0],data[1]);
	
	for(int i=0;i<N;i++)
	{ 
			
		//printf("i=%d  ,",i);
		if(data[i]<=(*Threshold)&&data[i+1]>(*Threshold)&&flag==0)
		{
			p[j++] = i+1;
			flag=1;
			
			
			//printf_debug("p[0] =%d",p[0]);
		}
		else if(data[i]>=(*Threshold)&&data[i+1]<(*Threshold)&&flag==1)
		{
				(*cenfrepointnum)++;
				q[x++]=i;
				flag=0;
				//printf("z[i]=%d,  i=%d ",z[j],i);
				//printf_debug("q[0] =%d\n",q[0]);	
		}
        if(*cenfrepointnum>SIGNALNUM)
        {
            printf_err("Your threshold may be a little low, please re-issue the threshold\n");
            signalflg=SIGNALNUM_ABNORMAL;
            return signalflg ;

        }
	}
    printf_debug("*Threshold=%f\n",*Threshold);
	printf_debug("find centfeqpoint n=%d\n",*cenfrepointnum);
	int a=0;
	j=0;x=0;
    int findpoint[SIGNALNUM]={0};
    int findpointmax=q[0]-p[0];
    for(int i=0;i<*cenfrepointnum;i++)
    {
        if(findpointmax<q[i]-p[i])
        {
            findpointmax=q[i]-p[i];
        }
    }
    
    //SHIELDPOINTS=findpointmax/2;
    SHIELDPOINTS=findpointmax;
    printf_debug("\n\nSHIELDPOINTS=%d\n\n",SHIELDPOINTS);

	for(int i=0;i<*cenfrepointnum;i++)
	{	
	    
		
		//a[i]=(z[i]-y[i])/2;
		printf_debug("q[i] = %d p[i] = %d q[i]-p[i]=%d  i=%d    \n",q[i],p[i],(q[i]-p[i]),i);
		//printf("a[i]=%d\n",a[i]);
		if(((q[i]-p[i]))>=SHIELDPOINTS)                     //修改1
		{
			*h=q[i]-((q[i]-p[i])/2);	
			h++;
			a++;
			y[j++]=p[i];
			z[x++]=q[i];
		}
	  
		
	}	
	*cenfrepointnum=a;
	printf_debug("find centfeqpoint n=%d\n",*cenfrepointnum);
	/*int maxband=z[0]-y[0];
	int maxzuobiao=0;
	for(int j=0;j<*cenfrepointnum;j++)
	{
		if(maxband<(z[j]-y[j]))
		{
			maxband=z[j]-y[j];
			maxzuobiao=j;
		}	
	
		printf("centfrepoint[j]=%d\n",centfeqpoint[j]);
		printf("z[i]=%d,y[i]=%d\n",z[j],y[j]);
		
	}*/
	//*Centerpoint=y[maxzuobiao]+(z[axzuobiao]-y[maxzuobiao])/2;
  //  printf("*Centerpoint=%d",*Centerpoint);
	printf_debug(" findCentfreqpointover\n");
    signalflg=SIGNALNUM_NORMAL;
    return  signalflg;
}
signalnum_flag  findCentfreqpoint_check(float *data, int pointnum,int * centfeqpoint,float *Threshold ,int * cenfrepointnum,int *y,int *z,int  *Centerpoint)//大于阈值的第一个 z小于阈值的第一个数 ,计算中心频点
{
    printf_debug("findCentfreqpoint \n");
    int *h;	
    h=centfeqpoint;
    	
    int p[SIGNALNUM]={0};
    int q[SIGNALNUM]={0};
    int j =0,x=0,n=0,i=0;
    //int centpointnum1;
    int flag=0;
    signalnum_flag signalflg;

    printf_debug("data[0]=%lf   data[1]=%lf\n,",data[0],data[1]);
    for(i=0;i<fftstate.cenfrepointnum;i++)
    {
        for(n=fftstate.y[i];n<fftstate.z[i];n++)
        {
            if(data[n]<=(*Threshold)&&data[n+1]>(*Threshold)&&flag==0)
            {
                p[j++] = i+1;
                flag=1;
            }
            else if(data[n]>=(*Threshold)&&data[n+1]<(*Threshold)&&flag==1)
            {
                (*cenfrepointnum)++;
                q[x++]=i;
                flag=0;
            }
            if(*cenfrepointnum>SIGNALNUM)
            {
                printf_err("Your threshold may be a little low, please reissue the threshold\n");
                signalflg=SIGNALNUM_ABNORMAL;
                return signalflg ;
            }
        }


    }
    int a=0;
    for(int i=0;i<*cenfrepointnum;i++)
    {

        //a[i]=(z[i]-y[i])/2;
        printf_debug("q[i] = %d p[i] = %d q[i]-p[i]=%d  i=%d    \n",q[i],p[i],(q[i]-p[i]),i);
        //printf("a[i]=%d\n",a[i]);
        if(((q[i]-p[i]))>=SHIELDPOINTS)                     //修改1
        {
            *h=q[i]-((q[i]-p[i])/2);	
            h++;
            a++;
            y[j++]=p[i];
            z[x++]=q[i];
        }

    }
    *cenfrepointnum=a;
    printf_debug("find centfeqpoint n=%d\n",*cenfrepointnum);
    signalflg=SIGNALNUM_NORMAL;
    return  signalflg;
}

void  calculatecenterfrequency(float *fftdata,int fftnum)
{
    printf_debug("calculatecenterfrequency\n");
    int j=0;
    printf_debug("cenfrepointnum=%d\n",fftstate.cenfrepointnum);
    int max=0;
    for(int i=0;i<fftstate.cenfrepointnum;i++)
    {
        // fftstate.arvcentfreq[i]=fftdata[fftstate.centfeqpoint[i]];
        
            for(j=fftstate.y[i];j<fftstate.z[i];j++)
            {
                if(max<fftdata[j])
                {
                    max=fftdata[j];
                }

            }
            fftstate.arvcentfreq[i]=max;
            printf_debug("fftstate.arvcentfreq[i]=%f,y[i]=%d,z[i]=%d\n",fftstate.arvcentfreq[i],fftstate.y[i],fftstate.z[i]);
            max=0;
	}

}

//原始数据  原始数据个数 平均后的中频频率 第一个点的集合      第二个点的集合    平均后的中心频率
void  calculatebandwidth(float *fftdata,int fftnum ,float* temp)
{	
	int j=0;
	int first=0;
	int end=0;
	//int firstpoint[fftstate.cenfrepointnum];
	//int endpoint[fftstate.cenfrepointnum];
	int flag=0;
	float threedbpoint;
	float maxvalue;
	float maxz;
    float arvcentfreq;
    float dbvalue;
	

	//findcomplexcentfrequencypoint(fftdata,&maxvalue,&minvalue,fftnum);

	//findcentfrequencyabsmaxmin(threedbpoint);
	for(int i=0;i<fftstate.cenfrepointnum;i++)
	{ 
        arvcentfreq = fftstate.arvcentfreq[i]-*temp;
        dbvalue=(fftstate.arvcentfreq[i]-fftstate.Threshold)*2/3;
        if(arvcentfreq<dbvalue)
        {
            dbvalue=arvcentfreq;
        }
        if(dbvalue>14)
        {
            dbvalue=14;
        }
        //threedbpoint = fftstate.arvcentfreq[i]-(fftstate.arvcentfreq[i]-fftstate.Threshold)*2/3;
        threedbpoint = fftstate.arvcentfreq[i]-dbvalue;
        if(threedbpoint<fftstate.Threshold)
        {
            printf_debug("\nThe threshold is higher than 3db bandwidth\n");
        }
        //printf_debug("fftstate.arvcentfreq[i]=%f,*temp=%f",fftstate.arvcentfreq[i],*temp);
        printf_debug("threedbpoint=%f,Threshold=%f,db=%f\n",threedbpoint,fftstate.Threshold,(fftstate.arvcentfreq[i]-fftstate.Threshold)*2/3);
        fftstate.arvcentfreq[i]=fftstate.arvcentfreq[i]-CORRECTIONSIGNAL;
        
            //printf_debug("调试信息 maxz : %f fftstate.Bottomnoise:%f threedbpoint:%f\n",maxz,fftstate.Bottomnoise,threedbpoint);
            for(j=fftstate.y[i];j<fftstate.z[i];j++)
            {

                //if(fftdata[j]<=0.707946*arvcentfreq[i]&&fftdata[j+1]>0.707946*arvcentfreq[i])
                if(fftdata[j]<=threedbpoint&&fftdata[j+1]>threedbpoint)
                { 
                    if(flag == 0)
                    {
                        fftstate.firstpoint[i]=j+1;  //确定是第一个点
                        j=fftstate.z[i]-((fftstate.z[i]-fftstate.y[i])/2);
                        flag=1;
                        continue;
                    }
                }
                else if(fftdata[j]>=threedbpoint&&fftdata[j+1]<threedbpoint)
                {
                    fftstate.endpoint[i]=j;
                    flag=0;
                    break;
                }
        }
    }
    printf_debug("threedbpoint=%f  \n",threedbpoint);
    int max=0;
    for(int i=0;i<fftstate.cenfrepointnum;i++)
    {
        if(fftstate.endpoint[i]>fftstate.firstpoint[i]&&fftstate.firstpoint[i]>0&&fftstate.endpoint[i]<fftnum)
        {
            fftstate.bandwidth[i]=fftstate.endpoint[i]-fftstate.firstpoint[i];
            printf_debug(" bandwidth=%d,endpoint=%d,firstpoint=%d\n",fftstate.bandwidth[i],fftstate.endpoint[i],fftstate.firstpoint[i]);
        }

    }
}
void  calculatebandwidth2(float *fftdata,int fftnum ,float* temp,float *maxvalue)
{	
	int j=0;
	int first=0;
	int end=0;
	//int firstpoint[fftstate.cenfrepointnum];
	//int endpoint[fftstate.cenfrepointnum];
	int flag=0;
	float threedbpoint;
	float maxz;
    float arvcentfreq;
    float dbvalue;
	

	//findcomplexcentfrequencypoint(fftdata,&maxvalue,&minvalue,fftnum);

	//findcentfrequencyabsmaxmin(threedbpoint);
	for(int i=0;i<fftstate.cenfrepointnum;i++)
	{ 
        arvcentfreq = fftstate.arvcentfreq[i]-*temp;
        dbvalue=(fftstate.arvcentfreq[i]-*maxvalue)*2/3;
        if(arvcentfreq<dbvalue)
        {
            dbvalue=arvcentfreq;
        }
        if(dbvalue>14)
        {
            dbvalue=14;
        }
        //threedbpoint = fftstate.arvcentfreq[i]-(fftstate.arvcentfreq[i]-fftstate.Threshold)*2/3;
        threedbpoint = fftstate.arvcentfreq[i]-dbvalue;
        if(threedbpoint<*maxvalue)
        {
            printf_debug("\nThe threshold is higher than 3db bandwidth\n");
        }
        //printf_debug("fftstate.arvcentfreq[i]=%f,*temp=%f",fftstate.arvcentfreq[i],*temp);
        printf_warn("threedbpoint=%f,Threshold=%f,dbvalue=%f\n",threedbpoint,*maxvalue,dbvalue);
        fftstate.arvcentfreq[i]=fftstate.arvcentfreq[i]-CORRECTIONSIGNAL;
        int endzuobiao;
        
            //printf_debug("调试信息 maxz : %f fftstate.Bottomnoise:%f threedbpoint:%f\n",maxz,fftstate.Bottomnoise,threedbpoint);
        for(j=fftstate.y[i];j<fftstate.z[i];j++)
        {

            //if(fftdata[j]<=0.707946*arvcentfreq[i]&&fftdata[j+1]>0.707946*arvcentfreq[i])
            if(fftdata[j]<=threedbpoint&&fftdata[j+1]>=threedbpoint)
            { 
                if(flag == 0)
                {
                    fftstate.firstpoint[i]=j+1;  //确定是第一个点
                    //j=fftstate.z[i]-((fftstate.z[i]-fftstate.y[i])/2);
                    flag=1;
                    continue;
                }
            }
            else if(fftdata[j]>=threedbpoint&&fftdata[j+1]<=threedbpoint)
            {
                endzuobiao=j;
                //break;
            }
            if(j==fftstate.z[i]-1)
            {
                fftstate.endpoint[i]=endzuobiao;
                flag=0;
            }
            
        }
    }
    printf_debug(" bandwidth=%d,endpoint=%d,firstpoint=%d\n",fftstate.bandwidth[0],fftstate.endpoint[0],fftstate.firstpoint[0]);
    for(int i=0;i<fftstate.cenfrepointnum;i++)
    {
        if(fftstate.endpoint[i]>fftstate.firstpoint[i]&&fftstate.firstpoint[i]>0&&fftstate.endpoint[i]<fftnum)
        {
            fftstate.bandwidth[i]=fftstate.endpoint[i]-fftstate.firstpoint[i];
            printf_debug(" bandwidth=%d,endpoint=%d,firstpoint=%d\n",fftstate.bandwidth[i],fftstate.endpoint[i],fftstate.firstpoint[i]);
        }

    }
}

void wavrtranstxt(const char* filename,int trancount)
{   
	int  fileset=0;
	int  fileend=0;
	int  filelength=0;
	short  inputdata[W]={0};
    FILE *ifp,*ttt;
    ifp=fopen(filename,"rb");
    ttt=fopen("rawdata.txt","w+");

    fseek(ifp,0L,SEEK_END);
	
    fileend=ftell(ifp);
	 
    printf_debug("%d\n",fileend);
    
    rewind(ifp);
  
	//filelength=2*N;
	filelength=trancount;
    while(  filelength >0)
    {
         fread(inputdata,sizeof(short),W,ifp);
   
         for(int i=0;i<W;i++ )
         {
                    fprintf(ttt,"%d  ",inputdata[i]);

         }
         filelength-=  W;
         memset(inputdata,0,sizeof(inputdata));

    }
	fclose(ifp);
	fclose(ttt);
	printf_debug("over   ");

}
void Lowpassfiltering(float *data,int num)
{
	float Value=data[0];
	for(int i=0;i<num;i++)
	{
		Value = data[i] * FILTER_A + (1.0 - FILTER_A) * Value;
		data[i]=Value;

	}
	
}
void findBottomnoise(float *mozhi,int xiafamenxian,float *Bottomnoise,float *Threshold,int datalen)
{
    int sum=0;
    float minvalue;
    float maxvalue;

    findcomplexcentfrequencypoint(mozhi,&maxvalue,&minvalue,datalen);
    *Bottomnoise=minvalue+(maxvalue-minvalue)/3;
    *Threshold=*Bottomnoise+xiafamenxian;
    printf_debug("minvalue=%f,\n",minvalue);
    printf_debug("maxvalue=%f,\n",maxvalue);
    printf_debug("Bottomnoise=%f,",*Bottomnoise);
    printf_debug("Thresholdmenxian=%f\n",*Threshold);

}
void findBottomnoisenomax(float *mozhi,int xiafamenxian,float *Bottomnoise,float *Threshold,int datalen,float *maxvalue,float *minvalue)
{
    int sum=0;
    findcomplexcentfrequencypoint(mozhi,maxvalue,minvalue,datalen);
    *Bottomnoise=*minvalue+(*maxvalue-*minvalue)/3;
    *Threshold=*Bottomnoise+xiafamenxian;
    printf_debug("minvalue=%f,\n",*minvalue);
    printf_debug("maxvalue=%f,\n",*maxvalue);
    printf_debug("Bottomnoise=%f,",*Bottomnoise);
    printf_debug("Thresholdmenxian=%f\n",*Threshold);
}

void findBottomnoiseprecise(float *mozhi,int xiafamenxian,float *Bottomnoise,float *Threshold,int datalen ,float *maxvalue,float *minvalue)
{
    int sum=0;
    findcomplexcentfrequencypoint(mozhi,maxvalue,minvalue,datalen);
    *Bottomnoise=*minvalue+(*maxvalue-*minvalue)/3;
    int i=0;
    int zuobiao;
    float max=mozhi[datalen/2-200];
    for(i=(datalen/2-200);i<(datalen/2+200);i++)
    {
        //printf("i=%d,mozhi[i]=%f  ,",i,mozhi[i]);
        if(max<mozhi[i])
         {
            max=mozhi[i];
            zuobiao=i;
            //printf_debug("中间值=======================%f,\n",max);
         }
    }
    printf_debug("max=%f,*maxvalue=%f,*Bottomnoise=%f,\n,",max,*maxvalue,*Bottomnoise);
    if((*maxvalue>max)&&(max>(*Bottomnoise)))
    {
        printf_debug("阈值改变\n");
        *Threshold=max+xiafamenxian;
    }else{
        printf_debug("阈值不变\n");
        *Threshold=*Bottomnoise+xiafamenxian;
    }

    printf_debug("minvalue=%f,\n",*minvalue);
    printf_warn("maxvalue-minvalue=%f,\n",*maxvalue-*minvalue);
    printf_debug("Bottomnoise=%f,",*Bottomnoise);
    printf_debug("Thresholdmenxian=%f\n",*Threshold);
}
void findbaoluo(float *data,int num,float *max,int *maxzuobiao,int numsample,int interval )
{
	int j=0;
	int t=0;
	int maxx=0;
	float maxflag=data[0];

	int maxxflag=0;
	int minxflag;

	//printf("data[0]=%f  ,data[5]=%f\n",data[0],data[5]);
	
	for(int i=0;i<N;i++)
	{
		
		if(maxflag<data[i])
		{
			//printf("data[%d]=%f  \n,",i,data[i]);
			maxflag=data[i];
			maxxflag=i;
		}
		if((i+1)%interval==0)
		{
			max[j++]=maxflag;   
			maxzuobiao[maxx++]=maxxflag;
			maxflag=data[i+1];
			maxxflag=i+1;
		}
		if(j==numsample)
		{
			break;
		}
		
	}
	
}
void PartialDerivative2(int *x,float *y,int num,float *PartialDerivative )
{
	float *a=(float*)malloc(sizeof(float)*num);
	float *b=(float*)malloc(sizeof(float)*num);
	float *c=(float*)malloc(sizeof(float)*num);
	float *d=(float*)malloc(sizeof(float)*num);
	float *f=(float*)malloc(sizeof(float)*num);
	float *bt=(float*)malloc(sizeof(float)*num);
	float *gm=(float*)malloc(sizeof(float)*num);
	float *h=(float*)malloc(sizeof(float)*num);
	memset(a,0,sizeof(float)*num );
	memset(b,0,sizeof(float)*num );
	memset(c,0,sizeof(float)*num );
	memset(d,0,sizeof(float)*num );
	memset(f,0,sizeof(float)*num );
	memset(bt,0,sizeof(float)*num );
	memset(gm,0,sizeof(float)*num );
	memset(h,0,sizeof(float)*num );
	int rightb=0;int leftb=0;  //边界倒数 
	
	for(int i=0;i<num;i++)  b[i]=2;
	for(int i=0;i<num-1;i++)  h[i]=x[i+1]-x[i];
	for(int i=1;i<num-1;i++)  a[i]=h[i-1]/(h[i-1]+h[i]);
	a[num-1]=1;

	c[0]=1;
	for(int i=1;i<num-1;i++)  c[i]=h[i]/(h[i-1]+h[i]);

	for(int i=0;i<num-1;i++) 
		f[i]=(y[i+1]-y[i])/(x[i+1]-x[i]);

	for(int i=1;i<num-1;i++)  d[i]=6*(f[i]-f[i-1])/(h[i-1]+h[i]);

	d[1]=d[1]-a[1]*leftb;
	d[num-2]=d[num-2]-c[num-2]*rightb;

	bt[1]=c[1]/b[1];
	for(int i=2;i<num-2;i++)  bt[i]=c[i]/(b[i]-a[i]*bt[i-1]);

	gm[1]=d[1]/b[1];
	for(int i=2;i<=num-2;i++)  gm[i]=(d[i]-a[i]*gm[i-1])/(b[i]-a[i]*bt[i-1]);

	PartialDerivative[num-2]=gm[num-2];
	for(int i=num-3;i>=1;i--)  PartialDerivative[i]=gm[i]-bt[i]*PartialDerivative[i+1];

	PartialDerivative[0]=leftb;
	PartialDerivative[num-1]=rightb;
	free(a);
	free(b);
	free(c);
	free(d);
	free(f);
	free(bt);
	free(gm);
	free(h);

}
void spline(int *x,float *y,int num ,float *PartialDerivative)
{

	if((x==NULL)|(y==NULL)|(num<3))
	{
		printf_debug("构造失败，已知点数太少");
		
	}
	PartialDerivative2(x,y,num,PartialDerivative);
}

void hannwindow(int num,float *w)               //汉宁窗
{
    float *ret;
    ret=(float*)malloc(sizeof(float)*num);
    if(ret==NULL)
    {
    	printf_warn("hannwindow malloc fail\n");
    }
    for(int i=0;i<num;i++)
    {
    	ret[i]=0.5*(1-cos(2*3.1415926*(float)i/(num-1)));
    }
    memcpy(w,ret,sizeof(float)*num);
    free(ret);
}
/********************************************************************************

函数功能：在处理IQ数据之前的初始化操作
输入：
        无
返回值：
      无返回值；

*********************************************************************************/

void fft_init(void)
{
    fftdata.mozhi=(float*)malloc(sizeof(float)*N );
	//fftdata.x  = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * N);
	//fftdata.y = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * N);
    fftdata.smoothdata=(float*)malloc(sizeof(float)*N);
    fftdata.wavdata=(short*)malloc(sizeof(short)*(2*N)); 
    //fftdata.hann=(float*)malloc(sizeof(float)*N);

    memset(fftdata.mozhi,0,sizeof(float)*N );
    memset(fftdata.smoothdata,0,sizeof(float)*N );    
    memset(fftdata.wavdata,0,sizeof(short)*(2*N) );
   // memset(fftdata.hann,0,sizeof(float)*N );

    memset(fftstate.y,0,sizeof(int)*SIGNALNUM );
    memset(fftstate.z,0,sizeof(int)*SIGNALNUM );
    memset(fftstate.centfeqpoint,0,sizeof(int)*SIGNALNUM );
    memset(fftstate.arvcentfreq,0,sizeof(float)*SIGNALNUM);
    memset(fftstate.bandwidth,0,sizeof(float)*SIGNALNUM);
    memset(fftstate.firstpoint,0,sizeof(int)*SIGNALNUM);
    memset(fftstate.endpoint,0,sizeof(int)*SIGNALNUM);
   /* if((fftdata.x==NULL)||(fftdata.y==NULL))
    {
        printf("Error:insufficient available memory\n");
    }*/


    fftstate.cenfrepointnum=0;
    fftstate.Threshold=0;
    fftstate.Centerpoint=0;
    fftstate.Bottomnoise=0;
    fftstate.maxcentfrequency=0;
    fftstate.interval=0;	
}
void fft_exit(void)
{
    SAFE_FREE(fftdata.mozhi);
    //SAFE_FREE(fftdata.x);
    //SAFE_FREE(fftdata.y);
    SAFE_FREE(fftdata.smoothdata);
    SAFE_FREE(fftdata.wavdata);
    //SAFE_FREE(fftdata.hann);
   // if(fftdata.x!=NULL) fftwf_free(fftdata.x);
   // if(fftdata.y!=NULL) fftwf_free(fftdata.y);
}
void fft_read_iqdata(short *iqdata,int32_t fftsize,int datalen,complexType *data)
{
    short* wavdatafp;
	short* wandateimage;
    wavdatafp=iqdata;
	wandateimage=iqdata+1;
	int j=0;
	int q=0;
    //hannwindow(fftsize,fftdata.hann);/*hanning window*/
	for(int i=0;i<2*fftsize;i+=2)
	{
		if(i<datalen)
		{
			data[j++].Real=(*wavdatafp);	
			data[q++].Imag =(*wandateimage);
			wavdatafp+=2;
			wandateimage+=2;
			
		}else{
			
			data[j++].Real=0.0;
			data[q++].Imag =0.0;	
		}
	}
	for(int i=0;i<fftsize;i++)
	{
		//data[i].Real=data[i].Real*fftdata.hann[i];
		//data[i].Imag=data[i].Imag*fftdata.hann[i];
	}
}
void  fft_calculate_finaldata()
{
    int num1=0;
	int num2=0;
	int num3=0;
	int num=0;
	for(int i=0;i<fftstate.cenfrepointnum;i++)
	{
		if((fftstate.arvcentfreq[i]>0 )&&(fftstate.bandwidth[i]>0)&&fftstate.bandwidth[i]<N)
		{
			
			fftstate.arvcentfreq[num1++]=fftstate.arvcentfreq[i];
			fftstate.bandwidth[num2++]=fftstate.bandwidth[i];
			fftstate.centfeqpoint[num3++]=fftstate.centfeqpoint[i];	
			num++;
		}
	}
	fftstate.cenfrepointnum=num;
	//printf_debug("\n\n*********************First fuzzy calculation****************************\n");
	for(int i=0;i<fftstate.cenfrepointnum;i++)
	{
		printf_debug("fftstate.centfeqpoint[%d]=%d,fftstate.bandwidth[%d]=%d,maxcentfrequency[%d]=%f\n\n",
		i,fftstate.centfeqpoint[i],i,fftstate.bandwidth[i],i,fftstate.arvcentfreq[i]);
	}
}
void  fft_reduce_gain_CfftAbs(float *fftdata,float *data,int size,int datalen)
{
    int i=0;
    for(i=0;i<size;i++)
    {
        data[i]=fftdata[i]-(20*log10(size)-20*log10(datalen));
    }

}
void fft_find_midpoint(float *data,int datalen)
{
    int i=0;
    float max;
    int zuobiao;
    max=data[0];
    for(i=0;i<datalen;i++)
    {

        if(max<data[i])
        {

           max=data[i];
           zuobiao=i;

        }
    }
    fftstate.maximum_x=zuobiao;
}

signalnum_flag  fft_fuzzy_computing(int threshordnum,short *iqdata,int32_t fftsize,int datalen,int *flagcheck)
{
    printf_debug("\n\n****************fft_fuzzy_computing******************************\n");
     printf_debug("iqdata=%d,iqdata[1]=%d\n",iqdata[0],iqdata[1]);
    memset(fftdata.mozhi,0,sizeof(float)*N );
	memset(fftdata.smoothdata,0,sizeof(float)*N );
	memset(fftdata.wavdata,0,sizeof(short)*(2*N) );
	//memset(fftdata.hann,0,sizeof(float)*N );
	memset(fftstate.y,0,sizeof(int)*SIGNALNUM );
	memset(fftstate.z,0,sizeof(int)*SIGNALNUM );
	memset(fftstate.centfeqpoint,0,sizeof(int)*SIGNALNUM );
    memset(fftstate.arvcentfreq,0,sizeof(float)*SIGNALNUM);
    memset(fftstate.bandwidth   ,0,sizeof(float)*SIGNALNUM);
    fftstate.cenfrepointnum=0;
    fftstate.Threshold=0;
    fftstate.Bottomnoise=0;
    fftstate.maxcentfrequency=0;
    fftstate.maximum_x=0;
    signalnum_flag signalflg=0;
    fft_fftw_calculate(iqdata,fftsize,datalen,fftdata.mozhi);
    smooth2(fftdata.mozhi,fftsize,fftdata.smoothdata);

    float minvalue;
    float maxvalue;
    findBottomnoisenomax(fftdata.smoothdata,threshordnum,&fftstate.Bottomnoise,&fftstate.Threshold,fftsize,&maxvalue,&minvalue);
    signalflg=findCentfreqpoint(fftdata.smoothdata,fftsize, fftstate.centfeqpoint,&fftstate.Threshold ,&fftstate.cenfrepointnum,fftstate.y,fftstate.z,&maxvalue);
    if(signalflg==SIGNALNUM_ABNORMAL)
    {
       return SIGNALNUM_ABNORMAL; 
    }
    int signalnum;
    int i=0;
    printf_debug("===============================fftstate.cenfrepointnum=%d,",fftstate.cenfrepointnum);
    for(i=0;i<fftstate.cenfrepointnum;i++)
    {
        if(fftstate.z[i]-fftstate.y[i]<(fftsize/15))
        {
            //窄带信号
            printf_debug("============窄带信号===============\n");
            printf_debug("窄带信号 fftstate.y[i]-fftstate.z[i]=%d\n",fftstate.z[i]-fftstate.y[i]);

            memset(fftdata.mozhi,0,sizeof(float)*narrowbandlen );
            memset(fftdata.smoothdata,0,sizeof(float)*narrowbandlen );
            memset(fftdata.wavdata,0,sizeof(short)*(2*narrowbandlen) );
            //memset(fftdata.hann,0,sizeof(float)*narrowbandlen );
            memset(fftstate.y,0,sizeof(int)*SIGNALNUM );
            memset(fftstate.z,0,sizeof(int)*SIGNALNUM );
            memset(fftstate.centfeqpoint,0,sizeof(int)*SIGNALNUM );
            memset(fftstate.arvcentfreq,0,sizeof(float)*SIGNALNUM);
            memset(fftstate.bandwidth   ,0,sizeof(float)*SIGNALNUM);
            fftstate.cenfrepointnum=0;
            fftstate.Threshold=0;
            fftstate.Bottomnoise=0;
            fftstate.maxcentfrequency=0;
            fftstate.maximum_x=0;
            fft_fftw_calculate(iqdata,narrowbandlen,narrowbandlen*2,fftdata.mozhi);
            smooth2(fftdata.mozhi,narrowbandlen,fftdata.smoothdata);
#ifdef PLAT_FORM_ARCH_X86
            writefileArr("firstsmoothdatahann.txt",fftdata.smoothdata, narrowbandlen);
            writefileArr("firstmozhihann.txt",fftdata.mozhi, narrowbandlen);

#else
           // writefileArr("/run/firstsmoothdatahann.txt",fftdata.smoothdata, fftsize);
           // writefileArr("/run/firstmozhihann.txt",fftdata.mozhi, fftsize);
#endif
            fftstate.Threshold=0;
            fftstate.Bottomnoise=0;
            findBottomnoiseprecise(fftdata.smoothdata,threshordnum,&fftstate.Bottomnoise,&fftstate.Threshold,narrowbandlen,&maxvalue,&minvalue); 
            signalflg=findCentfreqpoint(fftdata.smoothdata,narrowbandlen, fftstate.centfeqpoint,&fftstate.Threshold ,&fftstate.cenfrepointnum,fftstate.y,fftstate.z,&maxvalue);
            if(signalflg==SIGNALNUM_ABNORMAL)
            {
               return SIGNALNUM_ABNORMAL; 
            }
            *flagcheck=1;
            calculatecenterfrequency(fftdata.mozhi,narrowbandlen);                   //5 计算中心频率
            int p=0;
            float max;
            max=fftdata.mozhi[p];
            for(p=0;p<narrowbandlen;p++)
            {
                if(max<fftdata.mozhi[p])
                {
                   // printf_debug("fftdata.mozhi[%d]=%f,",p,fftdata.mozhi[p]);
                   max= fftdata.mozhi[p];
                   fftstate.maximum_x=p;
                }
            }
            
            //float temp;
            //temp=(max-maxvalue)*2/3;
           // printf_debug("max=%f,tempdbvalue=%f\n",max,temp);
           // calculatebandwidth2(fftdata.smoothdata,narrowbandlen,&temp,&maxvalue);
           
            float temp;
            temp=(maxvalue-fftstate.Threshold)/3;
            calculatebandwidth(fftdata.smoothdata,fftsize,&temp);
            printf_debug("\n\n*********************模糊数据****************************\n");
            fft_calculate_finaldata();
            fft_find_midpoint(fftdata.smoothdata,narrowbandlen);
        }else{
            printf_debug("\n============宽带信号===============\n");
#ifdef PLAT_FORM_ARCH_X86
            writefileArr("firstsmoothdatahann.txt",fftdata.smoothdata, fftsize);
            writefileArr("firstmozhihann.txt",fftdata.mozhi, fftsize);
        
#else
           // writefileArr("/run/firstsmoothdatahann.txt",fftdata.smoothdata, fftsize);
           // writefileArr("/run/firstmozhihann.txt",fftdata.mozhi, fftsize);
#endif

            calculatecenterfrequency(fftdata.mozhi,fftsize);                   //5 计算中心频率
            int p=0;
            float max;
            max=fftdata.mozhi[p];
            for(p=0;p<fftsize;p++)
            {
                if(max<fftdata.mozhi[p])
                {
                   // printf_debug("fftdata.mozhi[%d]=%f,",p,fftdata.mozhi[p]);
                   max= fftdata.mozhi[p];
                   fftstate.maximum_x=p;
                }
            }
            
            float temp;
            temp=(max-maxvalue)*2/3;
            printf_debug("max=%f,tempdbvalue=%f\n",max,temp);
            
            calculatebandwidth2(fftdata.mozhi,fftsize,&temp,&maxvalue);
            
            printf_debug("\n\n*********************模糊数据****************************\n");
            fft_calculate_finaldata();
            fft_find_midpoint(fftdata.smoothdata,fftsize);
        }
        
    }

}
int fft_Precise_calculation(int threshordnum,short *iqdata,int32_t fftsize,int datalen,int flagcheck)
{
    printf_debug("\n\n****************fft_Precise_calculation******************************\n");
    printf_debug("iqdata=%d,iqdata[1]=%d",iqdata[0],iqdata[1]);
    signalnum_flag signalflg=0;
    N=fftsize;
    short* wavdatafp;
    short* wandateimage;
    memset(fftdata.mozhi,0,sizeof(float)*N );
    memset(fftdata.wavdata,0,sizeof(short)*(2*N) );
   //  memset(fftdata.hann,0,sizeof(float)*N);
    memset(fftstate.centfeqpoint,0,sizeof(int)*SIGNALNUM );
    memset(fftstate.arvcentfreq,0,sizeof(float)*SIGNALNUM);
    memset(fftstate.bandwidth   ,0,sizeof(float)*SIGNALNUM);
    fftstate.Threshold=0;
    fftstate.Bottomnoise=0;
    fftstate.maximum_x=0;
    fft_fftw_calculate(iqdata,fftsize,datalen,fftdata.mozhi);
    smooth2(fftdata.mozhi,N,fftdata.smoothdata);
#ifdef PLAT_FORM_ARCH_X86
    writefileArr("secondsmoothdatahann.txt",fftdata.smoothdata, fftsize);
    writefileArr("secondmozhihann.txt",fftdata.mozhi, fftsize);

#else
   // writefileArr("/run/secondsmoothdatahann.txt",fftdata.smoothdata, fftsize);
   // writefileArr("/run/secondmozhihann.txt",fftdata.mozhi, fftsize);
#endif
    float minvalue;
    float maxvalue;
    
    int i=0;int j=0;int num=0;int num1=0;int p=0;int count=0;
    int firsttemp=0;
    int secondtemp=0;
    int flag=0;
    int multiple=0;
    int firstpoint[fftstate.cenfrepointnum];
    int endpoint[fftstate.cenfrepointnum];
    memset(firstpoint,0,sizeof(int)*fftstate.cenfrepointnum);
    memset(endpoint,0,sizeof(int)*fftstate.cenfrepointnum);
    
    if(flagcheck==1)
    {
        printf_debug("+++++++++++++++窄带信号++++++++++++++\n");
        findBottomnoiseprecise(fftdata.smoothdata,threshordnum,&fftstate.Bottomnoise,&fftstate.Threshold,N,&maxvalue,&minvalue);
        /*窄带信号，即原始处理过程*/
        multiple=fftsize/narrowbandlen;
        printf_debug("fftstate.cenfrepointnum=%d\n",fftstate.cenfrepointnum);
        float impairment=0;
        for(i=0;i<fftstate.cenfrepointnum;i++)  /*计算信号个数*/
        {
                impairment=0;
                printf_debug("fftstate.y[i]=%d,fftstate.z[i]=%d\n",fftstate.y[i],fftstate.z[i]);
                int temp;
                temp=(fftstate.z[i]*multiple-fftstate.y[i]*multiple)/2;
                printf_debug("fftstate.z[i]*multiple=%d,j>=fftstate.y[i]*multiple-temp=%d\n",fftstate.z[i]*multiple,j>=fftstate.y[i]*multiple-temp);
                for(j=fftstate.z[i]*multiple;j>=fftstate.y[i]*multiple-temp;j--)
                {
                    
                    if(fftdata.smoothdata[j]<fftstate.Threshold&&fftdata.smoothdata[j+1]>fftstate.Threshold)
                    {
                        firsttemp=j;
                    }

                }
                for(j=fftstate.y[i]*multiple;j<=fftstate.z[i]*multiple+temp;j++)
                {
                    if(fftdata.smoothdata[j]>fftstate.Threshold&&fftdata.smoothdata[j+1]<fftstate.Threshold)
                    {
                        secondtemp=j;
                    }
                }
                fftstate.centfeqpoint[i]=(secondtemp-firsttemp)/2+firsttemp;//计算中心频点

                printf_debug("firsttemp=%d  ,secondtemp=%d\n",firsttemp,secondtemp);

                
                float max;
                int end;
                max=fftdata.smoothdata[firsttemp];


                for(int p=firsttemp;p<secondtemp;p++)
                {
                    if(max<fftdata.smoothdata[p])
                    {
                       max= fftdata.smoothdata[p];
                       fftstate.maximum_x=p;
                    }
                }
                

                impairment=max-(max-fftstate.Threshold)*2/3;
                printf_debug("impairment=%f\n",impairment);
                for(int p=firsttemp;p<secondtemp;p++)
                {
                   //printf_info("smoothdata[%d]=%f   ,",p,fftdata.smoothdata[p]);

                     
                    fftstate.arvcentfreq[i]=max-CORRECTIONSIGNAL;      //计算中心频率
                    

                    if((fftdata.smoothdata[p]<=impairment)&&(fftdata.smoothdata[p+1]>=impairment))//带宽阈值处
                    { 
                        
                        if(flag == 0)
                        {
                            firstpoint[i]=p+1;  //确定是第一个点
                            printf_debug("p=%d\n",p+1);
                            flag=1;
                            //continue;
                        }

                    }
                    
                    if(flag==1)
                    {
                        if(fftdata.smoothdata[p]>impairment&&fftdata.smoothdata[p+1]<impairment)
                        {

                            end=p;
                           // printf_debug("end=%d\n",end);
                        }
                        if(p==secondtemp-1)
                        {
                            endpoint[i]=end;
                            count++;
                            flag=0;
                        }
                    }
                    
                }
        }
    }else{
        printf_debug("+++++++++++++++宽带信号+++++++++++++=\n");
        findBottomnoisenomax(fftdata.smoothdata,threshordnum,&fftstate.Bottomnoise,&fftstate.Threshold,N,&maxvalue,&minvalue);
        /*宽带信号，新增处理过程*/
        multiple=fftsize/firstfftlen;
        printf_debug("fftstate.cenfrepointnum=%d\n",fftstate.cenfrepointnum);
        float impairment=0;
        for(i=0;i<fftstate.cenfrepointnum;i++)  /*计算信号个数*/
        {   
                impairment=0;
                //printf("fftstate.y[i]=%d,fftstate.z[i]=%d\n",fftstate.y[i],fftstate.z[i]);
                int temp;
                temp=(fftstate.z[i]*multiple-fftstate.y[i]*multiple)/2;
               // printf_debug("fftstate.y[i]*multiple+temp=%d,fftstate.z[i]=%d\n",fftstate.y[i]*multiple+temp,fftstate.z[i]);
                for(j=fftstate.z[i]*multiple;j>=fftstate.y[i]*multiple-temp;j--)
                {
                    if(fftdata.mozhi[j]<maxvalue&&fftdata.mozhi[j+1]>maxvalue)
                    {
                        firsttemp=j;
                    }

                }
                for(j=fftstate.y[i]*multiple;j<=fftstate.z[i]*multiple+temp;j++)
                {
                    if(fftdata.mozhi[j]>maxvalue&&fftdata.mozhi[j+1]<maxvalue)
                    {
                        secondtemp=j;
                    }
                }
                fftstate.centfeqpoint[i]=(secondtemp-firsttemp)/2+firsttemp;//计算中心频点

                printf_warn("firsttemp=%d  ,secondtemp=%d\n",firsttemp,secondtemp);

                
                float max;
                int end;
                max=fftdata.mozhi[firsttemp];


                for(int p=firsttemp;p<secondtemp;p++)
                {
                    if(max<fftdata.mozhi[p])
                    {
                       max= fftdata.mozhi[p];
                       fftstate.maximum_x=p;
                    }
                }
               // impairment=max-(max-minvalue)/3;
                impairment=max-(max-maxvalue)*2/3;
                printf_warn("threedbvalue=%f,dbvalue=%f\n",impairment,(max-maxvalue)*2/3);
                for(int p=firsttemp;p<secondtemp;p++)
                {
                   //printf_info("smoothdata[%d]=%f   ,",p,fftdata.smoothdata[p]);

                     
                    fftstate.arvcentfreq[i]=max-CORRECTIONSIGNAL;      //计算中心频率
                    

                    if((fftdata.mozhi[p]<=impairment)&&(fftdata.mozhi[p+1]>=impairment))//带宽阈值处
                    { 
                        if(flag == 0)
                        {
                            firstpoint[i]=p+1;  //确定是第一个点
                            printf_debug("p=%d\n",p+1);
                            flag=1;
                            //continue;
                        }

                    }
                    //printf_debug("==================flag=%d\n",flag);
                    if(flag==1)
                    {
                        if(fftdata.mozhi[p]>impairment&&fftdata.mozhi[p+1]<impairment)
                        {

                            end=p;
                           // printf_debug("end=%d\n",end);
                        }
                        if(p==secondtemp-1)
                        {
                            endpoint[i]=end;
                            count++;
                            flag=0;
                        }
                    }
                    
                }
            }
    }

    fftstate.cenfrepointnum=count;
    printf_debug("fftstate.cenfrepointnum=%d\n",fftstate.cenfrepointnum);
    for(int i=0;i<fftstate.cenfrepointnum;i++)
    {
        if(endpoint[i]>firstpoint[i]&&firstpoint[i]>0&&endpoint[i]<N)
        {
            fftstate.bandwidth[i]=endpoint[i]-firstpoint[i];   //计算带宽
            printf_debug(" bandwidth=%d,endpoint=%d,firstpoint=%d\n",fftstate.bandwidth[i],endpoint[i],firstpoint[i]);
        }
    }

    
    printf_debug("\n\n*********************最终数据****************************\n");
    fft_calculate_finaldata();

}

/********************************************************************************

函数功能：通过传入的iq数据，计算所输入信号的中心频率，带宽和中心频点
输入：
    int bd; 下发门限；
    short *data;所要传入的iq数据
    int fftize;  所要做的fft大小，
    int datalen;  IQ数据长度的2倍；
               eg:比如数据IQIQ排列，
               IQIQ的datalen等于4；
返回值：
      无返回值；

*********************************************************************************/

void fft_iqdata_handle(int bd,short *data,int fftsize ,int datalen)
{
    if(data==NULL)
    {
        printf_warn("\n\nThe IQ data you entered is empty, please enter again！\n\n");
        return ;
    }
    testfrequency(bd,data,fftsize,datalen);//iq数据，下发门限，fft大小，下发数据长度
}

/********************************************************************************

函数功能：获取频谱数据
输入：
     float *data；
     data是获取到的频谱数据
返回值：
      无返回值；

*********************************************************************************/


float *fft_get_data(uint32_t *len)
{
    *len = N;
    return fftdata.mozhi;

}
/********************************************************************************

函数功能：获取找到的信号的信息
输入：
        无
返回值：
      返回类型fft_result；
      找到的信号的具体信息的结构体

*********************************************************************************/

fft_result *fft_get_result(void)
{
    fftresult.signalsnumber=fftstate.cenfrepointnum;
    fftresult.maximum_x=fftstate.maximum_x;
    memcpy(fftresult.centfeqpoint,fftstate.centfeqpoint,sizeof(int)*SIGNALNUM);
    memcpy(fftresult.bandwidth,fftstate.bandwidth,sizeof(int)*SIGNALNUM);
    memcpy(fftresult.arvcentfreq,fftstate.arvcentfreq,sizeof(float)*SIGNALNUM);
    return &fftresult;
}

int testfrequency(int threshordnum,short *iqdata,int32_t fftsize,int datalen)
{
    int flag=0;
    signalnum_flag signalflg=0;
    if(fftsize<=firstfftlen)
    {
        signalflg=fft_fuzzy_computing(threshordnum,iqdata,fftsize,datalen,&flag);
        if(signalflg==SIGNALNUM_ABNORMAL)
        {
            printf_note("There are too many signals\n");
            return 0;
        }
    }else if(fftsize>firstfftlen){
    signalflg= fft_fuzzy_computing(threshordnum,iqdata,firstfftlen,2*firstfftlen,&flag);
    if(signalflg==SIGNALNUM_ABNORMAL)
    {
        printf_note("There are too many signals\n");
        return 0;
    }
    fft_Precise_calculation(threshordnum,iqdata,fftsize,datalen,flag);
  }
}

void xulitestfft(void)
{
    double costtime;
    clock_t start,end;
    start=clock();
    fft_init();
    short *data=(short*)malloc(sizeof(short)*2*N);
    memset(data,0,sizeof(short)*2*N );

    int fftsize=1024*1024;
    int i=0;
    fft_result *temp;
    Verificationfloat("rawdata0828.txt",data,2*1024*1024);
    //Verificationfloat("rawdata0813.txt",data,2*1024*1024);
    fft_iqdata_handle(0,data,fftsize,2*1024*1024);//下发门限，iq数据，fft大小，下发数据长度
 
    temp=fft_get_result();
    printf_debug("temp=%d",temp->maximum_x);
    fft_exit(); 
    end=clock();
    costtime=(double)(end-start)/CLOCKS_PER_SEC;
    printf_debug("costtime=%.20lf\n",costtime);
    return ;
	
}


