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






//#define    INTERVALNUM       20 //15000//采样间隔点数30000
//#define   SHIELDPOINTS        10  //屏蔽点数
//#define    INTERVALNUMTOW           2//采样间隔点数30000
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
	
	float *maxsamlpe;
	int *zuobiao;
	float *mozhi;
	complexType *x;
	complexType *y;
	float *smoothdata;
	float *fftphoto;
	short *wavdata;
	float *PartialDerivative;
	int *xx;
	float *yy;
	float *z;
	float *hann;
	
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
	long i = 0, j = 0, k = 0, m = 0;
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
		amplitude[i] = 20.0 * log10(sqrt(x[i].Real*x[i].Real + x[i].Imag*x[i].Imag));   
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
		printf("%4d  %8.8lf ", i, x[i].Real);
		if (x[i].Imag >= 0.0001)printf(" +%8.488fj\n", x[i].Imag);
		else if (fabs(x[i].Imag)<0.0001) printf("\n");
		else printf("  %8.8fj\n", x[i].Imag);
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
		printf("%4d  %8.8lf\n", i, x[i]);
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

void findcomplexcentfrequencypoint(float *data,float* maxaverage,float* minaverage)  //找到信号中的最大值和最小值，求平均得到平均最大值和最小值，并选择计算寻找中心频点的方式
{
	int  count;
	float max[12];
	float min[12];
	for(int i=0;i<12;i++)
	{
		max[i]=data[0];
		min[i]=data[0];	
	}	
	for(int i=0;i<N;i++)
	{
		if(max[0]<data[i])
		{
			max[0]=data[i];
		}
		else if(min[0]>data[i])
		{
			min[0]=data[i];
		}
	}

	for(int i=0;i<N;i++)
	{
		if(max[1]<data[i]&&data[i]!=max[0])
		{
			max[1]=data[i];
		}
		else if(min[1]>data[i]&&data[i]!=min[0])
		{
			min[1]=data[i];
		}

	}

	for(int i=0;i<N;i++)
	{
		if(max[2]<data[i]&&data[i]!=max[0]&&data[i]!=max[1])
		{
			max[2]=data[i];
		}
		else if(min[2]>data[i]&&data[i]!=min[0]&&data[i]!=min[1])
		{
			min[2]=data[i];
		}

	}

	for(int i=0;i<N;i++)
	{
		if(max[3]<data[i]&&data[i]!=max[0]&&data[i]!=max[1]&&data[i]!=max[2])
		{
			max[3]=data[i];
		}
		else if(min[3]>data[i]&&data[i]!=min[0]&&data[i]!=min[1]&&data[i]!=min[2])
		{
			min[3]=data[i];
		}

	}
	for(int i=0;i<N;i++)
	{
		if(max[4]<data[i]&&data[i]!=max[0]&&data[i]!=max[1]&&data[i]!=max[2]&&data[i]!=max[3])
		{
			max[4]=data[i];
		}
		else if(min[4]>data[i]&&data[i]!=min[0]&&data[i]!=min[1]&&data[i]!=min[2]&&data[i]!=min[3])
		{
			min[4]=data[i];
		}

	}
	for(int i=0;i<N;i++)
	{
		if(max[5]<data[i]&&data[i]!=max[0]&&data[i]!=max[1]&&data[i]!=max[2]&&data[i]!=max[3]&&data[i]!=max[4])
		{
			max[5]=data[i];
		}
		else if(min[5]>data[i]&&data[i]!=min[0]&&data[i]!=min[1]&&data[i]!=min[2]&&data[i]!=min[3]&&data[i]!=min[4])
		{
			min[5]=data[i];
		}

	}
	for(int i=0;i<N;i++)
	{
		if(max[6]<data[i]&&data[i]!=max[0]&&data[i]!=max[1]&&data[i]!=max[2]&&data[i]!=max[3]&&data[i]!=max[4]&&data[i]!=max[5])
		{
			max[6]=data[i];
		}
		else if(min[6]>data[i]&&data[i]!=min[0]&&data[i]!=min[1]&&data[i]!=min[2]&&data[i]!=min[3]&&data[i]!=min[4]&&data[i]!=min[5])
		{
			min[6]=data[i];
		}

	}
	for(int i=0;i<N;i++)
	{
		if(max[7]<data[i]&&data[i]!=max[0]&&data[i]!=max[1]&&data[i]!=max[2]&&data[i]!=max[3]&&data[i]!=max[4]&&data[i]!=max[5]&&data[i]!=max[6])
		{
			max[7]=data[i];
		}
		else if(min[7]>data[i]&&data[i]!=min[0]&&data[i]!=min[1]&&data[i]!=min[2]&&data[i]!=min[3]&&data[i]!=min[4]&&data[i]!=min[5]&&data[i]!=min[6])
		{
			min[7]=data[i];
		}

	}
	for(int i=0;i<N;i++)
	{
		if(max[8]<data[i]&&data[i]!=max[0]&&data[i]!=max[1]&&data[i]!=max[2]&&data[i]!=max[3]&&data[i]!=max[4]&&data[i]!=max[5]&&data[i]!=max[6]&&data[i]!=max[7])
		{
			max[8]=data[i];
		}
		else if(min[8]>data[i]&&data[i]!=min[0]&&data[i]!=min[1]&&data[i]!=min[2]&&data[i]!=min[3]&&data[i]!=min[4]&&data[i]!=min[5]&&data[i]!=min[6]&&data[i]!=min[7])
		{
			min[8]=data[i];
		}

	}
	for(int i=0;i<N;i++)
	{
		if(max[9]<data[i]&&data[i]!=max[0]&&data[i]!=max[1]&&data[i]!=max[2]&&data[i]!=max[3]&&data[i]!=max[4]&&data[i]!=max[5]&&data[i]!=max[6]&&data[i]!=max[7]&&data[i]!=max[8])
		{
			max[9]=data[i];
		}
		else if(min[9]>data[i]&&data[i]!=min[0]&&data[i]!=min[1]&&data[i]!=min[3]&&data[i]!=min[3]&&data[i]!=min[4]&&data[i]!=min[5]&&data[i]!=min[6]&&data[i]!=min[7]&&data[i]!=min[8])
		{
			min[9]=data[i];
		}

	}
	for(int i=0;i<N;i++)
	{
		if(max[10]<data[i]&&data[i]!=max[0]&&data[i]!=max[1]&&data[i]!=max[2]&&data[i]!=max[3]&&data[i]!=max[4]&&data[i]!=max[5]&&data[i]!=max[6]&&data[i]!=max[7]&&data[i]!=max[8]&&data[i]!=max[9])
		{
			max[10]=data[i];
		}
		else if(min[10]>data[i]&&data[i]!=min[0]&&data[i]!=min[1]&&data[i]!=min[2]&&data[i]!=min[3]&&data[i]!=min[4]&&data[i]!=min[5]&&data[i]!=min[6]&&data[i]!=min[7]&&data[i]!=min[8]&&data[i]!=min[9])
		{
			min[10]=data[i];
		}

	}
	for(int i=0;i<N;i++)
	{
		if(max[11]<data[i]&&data[i]!=min[0]&&data[i]!=max[1]&&data[i]!=max[2]&&data[i]!=max[3]&&data[i]!=max[4]&&data[i]!=max[5]&&data[i]!=max[6]&&data[i]!=max[7]&&data[i]!=max[8]&&data[i]!=max[9]&&data[i]!=max[10])
		{
			max[11]=data[i];
		}
		else if(min[11]>data[i]&&data[i]!=min[0]&&data[i]!=min[1]&&data[i]!=min[2]&&data[i]!=min[3]&&data[i]!=min[4]&&data[i]!=min[5]&&data[i]!=min[6]&&data[i]!=min[7]&&data[i]!=min[8]&&data[i]!=min[9]&&data[i]!=min[10])
		{
			min[11]=data[i];
		}

	};
	*maxaverage=0;
	*minaverage=0;
	for(int i=0;i<12;i++)
	{
		*maxaverage+=max[i];
		*minaverage+=min[i];
		if(i==11)
		{
			*maxaverage=*maxaverage/12;
			*minaverage=*minaverage/12;
			//printf("*maxaverage=%f",*maxaverage);
			//printf("8minaverage=%f",*minaverage);
			
		}
		
	}
	
}
void  findCentfreqpoint(float *data, int pointnum,int * centfeqpoint,float *Threshold ,int * cenfrepointnum,int *y,int *z,int  *Centerpoint)//大于阈值的第一个 z小于阈值的第一个数 ,计算中心频点
{
	printf_debug("findCentfreqpoint \n");
	int *h;	
	h=centfeqpoint;
		
	int p[SIGNALNUM]={0};
	
	int q[SIGNALNUM]={0};
	int j =0,x=0;
	//int centpointnum1;
	int flag=0;
	
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
	}
    printf_debug("*=================Threshold=%f",*Threshold);
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
    
    SHIELDPOINTS=findpointmax/2;
    printf_debug("\n\n===============SHIELDPOINTS=%d\n\n",SHIELDPOINTS);

	for(int i=0;i<*cenfrepointnum;i++)
	{	
	    
		
		//a[i]=(z[i]-y[i])/2;
		printf_debug("q[i] = %d p[i] = %d q[i]-p[i]=%d  i=%d    \n",q[i],p[i],(q[i]-p[i]),i);
		//printf("a[i]=%d\n",a[i]);
		if(((q[i]-p[i]))>SHIELDPOINTS)                     //修改1
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
}
void  calculatecenterfrequency(float *fftdata,int fftnum)
{
	printf_debug("calculatecenterfrequency\n");
	int j=0;
	printf_debug("cenfrepointnum=%d\n",fftstate.cenfrepointnum);
	/*for(int i=0;i<fftstate.cenfrepointnum;i++)
	{
		float sum=0.0;

		for(j=fftstate.centfeqpoint[i]-20;j<fftstate.centfeqpoint[i]+20;j++)
		{
			sum+=fftdata[j];
		}
		
		fftstate.arvcentfreq[i]=sum/21;
		
	}*/
	for(int i=0;i<fftstate.cenfrepointnum;i++)
	{
		fftstate.arvcentfreq[i]=fftdata[fftstate.centfeqpoint[i]];	
	}
	for(int i=0;i<fftstate.cenfrepointnum;i++)
	{
		
		printf_warn("arvcentfreq[%d]=%lf\n",i,fftstate.arvcentfreq[i]);
	}
}

//原始数据  原始数据个数 平均后的中频频率 第一个点的集合      第二个点的集合    平均后的中心频率
void  calculatebandwidth(float *fftdata,int fftnum )
{	
	int j=0;
	int first=0;
	int end=0;
	int firstpoint[fftstate.cenfrepointnum];
	int endpoint[fftstate.cenfrepointnum];
	int flag=0;
	float threedbpoint;
	float maxvalue;
	float minvalue;
	float maxz;
	
	maxz=fftdata[0];
	
	for(int i=0;i<fftnum;i++)
	{
		
		if(maxz<fftdata[i])
		{
			maxz=fftdata[i];
		}
	}
	printf_debug("maxz=%f\n,",maxz);
	findcomplexcentfrequencypoint(fftdata,&maxvalue,&minvalue);

	//findcentfrequencyabsmaxmin(threedbpoint);
	for(int i=0;i<fftstate.cenfrepointnum;i++)
	{	
		//threedbpoint=(fftstate.arvcentfreq[i]-minvalue)*0.707496+minvalue;
		//threedbpoint=(fftstate.arvcentfreq[i]-fftstate.Bottomnoise)*0.707496+fftstate.Bottomnoise;
		//threedbpoint=(maxz-fftstate.Bottomnoise)*0.707496+fftstate.Bottomnoise;
		//threedbpoint=maxz-5;
        threedbpoint = fftstate.arvcentfreq[i]-4;
		
		printf_debug("调试信息 maxz : %f fftstate.Bottomnoise:%f threedbpoint:%f\n",maxz,fftstate.Bottomnoise,threedbpoint);
		for(j=fftstate.y[i];j<fftstate.z[i];j++)
		{
		
			//if(fftdata[j]<=0.707946*arvcentfreq[i]&&fftdata[j+1]>0.707946*arvcentfreq[i])
			if(fftdata[j]<=threedbpoint&&fftdata[j+1]>threedbpoint)
			{ 
				if(flag == 0)
				{
						firstpoint[i]=j+1;  //确定是第一个点
						j=fftstate.z[i]-((fftstate.z[i]-fftstate.y[i])/2);
						flag=1;
						continue;

				}

			}
			else if(fftdata[j]>=threedbpoint&&fftdata[j+1]<threedbpoint)
			{
					endpoint[i]=j;
					flag=0;
					break;
			}
		}
	}
	printf_debug("threedbpoint=%f  minvalue=%f  \n",threedbpoint,minvalue);
	for(int i=0;i<fftstate.cenfrepointnum;i++)
	{
		if(endpoint[i]>firstpoint[i]&&firstpoint[i]>0&&endpoint[i]<N)
		{
			fftstate.bandwidth[i]=endpoint[i]-firstpoint[i];
			printf_debug(" bandwidth=%d,endpoint=%d,firstpoint=%d\n",fftstate.bandwidth[i],endpoint[i],firstpoint[i]);
			
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


void findBottomnoise(float *mozhi,int xiafamenxian,float *Bottomnoise,float *Threshold )
{
	int sum=0;
	float minvalue;
	float maxvalue;
	
	findcomplexcentfrequencypoint(mozhi,&maxvalue,&minvalue);
	printf_debug("minvalue=%f,\n",minvalue);
	//Elementsortingfindthreshold(mozhi,data);
	
	*Bottomnoise=minvalue;
	printf_debug("Bottomnoise=%f,",*Bottomnoise);
	*Threshold=*Bottomnoise+xiafamenxian;
	
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
		printf_warn("构造失败，已知点数太少");
		
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
	for(int i=0;i<N;i++)
	{
		ret[i]=0.5*(1-cos(2*3.1415926*(float)i/(N-1)));
	}
	memcpy(w,ret,sizeof(float)*N);
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
	fftdata.maxsamlpe=(float*)malloc(sizeof(float)*(N/INTERVALNUMTOW));
	fftdata.zuobiao=(int*)malloc(sizeof(int)*(N/INTERVALNUMTOW ));
	fftdata.mozhi=(float*)malloc(sizeof(float)*N );
	fftdata.x=(complexType*)malloc(sizeof(complexType)*N );
	fftdata.y=(complexType*)malloc(sizeof(complexType)*N );
	fftdata.smoothdata=(float*)malloc(sizeof(float)*N);
	fftdata.fftphoto=(float*)malloc(sizeof(float)*N);    //fftphoto用于上传数据在FFTdata未改变之前
	fftdata.wavdata=(short*)malloc(sizeof(short)*(2*N)); 
	
	fftdata.PartialDerivative=(float*)malloc(sizeof(float)*(N/INTERVALNUMTOW));
	fftdata.xx=(int*)malloc(sizeof(int)*N);
	fftdata.yy=(float*)malloc(sizeof(float)*N);
	fftdata.z=(float*)malloc(sizeof(float)*N);
	fftdata.hann=(float*)malloc(sizeof(float)*N);
	
	memset(fftdata.mozhi,0,sizeof(float)*N );
	memset(fftdata.x,0,sizeof(complexType)*N );
	memset(fftdata.y,0,sizeof(complexType)*N );
	memset(fftdata.smoothdata,0,sizeof(float)*N );
	memset(fftdata.fftphoto,0,sizeof(float)*N );
	memset(fftdata.wavdata,0,sizeof(short)*(2*N) );
	memset(fftdata.maxsamlpe,0,sizeof(float)*(N/INTERVALNUMTOW) );
	memset(fftdata.zuobiao,0,sizeof(int)*(N/INTERVALNUMTOW) );
	memset(fftdata.PartialDerivative,0,sizeof(float)*(N/INTERVALNUMTOW));
	memset(fftdata.xx,0,sizeof(int)*N );
	memset(fftdata.yy,0,sizeof(float)*N );
	memset(fftdata.z,0,sizeof(float)*N );
	memset(fftdata.hann,0,sizeof(float)*N );
	
	memset(fftstate.y,0,sizeof(int)*SIGNALNUM );
	memset(fftstate.z,0,sizeof(int)*SIGNALNUM );
	memset(fftstate.centfeqpoint,0,sizeof(int)*SIGNALNUM );
    memset(fftstate.arvcentfreq,0,sizeof(float)*SIGNALNUM);
    memset(fftstate.bandwidth   ,0,sizeof(float)*SIGNALNUM);
    
	fftstate.cenfrepointnum=0;
	//maxcentfeqpoint=0;
	fftstate.Threshold=0;
	fftstate.Centerpoint=0;
	fftstate.Bottomnoise=0;
	fftstate.maxcentfrequency=0;
	fftstate.interval=0;	
}
void fft_exit(void)
{
	
	SAFE_FREE(fftdata.maxsamlpe);
	SAFE_FREE(fftdata.zuobiao);
	SAFE_FREE(fftdata.mozhi);
	SAFE_FREE(fftdata.x);
	SAFE_FREE(fftdata.y);
	SAFE_FREE(fftdata.smoothdata);
	SAFE_FREE(fftdata.wavdata);
	SAFE_FREE(fftdata.fftphoto);
	SAFE_FREE(fftdata.hann);
	SAFE_FREE(fftdata.PartialDerivative);
	SAFE_FREE(fftdata.xx);
	SAFE_FREE(fftdata.yy);
	SAFE_FREE(fftdata.z);
	
}
//int testfrequency(const char* filename,int threshordnum,int fftlen,short *iqdata)
void testfrequency(int threshordnum,short *iqdata,int fftsize,int datalen)
{
    N=fftsize;
	fftstate.interval=0;	
	fftstate.interval=INTERVALNUM;
	short* wavdatafp;
	short* wandateimage;

	memset(fftdata.mozhi,0,sizeof(float)*N );
	memset(fftdata.x,0,sizeof(complexType)*N );
	memset(fftdata.y,0,sizeof(complexType)*N );
	memset(fftdata.smoothdata,0,sizeof(float)*N );
	memset(fftdata.fftphoto,0,sizeof(float)*N );
	memset(fftdata.wavdata,0,sizeof(short)*(2*N) );
	memset(fftdata.maxsamlpe,0,sizeof(float)*(N/INTERVALNUMTOW) );
	memset(fftdata.zuobiao,0,sizeof(int)*(N/INTERVALNUMTOW));
	memset(fftdata.PartialDerivative,0,sizeof(float)*(N/INTERVALNUMTOW) );
	memset(fftdata.xx,0,sizeof(int)*N );
	memset(fftdata.yy,0,sizeof(float)*N );
	memset(fftdata.z,0,sizeof(float)*N );
	memset(fftdata.hann,0,sizeof(float)*N );
	
	memset(fftstate.y,0,sizeof(int)*SIGNALNUM );
	memset(fftstate.z,0,sizeof(int)*SIGNALNUM );
	memset(fftstate.centfeqpoint,0,sizeof(int)*SIGNALNUM );
    memset(fftstate.arvcentfreq,0,sizeof(float)*SIGNALNUM);
    memset(fftstate.bandwidth   ,0,sizeof(float)*SIGNALNUM);
	fftstate.cenfrepointnum=0;
	fftstate.Threshold=0;
	fftstate.Centerpoint=0;
	fftstate.Bottomnoise=0;
	fftstate.maxcentfrequency=0;
	hannwindow(N,fftdata.hann);
    
	wavdatafp=iqdata;
	wandateimage=iqdata+1;
	int j=0;
	int q=0;	
	for(int i=0;i<2*fftsize;i+=2)
	{
		if(i<datalen)
		{
			fftdata.x[j++].Real=(*wavdatafp);	
			fftdata.x[q++].Imag =(*wandateimage);
			wavdatafp+=2;
			wandateimage+=2;
			
		}else{
			
			fftdata.x[j++].Real=0.0;
			fftdata.x[q++].Imag =0.0;	
		}
	}
   /*wavrtranstxt(filename,datalen);     //数据转换
   

	wavdatafp=fftdata.wavdata+87;
	wandateimage=fftdata.wavdata+88;
	int j=0;
	int q=0; 
	for(int i=0;i<(2*N);i+=2)
	{
		if(i<datalen)
		{
			fftdata.x[j++].Real=(*wavdatafp);	
			fftdata.x[q++].Imag =(*wandateimage);
			wavdatafp+=2;
			wandateimage+=2;

		}else{
			
			fftdata.x[j++].Real=0.0;
			fftdata.x[q++].Imag =0.0;
		}
	}*/

	
	
	for(int i=0;i<N;i++)
	{
		fftdata.x[i].Real=fftdata.x[i].Real*fftdata.hann[i];
		fftdata.x[i].Imag=fftdata.x[i].Imag*fftdata.hann[i];
		
	}
	Cfft(fftdata.x, N, fftdata.y);            // 1快速傅里叶变换  
	CfftAbs(fftdata.y,N,fftdata.mozhi);	// 2 计算20log10(abs),存进模值里
	Rfftshift(fftdata.mozhi, N);
	writefileArr("mozhi0801.txt",fftdata.mozhi, N);
    
	smooth(fftdata.mozhi,N,fftdata.smoothdata);
    writefileArr("smoothdata0801.txt",fftdata.smoothdata, N);
    
	findBottomnoise(fftdata.smoothdata,threshordnum,&fftstate.Bottomnoise,&fftstate.Threshold);   //计算底噪
	
	findCentfreqpoint(fftdata.smoothdata,N, fftstate.centfeqpoint,&fftstate.Threshold ,&fftstate.cenfrepointnum,fftstate.y,fftstate.z,&fftstate.Centerpoint);
	
	calculatecenterfrequency(fftdata.smoothdata,N);                   //5 计算中心频率
	
	calculatebandwidth(fftdata.smoothdata,N);  
	//6 计算带宽
	int num1=0;
	int num2=0;
	int num3=0;
	int num=0;
	for(int i=0;i<fftstate.cenfrepointnum;i++)
	{
	    printf("fftstate.centfeqpoint[i]=%d,fftstate.bandwidth[]=%d,fftstate.arvcentfreq[i]=%dm\n",fftstate.centfeqpoint[i],
                                                                                                        fftstate.bandwidth[i],
                                                                                                        fftstate.centfeqpoint[i]);
		if((fftstate.arvcentfreq[i]>0 )&&(fftstate.bandwidth[i]>0)&&fftstate.bandwidth[i]<N)
		{
			
			fftstate.arvcentfreq[num1++]=fftstate.arvcentfreq[i];
			fftstate.bandwidth[num2++]=fftstate.bandwidth[i];
			fftstate.centfeqpoint[num3++]=fftstate.centfeqpoint[i];	

			num++;
			
		}
	}
	fftstate.cenfrepointnum=num;
	printf_debug("\n\n*********************最终数据****************************\n");
	for(int i=0;i<fftstate.cenfrepointnum;i++)
	{
		
		
		printf_warn("fftstate.centfeqpoint[%d]=%d,fftstate.bandwidth[%d]=%d,maxcentfrequency[%d]=%f\n\n",
		i,fftstate.centfeqpoint[i],i,fftstate.bandwidth[i],i,fftstate.arvcentfreq[i]);
		
	}

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


unsigned int fft_get_data(float *data)
{
    
    memcpy(data,fftdata.mozhi,sizeof(float)*N);
    return N;

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
    memcpy(fftresult.centfeqpoint,fftstate.centfeqpoint,sizeof(int)*SIGNALNUM);
    memcpy(fftresult.bandwidth,fftstate.bandwidth,sizeof(int)*SIGNALNUM);
    memcpy(fftresult.arvcentfreq,fftstate.arvcentfreq,sizeof(float)*SIGNALNUM);
    return &fftresult;
}

void xulitestfft(void)
{
	double costtime;
	clock_t start,end;
	start=clock();
	fft_init();
    short *data=(short*)malloc(sizeof(short)*2*N);
    memset(data,0,sizeof(short)*2*N );
    
	int fftsize=8*1024;
    int i=0;
    fft_result *temp;

    Verificationfloat("rawdata.txt",data,1024*1024);
	//testfrequency("50miq.wav",10,NULL,fftsize,1024*1024);
	fft_iqdata_handle(6,data,8*1024 ,16*1024);//下发门限，iq数据，fft大小，下发数据长度
     printf_debug("\n\n=====================tempytest===============================\n");
    temp=fft_get_result();
    printf_debug("temp.signalsnumber=%d\n",temp->signalsnumber);

	for(int i=0;i<temp->signalsnumber;i++)
	{
		
		
		printf_warn("temp.centfeqpoint[%d]=%d,temp.bandwidth[%d]=%d,temp[%d]=%f\n\n",
		i,temp->centfeqpoint[i],i,temp->bandwidth[i],i,temp->arvcentfreq[i]);
		
	}

	fft_exit(); 
	end=clock();
	costtime=(end-start)/CLOCKS_PER_SEC;
	printf_debug("costtime=%.20lf\n",costtime);
	return ;
	
}





