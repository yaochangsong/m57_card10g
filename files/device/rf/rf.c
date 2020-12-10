#include "config.h"
//#include "../../executor/spm/io_fpga.h"
#include "rf.h"

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
            printf_debug("[**RF**]ch=%d, middle_freq=%llu\n",ch, *(uint64_t*)data);
#ifdef SUPPORT_RF_ADRV
            uint64_t mid_freq = *(uint64_t*)data;
       #ifdef SUPPORT_PROJECT_SSA_MONITOR
            mid_freq = lvttv_freq_convert(mid_freq);
       #endif
            gpio_select_rf_channel(mid_freq);
            adrv_set_rx_freq(mid_freq);
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
    #if defined(SUPPORT_DIRECT_SAMPLE)
            #define DIRECT_FREQ_THR (200000000)
            if(old_freq < DIRECT_FREQ_THR ){
                break;
            }
    #endif
            _reg_set_rf_frequency(ch, 0, old_freq, get_fpga_reg());
#endif
            break; 
        }
         case EX_RF_MID_BW :   {
            uint32_t mbw;
            if(data == NULL){
                if(config_read_by_cmd(EX_RF_FREQ_CMD, EX_RF_MID_BW, ch, &mbw) == -1){
                    printf_err("Error read scan bandwidth=%u\n", mbw);
                    break;
                }
            }else{
                mbw= *(uint32_t *)data;
            }
            printf_note("[**RF**]ch=%d, middle bw=%uHz\n", ch, mbw);
            
#ifdef SUPPORT_RF_ADRV
            adrv_set_rx_bw(mbw);
#elif  SUPPORT_RF_SPI
            //uint32_t filter=htobe32(*(uint32_t *)data) >> 24;
            ret = spi_rf_set_command(SPI_RF_MIDFREQ_BW_FILTER_SET, &mbw);
            usleep(300);
            ret = spi_rf_get_command(SPI_RF_MIDFREQ_BW_FILTER_GET, &mbw);
#elif defined(SUPPORT_RF_FPGA)
           _reg_set_rf_bandwidth(ch, 0, mbw, get_fpga_reg());
#endif
            break; 
        }

        case EX_RF_MODE_CODE :{
            uint8_t noise_mode;
            /* noise_mode: 低失真模式(0x00)     常规模式(0x01) 低噪声模式(0x02) */
            noise_mode = *((uint8_t *)data);
            printf_note("[**RF**]ch=%d, noise_mode=%d\n", ch, noise_mode);
#ifdef defined(SUPPORT_RF_ADRV)
#elif  defined(SUPPORT_RF_SPI)
            ret = spi_rf_set_command(SPI_RF_NOISE_MODE_SET, &noise_mode);
            usleep(300);
            ret = spi_rf_get_command(SPI_RF_NOISE_MODE_GET, &noise_mode);
#elif defined(SUPPORT_RF_FPGA)
            _reg_set_rf_mode_code(ch, 0, noise_mode, get_fpga_reg());
#endif
            break; 
        }
        case EX_RF_GAIN_MODE :{
            break; 

        }
        case EX_RF_MGC_GAIN : {
            int8_t mgc_gain_value;
            mgc_gain_value = *((int8_t *)data);
            /* only set once when value changed */
            static int32_t old_data = -1000;
            if(old_data != mgc_gain_value){
                old_data = mgc_gain_value;
            }else{
                break;
            }
            printf_note("[**RF**]ch=%d, mgc_gain_value=%d\n",ch, mgc_gain_value);
#ifdef SUPPORT_RF_ADRV
            adrv_set_rx_gain(mgc_gain_value);
#elif defined(SUPPORT_RF_SPI)
            ret = spi_rf_set_command(SPI_RF_MIDFREQ_GAIN_SET, &mgc_gain_value);
            usleep(300);
            ret = spi_rf_get_command(SPI_RF_MIDFREQ_GAIN_GET, &mgc_gain_value);
#elif defined(SUPPORT_RF_FPGA)
            _reg_set_rf_mgc_gain(ch, 0, mgc_gain_value, get_fpga_reg());
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
#ifdef SUPPORT_RF_ADRV
            gpio_select_rf_attenuation(rf_gain_value);
#elif defined(SUPPORT_RF_SPI)
            ret = spi_rf_set_command(SPI_RF_GAIN_SET, &rf_gain_value);
            usleep(300);
            ret = spi_rf_get_command(SPI_RF_GAIN_GET, &rf_gain_value);
#elif defined(SUPPORT_RF_FPGA)
            _reg_set_rf_attenuation(ch, 0, rf_gain_value, get_fpga_reg());
#endif
            break; 
        }
        case EX_RF_CALIBRATE:
        {
            CALIBRATION_SOURCE_ST *akt_cs;
            struct calibration_source_t cs;
            akt_cs = (CALIBRATION_SOURCE_ST *)data;
            cs.source = (akt_cs->enable == 0) ? 0 : 1;
            cs.middle_freq_khz = akt_cs->middle_freq_hz/1000;
            cs.power = akt_cs->power + 30;
            if(cs.power > 0){
                cs.power = 0;
            }else if(cs.power < -60){
                cs.power = -60;
            }
            printf_note("source=%d, middle_freq_khz=%uKhz, power=%f\n", cs.source, cs.middle_freq_khz, cs.power);
#if defined(SUPPORT_RF_SPI)
            ret = spi_rf_set_command(SPI_RF_CALIBRATE_SOURCE_SET, &cs);
#endif
            break;
        }
        case EX_RF_SAMPLE_CTRL:
        {
#if defined(SUPPORT_RF_FPGA)
           
            uint8_t val = 0, _data;
            _data = *(uint8_t *)data;
             _reg_set_rf_direct_sample_ctrl(0, 0, _data, get_fpga_reg());
#endif
            break;
        }
        case EX_RF_LOW_NOISE:
        {
#if defined(SUPPORT_RS485_AMPLIFIER)
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
                printf_info("NOT find %uMhz in tables\n", freq_mhz);
                break;
            }
            
           static int8_t vdata_dup = -1;
            if(vdata_dup == vdata){
                printf_info("set EX_RF_LOW_NOISE value is equal:%d no need set\n" , vdata_dup);
                break;
            }else{
                vdata_dup = vdata;
                #if defined(SUPPORT_RS485)
                rs485_com_set_v2(RS_485_LOW_NOISE_SET_CMD, &vdata);
                #endif
                usleep(20000);
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

uint8_t rf_read_interface(uint8_t cmd,uint8_t ch,void *data, va_list ap){
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
            int i;
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_RF_SPI)
            ret = spi_rf_get_command(SPI_RF_TEMPRATURE_GET, &rf_temperature);
#elif defined(SUPPORT_RF_FPGA)
            rf_temperature = _reg_get_rf_temperature(ch, 0, get_fpga_reg());
            printf_note("[ch=%d]get rf temperature = %d\n", ch, rf_temperature);
#endif
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
#ifdef SUPPORT_RF_ADRV
    adrv_init();
#endif
#if defined(SUPPORT_RF_SPI)
    ret = spi_rf_init();
#endif
    return ret;
}



