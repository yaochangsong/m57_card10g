#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <poll.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/stat.h>
#include <termios.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include "reg_if.h"


static  bool _get_adc_status(void)
{
    FPGA_CONFIG_REG *reg = get_fpga_reg();
    uint32_t _reg = 0;
    #ifdef GET_ADC_STATUS
        _reg = GET_ADC_STATUS(reg);
    #endif
    if ((_reg & 0x03) == 0x03) {
        return true;
    } else {
        return false;
    }
}

static void _set_narrow_channel(int ch, int subch, int enable)
{
    FPGA_CONFIG_REG *reg = get_fpga_reg();
    uint32_t _reg;
    uint32_t val;
    val = reg->narrow_band[subch]->enable;
    _reg = (enable &0x01)|val;
    reg->narrow_band[subch]->enable = _reg;
    printf_debug("[subch:%d] _ch: %d, reg: 0x%x, val=0x%x enable:0x%x,%d\n", subch, ch, _reg, val, reg->narrow_band[subch]->enable, enable);
}

static void _set_narrow_agc_time(int ch, int subch, int mode)
{
    FPGA_CONFIG_REG *reg = get_fpga_reg();
    struct  agc_time_t{
        int mode;
        uint32_t val;
    }agc_time_table [] = {
        {1, 1},  //1*50 ms
        {2, 10}, //10*50
        {3, 50}, //50*50
    };

    uint32_t _val;
    int found = 0;
    for(int i = 0; i< ARRAY_SIZE(agc_time_table); i++){
        if (mode == agc_time_table[i].mode) {
            _val = agc_time_table[i].val;
            found ++;
            break;
        }
    }
    if(found == 0){
        printf_warn("agc_time:mode %d not find in table!",  mode);
        return;
    }
    printf_note("[subch:%d]agc time: mode=%d, val = %u\n", subch,  mode, _val);
    reg->narrow_band[subch]->agc_time = _val;
}


static void _set_narrow_audio_sample_rate(int ch, int subch, int rate)
{
    FPGA_CONFIG_REG *reg = get_fpga_reg();
    struct  sample_rate_t{
        int val;
        uint32_t sample_rate_khz;
    }sample_rate_table [] = {
        {0, 32},
        {1, 16},
        {3, 8},
    };

    uint32_t _reg, _val;
    int found = 0;
    
    for(int i = 0; i< ARRAY_SIZE(sample_rate_table); i++){
        if(rate == sample_rate_table[i].sample_rate_khz){
            found ++;
            _val = sample_rate_table[i].val;
            break;
        }
    }
    if(found == 0){
        printf_warn("sample rate:%dkhz not find in table!",  rate);
        return;
    }
    
    _reg = reg->narrow_band[subch]->audio_cfg;
    _reg = _reg & 0x0ffff;
    printf_note("sample rate raw val = %dKhz[0x%x]\n",  rate, _val);
    _val = (_val << 16) & 0x0ff0000;
    _reg = _reg | _val;
    printf_note("[subch:%d]sample rate reg val: regval=0x%x, val = 0x%x\n", subch,  _reg, _val);
    reg->narrow_band[subch]->audio_cfg = _reg;
    
}

/*
    agc -mgc gin mode set
    bit0-14 :mgc gain
    bit15 :mode
    bit16-31 :agc gain
*/
static inline void _set_narrow_audio_gain(int ch, int subch, float gain)
{
	#define SUB_CH_MAX_GAIN_VAL 90//(57.855)
    #define SUB_CH_MIN_GAIN_VAL 0 //(-30.0)
    uint32_t _reg;
    int32_t _val;
    FPGA_CONFIG_REG *reg = get_fpga_reg();
    /*  gain: -30dbm~64dbm(57.855dmb)
        reg = 10^(gain/20) * 32;  [1~25000]
        reg: 1~47546 [1~25000]
    */
    printf_note("set audio gain: raw gain = %f\n",  gain);
    if(f_sgn(gain-SUB_CH_MAX_GAIN_VAL) > 0){
        gain = SUB_CH_MAX_GAIN_VAL;
    } else if(f_sgn(gain-SUB_CH_MIN_GAIN_VAL) < 0){
        gain = SUB_CH_MIN_GAIN_VAL;
    }
    _reg = reg->narrow_band[subch]->agc_mgc_gain;
    _val = (int32_t)(pow(10, (float)gain/20.0));
    int mode = ((_reg & 0x08000) ?  POAL_MGC_MODE : POAL_AGC_MODE);
    if (mode == POAL_MGC_MODE) {
        _reg = _reg & 0xffff8000;
        _val = _val & 0x7fff;
        _reg = _reg | _val;
    } else {
        _reg = _reg & 0x0ffff;
        _val = (_val << 16) & 0xffff0000;
         _reg = _reg | _val;
    }
    printf_note("[subch:%d]set audio gain: %f, regval=0x%x, val = 0x%x\n", subch, gain, _reg, _val);
    reg->narrow_band[subch]->agc_mgc_gain = _reg;
}


static inline void _set_narrow_audio_gain_mode(int ch, int subch, int mode)
{
    #define SUB_CH_AGC_VAL 80.0//(57.855) /* 57dbm */
    FPGA_CONFIG_REG *reg = get_fpga_reg();
    uint32_t _reg, _val;
    _val = (mode == POAL_MGC_MODE ? 1 : 0); /* val=0: agc;  val=1, mgc */
    _reg = reg->narrow_band[subch]->agc_mgc_gain;
    _reg = _reg & (~0x08000);
     _val = (_val << 15) & 0x08000;
    _reg = _reg | _val;
    printf_note("[subch:%d]set audio gain mode: %s, regval=0x%x, val = 0x%x\n", subch, mode == POAL_MGC_MODE ? "mgc" : "agc", _reg, _val);
    reg->narrow_band[subch]->agc_mgc_gain = _reg;

#if 1
    if(mode == POAL_AGC_MODE){  /* agc */
        printf_note("AGC Mode,Set Agc gain: %fDbm\n", SUB_CH_AGC_VAL);
        _set_narrow_audio_gain(ch, subch, SUB_CH_AGC_VAL);
    }
#endif
}


static void _set_niq_channel(int ch, int subch, void *args, int enable)
{
    FPGA_CONFIG_REG *reg = get_fpga_reg();
    uint32_t _reg;

    _reg = enable &0x01;
    reg->narrow_band[subch]->enable = _reg;
}

static void _set_fft_channel(int ch, void *args)
{
/*
    ch   频段范围（MHz）
    0	108～228
    1	220～340
    2	335～455
    3	450～570
*/
    FPGA_CONFIG_REG *reg = get_fpga_reg();
    struct  freq_rang_t{
        int ch;
        uint64_t s_freq;
        uint64_t e_freq;
    }freq_rang [] = {
            {0, MHZ(108), MHZ(228)},
            {1, MHZ(220), MHZ(340)},
            {2, MHZ(335), MHZ(455)},
            {3, MHZ(450), MHZ(570)},
        };
    uint32_t _reg;
    uint32_t val;
    int _ch = ch;

    if(args != NULL && _ch < 0){
        uint64_t freq = *(uint64_t *)args;
        for(int i = 0; i< ARRAY_SIZE(freq_rang); i++){
            if(freq >= freq_rang[i].s_freq && freq <= freq_rang[i].e_freq){
                _ch = freq_rang[i].ch;
                break;
            }
        }
    }

    _reg = reg->broad_band[0]->enable;
    val = _ch << 8;
    _reg = (_reg & 0x00ff)|val;
    reg->broad_band[0]->enable = _reg;
    printf_debug("[ch:%d]set reg: 0x%x, val=0x%x 0x%x\n", ch,  _reg, val, reg->broad_band[0]->enable);
}



static void _set_biq_channel(int ch, void *args)
{
    FPGA_CONFIG_REG *reg = get_fpga_reg();
 /*
    ch   频段范围（MHz）
    0	108～228
    1	220～340
    2	335～455
    3	450～570
*/
    struct  freq_rang_t{
        int ch;
        uint64_t s_freq;
        uint64_t e_freq;
    }freq_rang [] = {
            {0, MHZ(108), MHZ(228)},
            {1, MHZ(220), MHZ(340)},
            {2, MHZ(335), MHZ(455)},
            {3, MHZ(450), MHZ(570)},
        };
    uint32_t _reg;
    uint32_t val;
    int _ch = ch;
    if(args != NULL && _ch < 0){
        uint64_t freq = *(uint64_t *)args;
        for(int i = 0; i< ARRAY_SIZE(freq_rang); i++){
            if(freq >= freq_rang[i].s_freq && freq <= freq_rang[i].e_freq){
                _ch = freq_rang[i].ch;
                break;
            }
        }
    }

    _reg = reg->broad_band[1]->enable;
    val = _ch << 8;
    _reg = (_reg & 0x00ff)|val;
    reg->broad_band[1]->enable = _reg;
    printf_note(" BIQ [ch:%d]set reg: 0x%x, val=0x%x 0x%x\n", ch,  _reg, val, reg->broad_band[1]->enable);
}



static const struct if_reg_ops if_reg = {
    .get_adc_status = _get_adc_status,
    .set_niq_channel = _set_niq_channel,
    .set_fft_channel = _set_fft_channel,
    .set_biq_channel = _set_biq_channel,
    .set_narrow_channel = _set_narrow_channel,
    .set_narrow_audio_sample_rate = _set_narrow_audio_sample_rate,
    .set_narrow_audio_gain = _set_narrow_audio_gain,
    .set_narrow_audio_gain_mode = _set_narrow_audio_gain_mode,
    .set_narrow_agc_time = _set_narrow_agc_time,
};


struct if_reg_ops * if_create_reg_ctx(void)
{
    struct if_reg_ops *ctx = calloc(1, sizeof(*ctx));
     if (!ctx)
        return NULL;
     
    ctx = &if_reg;
    return ctx;
}
