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
#if defined(SUPPORT_DIRECT_SAMPLE)
            #define DIRECT_FREQ_THR (200000000)
            if(old_freq < DIRECT_FREQ_THR ){
                break; 
            }
#endif
            get_fpga_reg()->rfReg->freq_khz = freq_khz;
            usleep(2000);
#endif
            break; 
        }
         case EX_RF_MID_BW :   {
            printf_note("[**RF**]ch=%d, middle bw=%uHz\n", ch, *(uint32_t *) data);
            uint32_t mbw= *(uint32_t *)data;
#ifdef  SUPPORT_RF_SPI
            //uint32_t filter=htobe32(*(uint32_t *)data) >> 24;
            ret = spi_rf_set_command(SPI_RF_MIDFREQ_BW_FILTER_SET, &mbw);
            usleep(300);
            ret = spi_rf_get_command(SPI_RF_MIDFREQ_BW_FILTER_GET, &mbw);
#elif defined(SUPPORT_RF_FPGA)
            int i, found = 0;
            uint8_t set_val = 0;
            /* 400KH(0) 4MHz(0x01) 40MHz(0x02) 200MHz(0x03)  */
            struct rf_bw_reg{
                uint32_t bw_hz;
                uint8_t reg_val;
            };
            struct rf_bw_reg reg[] = {
                    {400000,    0x00},
                    {4000000,   0x01},
                    {40000000,  0x02},
                    {200000000, 0x03},
            };
            
            for(i = 0; i < ARRAY_SIZE(reg); i++){
                if(mbw == reg[i].bw_hz){
                    set_val = reg[i].reg_val;
                    found ++;
                }
            }
            if(found == 0){
                printf_note("NOT found bandwidth %uHz in tables,use default[200Mhz]\n", mbw);
                set_val = 0x03; /* default 200MHz */
            }
            get_fpga_reg()->rfReg->mid_band = 0x03;//set_val;
#endif
            break; 
        }

        case EX_RF_MODE_CODE :{
            uint8_t noise_mode;
            /* noise_mode: 低失真模式(0x00)     常规模式(0x01) 低噪声模式(0x02) */
            noise_mode = *((uint8_t *)data);
            printf_note("[**RF**]ch=%d, noise_mode=%d\n", ch, noise_mode);
#ifdef defined(SUPPORT_RF_ADRV9009)
#elif  defined(SUPPORT_RF_SPI)
            ret = spi_rf_set_command(SPI_RF_NOISE_MODE_SET, &noise_mode);
            usleep(300);
            ret = spi_rf_get_command(SPI_RF_NOISE_MODE_GET, &noise_mode);
#elif defined(SUPPORT_RF_FPGA)
            int i, found = 0;
            uint8_t set_val = 0;
            /*这里需要转换 =>常规模式(0) 低噪声模式(0x01) 低失真模式(0x02)    */
            struct _reg{
                uint8_t mode;
                uint8_t reg_val;
            };
            struct _reg reg[] = {
                    {0,     0x02},
                    {1,     0x00},
                    {2,     0x01},
            };
            
            for(i = 0; i < ARRAY_SIZE(reg); i++){
                if(noise_mode == reg[i].mode){
                    set_val = reg[i].reg_val;
                    found ++;
                }
            }
            if(found == 0){
                printf_note("NOT found noise mode %uHz in tables,use default normal mode[0]\n", noise_mode);
                set_val = 0x00; /* default normal mode */
            }
            printf_note("[**RF**]ch=%d, set rel noise mode=%d\n", ch, noise_mode);
            get_fpga_reg()->rfReg->rf_mode = set_val;
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
#ifdef SUPPORT_RF_ADRV9009
#elif defined(SUPPORT_RF_SPI)
            ret = spi_rf_set_command(SPI_RF_MIDFREQ_GAIN_SET, &mgc_gain_value);
            usleep(300);
            ret = spi_rf_get_command(SPI_RF_MIDFREQ_GAIN_GET, &mgc_gain_value);
#elif defined(SUPPORT_RF_FPGA)
            if(mgc_gain_value > 30)
                mgc_gain_value = 30;
            else if(mgc_gain_value < 0)
                mgc_gain_value = 0;
            get_fpga_reg()->rfReg->midband_minus = mgc_gain_value;
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
#ifdef SUPPORT_RF_ADRV9009
            gpio_select_rf_attenuation(rf_gain_value);
#elif defined(SUPPORT_RF_SPI)
            ret = spi_rf_set_command(SPI_RF_GAIN_SET, &rf_gain_value);
            usleep(300);
            ret = spi_rf_get_command(SPI_RF_GAIN_GET, &rf_gain_value);
#elif defined(SUPPORT_RF_FPGA)
            if(rf_gain_value > 30)
                rf_gain_value = 30;
            else if(rf_gain_value < 0)
                rf_gain_value = 0;
            get_fpga_reg()->rfReg->rf_minus = rf_gain_value;
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
        case EX_RF_SAMPLE_CTRL:
        {
#if defined(SUPPORT_RF_FPGA)
            uint8_t val = 0;
            val = (*((int8_t *)data) == 0 ? 0 : 0x01);
            /* 0关闭直采，1开启直采 */
            printf_note("[**RF**] direct control=%d\n");
            get_fpga_reg()->rfReg->direct_control = val;
#endif
            break;
        }
        case EX_RF_LOW_NOISE:
        {
            uint32_t freq_mhz;
            int found = 0, i, r;
            int8_t vdata = 0, rdata = 0;
            
            static struct _amp_segment_table{
                int  index;
                uint32_t s_freq_mhz;
                uint32_t e_freq_mhz;
            }__attribute__ ((packed));
            
            struct _amp_segment_table amp_seg_table[] = {
                {1, 30,     1350},
                {2, 1350,   2700},
                {3, 2700,   3000},
            };
            
            freq_mhz = *((uint64_t *)data)/1000000;/* hz=>mhz */
            printf_info("freq_mhz = %u\n", freq_mhz);
            
            for(i = 0; i<ARRAY_SIZE(amp_seg_table); i++){
                if(freq_mhz >= amp_seg_table[i].s_freq_mhz && freq_mhz <= amp_seg_table[i].e_freq_mhz){
                    printf_info("find [%uMhz] in index[%d]\n", freq_mhz, amp_seg_table[i].index);
                    vdata = amp_seg_table[i].index;
                    found = 1;
                    break;
                }
            }
            if(found == 0){
                ret = -1;
                printf_warn("NOT find %uMhz in tables", freq_mhz);
                break;
            }
            
#if defined(SUPPORT_RS485)
           static int8_t vdata_dup = -1;
            if(vdata_dup == vdata){
                printf_info("set EX_RF_LOW_NOISE value is equal:%d no need set\n" , vdata_dup);
                break;
            }else{
                vdata_dup = vdata;
                rs485_com_set_v2(RS_485_LOW_NOISE_SET_CMD, &vdata);
                usleep(10000);
            }
           // rs485_com_set(RS_485_LOW_NOISE_SET_CMD, &vdata, sizeof(vdata));
            //r= rs485_com_get(RS_485_LOW_NOISE_GET_CMD, &rdata);
            //printf_note("get data:%s, %d\n", (r <= 0 ? "error" : "ok"), rdata);
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



