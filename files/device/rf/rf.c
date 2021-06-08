#include "config.h"
#include "../../bsp/io.h"

//#include "../../executor/spm/io_fpga.h"



uint8_t rf_set_interface(uint8_t cmd,uint8_t ch,void *data){
    uint8_t ret = -1;

    switch(cmd){
        case EX_RF_MID_FREQ :{
            /* only set once when value changed */
            static uint64_t old_freq = 0;/* Hz */
            static int32_t old_ch=-1;
            if((old_freq == *(uint64_t*)data) && (ch == old_ch)){
                /* 避免重复设置相同参数 */
                break;
            }
            old_freq = *(uint64_t*)data;
            old_ch = ch;
            printf_debug("[**RF**]ch=%d, middle_freq=%"PRIu64"\n",ch, *(uint64_t*)data);
#ifdef SUPPORT_RF_ADRV
            uint64_t mid_freq = *(uint64_t*)data;
       #ifdef SUPPORT_PROJECT_SSA_MONITOR
            mid_freq = lvttv_freq_convert(mid_freq);
       #endif
            gpio_select_rf_channel(mid_freq);
            adrv_set_rx_freq(mid_freq);
#elif defined(SUPPORT_RF_SPI)
    #if defined(SUPPORT_RF_SPI_TF713)
            uint64_t freq_khz = old_freq/1000;/* NOTE: Hz => KHz */
            ret = spi_rf_set_command(ch, SPI_RF_FREQ_SET, &freq_khz);
            usleep(30);
    #else
            uint64_t freq_khz = old_freq/1000;/* NOTE: Hz => KHz */
            uint64_t host_freq=htobe64(freq_khz) >> 24; //小端转大端（文档中心频率为大端字节序列，5个字节,单位为Hz,实际为Khz）
            uint64_t recv_freq = 0, recv_freq_htob;

            for(int i = 0; i<3; i++){
                ret = spi_rf_set_command(ch, SPI_RF_FREQ_SET, &host_freq);
                usleep(300);
                recv_freq = 0;
                ret = spi_rf_get_command(ch, SPI_RF_FREQ_GET, &recv_freq, 0);
                recv_freq_htob =  (htobe64(recv_freq) >> 24) ;  /* khz */
                printf_debug("host_freq=%"PRIu64", 0x%llx, recv_freq = %"PRIu64", 0x%llx, htobe64=0x%llx\n",freq_khz, freq_khz,recv_freq ,recv_freq,recv_freq_htob);
                if(recv_freq_htob == freq_khz){
                        break;
                }
            }
     #endif
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
#if defined(SUPPORT_RF_SPI_TF713)
            if(mbw == MHZ(160)) mbw = 1;
            else    mbw = 0;
#endif
            ret = spi_rf_set_command(ch, SPI_RF_MIDFREQ_BW_FILTER_SET, &mbw);
            //usleep(300);
            //ret = spi_rf_get_command(ch, SPI_RF_MIDFREQ_BW_FILTER_GET, &mbw);
#elif defined(SUPPORT_RF_FPGA)
           _reg_set_rf_bandwidth(ch, 0, mbw, get_fpga_reg());
#endif
            break; 
        }

        case EX_RF_MODE_CODE :{
            static uint8_t old_mode = 0;/* Hz */
            static int32_t old_ch=-1;
            uint8_t noise_mode;
            /* noise_mode: 低失真模式(0x00)     常规模式(0x01) 低噪声模式(0x02) */
            noise_mode = *((uint8_t *)data);
            if((old_mode == noise_mode) && (ch == old_ch)){
                break;
            }
            old_mode = noise_mode;
            old_ch = ch;
#if defined(SUPPORT_RF_SPI_TF713)
            /* 常规和低噪声参数交换 */
            if(noise_mode == 0x02) noise_mode = 0x01;
            else  noise_mode = 0x0;
#endif
            printf_note("[**RF**]ch=%d, noise_mode=%d\n", ch, noise_mode);
#if defined(SUPPORT_RF_ADRV)
        
#elif  defined(SUPPORT_RF_SPI)
            ret = spi_rf_set_command(ch, SPI_RF_NOISE_MODE_SET, &noise_mode);
            //usleep(300);
            //ret = spi_rf_get_command(ch, SPI_RF_NOISE_MODE_GET, &noise_mode);
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
            //static int32_t old_data = -1000;
            //if(old_data != mgc_gain_value){
            //    old_data = mgc_gain_value;
           // }else{
           //     break;
           // }
            printf_note("[**RF**]ch=%d, mgc_gain_value=%d\n",ch, mgc_gain_value);
#ifdef SUPPORT_RF_ADRV
            adrv_set_rx_gain(mgc_gain_value);
#elif defined(SUPPORT_RF_SPI)
            ret = spi_rf_set_command(ch, SPI_RF_MIDFREQ_GAIN_SET, &mgc_gain_value);
            //usleep(300);
            //ret = spi_rf_get_command(ch, SPI_RF_MIDFREQ_GAIN_GET, &mgc_gain_value);
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
            ret = spi_rf_set_command(ch, SPI_RF_GAIN_SET, &rf_gain_value);
            //usleep(300);
            //ret = spi_rf_get_command(ch, SPI_RF_GAIN_GET, &rf_gain_value);
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
            printf_note("source=%d, middle_freq_khz=%uKhz, power=%d\n", cs.source, cs.middle_freq_khz, cs.power);
#if defined(SUPPORT_RF_SPI)
            ret = spi_rf_set_command(ch, SPI_RF_CALIBRATE_SOURCE_SET, &cs);
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

int8_t rf_read_interface(uint8_t cmd,uint8_t ch,void *data, va_list ap){
    int8_t ret = -1;

    switch(cmd){
        case EX_RF_MID_FREQ : {
            printf_debug("rf_read_interface %d\n",EX_RF_MID_FREQ);
            break;
        }
        case EX_RF_MID_BW :   {
            break; 
        }
        case EX_RF_MODE_CODE :{
#if defined(SUPPORT_RF_SPI_TF713)
            uint8_t str[6]={0};
            uint8_t *p_str = NULL;
            ret = spi_rf_get_command(ch, SPI_RF_WORK_PARA_GET, str, sizeof(str));
            p_str=str;
            *(uint8_t *)data = *(p_str+4)&0x07;
#endif
            break; 
        }
        case EX_RF_GAIN_MODE :{
            break; 
        }
        case EX_RF_MGC_GAIN : {
#if defined(SUPPORT_RF_SPI_TF713)
            uint8_t str[6] = {0};
            uint8_t *p_str = NULL;
            ret = spi_rf_get_command(ch, SPI_RF_WORK_PARA_GET, str, sizeof(str));
            p_str=str;
            *(int8_t *)data =((*(p_str+4)&0xf8)>>3)|((*(p_str+3)&0x01)<<5);
#endif
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
#if defined(SUPPORT_RF_SPI_TF713)
            uint8_t str[6]={0};
            uint8_t *p_str = NULL;
            ret = spi_rf_get_command(ch, SPI_RF_WORK_PARA_GET, str, sizeof(str));
            p_str=str;
            *(int8_t *)data = ((*(p_str+3)&0x7e)>>1);
#endif
            break; 
        }
        case EX_RF_STATUS_TEMPERAT :{
            int16_t  rf_temperature = 0;
#if defined(SUPPORT_PLATFORM_ARCH_ARM)
#if defined(SUPPORT_RF_SPI)
            ret = spi_rf_get_command(ch, SPI_RF_TEMPRATURE_GET, &rf_temperature, sizeof(int16_t));
#if defined(SUPPORT_RF_SPI_TF713)
            int16_t sign_bit;
            rf_temperature = ntohs(rf_temperature);
            sign_bit=(rf_temperature&0x2000)>>13;    //sign_bit=0正数， sign_bit=1负数
            if(sign_bit == 0){
              rf_temperature = rf_temperature/32.0;
            }
            else if(sign_bit == 1){
              rf_temperature= (rf_temperature-16384)/32.0;
            }
#endif
#elif defined(SUPPORT_RF_FPGA)
            rf_temperature = _reg_get_rf_temperature(ch, 0, get_fpga_reg());
            printf_debug("[ch=%d]get rf temperature = %d\n", ch, rf_temperature);
#endif
#endif
            *(int16_t *)data = rf_temperature;
            printf_info("ch:%d, rf temprature:%d\n", ch, *(int16_t *)data);
            break;
        }
        case EX_RF_STATUS:
        {
            uint8_t status = 0;
#if defined(SUPPORT_RF_SPI_TF713)
            spi_rf_get_command(ch, SPI_RF_HANDSHAKE, &status, sizeof(status));
            status = status & 0x000ff;
            if((status == 0xffffffa8) || (status == 0xa8)){
                ret = 0; /* ok */
            }
            else
            ret = -1;
#endif
            if(ret == -1){
                io_set_rf_status(false);
            }else
                io_set_rf_status(true);
            
            if(data)
                *(int8_t *)data = ret;
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
    ret = executor_get_command(EX_RF_FREQ_CMD, EX_RF_STATUS, 0,  NULL);
    printf_warn("RF SPI Check[%s]!\n", ret == 0 ? "OK" : "Faild");
#endif
    return ret;
}



