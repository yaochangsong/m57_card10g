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

static void _set_narrow_audio_gain(int ch, int subch, float gain)
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

    _reg = reg->narrow_band[subch]->audio_cfg;
    _reg = _reg & 0xff8000;
    _val = (int32_t)(pow(10, (float)gain/20.0));
    _reg = _reg | _val;
    printf_note("[subch:%d]set audio gain: %f, regval=0x%x, val = 0x%x\n", subch, gain, _reg, _val);
    reg->narrow_band[subch]->audio_cfg = _reg;
}

static inline void _set_narrow_audio_gain_mode(int ch, int subch, int mode)
{
    #define SUB_CH_AGC_VAL 80.0//(57.855) /* 57dbm */
    FPGA_CONFIG_REG *reg = get_fpga_reg();
    uint32_t _reg, _val;
    _val = (mode == POAL_MGC_MODE ? 1 : 0); /* val=0: agc;  val=1, mgc */
    _reg = reg->narrow_band[subch]->audio_cfg;
    _reg = _reg & (~0x08000);
    _reg = _reg & 0x0ffffff;
    _val = (_val << 15) & 0x08000;
    _reg = _reg | _val;
    printf_note("[subch:%d]set audio gain mode: %s, regval=0x%x, val = 0x%x\n", subch, mode == POAL_MGC_MODE ? "mgc" : "agc", _reg, _val);
    reg->narrow_band[subch]->audio_cfg = _reg;
    if(mode == POAL_AGC_MODE){  /* agc */
        printf_note("AGC Mode,Set Agc gain: %fDbm\n", SUB_CH_AGC_VAL);
        _set_narrow_audio_gain(ch, subch, SUB_CH_AGC_VAL);
    }
}

static void _set_niq_channel(int ch, int subch, void *args, int enable)
{
    uint32_t _reg;
    uint32_t val;
    FPGA_CONFIG_REG *reg = get_fpga_reg();
    
    if(enable)
        _reg = 0x01;
    else
        _reg = 0x00;
    val = ch << 8;
    _reg = (_reg & 0x00ff)|val;
    reg->narrow_band[subch]->enable = _reg;
    
    if(enable)
        printf_debug("[subch:%d] _ch: %d, set reg: 0x%x, val=0x%x 0x%x, freq=%"PRIu64"\n", subch, ch, _reg, val, reg->narrow_band[subch]->enable, *(uint64_t *)args);
    else
        printf_debug("[subch:%d] _ch: %d, set reg: 0x%x, val=0x%x 0x%x\n", subch, ch, _reg, val, reg->narrow_band[subch]->enable);
}

static void _set_biq_channel(int ch, void *args)
{
    uint32_t _reg;
    uint32_t val;
    FPGA_CONFIG_REG *reg = get_fpga_reg();
    int _ch = ch;

    _reg = reg->broad_band[2]->enable;
    val = _ch << 8;
    _reg = (_reg & 0x00ff)|val;
    reg->broad_band[2]->enable = _reg;
    printf_note(" BIQ [ch:%d]set reg: 0x%x, val=0x%x 0x%x\n", ch,  _reg, val, reg->broad_band[2]->enable);
}



static const struct if_reg_ops if_reg = {
    .get_adc_status = _get_adc_status,
    .set_niq_channel = _set_niq_channel,
};


struct if_reg_ops * if_create_reg_ctx(void)
{
    struct if_reg_ops *ctx = calloc(1, sizeof(*ctx));
     if (!ctx)
        return NULL;
     
    ctx = &if_reg;
    return ctx;
}
