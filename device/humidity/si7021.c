#include <linux/i2c.h>  
#include <linux/i2c-dev.h> 
#include "config.h" 

static int i2c_fd = 0;

static int si7021_read_i2c_register(int file,  
    unsigned char addr,  
    unsigned char cmd,  
    unsigned char *value,  
    int len) 
{  
    int ret = 0;
    unsigned char outbuf[2] = {0};  

    struct i2c_rdwr_ioctl_data packets;  
    struct i2c_msg messages[2];  

    outbuf[0] = cmd;

    messages[0].addr  = addr;  
    messages[0].flags = 0;  
    messages[0].len   = 1;    
    messages[0].buf   = outbuf;  


    messages[1].addr  = addr;  
    messages[1].flags = I2C_M_RD;  
    messages[1].len   = len;    
    messages[1].buf  = value;  


    /* Transfer the i2c packets to the kernel and verify it worked */  
    packets.msgs  = messages;  
    packets.nmsgs = 2;  
    if(ioctl(file, I2C_RDWR, &packets) < 0) {  
        perror("Unable to send data");  
        ret = -1;
    }

    return ret;  
}

static int si7021_write_i2c_register(int file,  
        unsigned char addr,  
        unsigned char reg,  
        unsigned char *value,  
        int len) {  
    int ret = 0;
    unsigned char *outbuf = (unsigned char *)malloc(sizeof(unsigned char)*(len+2));  
    if(outbuf==NULL)  
    {  
        perror("MALLOC");  
        printf_err("malloc failed for i2c send buf\n");
        return -1;  
    }  
    struct i2c_rdwr_ioctl_data packets;  
    struct i2c_msg messages[1];  

    messages[0].addr  = addr;  
    messages[0].flags = 0;  
    messages[0].len   = len+2;    
    messages[0].buf   = outbuf;  


    /* The first byte indicates which register we'll write */  
    outbuf[0] = 0x40;  
    outbuf[1] = reg;  

    /*  
     * The second byte indicates the value to write.  Note that for many 
     * devices, we can write multiple, sequential registers at once by 
     * simply making outbuf bigger. 
     */  
    //    outbuf[1] = value;  
    memcpy(outbuf+2, value, len);  

    /* Transfer the i2c packets to the kernel and verify it worked */  
    packets.msgs  = messages;  
    packets.nmsgs = 1;  
    if(ioctl(file, I2C_RDWR, &packets) < 0) {  
        perror("Unable to send data");  
        ret = -1;
    }  
    if(outbuf){
        free(outbuf);
    }
    return ret;  
}

float si7021_read_temperature(void)
{
    uint8_t temp[2];
    uint16_t ushort_temp;
    float f_temp;
    
    si7021_read_i2c_register(i2c_fd, 
                            SI7021_I2C_SLAVE_ADDR, 
                            SI7021CMD_TEMP_HOLD,
                            temp, 2
                            );
    ushort_temp = (temp[0]<<8)|temp[1];
    f_temp = (float)ushort_temp * 175.72/65535 - 46.85;
    
    return f_temp;
}

float si7021_read_humidity(void)
{
    uint8_t humidity[2];
    uint16_t ushort_humidity;
    float f_humidity;
    
    si7021_read_i2c_register(i2c_fd, 
                            SI7021_I2C_SLAVE_ADDR, 
                            SI7021CMD_RH_HOLD,
                            humidity, 2
                            );
    ushort_humidity = (humidity[0]<<8)|humidity[1];
    f_humidity = (float)ushort_humidity * 125/65535 - 6;

    return f_humidity;
}



void si7021_init(void)
{
    printf_note("si7021 init\n");  
    i2c_fd = open("/dev/i2c-1", O_RDWR);
    if(i2c_fd < 0){  
        printf_err("####i2c device open failed####\n");  
        return (-1);  
    }

    printf_note("Ambient temperature: %.2f 'C\n", si7021_read_temperature());
    printf_note("Ambient humidity: %.2f%\n", si7021_read_humidity());
}


