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
#include "reg_misc.h"


/*
    @back:1 playback  0: normal
*/
static  void _set_ssd_mode(int ch,int back)
{
    FPGA_CONFIG_REG *reg = get_fpga_reg();
    uint32_t _reg = 0;
#if 0
    if(ch >= 0)
        _reg = ((back &0x01) << ch);
    else
        _reg = back &0x01;
#else
    if(back)
        _reg = 0x1;
    else
        _reg = 0x0;
#endif
    reg->system->ssd_mode = _reg;
}

static void _set_iq_time_mark(int ch, bool is_mark)
{
    FPGA_CONFIG_REG *reg = get_fpga_reg();
    /* bit16为时标使能位 */
    ch = ch;
    if(is_mark)
        reg->signal->trig = 0x10000;
    else
        reg->signal->trig = 0x00000;
}

static uint64_t _get_middle_freq_by_channel(int ch, void *args)
{
    /*
    ch   中心频率（MHz）
    0	165
    1	280
    2	395
    3	510
*/
    struct  freq_rang_t{
        int ch;
        uint64_t m_freq;
    }freq_rang [] = {
            {0, MHZ(165)},
            {1, MHZ(280) - MHZ(0.1)},
            {2, MHZ(395) - MHZ(0.2)},
            {3, MHZ(510)},
        };
    int _ch = ch;
    
    args = args;
    for(int i = 0; i< ARRAY_SIZE(freq_rang); i++){
        if(ch == freq_rang[i].ch){
            return freq_rang[i].m_freq;
        }
            
    }
   // printf_warn("ch[%d]can't find frequeny, use default:%"PRIu64"Hz\n",  freq_rang[0].m_freq);
    return freq_rang[0].m_freq;
}

static uint64_t _get_rel_middle_freq_by_channel(int ch, void *args)
{
    /*
    ch   中心频率（MHz）
    0	165
    1	280
    2	395
    3	510
*/
    struct  freq_rang_t{
        int ch;
        uint64_t m_freq;
    }freq_rang [] = {
            {0, MHZ(165)},
            {1, MHZ(280)},
            {2, MHZ(395)},
            {3, MHZ(510)},
        };
    int _ch = ch;
    
    args = args;
    for(int i = 0; i< ARRAY_SIZE(freq_rang); i++){
        if(ch == freq_rang[i].ch){
            return freq_rang[i].m_freq;
        }
            
    }
    return freq_rang[0].m_freq;
}



static uint64_t _get_middle_freq(int ch, uint64_t rang_freq, void *args)
{
    /*
    ch	 频段范围（MHz）
    0	108～228
    1	220～340
    2	335～455
    3	450～570
    */
    struct	freq_rang_t{
        int ch;
        uint64_t s_freq;
        uint64_t e_freq;
    }freq_rang [] = {
        {0, MHZ(105), MHZ(222.5)},//105~225
        {1, MHZ(222.5), MHZ(337.5)},//220~340
        {2, MHZ(337.5), MHZ(452.5)},//335~455
        {3, MHZ(452.5), MHZ(570)},//450~570
    };
    args = args;
    for(int i = 0; i< ARRAY_SIZE(freq_rang); i++){
        if(rang_freq >= freq_rang[i].s_freq && rang_freq < freq_rang[i].e_freq){
            return _get_middle_freq_by_channel(freq_rang[i].ch,NULL);
        }
    }
    return 0;
}

static int _get_channel_by_mfreq(uint64_t rang_freq, void *args)
{
    /*
    ch	 频段范围（MHz）
    0	108～228
    1	220～340
    2	335～455
    3	450～570
    */
    struct	freq_rang_t{
        int ch;
        uint64_t s_freq;
        uint64_t e_freq;
    }freq_rang [] = {
        {0, MHZ(105), MHZ(222.5)},//105~225
        {1, MHZ(222.5), MHZ(337.5)},//220~340
        {2, MHZ(337.5), MHZ(452.5)},//335~455
        {3, MHZ(452.5), MHZ(570)},//450~570
    };
    args = args;
    for(int i = 0; i< ARRAY_SIZE(freq_rang); i++){
        if(rang_freq >= freq_rang[i].s_freq && rang_freq < freq_rang[i].e_freq){
            return freq_rang[i].ch;
        }
    }
    return 0;
}


static const struct misc_reg_ops misc_reg = {
    .set_ssd_mode = _set_ssd_mode,
    .set_iq_time_mark = _set_iq_time_mark,
    .get_channel_by_mfreq = _get_channel_by_mfreq,
    .get_channel_middle_freq = _get_middle_freq,
    .get_middle_freq_by_channel = _get_middle_freq_by_channel,
    .get_rel_middle_freq_by_channel = _get_rel_middle_freq_by_channel,
};


struct misc_reg_ops * misc_create_reg_ctx(void)
{
    struct misc_reg_ops *ctx = calloc(1, sizeof(*ctx));
     if (!ctx)
        return NULL;
     
    ctx = &misc_reg;
    return ctx;
}
