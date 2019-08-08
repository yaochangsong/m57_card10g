
#include "gpio.h"



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
        printf("higt\n");
    }
    else {
        write(valuefd,"0", 2);
        printf("low\n");		
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

void gpio_rf_channel(Channel_Rf channel_val)
{
    switch(channel_val)
    {	
        case HPF1:
        gpio_set(PRI_42422_V1,1);
        gpio_set(PRI_42422_LS,1);		
        gpio_set(PRI_L_42442_V1,1);
        gpio_set(PRI_L_42442_V2,0);	
        gpio_set(SEC_L_42442_V1,1);
        gpio_set(SEC_L_42442_V2,0);
        gpio_set(SEC_42422_V1,1);
        gpio_set(SEC_42422_LS,1);
        break;

        case HPF2:
        gpio_set(PRI_42422_V1,1);
        gpio_set(PRI_42422_LS,1);		
        gpio_set(PRI_L_42442_V1,0);
        gpio_set(PRI_L_42442_V2,1);	
        gpio_set(SEC_L_42442_V1,0);
        gpio_set(SEC_L_42442_V2,1);	
        gpio_set(SEC_42422_V1,1);
        gpio_set(SEC_42422_LS,1);
        break;

        case HPF3:
        gpio_set(PRI_42422_V1,1);
        gpio_set(PRI_42422_LS,1);	
        gpio_set(PRI_L_42442_V1,1);
        gpio_set(PRI_L_42442_V2,1);
        gpio_set(SEC_L_42442_V1,1);
        gpio_set(SEC_L_42442_V2,1);	
        gpio_set(SEC_42422_V1,1);
        gpio_set(SEC_42422_LS,1);		
        break;

        case HPF4:
        gpio_set(PRI_42422_V1,1);
        gpio_set(PRI_42422_LS,1);	
        gpio_set(PRI_L_42442_V1,0);
        gpio_set(PRI_L_42442_V2,0);
        gpio_set(SEC_L_42442_V1,0);
        gpio_set(SEC_L_42442_V2,0);
        gpio_set(SEC_42422_V1,1);
        gpio_set(SEC_42422_LS,1);		
        break;

        case HPF5:
        gpio_set(PRI_42422_V1,1);
        gpio_set(PRI_42422_LS,0);	
        gpio_set(PRI_H_42442_V1,1);
        gpio_set(PRI_H_42442_V2,0);
        gpio_set(SEC_H_42442_V1,1);
        gpio_set(SEC_H_42442_V2,0);
        gpio_set(SEC_42422_V1,1);
        gpio_set(SEC_42422_LS,0);
        break;

        case HPF6:
        gpio_set(PRI_42422_V1,1);
        gpio_set(PRI_42422_LS,0);	
        gpio_set(PRI_H_42442_V1,0);
        gpio_set(PRI_H_42442_V2,1);
        gpio_set(SEC_H_42442_V1,0);
        gpio_set(SEC_H_42442_V2,1);
        gpio_set(SEC_42422_V1,1);
        gpio_set(SEC_42422_LS,0);
        break;

        case HPF7:
        gpio_set(PRI_42422_V1,1);
        gpio_set(PRI_42422_LS,0);	
        gpio_set(PRI_H_42442_V1,1);
        gpio_set(PRI_H_42442_V2,1);
        gpio_set(SEC_H_42442_V1,1);
        gpio_set(SEC_H_42442_V2,1);
        gpio_set(SEC_42422_V1,1);
        gpio_set(SEC_42422_LS,0);
        break;

        case HPF8:
        gpio_set(PRI_42422_V1,1);
        gpio_set(PRI_42422_LS,0);	
        gpio_set(PRI_H_42442_V1,0);
        gpio_set(PRI_H_42442_V2,0);
        gpio_set(SEC_H_42442_V1,0);
        gpio_set(SEC_H_42442_V2,0);
        gpio_set(SEC_42422_V1,1);
        gpio_set(SEC_42422_LS,0);
        break;
        default :
        break;
    }

}

void gpio_rf_attenuation(Attenuation_Rf attenuation_val)
{
    switch(attenuation_val)
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

void gpio_control_rf(Channel_Rf channel_val,Attenuation_Rf attenuation_val)
{
  gpio_rf_channel(channel_val);
  gpio_rf_attenuation(attenuation_val); 
  printf_info("channel : %d, attenuation : %d\n",channel_val,attenuation_val);
}




void gpio_init_control() //初始化24个IO口
{
    printf_info("gpio_rf init\n");
	int i;
	for(i=0;i<24;i++) {
	  gpio_init(i);	  
    }	
}

