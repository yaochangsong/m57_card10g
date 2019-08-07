
#include "gpio.h"


int set_gpio(int spidev_index,char val){
    char  spi_offset_num[5];
    char  spi_gpio_value[128];
    int fd;
    sprintf(spi_offset_num,"%d",GPIO_BASE_OFFSET+spidev_index);
    sprintf(spi_gpio_value,"/sys/class/gpio/gpio%s/value",spi_offset_num);

    fd = open(spi_gpio_value, O_WRONLY);
    if (fd < 0){
        printf("Cannot open GPIO value\n");
        return -1;
    }

    if(fd <= 0){
    return -1;
    }

    if(val == 1){
        write(fd,"1", 2);
        printf("higt\n");
    }
    else {
        write(fd,"0", 2);
        printf("low\n");		
    }
    close(fd);
    return 0;
}

void rf_channel(Channel_Rf channel_val)
{
    switch(channel_val)
    {	
        case HPF1:
        set_gpio(PRI_42422_V1,1);
        set_gpio(PRI_42422_LS,1);		
        set_gpio(PRI_L_42442_V1,1);
        set_gpio(PRI_L_42442_V2,0);	
        set_gpio(SEC_L_42442_V1,1);
        set_gpio(SEC_L_42442_V2,0);
        set_gpio(SEC_42422_V1,1);
        set_gpio(SEC_42422_LS,1);
        break;

        case HPF2:
        set_gpio(PRI_42422_V1,1);
        set_gpio(PRI_42422_LS,1);		
        set_gpio(PRI_L_42442_V1,0);
        set_gpio(PRI_L_42442_V2,1);	
        set_gpio(SEC_L_42442_V1,0);
        set_gpio(SEC_L_42442_V2,1);	
        set_gpio(SEC_42422_V1,1);
        set_gpio(SEC_42422_LS,1);
        break;

        case HPF3:
        set_gpio(PRI_42422_V1,1);
        set_gpio(PRI_42422_LS,1);	
        set_gpio(PRI_L_42442_V1,1);
        set_gpio(PRI_L_42442_V2,1);
        set_gpio(SEC_L_42442_V1,1);
        set_gpio(SEC_L_42442_V2,1);	
        set_gpio(SEC_42422_V1,1);
        set_gpio(SEC_42422_LS,1);		
        break;

        case HPF4:
        set_gpio(PRI_42422_V1,1);
        set_gpio(PRI_42422_LS,1);	
        set_gpio(PRI_L_42442_V1,0);
        set_gpio(PRI_L_42442_V2,0);
        set_gpio(SEC_L_42442_V1,0);
        set_gpio(SEC_L_42442_V2,0);
        set_gpio(SEC_42422_V1,1);
        set_gpio(SEC_42422_LS,1);		
        break;

        case HPF5:
        set_gpio(PRI_42422_V1,1);
        set_gpio(PRI_42422_LS,0);	
        set_gpio(PRI_H_42442_V1,1);
        set_gpio(PRI_H_42442_V2,0);
        set_gpio(SEC_H_42442_V1,1);
        set_gpio(SEC_H_42442_V2,0);
        set_gpio(SEC_42422_V1,1);
        set_gpio(SEC_42422_LS,0);
        break;

        case HPF6:
        set_gpio(PRI_42422_V1,1);
        set_gpio(PRI_42422_LS,0);	
        set_gpio(PRI_H_42442_V1,0);
        set_gpio(PRI_H_42442_V2,1);
        set_gpio(SEC_H_42442_V1,0);
        set_gpio(SEC_H_42442_V2,1);
        set_gpio(SEC_42422_V1,1);
        set_gpio(SEC_42422_LS,0);
        break;

        case HPF7:
        set_gpio(PRI_42422_V1,1);
        set_gpio(PRI_42422_LS,0);	
        set_gpio(PRI_H_42442_V1,1);
        set_gpio(PRI_H_42442_V2,1);
        set_gpio(SEC_H_42442_V1,1);
        set_gpio(SEC_H_42442_V2,1);
        set_gpio(SEC_42422_V1,1);
        set_gpio(SEC_42422_LS,0);
        break;

        case HPF8:
        set_gpio(PRI_42422_V1,1);
        set_gpio(PRI_42422_LS,0);	
        set_gpio(PRI_H_42442_V1,0);
        set_gpio(PRI_H_42442_V2,0);
        set_gpio(SEC_H_42442_V1,0);
        set_gpio(SEC_H_42442_V2,0);
        set_gpio(SEC_42422_V1,1);
        set_gpio(SEC_42422_LS,0);
        break;
        default :
        break;
    }

}

void rf_attenuation(Attenuation_Rf attenuation_val)
{
    switch(attenuation_val)
    {
        case U10_0_DB:
        set_gpio(SEC_624A_D0,1);
        set_gpio(SEC_624A_D1,1);
        set_gpio(SEC_624A_D2,1);
        set_gpio(SEC_624A_D3,1);
        set_gpio(SEC_624A_D4,1);
        set_gpio(SEC_624A_D5,1);
        break;

        case U10_0_5_DB:
        set_gpio(SEC_624A_D0,0);
        set_gpio(SEC_624A_D1,1);
        set_gpio(SEC_624A_D2,1);
        set_gpio(SEC_624A_D3,1);
        set_gpio(SEC_624A_D4,1);
        set_gpio(SEC_624A_D5,1);
        break;

        case U10_1_DB:
        set_gpio(SEC_624A_D0,1);
        set_gpio(SEC_624A_D1,0);
        set_gpio(SEC_624A_D2,1);
        set_gpio(SEC_624A_D3,1);
        set_gpio(SEC_624A_D4,1);
        set_gpio(SEC_624A_D5,1);
        break;

        case U10_2_DB:
        set_gpio(SEC_624A_D0,1);
        set_gpio(SEC_624A_D1,1);
        set_gpio(SEC_624A_D2,0);
        set_gpio(SEC_624A_D3,1);
        set_gpio(SEC_624A_D4,1);
        set_gpio(SEC_624A_D5,1);
        break;

        case U10_4_DB:
        set_gpio(SEC_624A_D0,1);
        set_gpio(SEC_624A_D1,1);
        set_gpio(SEC_624A_D2,1);
        set_gpio(SEC_624A_D3,0);
        set_gpio(SEC_624A_D4,1);
        set_gpio(SEC_624A_D5,1);
        break;

        case U10_8_DB:
        set_gpio(SEC_624A_D0,1);
        set_gpio(SEC_624A_D1,1);
        set_gpio(SEC_624A_D2,1);
        set_gpio(SEC_624A_D3,1);
        set_gpio(SEC_624A_D4,0);
        set_gpio(SEC_624A_D5,1);
        break;

        case U10_16_DB:
        set_gpio(SEC_624A_D0,1);
        set_gpio(SEC_624A_D1,1);
        set_gpio(SEC_624A_D2,1);
        set_gpio(SEC_624A_D3,1);
        set_gpio(SEC_624A_D4,1);
        set_gpio(SEC_624A_D5,0);
        break;

        case U10_31_5_DB:
        set_gpio(SEC_624A_D0,0);
        set_gpio(SEC_624A_D1,0);
        set_gpio(SEC_624A_D2,0);
        set_gpio(SEC_624A_D3,0);
        set_gpio(SEC_624A_D4,0);
        set_gpio(SEC_624A_D5,0);
        break;

        case U2_0_DB:
        set_gpio(PRI_624A_D0,0);
        set_gpio(PRI_624A_D1,1);
        set_gpio(PRI_624A_D2,1);
        set_gpio(PRI_624A_D3,1);
        set_gpio(PRI_624A_D4,1);
        set_gpio(PRI_624A_D5,1);
        break;

        case U2_0_5_DB:
        set_gpio(PRI_624A_D0,1);
        set_gpio(PRI_624A_D1,0);
        set_gpio(PRI_624A_D2,1);
        set_gpio(PRI_624A_D3,1);
        set_gpio(PRI_624A_D4,1);
        set_gpio(PRI_624A_D5,1);
        break;

        case U2_1_DB:
        set_gpio(PRI_624A_D0,1);
        set_gpio(PRI_624A_D1,0);
        set_gpio(PRI_624A_D2,1);
        set_gpio(PRI_624A_D3,1);
        set_gpio(PRI_624A_D4,1);
        set_gpio(PRI_624A_D5,1);
        break;

        case U2_2_DB:
        set_gpio(PRI_624A_D0,1);
        set_gpio(PRI_624A_D1,1);
        set_gpio(PRI_624A_D2,0);
        set_gpio(PRI_624A_D3,1);
        set_gpio(PRI_624A_D4,1);
        set_gpio(PRI_624A_D5,1);
        break;

        case U2_4_DB:
        set_gpio(PRI_624A_D0,1);
        set_gpio(PRI_624A_D1,1);
        set_gpio(PRI_624A_D2,1);
        set_gpio(PRI_624A_D3,0);
        set_gpio(PRI_624A_D4,1);
        set_gpio(PRI_624A_D5,1);
        break;

        case U2_8_DB:
        set_gpio(PRI_624A_D0,1);
        set_gpio(PRI_624A_D1,1);
        set_gpio(PRI_624A_D2,1);
        set_gpio(PRI_624A_D3,1);
        set_gpio(PRI_624A_D4,0);
        set_gpio(PRI_624A_D5,1);
        break;

        case U2_16_DB:
        set_gpio(PRI_624A_D0,1);
        set_gpio(PRI_624A_D1,1);
        set_gpio(PRI_624A_D2,1);
        set_gpio(PRI_624A_D3,1);
        set_gpio(PRI_624A_D4,1);
        set_gpio(PRI_624A_D5,0);
        break;

        case U2_31_5_DB:
        set_gpio(PRI_624A_D0,0);
        set_gpio(PRI_624A_D1,0);
        set_gpio(PRI_624A_D2,0);
        set_gpio(PRI_624A_D3,0);
        set_gpio(PRI_624A_D4,0);
        set_gpio(PRI_624A_D5,0);
        break;

        default :
        break;	
    }	
}

void control_rf(Channel_Rf channel_val,Attenuation_Rf attenuation_val)
{
  rf_channel(channel_val);
  rf_attenuation(attenuation_val); 
}


int gpio_init(int spidev_index){

    char spi_offset_num[5];
    char spi_gpio_direction[128];
    char  spi_gpio_value[128];
    int exportfd, directionfd;
	int valuefd;
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

    valuefd = open(spi_gpio_value, O_WRONLY);
    if (valuefd < 0){
        printf("Cannot open GPIO value\n");
        return -1;
    }
	set_gpio(valuefd,0);
	close(valuefd);
    return 0;
}

void init_control_gpio() //初始化24个IO口
{
	int i;
	for(i=0;i<24;i++) {
	  gpio_init(i);	  
    }	
}

