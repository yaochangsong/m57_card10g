#include "config.h"
#include "../../executor/spm/io_fpga.h"

uint8_t rf_set_interface(uint8_t cmd,uint8_t ch,void *data){
    uint8_t ret = -1;

    switch(cmd){
        case EX_RF_MID_FREQ :{
            /* only set once when value changed */
            static uint64_t old_freq = 0;/* Hz */
            if(old_freq != *(uint64_t*)data){
                old_freq = *(uint64_t*)data;
            }else{
                break;
            }
            printf_note("[**RF**]ch=%d, middle_freq=%llu\n",ch, *(uint64_t*)data);
#ifdef SUPPORT_RF_ADRV9009
            gpio_select_rf_channel(*(uint64_t*)data);
            adrv9009_iio_set_freq(*(uint64_t*)data);
#elif defined(SUPPORT_RF_SPI)
            uint64_t freq_khz = old_freq/1000;/* NOTE: Hz => KHz */
            uint64_t host_freq=htobe64(freq_khz) >> 24; //小端转大端（文档中心频率为大端字节序列，5个字节,单位为Hz,实际为Khz）
            uint64_t recv_freq = 0, recv_freq_htob;

            for(int i = 0; i<3; i++){
                ret = spi_rf_set_command(SPI_RF_FREQ_SET, &host_freq);
                usleep(300);
                recv_freq = 0;
                ret = spi_rf_get_command(SPI_RF_FREQ_GET, &recv_freq);
                recv_freq_htob =  (htobe64(recv_freq) >> 24) ;  /* khz */
                printf_debug("host_freq=%llu, 0x%llx, recv_freq = %llu, 0x%llx, htobe64=0x%llx\n",freq_khz, freq_khz,recv_freq ,recv_freq,recv_freq_htob);
                if(recv_freq_htob == freq_khz){
                        break;
                }
            }
#elif defined(SUPPORT_RF_FPGA)
            uint32_t freq_khz = old_freq/1000;/* NOTE: Hz => KHz */
            get_fpga_reg()->rfReg->freq_khz = freq_khz;
            usleep(100);
#endif
            break; 
        }
         case EX_RF_MID_BW :   {
            printf_note("[**RF**]ch=%d, middle bw=%u\n", ch, *(uint32_t *) data);
            uint32_t mbw= *(uint32_t *)data;
#ifdef  SUPPORT_RF_SPI
            //uint32_t filter=htobe32(*(uint32_t *)data) >> 24;
            ret = spi_rf_set_command(SPI_RF_MIDFREQ_BW_FILTER_SET, &mbw);
            usleep(300);
            ret = spi_rf_get_command(SPI_RF_MIDFREQ_BW_FILTER_GET, &mbw);
#elif defined(SUPPORT_RF_FPGA)
            get_fpga_reg()->rfReg->mid_band = 0x03;
            #if 0
            struct rf_bw_reg{
                uint32_t bw_hz;
            };
            bw_reg[]
            get_fpga_reg()->rfReg->mid_band = ;
            #endif
#endif
            break; 
        }

        case EX_RF_MODE_CODE :{
            uint8_t noise_mode;
            noise_mode = *((uint8_t *)data);
            printf_info("[**RF**]ch=%d, noise_mode=%d\n", ch, noise_mode);
        #if defined(SUPPORT_PLATFORM_ARCH_ARM)
            #ifdef SUPPORT_RF_ADRV9009
            #elif defined(SUPPORT_RF_SPI)
            ret = spi_rf_set_command(SPI_RF_NOISE_MODE_SET, &noise_mode);
            usleep(300);
            ret = spi_rf_get_command(SPI_RF_NOISE_MODE_GET, &noise_mode);
            #else

            #endif
        #endif
            break; 
        }
        case EX_RF_GAIN_MODE :{
            break; 

        }
        case EX_RF_MGC_GAIN : {
            int8_t mgc_gain_value;
            mgc_gain_value = *((int8_t *)data);
            printf_note("[**RF**]ch=%d, mgc_gain_value=%d\n",ch, mgc_gain_value);
        #if defined(SUPPORT_PLATFORM_ARCH_ARM)
            #ifdef SUPPORT_RF_ADRV9009
            #elif defined(SUPPORT_RF_SPI)
            ret = spi_rf_set_command(SPI_RF_MIDFREQ_GAIN_SET, &mgc_gain_value);
            usleep(300);
            ret = spi_rf_get_command(SPI_RF_MIDFREQ_GAIN_GET, &mgc_gain_value);
            #else

            #endif
        #endif
            break; 
        }
        case EX_RF_AGC_CTRL_TIME :{
            break; 
        }
        case EX_RF_AGC_OUTPUT_AMP :{
            break; 
        }
        case EX_RF_ANTENNA_SELECT :{
            break; 
        }
        case EX_RF_ATTENUATION :{
            int8_t rf_gain_value = 0;
            rf_gain_value = *((int8_t *)data);
            printf_note("[**RF**]ch=%d, rf_gain_value=%d\n",ch, rf_gain_value);
        #if defined(SUPPORT_PLATFORM_ARCH_ARM)
            #ifdef SUPPORT_RF_ADRV9009
            gpio_select_rf_attenuation(rf_gain_value);
            #elif defined(SUPPORT_RF_SPI)
            ret = spi_rf_set_command(SPI_RF_GAIN_SET, &rf_gain_value);
            usleep(300);
            ret = spi_rf_get_command(SPI_RF_GAIN_GET, &rf_gain_value);
            #else

            #endif
        #endif
            break; 
        }
        case EX_RF_CALIBRATE:
        {
            CALIBRATION_SOURCE_ST *akt_cs;
            struct calibration_source_t cs;
            akt_cs = (CALIBRATION_SOURCE_ST *)data;
            cs.source = akt_cs->enable;
            cs.middle_freq_mhz = akt_cs->middle_freq_hz/1000000;
            cs.power = (float)akt_cs->power;
            printf_note("source=%d, middle_freq_mhz=%uMhz, power=%f\n", cs.source, cs.middle_freq_mhz, cs.power);
#if defined(SUPPORT_RF_SPI)
            ret = spi_rf_set_command(SPI_RF_CALIBRATE_SOURCE_SET, &cs);
#endif
            break;
        }

        default:{
            break;
        }
    }
    return ret;
}

uint8_t rf_read_interface(uint8_t cmd,uint8_t ch,void *data){
    uint8_t ret = -1;

    switch(cmd){
        case EX_RF_MID_FREQ : {
            printf_debug("rf_read_interface %d\n",EX_RF_MID_FREQ);
            break;
        }
        case EX_RF_MID_BW :   {
            break; 
        }
        case EX_RF_MODE_CODE :{
            break; 
        }
        case EX_RF_GAIN_MODE :{
            break; 
        }
        case EX_RF_MGC_GAIN : {
            break; 
        }
        case EX_RF_AGC_CTRL_TIME : {
            break; 
        }
        case EX_RF_AGC_OUTPUT_AMP :{
            break; 
        }
        case EX_RF_ANTENNA_SELECT :{
            break; 
        }
        case EX_RF_ATTENUATION :{
            break; 
        }
        case EX_RF_STATUS_TEMPERAT :{
            int16_t  rf_temperature = 0;
            #if defined(SUPPORT_PLATFORM_ARCH_ARM)
            #elif defined(SUPPORT_RF_SPI)
            ret = spi_rf_get_command(SPI_RF_TEMPRATURE_GET, &rf_temperature);
            #else
            
            #endif
            *(int16_t *)data = rf_temperature;
            printf_info("rf temprature:%d\n", *(int16_t *)data);
            break;
        }
        default:{
            break;
        }

    }
    return ret;

}

int8_t rf_init(void)
{
    int ret = -1;
    uint8_t status = 0;
    printf_note("RF init...!\n");
#ifdef SUPPORT_RF_ADRV9009
    adrv9009_iio_init();
#endif
#if defined(SUPPORT_RF_SPI)
    ret = spi_init();
#endif
    return ret;
}



