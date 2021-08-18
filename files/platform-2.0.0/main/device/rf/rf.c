#include "config.h"
#include "../../bsp/io.h"



struct rf_ctx *rfctx = NULL;

struct rf_ctx *get_rfctx(void)
{
    return rfctx;
}


static int rf_set_low_noise(uint64_t midfreq)
{
    uint32_t freq_mhz;
    int found = 0, i, ret = -1;
    int8_t vdata = 0;

    struct _amp_segment_table{
        int  index;
        uint32_t s_freq_mhz;
        uint32_t e_freq_mhz;
    } amp_seg_table[] = {
        {1, 30,     1350},
        {2, 1350,   2700},
        {3, 2700,   3000},
    };

    freq_mhz = midfreq/1000000;/* hz=>mhz */
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
        printf_info("NOT find %uMhz in tables\n", freq_mhz);
        return -1;
    }

    static int8_t vdata_dup = -1;
    if(vdata_dup == vdata){
        printf_info("set EX_RF_LOW_NOISE value is equal:%d no need set\n" , vdata_dup);
        return -1;
    }else{
        vdata_dup = vdata;
#if defined(CONFIG_DEVICE_RS485)
        ret = rs485_com_set_v2(RS_485_LOW_NOISE_SET_CMD, &vdata);
#endif
        usleep(20000);
    }
    return ret;
}


uint8_t rf_set_interface(uint8_t cmd,uint8_t ch,void *data)
{
    uint8_t ret = -1;
    switch(cmd){
        case EX_RF_MID_FREQ :
        {
            static uint64_t freq_hz = 0;/* Hz */
            static int32_t old_ch=-1;
            if((freq_hz == *(uint64_t*)data) && (ch == old_ch)){
                /* 避免重复设置相同参数 */
                break;
            }
            freq_hz = *(uint64_t*)data;
            old_ch = ch;
            printf_debug("[**RF**]ch=%d, middle_freq=%"PRIu64"\n",ch, *(uint64_t*)data);
            if(rfctx && rfctx->ops->set_mid_freq)
                ret = rfctx->ops->set_mid_freq(ch, freq_hz);
            break;
        }
        case EX_RF_MID_BW:
        {
            uint32_t bw_hz;
            /* data=NULL, 读取配置中频带宽 */
            if(data == NULL){
                if(config_read_by_cmd(EX_RF_FREQ_CMD, EX_RF_MID_BW, ch, &bw_hz) == -1){
                    printf_err("Error read scan bandwidth=%u\n", bw_hz);
                    break;
                }
            }else{
                bw_hz= *(uint32_t *)data;
            }
            printf_note("[**RF**]ch=%d, middle bw=%uHz\n", ch, bw_hz);
            if(rfctx && rfctx->ops->set_bindwidth)
                ret = rfctx->ops->set_bindwidth(ch, bw_hz);
            break;
        }
        case EX_RF_MODE_CODE:
        {
            static uint8_t old_mode = 0;
            static int32_t old_ch=-1;
            volatile uint8_t noise_mode;
            noise_mode = *((uint8_t *)data);
            if((old_mode == noise_mode) && (ch == old_ch)){
                /* 避免重复设置相同参数 */
                break;
            }
            old_mode = noise_mode;
            old_ch = ch;
            /* noise_mode: 低失真模式(0x00)     常规模式(0x01) 低噪声模式(0x02) */
            if(rfctx && rfctx->ops->set_work_mode)
                ret = rfctx->ops->set_work_mode(ch, noise_mode);
            break;
        }
        case EX_RF_MGC_GAIN:
        {
            volatile int8_t mgc_gain_value;
            mgc_gain_value = *((int8_t *)data);
            printf_note("[**RF**]ch=%d, mgc_gain_value=%d\n",ch, mgc_gain_value);
            if(rfctx && rfctx->ops->set_mgc_gain)
                ret = rfctx->ops->set_mgc_gain(ch, mgc_gain_value);
            break;
        }
        case EX_RF_ATTENUATION:
        {
            int8_t rf_gain_value;
            rf_gain_value = *((int8_t *)data);
            printf_note("[**RF**]ch=%d, rf_gain_value=%d\n",ch, rf_gain_value);
            if(rfctx && rfctx->ops->set_rf_gain)
                ret = rfctx->ops->set_rf_gain(ch, rf_gain_value);
            break;
        }
        case EX_RF_LOW_NOISE:
        {
            ret = rf_set_low_noise(*((uint64_t *)data));
            break;
        }
        default:{
            break;
        }
    }
    
    return ret;
}

int8_t rf_read_interface(uint8_t cmd,uint8_t ch,void *data, va_list ap)
{
    int8_t ret = -1;
    
    switch(cmd){
        case EX_RF_MODE_CODE :
        {
            uint8_t mode = 0;
            if(rfctx && rfctx->ops->get_work_mode)
                ret = rfctx->ops->get_work_mode(ch, &mode);
            if(ret != -1)
                *(uint8_t *)data = mode;
            break;
        }
        case EX_RF_MGC_GAIN:
        {
            uint8_t gain = 0;
            if(rfctx && rfctx->ops->get_mgc_gain)
                ret = rfctx->ops->get_mgc_gain(ch, &gain);
            if(ret != -1)
                *(uint8_t *)data = gain;
            break;
        }
        case EX_RF_ATTENUATION:
        {
            uint8_t gain = 0;
            if(rfctx && rfctx->ops->get_rf_gain)
                ret = rfctx->ops->get_rf_gain(ch, &gain);
            if(ret != -1)
                *(uint8_t *)data = gain;
            break;
        }
        case EX_RF_STATUS_TEMPERAT:
        {
            int16_t temperature = 0;
            if(rfctx && rfctx->ops->get_temperature)
                ret = rfctx->ops->get_temperature(ch, &temperature);
            if(ret != -1)
                *(int16_t *)data = temperature;
            break;
        }
        case EX_RF_STATUS:
        {
            bool status = false;
            if(rfctx && rfctx->ops->get_status){
                status = rfctx->ops->get_status(ch);
            }
            *(int8_t *)data = status;
            ret = (status == false ? -1 : 0);
            break;
        }
        default:{
            break;
        }
    }

    return ret;
}


int rf_init(void)
{
    bool ret = false;
    struct rf_ctx *rf;
    rf = rf_create_context();
    if(rf){
        if(rf->ops->get_status)
            ret = rf->ops->get_status(0);
        rfctx = rf;
        io_set_rf_status(ret);
    }
    printf_note("RF Check[%s]!\n", ret == true ? "OK" : "Faild");
    
    return ret;
}
