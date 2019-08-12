
#include "config.h"


static RF_CHANNEL_SN rf_bw_data[]= {
    {HPF1,RF_START_0M,RF_END_80M},
    {HPF2,RF_START_0M,RF_END_160M},
    {HPF3,RF_START_145M,RF_END_320M},
    {HPF4,RF_START_145M,RF_END_630M},
    {HPF5,RF_START_650M,RF_END_1325M},
    {HPF6,RF_START_1150M,RF_END_2750M},
    {HPF7,RF_START_2100M,RF_END_3800M},
    {HPF8,RF_START_2700M,RF_END_6000M}
};

float pre_buf[8] ={0,0.5,1,2,4,8,16,31.5};
float pos_buf[8] ={0,0.5,1,2,4,8,16,31.5};

float db_array[64];
int   db_arrange[64];

void rf_db_arrange()       //剔除衰减库里重复的元素
{
   int i = 0,j=0;
   for(i=0;i<64;i++){
       if((int)db_array[i] != (int)db_array[i+1]){
            db_arrange[j++] = (int)db_array[i];
       }
   }
}
void BubbleSort(float a[],int n)   //将数从小到大排序
{
    int i,j;
    float temp;
    for(i=0;i<n-1;i++)        //比较的趟数
        for(j=0;j<n-1-i;j++)  //每趟比较的次数
        {
            //由小到大排序
            if(a[j]>a[j+1])  //a[j]<a[j+1]由大到小排序
            {
                temp=a[j];
                a[j]=a[j+1];
                a[j+1]=temp;
            }
        }
}

int rf_db_attenuation_init()        //生成衰减库
{
   uint8_t i,j,k = 0;
   for(i=0;i<8;i++){
       for(j=0;j<8;j++){
            db_array[k++] = pre_buf[i] + pos_buf[j];
       }
   }
   BubbleSort(db_array,64);
   rf_db_arrange();
}

int  rf_db_select(uint8_t db_attenuation){     //找出衰减库里DB值值
    uint8_t i;
    for(i = 0;i<64;i++){
        if(db_attenuation == db_arrange[i+1]) {
            db_attenuation = db_arrange[i+1];
            return db_attenuation;
        }
        else if((db_attenuation >= db_arrange[i]) && (db_attenuation < db_arrange[i+1])){
           if(db_attenuation >= db_arrange[i]) db_attenuation = db_arrange[i];
           return db_attenuation;
        }
    }
    return -1;
}

int count_pre_pos_rf(uint8_t attenuation_val)   //衰减DB值
{
     uint8_t attenuation;
     attenuation = rf_db_select(attenuation_val);
     if(attenuation != -1){
         uint8_t pre,pos;
         for(pre=0;pre<8;pre++){
             for(pos=0;pos<8;pos++){
                 if(attenuation == (uint8_t)(pre_buf[pre] + pos_buf[pos])){
                    gpio_attenuation_rf(pre,pos);
                    printf_note("pre :%d pos :%d\n",pre,pos);
                    return 0;
                  }
             }
         }
     }
     else{
       gpio_attenuation_rf(U10_0_5_DB,U2_31_5_DB); //默认衰减31.5DB
       printf_note("默认设置31.5DB\n"); //默认衰减31.5DB
     }
}


void gpio_select_rf_channel(uint64_t mid_freq)  //射频通道选择
{
    int i = 0;
    int found = 0;
    static uint8_t rf_channel_value = 0;
    if((int64_t)(mid_freq - BAND_WITH_100M) < 0){
        printf_warn("middle freq is less than band, set defaut gpio ctrl pin:2\n");
        if(rf_channel_value != HPF2){
            rf_channel_value = HPF2;
            gpio_control_rf(rf_channel_value);  //2通道  0 - 160M
        }
        return;
    }
    else{
        uint64_t mid_freq_val = mid_freq - BAND_WITH_100M;
        for(i=0;i<ARRAY_SIZE(rf_bw_data);i++){
            printf_note("freq=%llu, s_freq=%llu, end_freq=%llu\n", mid_freq_val, rf_bw_data[i].s_freq_rf, rf_bw_data[i].e_freq_rf);
            if((mid_freq_val > rf_bw_data[i].s_freq_rf) && (mid_freq_val < rf_bw_data[i].e_freq_rf)){
                if(rf_channel_value != rf_bw_data[i].index_rf){   //扫频范围有变化
                    rf_channel_value = rf_bw_data[i].index_rf;    //选择新的通道
                    gpio_control_rf(rf_channel_value);
                }
                found++;
            }
        }
    }
    printf_debug("rf channel found=%d\n", found);
    if(found == 0){
        printf_warn("not find, set defaut gpio ctrl pin:2\n");
        gpio_control_rf(HPF2);
    }
}

int gpio_set(int spidev_index,char val){
    char  spi_offset_num[5];
    char  spi_gpio_value[128];
    int valuefd;
    sprintf(spi_offset_num,"%d",GPIO_BASE_OFFSET+spidev_index);
    sprintf(spi_gpio_value,"/sys/class/gpio/gpio%s/value",spi_offset_num);

    valuefd = open(spi_gpio_value, O_WRONLY);
    if (valuefd < 0){
        printf("Cannot open GPIO value\n"); 
        return -1;
    }

    if(val == 1){
        write(valuefd,"1", 2);
       // printf("higt\n");
    }
    else {
        write(valuefd,"0", 2);
       // printf("low\n");		
    }
    close(valuefd);
    return 0;
}

int gpio_init(int spidev_index){

    char spi_offset_num[5];
    char spi_gpio_direction[128];
    char  spi_gpio_value[128];
    int exportfd, directionfd;

    sprintf(spi_offset_num,"%d",GPIO_BASE_OFFSET+spidev_index);
    sprintf(spi_gpio_direction,"/sys/class/gpio/gpio%s/direction",spi_offset_num);
    sprintf(spi_gpio_value,"/sys/class/gpio/gpio%s/value",spi_offset_num);
    exportfd = open("/sys/class/gpio/export", O_WRONLY);
    if (exportfd < 0)
    {
        printf("Cannot open GPIO to export it\n");
        return -1;
    }
    write(exportfd, spi_offset_num, strlen(spi_offset_num)+1);
    close(exportfd);
    // Update the direction of the GPIO to be an output
    directionfd = open(spi_gpio_direction, O_WRONLY);
    if (directionfd < 0)
    {
        printf("Cannot open GPIO direction it\n");
        return -1;
    }
    write(directionfd, "out", 4);
    close(directionfd);

    // Get the GPIO value ready to be toggled

	gpio_set(spidev_index,0);
    return 0;
}


void gpio_rf_channel(rf_channel channel_val)
{
    switch(channel_val)
    {	
        case HPF1:
        gpio_set(PRI_42422_V1,1);
        gpio_set(PRI_42422_LS,1);		
        gpio_set(PRI_L_42442_V1,1);
        gpio_set(PRI_L_42442_V2,0);	
        gpio_set(SEC_L_42442_V1,0);
        gpio_set(SEC_L_42442_V2,0);
        gpio_set(SEC_42422_V1,0);
        gpio_set(SEC_42422_LS,1);
        break;

        case HPF2:
        gpio_set(PRI_42422_V1,1);
        gpio_set(PRI_42422_LS,1);		
        gpio_set(PRI_L_42442_V1,0);
        gpio_set(PRI_L_42442_V2,1);	
        gpio_set(SEC_L_42442_V1,1);
        gpio_set(SEC_L_42442_V2,1);	
        gpio_set(SEC_42422_V1,0);
        gpio_set(SEC_42422_LS,1);
        break;

        case HPF3:
        gpio_set(PRI_42422_V1,1);
        gpio_set(PRI_42422_LS,1);	
        gpio_set(PRI_L_42442_V1,1);
        gpio_set(PRI_L_42442_V2,1);
        gpio_set(SEC_L_42442_V1,0);
        gpio_set(SEC_L_42442_V2,1);	
        gpio_set(SEC_42422_V1,0);
        gpio_set(SEC_42422_LS,1);		
        break;

        case HPF4:
        gpio_set(PRI_42422_V1,1);
        gpio_set(PRI_42422_LS,1);	
        gpio_set(PRI_L_42442_V1,0);
        gpio_set(PRI_L_42442_V2,0);
        gpio_set(SEC_L_42442_V1,1);
        gpio_set(SEC_L_42442_V2,0);
        gpio_set(SEC_42422_V1,0);
        gpio_set(SEC_42422_LS,1);	
        break;

        case HPF5:
        gpio_set(PRI_42422_V1,1);
        gpio_set(PRI_42422_LS,0);	
        gpio_set(PRI_H_42442_V1,1);
        gpio_set(PRI_H_42442_V2,0);
        gpio_set(SEC_H_42442_V1,0);
        gpio_set(SEC_H_42442_V2,0);
        gpio_set(SEC_42422_V1,1);
        gpio_set(SEC_42422_LS,1);
        break;

        case HPF6:
        gpio_set(PRI_42422_V1,1);
        gpio_set(PRI_42422_LS,0);	
        gpio_set(PRI_H_42442_V1,0);
        gpio_set(PRI_H_42442_V2,1);
        gpio_set(SEC_H_42442_V1,1);
        gpio_set(SEC_H_42442_V2,1);
        gpio_set(SEC_42422_V1,1);
        gpio_set(SEC_42422_LS,1);
        break;

        case HPF7:
        gpio_set(PRI_42422_V1,1);
        gpio_set(PRI_42422_LS,0);	
        gpio_set(PRI_H_42442_V1,1);
        gpio_set(PRI_H_42442_V2,1);
        gpio_set(SEC_H_42442_V1,0);
        gpio_set(SEC_H_42442_V2,1);
        gpio_set(SEC_42422_V1,1);
        gpio_set(SEC_42422_LS,1);
        break;

        case HPF8:
        gpio_set(PRI_42422_V1,1);
        gpio_set(PRI_42422_LS,0);	
        gpio_set(PRI_H_42442_V1,0);
        gpio_set(PRI_H_42442_V2,0);
        gpio_set(SEC_H_42442_V1,1);
        gpio_set(SEC_H_42442_V2,0);
        gpio_set(SEC_42422_V1,1);
        gpio_set(SEC_42422_LS,1);
        break;
        default :
        break;
    }

}

void gpio_rf_pre_reduce(rf_pre_reduce pre_reduce_val)
{
    switch(pre_reduce_val)
    {
        case U10_0_DB:
        gpio_set(SEC_624A_D0,1);
        gpio_set(SEC_624A_D1,1);
        gpio_set(SEC_624A_D2,1);
        gpio_set(SEC_624A_D3,1);
        gpio_set(SEC_624A_D4,1);
        gpio_set(SEC_624A_D5,1);
        break;

        case U10_0_5_DB:
        gpio_set(SEC_624A_D0,0);
        gpio_set(SEC_624A_D1,1);
        gpio_set(SEC_624A_D2,1);
        gpio_set(SEC_624A_D3,1);
        gpio_set(SEC_624A_D4,1);
        gpio_set(SEC_624A_D5,1);
        break;

        case U10_1_DB:
        gpio_set(SEC_624A_D0,1);
        gpio_set(SEC_624A_D1,0);
        gpio_set(SEC_624A_D2,1);
        gpio_set(SEC_624A_D3,1);
        gpio_set(SEC_624A_D4,1);
        gpio_set(SEC_624A_D5,1);
        break;

        case U10_2_DB:
        gpio_set(SEC_624A_D0,1);
        gpio_set(SEC_624A_D1,1);
        gpio_set(SEC_624A_D2,0);
        gpio_set(SEC_624A_D3,1);
        gpio_set(SEC_624A_D4,1);
        gpio_set(SEC_624A_D5,1);
        break;

        case U10_4_DB:
        gpio_set(SEC_624A_D0,1);
        gpio_set(SEC_624A_D1,1);
        gpio_set(SEC_624A_D2,1);
        gpio_set(SEC_624A_D3,0);
        gpio_set(SEC_624A_D4,1);
        gpio_set(SEC_624A_D5,1);
        break;

        case U10_8_DB:
        gpio_set(SEC_624A_D0,1);
        gpio_set(SEC_624A_D1,1);
        gpio_set(SEC_624A_D2,1);
        gpio_set(SEC_624A_D3,1);
        gpio_set(SEC_624A_D4,0);
        gpio_set(SEC_624A_D5,1);
        break;

        case U10_16_DB:
        gpio_set(SEC_624A_D0,1);
        gpio_set(SEC_624A_D1,1);
        gpio_set(SEC_624A_D2,1);
        gpio_set(SEC_624A_D3,1);
        gpio_set(SEC_624A_D4,1);
        gpio_set(SEC_624A_D5,0);
        break;

        case U10_31_5_DB:
        gpio_set(SEC_624A_D0,0);
        gpio_set(SEC_624A_D1,0);
        gpio_set(SEC_624A_D2,0);
        gpio_set(SEC_624A_D3,0);
        gpio_set(SEC_624A_D4,0);
        gpio_set(SEC_624A_D5,0);
        break;
        default :
        break;	
    }	
}


void gpio_rf_pos_reduce(rf_pos_reduce pos_reduce_val)
{
    switch(pos_reduce_val)
    {
        case U2_0_DB:
        gpio_set(PRI_624A_D0,0);
        gpio_set(PRI_624A_D1,1);
        gpio_set(PRI_624A_D2,1);
        gpio_set(PRI_624A_D3,1);
        gpio_set(PRI_624A_D4,1);
        gpio_set(PRI_624A_D5,1);
        break;

        case U2_0_5_DB:
        gpio_set(PRI_624A_D0,1);
        gpio_set(PRI_624A_D1,0);
        gpio_set(PRI_624A_D2,1);
        gpio_set(PRI_624A_D3,1);
        gpio_set(PRI_624A_D4,1);
        gpio_set(PRI_624A_D5,1);
        break;

        case U2_1_DB:
        gpio_set(PRI_624A_D0,1);
        gpio_set(PRI_624A_D1,0);
        gpio_set(PRI_624A_D2,1);
        gpio_set(PRI_624A_D3,1);
        gpio_set(PRI_624A_D4,1);
        gpio_set(PRI_624A_D5,1);
        break;

        case U2_2_DB:
        gpio_set(PRI_624A_D0,1);
        gpio_set(PRI_624A_D1,1);
        gpio_set(PRI_624A_D2,0);
        gpio_set(PRI_624A_D3,1);
        gpio_set(PRI_624A_D4,1);
        gpio_set(PRI_624A_D5,1);
        break;

        case U2_4_DB:
        gpio_set(PRI_624A_D0,1);
        gpio_set(PRI_624A_D1,1);
        gpio_set(PRI_624A_D2,1);
        gpio_set(PRI_624A_D3,0);
        gpio_set(PRI_624A_D4,1);
        gpio_set(PRI_624A_D5,1);
        break;

        case U2_8_DB:
        gpio_set(PRI_624A_D0,1);
        gpio_set(PRI_624A_D1,1);
        gpio_set(PRI_624A_D2,1);
        gpio_set(PRI_624A_D3,1);
        gpio_set(PRI_624A_D4,0);
        gpio_set(PRI_624A_D5,1);
        break;

        case U2_16_DB:
        gpio_set(PRI_624A_D0,1);
        gpio_set(PRI_624A_D1,1);
        gpio_set(PRI_624A_D2,1);
        gpio_set(PRI_624A_D3,1);
        gpio_set(PRI_624A_D4,1);
        gpio_set(PRI_624A_D5,0);
        break;

        case U2_31_5_DB:
        gpio_set(PRI_624A_D0,0);
        gpio_set(PRI_624A_D1,0);
        gpio_set(PRI_624A_D2,0);
        gpio_set(PRI_624A_D3,0);
        gpio_set(PRI_624A_D4,0);
        gpio_set(PRI_624A_D5,0);
        break;

        default :
        break;	
    }	
}

void gpio_control_rf(rf_channel channel_val)
{
   gpio_rf_channel(channel_val);
   printf_info("channel : %d\n",channel_val);
}

void gpio_attenuation_rf(rf_pre_reduce pre_reduce_val,rf_pos_reduce pos_reduce_val)
{
   gpio_rf_pre_reduce(pre_reduce_val); 
   gpio_rf_pos_reduce(pos_reduce_val); 
   printf_info("pre_reduce_val : %d, pos_reduce_val : %d\n",pre_reduce_val,pos_reduce_val);
}


void gpio_init_control() //初始化24个IO口
{
    printf_info("gpio_rf init\n");
	int i;
	for(i=0;i<24;i++) {
	  gpio_init(i);	  
    }	
    rf_db_attenuation_init();
}

