#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>  
#include <linux/i2c-dev.h> 
#include "../../log/log.h"
#include "audio.h"

//----------------------------------------------------
// PROTOTYPE FUNCTIONS
//----------------------------------------------------
void AudioPllConfig();
void AudioWriteToReg(unsigned char u8RegAddr, unsigned char u8Data);
void AudioConfigureJacks();
void LineinLineoutConfig();
//Global variables

static unsigned char slave_addr=0;
static int i2c_fd = 0;

static int read_i2c_register_for_adau1761(int file,  
		unsigned char addr,  
		unsigned char reg,  
		unsigned char *value,  
		int len) {  
    int ret = 0;
	unsigned char outbuf[2] = {0};  

	struct i2c_rdwr_ioctl_data packets;  
	struct i2c_msg messages[2];  
    
    outbuf[0] = 0x40;
    outbuf[1] = reg;

	messages[0].addr  = addr;  
	messages[0].flags = 0;  
	messages[0].len   = 2;    
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

static int set_i2c_register_for_adau1761(int file,  
		unsigned char addr,  
		unsigned char reg,  
		unsigned char *value,  
		int len) {  
    int ret = 0;
	unsigned char *outbuf = (unsigned char *)malloc(sizeof(unsigned char)*(len+2));  
	if(outbuf==NULL)  
	{  
		perror("MALLOC");  
        printf("malloc failed for i2c send buf\n");
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

/* ---------------------------------------------------------------------------- *
 *                              AudioPllConfig()                                *
 * ---------------------------------------------------------------------------- *
 * Configures audio codes's internal PLL. With MCLK = 10 MHz it configures the
 * PLL for a VCO frequency = 49.152 MHz, and an audio sample rate of 48 KHz.
 * ---------------------------------------------------------------------------- */
void AudioPllConfig(void ) {

    unsigned char u8TxData[8], u8RxData[6];
    int Status,retry=0;

      // Disable Core Clock
    AudioWriteToReg(R0_CLOCK_CONTROL, 0x0E);


    // Write 6 bytes to R1 @ register address 0x4002
    u8TxData[0] = 0x40; // Register write address [15:8]
    u8TxData[1] = 0x02; // Register write address [7:0]
    u8TxData[2] = 0x02; // byte 6 - M[15:8]
    u8TxData[3] = 0x71; // byte 5 - M[7:0]
    u8TxData[4] = 0x02; // byte 4 - N[15:8]
    u8TxData[5] = 0x3C; // byte 3 - N[7:0]
    u8TxData[6] = 0x21; // byte 2 - 7 = reserved, bits 6:3 = R[3:0], 2:1 = X[1:0], 0 = PLL operation mode
    u8TxData[7] = 0x01; // byte 1 - 7:2 = reserved, 1 = PLL Lock, 0 = Core clock enable

    // Write bytes to PLL Control register R1 @ 0x4002
    set_i2c_register_for_adau1761(i2c_fd,slave_addr,u8TxData[1],u8TxData+2,6); 

    
    // Register address set: 0x4002
    u8TxData[0] = 0x40;
    u8TxData[1] = 0x02;

    while(retry++<100){
        read_i2c_register_for_adau1761(i2c_fd,slave_addr,0x02,u8RxData,6);
        if((u8RxData[5] & 0x02) == 0){
            printf_debug("pll no lock\n");
        }else{
            printf_debug("pll lock\n");
            break;
        }
        usleep(1000);
    }

    AudioWriteToReg(R0_CLOCK_CONTROL, 0x0F);    // 1111
    // bit 3:       CLKSRC = PLL Clock input
    // bits 2:1:    INFREQ = 1024 x fs
    // bit 0:       COREN = Core Clock enabled
}


/* ---------------------------------------------------------------------------- *
 *                              AudioWriteToReg                                 *
 * ---------------------------------------------------------------------------- *
 * Function to write one byte (8-bits) to one of the registers from the audio
 * controller.
 * ---------------------------------------------------------------------------- */
void AudioWriteToReg(unsigned char u8RegAddr, unsigned char u8Data) {
    set_i2c_register_for_adau1761(i2c_fd,slave_addr,u8RegAddr,&u8Data,1); 
}

/* ---------------------------------------------------------------------------- *
 *                              AudioConfigureJacks()                           *
 * ---------------------------------------------------------------------------- *
 * Configures audio codes's various mixers, ADC's, DAC's, and amplifiers to
 * accept stereo input from line in and push stereo output to line out.
 * ---------------------------------------------------------------------------- */
void AudioConfigureJacks(void)
{
    //AudioWriteToReg(R4_RECORD_MIXER_LEFT_CONTROL_0, 0x01); //enable mixer 1
    /*
    AudioWriteToReg(R4_RECORD_MIXER_LEFT_CONTROL_0, 0x0f);
    AudioWriteToReg(R5_RECORD_MIXER_LEFT_CONTROL_1, 0x07); //unmute Left channel of line in into mxr 1 and set gain to 6 db
    AudioWriteToReg(R6_RECORD_MIXER_RIGHT_CONTROL_0, 0x0f); //enable mixer 2


    AudioWriteToReg(R8_LEFT_DIFFERENTIAL_INPUT_VOLUME_CONTROL,0x03);
    AudioWriteToReg(R9_RIGHT_DIFFERENTIAL_INPUT_VOLUME_CONTROL,0x03);
    AudioWriteToReg(R10_RECORD_MICROPHONE_BIAS_CONTROL, 0x01);

    AudioWriteToReg(R7_RECORD_MIXER_RIGHT_CONTROL_1, 0x07); //unmute Right channel of line in into mxr 2 and set gain to 6 db
    AudioWriteToReg(R19_ADC_CONTROL, 0x13); //enable ADCs
    */
    AudioWriteToReg(R22_PLAYBACK_MIXER_LEFT_CONTROL_0, 0x21); //unmute Left DAC into Mxr 3; enable mxr 3
    AudioWriteToReg(R24_PLAYBACK_MIXER_RIGHT_CONTROL_0, 0x41); //unmute Right DAC into Mxr4; enable mxr 4
    AudioWriteToReg(R26_PLAYBACK_LR_MIXER_LEFT_LINE_OUTPUT_CONTROL, 0x05); //unmute Mxr3 into Mxr5 and set gain to 6db; enable mxr 5
    AudioWriteToReg(R27_PLAYBACK_LR_MIXER_RIGHT_LINE_OUTPUT_CONTROL, 0x11); //unmute Mxr4 into Mxr6 and set gain to 6db; enable mxr 6
    AudioWriteToReg(R29_PLAYBACK_HEADPHONE_LEFT_VOLUME_CONTROL, 0xFF);//Mute Left channel of HP port (LHP)
    AudioWriteToReg(R30_PLAYBACK_HEADPHONE_RIGHT_VOLUME_CONTROL, 0xFF); //Mute Right channel of HP port (LHP)
    //AudioWriteToReg(R31_PLAYBACK_LINE_OUTPUT_LEFT_VOLUME_CONTROL, 0xE6); //set LOUT volume (0db); unmute left channel of Line out port; set Line out port to line out mode
    //AudioWriteToReg(R32_PLAYBACK_LINE_OUTPUT_RIGHT_VOLUME_CONTROL, 0xE6); // set ROUT volume (0db); unmute right channel of Line out port; set Line out port to line out mode
    AudioWriteToReg(R31_PLAYBACK_LINE_OUTPUT_LEFT_VOLUME_CONTROL, 0xFE); //set LOUT volume (0db); unmute left channel of Line out port; set Line out port to line out mode
    AudioWriteToReg(R32_PLAYBACK_LINE_OUTPUT_RIGHT_VOLUME_CONTROL, 0xFE); // set ROUT volume (0db); unmute right channel of Line out port; set Line out port to line out mode
    AudioWriteToReg(R35_PLAYBACK_POWER_MANAGEMENT, 0x03); //enable left and right channel playback (not sure exactly what this does...)
    AudioWriteToReg(R36_DAC_CONTROL_0, 0x03); //enable both DACs

    AudioWriteToReg(R58_SERIAL_INPUT_ROUTE_CONTROL, 0x01); //Connect I2S serial port output (SDATA_O) to DACs
   // AudioWriteToReg(R59_SERIAL_OUTPUT_ROUTE_CONTROL, 0x01); //connect I2S serial port input (SDATA_I) to ADCs

    AudioWriteToReg(R65_CLOCK_ENABLE_0, 0x7F); //Enable clocks
    AudioWriteToReg(R66_CLOCK_ENABLE_1, 0x03); //Enable rest of clocks
}

/* ---------------------------------------------------------------------------- *
 *                              LineinLineoutConfig()                           *
 * ---------------------------------------------------------------------------- *
 * Configures Line-In input, ADC's, DAC's, Line-Out and HP-Out.
 * ---------------------------------------------------------------------------- */
void LineinLineoutConfig(void) {

    AudioWriteToReg(R17_CONVERTER_CONTROL_0, 0x00);//48 KHz
    AudioWriteToReg(R64_SERIAL_PORT_SAMPLING_RATE, 0x00);//48 KHz
    /*
    AudioWriteToReg(R19_ADC_CONTROL, 0x13);
    */
    AudioWriteToReg(R36_DAC_CONTROL_0, 0x03);
    AudioWriteToReg(R35_PLAYBACK_POWER_MANAGEMENT, 0x03);
    AudioWriteToReg(R58_SERIAL_INPUT_ROUTE_CONTROL, 0x01);
    //AudioWriteToReg(R59_SERIAL_OUTPUT_ROUTE_CONTROL, 0x01);
    AudioWriteToReg(R65_CLOCK_ENABLE_0, 0x7F);
    AudioWriteToReg(R66_CLOCK_ENABLE_1, 0x03);

    AudioWriteToReg(R4_RECORD_MIXER_LEFT_CONTROL_0, 0x01);
    AudioWriteToReg(R5_RECORD_MIXER_LEFT_CONTROL_1, 0x05);//0 dB gain
    AudioWriteToReg(R6_RECORD_MIXER_RIGHT_CONTROL_0, 0x01);
    AudioWriteToReg(R7_RECORD_MIXER_RIGHT_CONTROL_1, 0x05);//0 dB gain

    AudioWriteToReg(R22_PLAYBACK_MIXER_LEFT_CONTROL_0, 0x21);
    AudioWriteToReg(R24_PLAYBACK_MIXER_RIGHT_CONTROL_0, 0x41);
    AudioWriteToReg(R26_PLAYBACK_LR_MIXER_LEFT_LINE_OUTPUT_CONTROL, 0x03);//0 dB
    AudioWriteToReg(R27_PLAYBACK_LR_MIXER_RIGHT_LINE_OUTPUT_CONTROL, 0x09);//0 dB
    AudioWriteToReg(R29_PLAYBACK_HEADPHONE_LEFT_VOLUME_CONTROL, 0xa7);//0 dB
    AudioWriteToReg(R30_PLAYBACK_HEADPHONE_RIGHT_VOLUME_CONTROL, 0xa7);//0 dB
    AudioWriteToReg(R31_PLAYBACK_LINE_OUTPUT_LEFT_VOLUME_CONTROL, 0xE6);//0 dB
    AudioWriteToReg(R32_PLAYBACK_LINE_OUTPUT_RIGHT_VOLUME_CONTROL, 0xE6);//0 dB
}

int adau1761_set_volume(unsigned int val){
    unsigned char real;
    if(val >100){
        val = 100;
    }

    real = (unsigned char)((float)(val)*63.0/100.0);
    printf_info("set vol value: percentage=%d map value=%d\n",val,real);
    real = (real <<2) ;
    real |= 0x03;
    printf_info("set vol value:0x%02x\n",real);

    AudioWriteToReg(R29_PLAYBACK_HEADPHONE_LEFT_VOLUME_CONTROL,real);//Mute Left channel of HP port (LHP)
    AudioWriteToReg(R30_PLAYBACK_HEADPHONE_RIGHT_VOLUME_CONTROL, real); //Mute Right channel of HP port (LHP)
    return 0;
}

int init_adau1761(void)
{
    unsigned short regaddr;

    slave_addr = 0x38;
    i2c_fd = open("/dev/i2c-0", O_RDWR);
    if(i2c_fd < 0){  
        printf("####i2c  device open failed####/n");  
        return (-1);  
    }
    
    //Configure the Audio Codec's PLL
    AudioPllConfig();

    //Configure the Line in and Line out ports.
    //Call LineInLineOutConfig() for a configuration that
    //同时开启Miz702的两个输入（绿色）和输出（红色）
    AudioConfigureJacks();
    LineinLineoutConfig();
    
    return 0;
}

int audio_volume_set(unsigned int value)
{
    adau1761_set_volume(value);
    return 0;
}


int audio_init(void)
{
    if(init_adau1761() == -1){
        printf_note("Audio init Failed\n");
        return -1;
    }
    audio_volume_set(100);
    printf_note("Audio init OK\n");
    return 0;
}

