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
            printf_note("freq_khz=%ukhz\n", freq_khz);
            SET_RF_MID_FREQ(get_fpga_reg(),freq_khz);
            //get_fpga_reg()->rfReg->freq_khz = freq_khz;
            usleep(300);
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
            SET_RF_BAND(get_fpga_reg(),set_val);
            //get_fpga_reg()->rfReg->mid_band = set_val;
            usleep(100);
            SET_RF_BAND(get_fpga_reg(),set_val);
            //get_fpga_reg()->rfReg->mid_band = set_val;
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
           //get_fpga_reg()->rfReg->rf_mode = set_val;
            SET_RF_MODE(get_fpga_reg(),set_val);
            usleep(100);
            //get_fpga_reg()->rfReg->rf_mode = set_val;
            SET_RF_MODE(get_fpga_reg(),set_val);
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
            SET_RF_IF_ATTENUATION(get_fpga_reg(),mgc_gain_value);
            //get_fpga_reg()->rfReg->midband_minus = mgc_gain_value;
            usleep(100);
            //get_fpga_reg()->rfReg->midband_minus = mgc_gain_value;
            SET_RF_IF_ATTENUATION(get_fpga_reg(),mgc_gain_value);
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
            SET_RF_ATTENUATION(get_fpga_reg(),rf_gain_value);
            //get_fpga_reg()->rfReg->rf_minus = rf_gain_value;
            usleep(100);
            //get_fpga_reg()->rfReg->rf_minus = rf_gain_value;
            SET_RF_ATTENUATION(get_fpga_reg(),rf_gain_value);
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

#if defined(SUPPORT_RF_FPGA)
                        //uint8_t val = 0;
                        //val = (*((int8_t *)data) == 0 ? 0 : 0x01);
                        /* 0关闭直采，1开启直采 */
                        //usleep(500);
                        SET_RF_MID_FREQ(get_fpga_reg(),cs.middle_freq_khz);
                        //get_fpga_reg()->rfReg->freq_khz = cs.middle_freq_khz;
                        usleep(500);
                        SET_RF_CALIB_SOURCE_CHOISE(get_fpga_reg(),cs.source);
                        //get_fpga_reg()->rfReg->input = cs.source;
                        usleep(500);
                        SET_RF_CALIB_SOURCE_ATTENUATION(get_fpga_reg(),-akt_cs->power);
                        //get_fpga_reg()->rfReg->revise_minus = -akt_cs->power;
                        usleep(500);
                       // printf_note("write:input=%d revise_minus=%d  freq_khz=%x\n",cs.source,-akt_cs->power,cs.middle_freq_khz);
                        //printf_note("read: input=%d revise_minus=%d  freq_khz=%x\n",get_fpga_reg()->rfReg->input&0xffff,get_fpga_reg()->rfReg->revise_minus&0xffff,get_fpga_reg()->rfReg->freq_khz);
#endif

            break;
        }
        case EX_RF_SAMPLE_CTRL:
        {
#if defined(SUPPORT_RF_FPGA)
            uint8_t val = 0, _data;
            _data = *(uint8_t *)data;
            val = (_data == 0 ? 0 : 0x01);
            /* 0关闭直采，1开启直采 */
            printf_note("[**RF**] direct control=%d, %d\n", val, _data);
            SET_RF_DIRECT_SAMPLE_CTRL(get_fpga_reg(),val);
            //get_fpga_reg()->rfReg->direct_control = val;
            usleep(100);
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
                printf_info("NOT find %uMhz in tables\n", freq_mhz);
                break;
            }
            
#if defined(SUPPORT_RS485_AMPLIFIER)
           static int8_t vdata_dup = -1;
            if(vdata_dup == vdata){
                printf_info("set EX_RF_LOW_NOISE value is equal:%d no need set\n" , vdata_dup);
                break;
            }else{
                vdata_dup = vdata;
                rs485_com_set_v2(RS_485_LOW_NOISE_SET_CMD, &vdata);
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
#if defined(SUPPORT_RF_SPI)
            
            ret = spi_rf_get_command(SPI_RF_TEMPRATURE_GET, &rf_temperature);
#elif defined(SUPPORT_RF_FPGA)
#define RF_TEMPERATURE_FACTOR 0.0625
           // rf_temperature = get_fpga_reg()->rfReg->temperature;
            rf_temperature = GET_RF_TEMPERATURE(get_fpga_reg());
            usleep(100);
            //rf_temperature = get_fpga_reg()->rfReg->temperature;
            rf_temperature = GET_RF_TEMPERATURE(get_fpga_reg());
            printf_debug("rf temprature 0x%x\n", rf_temperature);
            rf_temperature = rf_temperature * RF_TEMPERATURE_FACTOR;
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

static int  _calibration_source_loop(void *args)
{
    struct  calibration_source_args_t *cs_args = args;
    int i, count = 0;
    uint32_t r_time_us = cs_args->r_time_ms *1000;
    uint64_t middle_freq_khz = 0;
    count = (cs_args->e_freq - cs_args->s_freq)/cs_args->step;
    middle_freq_khz = cs_args->rf_args.middle_freq_khz;
    io_set_rf_calibration_source_enable(1);
    io_set_rf_calibration_source_level(cs_args->rf_args.power);
    for(i = 0; i< count; i++){
        //get_fpga_reg()->rfReg->freq_khz = middle_freq_khz + i * cs_args->step/1000;
        SET_RF_MID_FREQ(get_fpga_reg(),middle_freq_khz + i * cs_args->step/1000);
        printf_note("[%d]freq_khz=%u, start_mid_khz=%d, count=%d, r_time_us=%d\n", i, middle_freq_khz+ i * cs_args->step/1000, middle_freq_khz, count, r_time_us);
        usleep(r_time_us);
        pthread_setcancelstate (PTHREAD_CANCEL_ENABLE , NULL);
        pthread_testcancel();
    }
    return 0;
}
#define THREAD_CAL_SOURCE_NAME "CAL_SOURCE"
static void  _calibration_source_exit(void *args)
{
    printf_note(">>>>>>>_calibration source exit!!\n");
    if(args != NULL)
        safe_free(args);
}

int rf_calibration_source_start(void *args)
{
    CALIBRATION_SOURCE_ST_V2 *cal = args;
    struct calibration_source_t cs;
    int ret = -1;
    struct  calibration_source_args_t *cs_args;

    cs_args = safe_malloc(sizeof(struct calibration_source_args_t));
    cs_args->rf_args.middle_freq_khz = cal->middle_freq_hz/1000;
    cs_args->rf_args.power = cal->power;
    cs_args->rf_args.source = cal->enable;
    cs_args->e_freq = cal->e_freq;
    cs_args->s_freq = cal->s_freq;
    cs_args->step = cal->step;
    cs_args->r_time_ms = cal->r_time_ms;
    if(cs_args->s_freq > cs_args->e_freq)
        goto error_exit;
    
    ret =  pthread_create_detach (NULL, _calibration_source_loop, _calibration_source_exit,  
                                    THREAD_CAL_SOURCE_NAME, cs_args, cs_args);
    if(ret != 0){
        perror("pthread create calibration source thread!!");
        goto error_exit;
    }
    
    return 0;
error_exit:
    safe_free(args);
    return -1;
}

int rf_calibration_source_stop(void *args)
{
    pthread_cancel_by_name(THREAD_CAL_SOURCE_NAME);
    io_set_rf_calibration_source_enable(0);
    return 0;
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



