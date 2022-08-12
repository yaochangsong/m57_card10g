#ifndef __RF_REG_H__
#define __RF_REG_H__

#include "config.h"

extern struct rf_reg_ops * rf_create_reg_ctx(void);


#if 0
/* RF */
/*GET*/
/* 
@ch: rf control channel
@index: a channel may have multiple RF controls
*/
static inline int32_t _reg_get_rf_temperature(int ch, int index, FPGA_CONFIG_REG *reg)
{
#define RF_TEMPERATURE_FACTOR 0.0625
    int16_t  rf_temp_reg = 0;
    uint32_t rf_temperature, tye_count = 0;
    bool read_ok = false, time_out = false;

    index = index;
    if(reg->rfReg[ch] == NULL || ch >= MAX_RF_NUM)
        return 0;

    do{
        rf_temp_reg = get_rf_safe(ch, &reg->rfReg[ch]->temperature);
        /* if temperature <30 or > 100, we think read false; try again 
          NOTE: reg*0.0625
        */
        if(rf_temp_reg > 1600 || rf_temp_reg < 480 || rf_temp_reg == 0){
            read_ok = false;
        }else{
            read_ok = true;
        }
        if(++tye_count > 4)
            time_out = true;
    }while(!read_ok && !time_out);
    printf_debug("read ch=%d,rf_temperature = 0x%x, tye_count=%d, read_ok=%d\n",
                ch, rf_temp_reg,  tye_count, read_ok);
    
    if(read_ok == false)
        return -1;
    
    rf_temperature = rf_temp_reg * RF_TEMPERATURE_FACTOR;
    return rf_temperature;
}


static inline void _reg_set_rf_attenuation(int ch, int index, uint8_t atten, FPGA_CONFIG_REG *reg)
{
    if(ch >= MAX_RADIO_CHANNEL_NUM)
        return;
    if(atten > 30)
        atten = 30;
    else if(atten < 0)
        atten = 0;
    
    if(reg->rfReg[ch] == NULL)
        return;
    
    set_rf_safe(ch, &reg->rfReg[ch]->rf_minus, atten);
    set_rf_safe(ch, &reg->rfReg[ch]->rf_minus, atten);
}

static inline void _reg_set_rf_bandwidth(int ch, int index, uint32_t bw_hz, FPGA_CONFIG_REG *reg)
{
    int i, found = 0;
    uint8_t set_val = 0;
    if(ch >= MAX_RADIO_CHANNEL_NUM)
        return;
    /* 400KH(0) 4MHz(0x01) 40MHz(0x02) 200MHz(0x03)  */
    struct rf_bw_reg{
        uint32_t bw_hz;
        uint8_t reg_val;
    };
    struct rf_bw_reg _reg[] = {
            {400000,    0x00},
            {4000000,   0x01},
            {40000000,  0x02},
            {200000000, 0x03},
    };

    for(i = 0; i < ARRAY_SIZE(_reg); i++){
        if(bw_hz == _reg[i].bw_hz){
            set_val = _reg[i].reg_val;
            found ++;
        }
    }
    if(found == 0){
        printf("NOT found bandwidth %uHz in tables,use default[200Mhz]\n", bw_hz);
        set_val = 0x03; /* default 200MHz */
    }
    
    if(reg->rfReg[ch] == NULL)
        return;
    
    set_rf_safe(ch, &reg->rfReg[ch]->mid_band, set_val);
    set_rf_safe(ch, &reg->rfReg[ch]->mid_band, set_val);
}

static inline void _reg_set_rf_mode_code(int ch, int index, uint8_t code, FPGA_CONFIG_REG *reg)
{
        int i, found = 0;
        uint8_t set_val = 0;
        /*这里需要转换 =>常规模式(0) 低噪声模式(0x01) 低失真模式(0x02)    */
        struct _reg{
            uint8_t mode;
            uint8_t reg_val;
        };
        
        struct _reg _reg[] = {
                {0,     0x02},
                {1,     0x00},
                {2,     0x01},
        };

        if(ch >= MAX_RADIO_CHANNEL_NUM)
            return;
        
        for(i = 0; i < ARRAY_SIZE(_reg); i++){
            if(code == _reg[i].mode){
                set_val = _reg[i].reg_val;
                found ++;
            }
        }
        if(found == 0){
            printf("NOT found noise mode %uHz in tables,use default normal mode[0]\n", code);
            set_val = 0x00; /* default normal mode */
        }
        
        if(reg->rfReg[ch] == NULL)
            return;

        set_rf_safe(ch, &reg->rfReg[ch]->rf_mode, set_val);
        set_rf_safe(ch, &reg->rfReg[ch]->rf_mode, set_val);
}

static inline void _reg_set_rf_mgc_gain(int ch, int index, uint8_t gain, FPGA_CONFIG_REG *reg)
{
    if(ch >= MAX_RADIO_CHANNEL_NUM)
        return;
    if(gain > 30)
        gain = 30;
    else if(gain < 0)
        gain = 0;
    
    if(reg->rfReg[ch] == NULL)
        return;

    set_rf_safe(ch, &reg->rfReg[ch]->midband_minus, gain);
    set_rf_safe(ch, &reg->rfReg[ch]->midband_minus, gain);
}


/* RF */
/*SET*/
static inline void _reg_set_rf_frequency(int ch, int index, uint64_t freq_hz, FPGA_CONFIG_REG *reg)
{
    uint32_t freq_khz = freq_hz/1000;
    reg->rfReg[0]->freq_khz = freq_khz;
    usleep(300);
    reg->rfReg[0]->freq_khz = freq_khz;
}

static inline void _reg_set_rf_cali_source_attenuation(int ch, int index, uint8_t level, FPGA_CONFIG_REG *reg)
{
    /*  射频模块-90～-30dBm 功率可调，步进 1dB 
        接收机校正衰减有效范围为 寄存器0 dB～60dB
    */
    int reg_val;
    index = index;
    
    if(ch >= MAX_RADIO_CHANNEL_NUM)
        return;
    
    if(level > -30)
        level = -30;
    else if(level < -90)
        level = -90;
    level = -level;
    reg_val = level - 30;

    if(reg->rfReg[ch] == NULL)
        return;
    
    set_rf_safe(ch, &reg->rfReg[ch]->revise_minus, reg_val);
    set_rf_safe(ch, &reg->rfReg[ch]->revise_minus, reg_val);
}


static inline void _reg_set_rf_direct_sample_ctrl(int ch, int index, uint8_t val, FPGA_CONFIG_REG *reg)
{
    uint8_t data;

    
    index = index;
    if(ch >= MAX_RADIO_CHANNEL_NUM)
        return;
    data = (val == 0 ? 0 : 0x01);
    /* 0关闭直采，1开启直采 */
    if(reg->rfReg[ch] == NULL)
        return;
    set_rf_safe(ch, &reg->rfReg[ch]->direct_control, data);
    set_rf_safe(ch, &reg->rfReg[ch]->direct_control, data);
}


static inline void _reg_set_rf_cali_source_choise(int ch, int index, uint8_t val, FPGA_CONFIG_REG *reg)
{
    int _reg;
    if(ch >= MAX_RADIO_CHANNEL_NUM)
        return;
    
    /* 0 为外部射频输入，1 为校正源输入 */
    _reg = (val == 0 ? 0 : 1);
    if(reg->rfReg[ch] == NULL)
        return;
    set_rf_safe(ch, &reg->rfReg[ch]->input, _reg);
    set_rf_safe(ch, &reg->rfReg[ch]->input, _reg);
}


static inline bool _reg_get_rf_lock_clk(int ch, int index, FPGA_CONFIG_REG *reg)
{
    int32_t  lock = 0;
    bool is_lock = false;

    index = index;
    if(reg->rfReg[ch] == NULL || ch >= MAX_RADIO_CHANNEL_NUM)
        return false;
    
    lock = get_rf_safe(ch, &reg->rfReg[ch]->clk_lock);
    lock = get_rf_safe(ch, &reg->rfReg[ch]->clk_lock);
    
    is_lock = (lock == 0 ? false : true);
    return is_lock;
}

static inline bool _reg_get_rf_ext_clk(int ch, int index, FPGA_CONFIG_REG *reg)
{
    int32_t  inout = 0;
    bool is_ext = false;

    index = index;
    if(reg->rfReg[ch] == NULL || ch >= MAX_RADIO_CHANNEL_NUM)
        return false;
    
    //inout = reg->rfReg[ch]->in_out_clk;
    inout = get_rf_safe(ch, &reg->rfReg[ch]->in_out_clk);
    inout = get_rf_safe(ch, &reg->rfReg[ch]->in_out_clk);
    //inout = reg->rfReg[ch]->in_out_clk;
    /*  1: out clock 0: in clock */
    is_ext = (((inout & 0x01) == 0) ? false : true);
    
    return is_ext;
}

/* 获取射频不同工作模式下放大倍数 
    @ch: 工作通道
    @index: 射频模式
    @args: 通道参数指针
*/
static inline int32_t _get_rf_magnification(int ch, int index,void *args, uint64_t freq)
{
    struct poal_config *config = args;

    if(config == NULL || ch >= MAX_RADIO_CHANNEL_NUM || index >= RF_MODE_NUMBER)
        return 0;

    if(ch == 0)
       return config->channel[ch].rf_para.rf_mode.mag[index];
    else
        return 0;
}
#endif
#endif

