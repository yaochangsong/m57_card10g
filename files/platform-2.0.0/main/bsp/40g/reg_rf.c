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
#include "reg_rf.h"

/* RF */
/*GET*/
/* 
@ch: rf control channel
@index: a channel may have multiple RF controls
*/
static  int32_t _get_temperature(int ch, int16_t *temperature)
{
#define RF_TEMPERATURE_FACTOR 0.0625
    int16_t  rf_temp_reg = 0;
    uint32_t tye_count = 0;
    bool read_ok = false, time_out = false;

    FPGA_CONFIG_REG *reg = get_fpga_reg();
    if(ch >= MAX_RF_NUM || reg->rfReg[ch] == NULL)
        return -1;

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
    
    *temperature = rf_temp_reg * RF_TEMPERATURE_FACTOR;
    return 0;
}


static bool _get_lock_clk(int ch, int index)
{
    int32_t  lock = 0;
    bool is_lock = false;

    FPGA_CONFIG_REG *reg = get_fpga_reg();
    index = index;
    if(reg->rfReg[ch] == NULL || ch >= MAX_RADIO_CHANNEL_NUM)
        return false;
    
    lock = get_rf_safe(ch, &reg->rfReg[ch]->clk_lock);
    lock = get_rf_safe(ch, &reg->rfReg[ch]->clk_lock);
    
    is_lock = (lock == 0 ? false : true);
    return is_lock;
}

static bool get_ext_clk(int ch, int index)
{
    int32_t  inout = 0;
    bool is_ext = false;

    FPGA_CONFIG_REG *reg = get_fpga_reg();
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

static uint64_t _set_freq_details_v2(int ch,  uint64_t freq_hz, uint64_t *rough_freq, uint64_t *fine_freq)
 {
    struct _param_freq_t{
     uint32_t start_rf_freq_mhz;
     uint32_t end_rf_freq_mhz;
     uint32_t mid_freq_mhz;
    };
    struct _param_freq_t _freqtable[] = {
         {18000,    19000, 7860},
         {19000,    19020, 7860},
         {19020,    26500, 5800},
         {26500,    26520, 5800},
         {26520,    30500, 7860},
         {30500,    30540, 7860},
         {30540,    32000, 5800},
         {32000,    32020, 5800},
         {32020,    32060, 5800},
         {32060,    34000, 5800},
         {34000,    34040, 5800},
         {34040,    38600, 7860},
         {38600,    38620, 7860},
         {38620,    38660, 5800},
         {38660,    40000, 5800},
    };
    uint64_t real_freq_hz = freq_hz;
    int found = 0;
    uint32_t if_mhz,  _start_freq_mhz = 0;
    uint64_t _rough_freq_val = 0, _fine_freq_val = 0;
    int s = -1;
    if(ch == 1){
     if(freq_hz >= GHZ(18)){
         for(int i = 0; i < ARRAY_SIZE(_freqtable); i++){
             if((real_freq_hz > MHZ(_freqtable[i].start_rf_freq_mhz)) && (real_freq_hz <= MHZ(_freqtable[i].end_rf_freq_mhz))){
                  if(MHZ(34040) == real_freq_hz){
                    if_mhz = 7860;
                    _start_freq_mhz = 34040;
                  } else if(MHZ(30540) == real_freq_hz){
                    if_mhz = 5800;
                     _start_freq_mhz = 30540;
                  } else{
                    if_mhz = _freqtable[i].mid_freq_mhz;
                     _start_freq_mhz = _freqtable[i].start_rf_freq_mhz;
                  }
                 printf_note("RF=%"PRIu64"Hz,start = %uMHz, end=%uMHz, if=%uMHz\n",real_freq_hz, _freqtable[i].start_rf_freq_mhz, _freqtable[i].end_rf_freq_mhz, if_mhz);
                 _rough_freq_val =  MHZ(_start_freq_mhz)+ ((real_freq_hz - MHZ(_start_freq_mhz))/MHZ(40)) *MHZ(40);  //_start_freq+((RF-_start_freq)/40)×40
                 if(freq_hz > MHZ(32020))
                    s = 1;
                 _fine_freq_val = MHZ(if_mhz) + MHZ(20) + s* ((real_freq_hz - MHZ(_start_freq_mhz)) % MHZ(40)); //IF+20-(RF-18000)%40
                 found ++;
                 break;
             }
         }
         if(found == 0){
             _rough_freq_val = GHZ(18); /* default 18GHz */
             _fine_freq_val = MHZ(7880);
             printf_note("NOT found freq %"PRIu64"Hz in tables, use default 18000mhz \n", _rough_freq_val);
         }
         printf_note("RF2(1840G30M)rough_freq_val =%"PRIu64"uHz, RF1(18G20M) fine_freq_val%"PRIu64"Hz\n", _rough_freq_val, _fine_freq_val);
         #if 0
         //_rough_freq_val = MHZ(40);
         if(freq_hz <= GHZ(32)){ //倒谱
            _fine_freq_val = MHZ(if_mhz) - freq_hz % MHZ(40);
         } else{
            _fine_freq_val = MHZ(if_mhz) + freq_hz % MHZ(40);
         }
         printf_note("daopu:[%lluhz] RF1(18G20M) fine_freq_val=%lluHz, if_mhz=%uMhz\n",  freq_hz, _fine_freq_val, if_mhz);
         #endif
     }
    }
    *rough_freq = _rough_freq_val;
    *fine_freq = _fine_freq_val;
    return real_freq_hz;
 }


static  void _reg_set_rf_frequency(int ch, int index, uint64_t freq_hz)
{
    FPGA_CONFIG_REG *reg = get_fpga_reg();
    uint32_t freq_10khz = freq_hz/10000, freq_set_val_mhz;
    uint32_t freq_mhz = freq_hz/1000000;
    int i, found = 0;
    int is_cepstrum[MAX_RADIO_CHANNEL_NUM] = {0, 0};
    static  int cepstrum_dup[MAX_RADIO_CHANNEL_NUM] = {-1, -1};
    static uint32_t freq_10khz_dup = 0; //only ch1

    if(ch >= MAX_RADIO_CHANNEL_NUM || freq_hz > GHZ(40))
        return;
    
    if(ch == 1){
        if(reg->rfReg[1] == NULL)
            return;
        if(freq_hz >= GHZ(18)){
            uint64_t rough_freq, fine_freq;
            _set_freq_details_v2(ch, freq_hz, &rough_freq, &fine_freq);
            //_set_freq_details(ch, freq_hz, &rough_freq, &fine_freq);
            //_set_freq_details_v3(ch, freq_hz, &rough_freq, &fine_freq);
            freq_10khz = rough_freq/10000;
            set_rf_safe(ch, &reg->rfReg[2]->freq_khz, freq_10khz);
            set_rf_safe(ch, &reg->rfReg[2]->freq_khz, freq_10khz);
            printf_info("rf2 ch=%d, freq=%u 10khz\n", ch, freq_10khz);
            //freq_10khz = freq_set_val_mhz * 100;
            freq_10khz = fine_freq/10000;
        } else{
            uint32_t _freq13g_10khz = GHZ(13)/10000;
            set_rf_safe(ch, &reg->rfReg[2]->freq_khz, _freq13g_10khz);
            set_rf_safe(ch, &reg->rfReg[2]->freq_khz, _freq13g_10khz);
        }
        if(freq_10khz != freq_10khz_dup){
            set_rf_safe(ch, &reg->rfReg[1]->freq_khz, freq_10khz);
            set_rf_safe(ch, &reg->rfReg[1]->freq_khz, freq_10khz);
            printf_info("rf1 ch=%d, freq=%u 10khz\n", ch, freq_10khz);
        }
        freq_10khz_dup = freq_10khz;
    }
    else{
        if(reg->rfReg[0] == NULL)
            return;
        //set_rf_safe(ch, &reg->rfReg[0]->freq_khz, freq_10khz);
        set_rf_safe(ch, &reg->rfReg[0]->freq_khz, freq_10khz);
        set_rf_safe(ch, &reg->rfReg[0]->freq_khz, freq_10khz);
        printf_info("rf0 ch=%d, freq=%u 10khz\n", ch, freq_10khz);
    }
    
   // if((freq_hz >= GHZ(13)) && (freq_hz <= GHZ(32.020)){
   if((freq_hz >= GHZ(13)) && (freq_hz <= MHZ(32020))){
        //倒谱
        is_cepstrum[ch] = 1;
        if(is_cepstrum[ch] != cepstrum_dup[ch]){
            printf_info("reg_set_cepstrum 1\n");
            reg_set_cepstrum(ch, 1);
        }
    }else{
        is_cepstrum[ch] = 0;
        if(is_cepstrum[ch] != cepstrum_dup[ch]){
            printf_info("reg_set_cepstrum 0\n");
            reg_set_cepstrum(ch, 0);
        }
    }
    cepstrum_dup[ch] = is_cepstrum[ch];
    
    usleep(100);
}


static void _reg_set_rf_bandwidth(int ch, int index, uint32_t bw_hz)
{
    FPGA_CONFIG_REG *reg = get_fpga_reg();
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
        printf_warn("NOT found bandwidth %uHz in tables,use default[200Mhz]\n", bw_hz);
        set_val = 0x03; /* default 200MHz */
    }
    
    if(reg->rfReg[ch] == NULL)
        return;
    
    //set_rf_safe(ch, &reg->rfReg[ch]->mid_band, set_val);
    set_rf_safe(ch, &reg->rfReg[ch]->mid_band, set_val);
}


static void _reg_set_rf_mode_code(int ch, int index, uint8_t code)
{
    FPGA_CONFIG_REG *reg = get_fpga_reg();
    int i, found = 0;
    uint8_t set_val = 0;
    uint64_t mid_freq;
    static int set_mode_dup[MAX_RADIO_CHANNEL_NUM] = {-1, -1};
    static int set_val_dup[MAX_RADIO_CHANNEL_NUM]= {-1, -1};
    int set_mode[MAX_RADIO_CHANNEL_NUM] = {0, 0};
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

    if(ch == 1){
        mid_freq = executor_get_mid_freq(ch);
        if(mid_freq >= GHZ(18)){
            set_mode[ch] = 0;
            if((set_mode_dup[ch] != set_mode[ch]) || (set_val_dup[ch] != set_val)){
                set_rf_safe(ch, &reg->rfReg[ch]->rf_mode, 0x00);     //rf1 normal
                set_rf_safe(ch, &reg->rfReg[ch+1]->rf_mode, set_val);
                set_mode_dup[ch] = set_mode[ch];
                set_val_dup[ch] = set_val;
                printf_debug("rf mode set: ch:%d, rf_mode[1]=00, rf_mode[2]=%d\n", ch, set_val);
            }
        } else{
            set_mode[ch] = 1;
            if((set_mode_dup[ch] != set_mode[ch]) || (set_val_dup[ch] != set_val)){
                set_rf_safe(ch, &reg->rfReg[ch]->rf_mode, set_val);
                set_mode_dup[ch] = set_mode[ch];
                set_val_dup[ch] = set_val;
                printf_debug("rf mode set: ch:%d, rf_mode[1]=%d\n", ch, set_val);
            }
        }
    }else{
        if(set_val_dup[ch] != set_val){
            set_rf_safe(ch, &reg->rfReg[ch]->rf_mode, set_val);
            set_val_dup[ch] = set_val;
            printf_debug("rf mode set: ch:%d, rf_mode[0]=%d\n", ch, set_val);
        }
    }
}


static void _reg_set_rf_mgc_gain(int ch, int index, uint8_t gain)
{
    FPGA_CONFIG_REG *reg = get_fpga_reg();
    uint64_t mid_freq;
    if(ch >= MAX_RADIO_CHANNEL_NUM)
        return;
    
    if(gain > 30)
        gain = 30;
    else if(gain < 0)
        gain = 0;
    
    if(reg->rfReg[ch] == NULL)
        return;

    if(ch == 1){
        mid_freq = executor_get_mid_freq(ch);
        if (mid_freq >= GHZ(18)) {
           set_rf_safe(ch, &reg->rfReg[ch]->midband_minus, 0);
           set_rf_safe(ch, &reg->rfReg[ch+1]->midband_minus, gain);
        } else {
            set_rf_safe(ch, &reg->rfReg[ch]->midband_minus, gain);
        }
    } else {
        set_rf_safe(ch, &reg->rfReg[ch]->midband_minus, gain);
    }
}



static void _reg_set_rf_attenuation(int ch, int index, uint8_t atten)
{
    FPGA_CONFIG_REG *reg = get_fpga_reg();
    uint64_t mid_freq;
    if(ch >= MAX_RADIO_CHANNEL_NUM)
        return;
    if(atten > 30)
        atten = 30;
    else if(atten < 0)
        atten = 0;
    
    if(reg->rfReg[ch] == NULL)
        return;

    if(ch == 1){
        mid_freq = executor_get_mid_freq(ch);
        if (mid_freq >= GHZ(18)) {
            set_rf_safe(ch, &reg->rfReg[ch]->rf_minus, atten);
            set_rf_safe(ch, &reg->rfReg[ch+1]->rf_minus, 0);
        } else {
            set_rf_safe(ch, &reg->rfReg[ch]->rf_minus, atten);
        }
    } else {
        set_rf_safe(ch, &reg->rfReg[ch]->rf_minus, atten);
    }
}


static void _reg_set_rf_cali_source_attenuation(int ch, int index, uint8_t level)
{
    FPGA_CONFIG_REG *reg = get_fpga_reg();
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
    
    //set_rf_safe(ch, &reg->rfReg[ch]->revise_minus, reg_val);
    set_rf_safe(ch, &reg->rfReg[ch]->revise_minus, reg_val);
}


static void _reg_set_rf_direct_sample_ctrl(int ch, int index, uint8_t val)
{
    uint8_t data;

    FPGA_CONFIG_REG *reg = get_fpga_reg();
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

static void _reg_set_rf_cali_source_choise(int ch, int index, uint8_t val)
{
    FPGA_CONFIG_REG *reg = get_fpga_reg();
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

/* 获取射频不同工作模式下放大倍数 ;
   通道1: 射频1常规模式放大倍数+射频2指定模式放大倍数
    @ch: 工作通道
    @index: 射频模式
    @args: 通道参数指针
*/
static  int32_t _get_rf_magnification(int ch, int mode,void *args, uint64_t mid_freq)
{
    struct poal_config *config = args;
    
    if(config == NULL || ch >= MAX_RADIO_CHANNEL_NUM || mode >= RF_MODE_NUMBER)
        return 0;

    if(ch == 0)
       return config->channel[ch].rf_para.rf_mode.mag[mode];
    else if(ch == 1){
        if(mid_freq < GHZ(18)){
            return config->channel[ch].rf_para.rf_mode.mag[mode];
        }else{
            return (config->channel[ch].rf_para.rf_mode.mag[POAL_NORMAL] + config->channel[ch+1].rf_para.rf_mode.mag[mode]);
        }
        
    }
    else
        return 0;
}


static const struct rf_reg_ops rf_reg = {
    .get_temperature = _get_temperature,
    .get_lock_clk   = _get_lock_clk,
    .get_ext_clk = get_ext_clk,
    .set_frequency = _reg_set_rf_frequency,
    .set_bandwidth = _reg_set_rf_bandwidth,
    .set_mode_code = _reg_set_rf_mode_code,
    .set_mgc_gain = _reg_set_rf_mgc_gain,
    .set_attenuation = _reg_set_rf_attenuation,
    .set_direct_sample_ctrl = _reg_set_rf_direct_sample_ctrl,
    .set_cali_source_attenuation = _reg_set_rf_cali_source_attenuation,
    .set_cali_source_choise =_reg_set_rf_cali_source_choise,
    .get_magnification = _get_rf_magnification,
};


struct rf_reg_ops * rf_create_reg_ctx(void)
{
    struct rf_reg_ops *ctx = calloc(1, sizeof(*ctx));
     if (!ctx)
        return NULL;
     
    ctx = &rf_reg;
    return ctx;
}

