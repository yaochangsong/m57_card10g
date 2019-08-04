

#include <gpio.h>

int init_gpio(int spidev_index){

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
    if (valuefd < 0)
    {
        printf("Cannot open GPIO value\n");
        return -1;
    }
	set_gpio_low(valuefd);
	close(valuefd);
    return 0;
}

int set_gpio(int spidev_index,char val){
	char  spi_offset_num[5];
    char  spi_gpio_value[128];
	int fd;
	sprintf(spi_offset_num,"%d",GPIO_BASE_OFFSET+spidev_index);
    sprintf(spi_gpio_value,"/sys/class/gpio/gpio%s/value",spi_offset_num);
	
	fd = open(spi_gpio_value, O_WRONLY);
    if (fd < 0)
    {
        printf("Cannot open GPIO value\n");
        return -1;
    }
	
    if(fd <= 0){
        return -1;
    }
	
    if(val == 1)
	{
		 write(fd,"1", 2);
		 printf("higt\n");
	}
	else 
	{
		write(fd,"0", 2);
		printf("low\n");		
	}
    
	close(fd);
	return 0;
}
