#include "config.h"
#include <math.h>
#include "../../../bsp/io.h"

struct agc_ctrl_args {
    int8_t method;      /* 自动增益or手动增益 */
    int8_t rf_mode;     /* 射频模式 */
    int8_t rf_attenuation;
    int8_t mgc_attenuation;
    int32_t dst_val;    /* AGC控制目标 */
    int32_t slide_up_val;
    int32_t slide_down_val;
};

static struct agc_ctrl_args *agc_init(void);

int32_t agc_ref_val_0dbm[MAX_RADIO_CHANNEL_NUM];     /* 信号为0DBm时对应的FPGA幅度值 */
static struct agc_ctrl_args *g_agc_ctrl = NULL;


#define AGC_RUN_MAX_TIMER 1
static struct timeval *runtimer[AGC_RUN_MAX_TIMER];
static int32_t agc_init_run_timer(uint8_t maxch)
{
    int i;
    for(i = 0; i< AGC_RUN_MAX_TIMER ; i++){
        runtimer[i] = (struct timeval *)malloc(sizeof(struct timeval)*maxch);
        memset(runtimer[i] , 0, sizeof(struct timeval)*maxch);
    }
    
    return 0;
}

static int32_t agc_get_run_timer(uint8_t index, uint8_t ch)
{
    struct timeval *oldTime; 
    struct timeval newTime; 
    int32_t _t_us, ntime_us; 
    if(ch >= MAX_RADIO_CHANNEL_NUM){
        return -1;
    }
    if(index >= AGC_RUN_MAX_TIMER){
        return -1;
    }
    oldTime = runtimer[index]+ch;
     if(oldTime->tv_sec == 0 && oldTime->tv_usec == 0){
        gettimeofday(oldTime, NULL);
        return 0;
    }
    gettimeofday(&newTime, NULL);
    //printf("newTime:%zd,%zd\n",newTime.tv_sec, newTime.tv_usec);
    //printf("oldTime:%zd,%zd\n",oldTime->tv_sec, oldTime->tv_usec);
    _t_us = (newTime.tv_sec - oldTime->tv_sec)*1000000 + (newTime.tv_usec - oldTime->tv_usec); 
    //printf("_t_us:%d\n",_t_us);
    if(_t_us > 0)
        ntime_us = _t_us; 
    else 
        ntime_us = 0; 
    
    return ntime_us;
}

static int32_t agc_reset_run_timer(uint8_t index, uint8_t ch)
{
    struct timeval newTime; 
    struct timeval *oldTime; 
    if(ch >= MAX_RADIO_CHANNEL_NUM){
        return -1;
    }
    if(index >= AGC_RUN_MAX_TIMER){
        return -1;
    }
    gettimeofday(&newTime, NULL);
    oldTime = runtimer[index]+ch;
    memcpy(oldTime, &newTime, sizeof(struct timeval)); 
    return 0;
}


/* 获取信号变化上限：
    @rf_mode: 射频模式
    @dst_val： 控制目标信号强度
    @max_val： 返回信号变化上限值
    
    return: 0 OK  -1: false
*/
static inline int agc_get_range_max(int ch, uint8_t rf_mode, int dst_val, int *max_val, struct spm_context *ctx)
{
    int mode = 0, i, mag_val = 0, dec_val = 0, found = 0, rang_max = 0;
    mode = rf_mode;
    /* 获取射频模式增益最大值 */
    for(i = 0; i< ARRAY_SIZE(ctx->pdata->channel[ch].rf_para.rf_mode.mag); i++){
        if(ctx->pdata->channel[ch].rf_para.rf_mode.mode[i] == mode){
            mag_val = ctx->pdata->channel[ch].rf_para.rf_mode.mag[i];
            found = 1;
            break;
        }
    }
    if(found == 0){
        printf_err("Rf mode error:%d !!!\n", mode);
        return -1;
    }
    /* 获取信号增益最大值 */
    rang_max = mag_val + dst_val;

    /* 获取模式信号检测最大值 */
    found = 0;
    for(i = 0; i< ARRAY_SIZE(ctx->pdata->channel[ch].rf_para.rf_mode.detection); i++){
        if(ctx->pdata->channel[ch].rf_para.rf_mode.mode[i] == mode){
            dec_val = ctx->pdata->channel[ch].rf_para.rf_mode.detection[i].start;
            found = 1;
            break;
        }
    }
    if(found == 0){
        printf_err("NOT find Rf detection MAX value!!!\n");
        return -1;
    }
    /* 获取信号变化上限 */
    *max_val = (rang_max < dec_val ? rang_max : dec_val);
    printf_debug("max_val=%d, rang_max=%d, magification =%d, dst_val=%d,detection_max=%d\n", *max_val,
                rang_max, mag_val, dst_val, dec_val);
    
    return 0;
}

/* 获取信号变化下限 */
static inline int agc_get_range_min(int ch, uint8_t rf_mode, int dst_val, int *min_val, struct spm_context *ctx)
{
    int mode = 0, i, mag_val = 0, dec_val = 0, found = 0;
    int max_attenuation_value = 0, rang_min = 0;
    mode = rf_mode;
    /* 获取射频模式增益最大值 */
    for(i = 0; i< ARRAY_SIZE(ctx->pdata->channel[ch].rf_para.rf_mode.mag); i++){
        if(ctx->pdata->channel[ch].rf_para.rf_mode.mode[i] == mode){
            mag_val = ctx->pdata->channel[ch].rf_para.rf_mode.mag[i];
            found = 1;
            break;
        }
    }
    if(found == 0){
        printf_err("Rf mode error:%d !!!\n", mode);
        return -1;
    }

    /* 获取最大衰减 */
    found = 0;
    for(i = 0; i< ARRAY_SIZE(ctx->pdata->channel[ch].rf_para.rf_mode.rf_attenuation); i++){
        if(ctx->pdata->channel[ch].rf_para.rf_mode.mode[i] == mode){
            max_attenuation_value = ctx->pdata->channel[ch].rf_para.rf_mode.rf_attenuation[i].end;
            found = 1;
            break;
        }
    }
    //max_attenuation_value += ctx->pdata->cal_level.rf_mode.mgc_distortion.end_range;
    max_attenuation_value += ctx->pdata->channel[ch].rf_para.rf_mode.mgc_attenuation[i].end;
    
    /* 获取衰减最小值后信号值 */
    rang_min = mag_val -max_attenuation_value + dst_val;
    
    printf_debug("Rang min:%d, max_attenuation_value=%d, magification=%d, dst_val=%d\n", 
                rang_min, max_attenuation_value, mag_val, dst_val);

    /* 获取检测范围最小值 */
    found = 0;
    for(i = 0; i< ARRAY_SIZE(ctx->pdata->channel[ch].rf_para.rf_mode.detection); i++){
        //if(ctx->pdata->cal_level.rf_mode.detection[i].mode == mode){
        if(ctx->pdata->channel[ch].rf_para.rf_mode.mode[i] == mode){
            dec_val = ctx->pdata->channel[ch].rf_para.rf_mode.detection[i].end;
            found = 1;
            break;
        }
    }
    if(found == 0){
        printf_err("NOT find Rf detection MAX value!!!\n");
        return -1;
    }
    /* 获取信号变化下限 */
    *min_val = (rang_min > dec_val ? rang_min : dec_val);
    printf_debug("min_val=%d, rang_min=%d, detection min val=%d\n", *min_val, rang_min, dec_val);
    
    return 0;
}

static inline int agc_set_half_attenuation_value(int ch, struct spm_context *ctx)
{
    int8_t  rf_attenuation = 0, mgc_attenuation = 0;
    uint8_t rf_mode = ctx->pdata->channel[ch].rf_para.rf_mode_code; /* 射频模式 */
    int i, half_attenuation_value = 0, found = 0;
    for(i = 0; i< ARRAY_SIZE(ctx->pdata->channel[ch].rf_para.rf_mode.rf_attenuation); i++){
        if(ctx->pdata->channel[ch].rf_para.rf_mode.mode[i] == rf_mode){
            //rf_attenuation = ctx->pdata->cal_level.rf_mode.rf_distortion[i].end_range;
            rf_attenuation = ctx->pdata->channel[ch].rf_para.rf_mode.rf_attenuation[i].end;
            mgc_attenuation= ctx->pdata->channel[ch].rf_para.rf_mode.mgc_attenuation[i].end;
            found = 1;
            break;
        }
    }
    //mgc_attenuation= ctx->pdata->cal_level.rf_mode.mgc_distortion.end_range;
    if(found == 0)
        return -1;
    
    half_attenuation_value = (rf_attenuation + mgc_attenuation)/2;
    if(half_attenuation_value > rf_attenuation){
        mgc_attenuation = half_attenuation_value - rf_attenuation; 
    }else{
        mgc_attenuation = 0;
    }
    executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, ch, &rf_attenuation);
    executor_set_command(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, ch, &mgc_attenuation);
    config_write_data(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, ch, &rf_attenuation);
    config_write_data(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, ch, &mgc_attenuation);
    
    printf_debug("half_attenuation_value=%d, rf_attenuation=%d, mgc_attenuation=%d\n",
                half_attenuation_value, rf_attenuation, mgc_attenuation);

    return half_attenuation_value;
}

static inline int agc_get_max_rf_attenuation_value(int ch, struct spm_context *ctx)
{
    uint8_t rf_mode_code = ctx->pdata->channel[ch].rf_para.rf_mode_code;
    int max_attenuation_value = 0;
    int i, found = 0;
    for(i = 0; i< ARRAY_SIZE(ctx->pdata->channel[ch].rf_para.rf_mode.rf_attenuation); i++){
        if(ctx->pdata->channel[ch].rf_para.rf_mode.mode[i] == rf_mode_code){
            //max_attenuation_value = ctx->pdata->cal_level.rf_mode.rf_distortion[i].end_range;
            max_attenuation_value = ctx->pdata->channel[ch].rf_para.rf_mode.rf_attenuation[i].end;
            found = 1;
            break;
        }
    }
    if(found == 0)
        printf_err("not find max rf attenuation value\n");
    printf_debug("max_attenuation_value: %d\n", max_attenuation_value);
    return max_attenuation_value;
}

static inline int agc_get_min_rf_attenuation_value(int ch, struct spm_context *ctx)
{
    uint8_t rf_mode_code = ctx->pdata->channel[ch].rf_para.rf_mode_code;
    int min_attenuation_value = 0;
    int i, found = 0;
    for(i = 0; i< ARRAY_SIZE(ctx->pdata->channel[ch].rf_para.rf_mode.rf_attenuation); i++){
        if(ctx->pdata->channel[ch].rf_para.rf_mode.mode[i] == rf_mode_code){
            min_attenuation_value = ctx->pdata->channel[ch].rf_para.rf_mode.rf_attenuation[i].start;
            found = 1;
            break;
        }
    }
    if(found == 0)
        printf_err("not find min rf attenuation value\n");
    printf_debug("min_attenuation_value: %d\n", min_attenuation_value);
    return min_attenuation_value;

}

static inline int   agc_get_min_mgc_attenuation_value(int ch, struct spm_context *ctx)
{
    uint8_t rf_mode_code = ctx->pdata->channel[ch].rf_para.rf_mode_code;
    int min_attenuation_value = 0;
    int i, found = 0;
    for(i = 0; i< ARRAY_SIZE(ctx->pdata->channel[ch].rf_para.rf_mode.mgc_attenuation); i++){
        if(ctx->pdata->channel[ch].rf_para.rf_mode.mode[i] == rf_mode_code){
            min_attenuation_value = ctx->pdata->channel[ch].rf_para.rf_mode.mgc_attenuation[i].start;
            found = 1;
            break;
        }
    }
    if(found == 0)
        printf_err("not find min mgc attenuation value\n");
    printf_debug("mgc min_attenuation_value: %d\n", min_attenuation_value);
    return min_attenuation_value;
}


static inline int agc_get_max_mgc_attenuation_value(int ch, struct spm_context *ctx)
{
    uint8_t rf_mode_code = ctx->pdata->channel[ch].rf_para.rf_mode_code;
    int max_attenuation_value = 0;
    int i, found = 0;
    for(i = 0; i< ARRAY_SIZE(ctx->pdata->channel[ch].rf_para.rf_mode.mgc_attenuation); i++){
        if(ctx->pdata->channel[ch].rf_para.rf_mode.mode[i] == rf_mode_code){
            max_attenuation_value = ctx->pdata->channel[ch].rf_para.rf_mode.mgc_attenuation[i].end;
            found = 1;
            break;
        }
    }
    if(found == 0)
        printf_err("not find max mgc attenuation value\n");
    printf_debug("mgc max_attenuation_value: %d\n", max_attenuation_value);
    return max_attenuation_value;
}

static inline int agc_set_up_step_attenuation_value(int ch, struct spm_context *ctx, int8_t  *rf_attenuation ,int8_t  *mgc_attenuation)
{
    int8_t  _rf_attenuation = 0, _mgc_attenuation = 0;
    
    _rf_attenuation = *rf_attenuation;
    _mgc_attenuation = *mgc_attenuation;
    if(_rf_attenuation < agc_get_max_rf_attenuation_value(ch, ctx)){
        _rf_attenuation ++;
        executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, ch, &_rf_attenuation);
    }else if(_mgc_attenuation < agc_get_max_mgc_attenuation_value(ch, ctx)){
        _mgc_attenuation++;
        executor_set_command(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, ch, &_mgc_attenuation);
    }
    *rf_attenuation = _rf_attenuation;
    *mgc_attenuation = _mgc_attenuation;
    printf_info("UP: rf_attenuation=%d, mgc_attenuation=%d\n", _rf_attenuation, _mgc_attenuation);
    return 0;
}

static inline int agc_set_down_step_attenuation_value(int ch, struct spm_context *ctx, int8_t  *rf_attenuation ,int8_t  *mgc_attenuation)
{
    int8_t  _rf_attenuation = 0, _mgc_attenuation = 0;
    _rf_attenuation = *rf_attenuation;
    _mgc_attenuation = *mgc_attenuation;
    if(_rf_attenuation > agc_get_min_rf_attenuation_value(ch, ctx)){
        _rf_attenuation --;
        executor_set_command(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, ch, &_rf_attenuation);
    }else if(_mgc_attenuation > agc_get_min_mgc_attenuation_value(ch, ctx)){
        _mgc_attenuation--;
        executor_set_command(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, ch, &_mgc_attenuation);
    }
    *rf_attenuation = _rf_attenuation;
    *mgc_attenuation = _mgc_attenuation;
    printf_info("DOWN: rf_attenuation=%d, mgc_attenuation=%d\n", _rf_attenuation, _mgc_attenuation);
    return 0;
}


static inline int8_t agc_spm_read_signal_value(int ch)
{
    int8_t agc_dbm_val = 0;     /* 读取信号db值 */
    uint16_t agc_reg_val = 0;   /* 读取信号寄存器值,sigal power level read from fpga register */
    if((agc_reg_val = io_get_agc_thresh_val(ch)) < 0){
        return -1;
    }
    
    /* 信号强度转换为db值 */
    agc_dbm_val = (int32_t)(20 * log10((double)agc_reg_val / ((double)agc_ref_val_0dbm[ch])));
    printf_debug("agc_reg_val=0x%x[%d], agc_dbm_val=%d\n", agc_reg_val, agc_reg_val, agc_dbm_val);
    return agc_dbm_val;
}


int agc_ctrl(int ch, struct spm_context *ctx)
{
    #define _AGC_CTRL_PRECISION_      1       /* 控制精度+-2dbm */
    volatile uint8_t gain_ctrl_method = ctx->pdata->channel[ch].rf_para.gain_ctrl_method; /* 自动增益or手动增益 */
    volatile uint16_t agc_ctrl_time= ctx->pdata->channel[ch].rf_para.agc_ctrl_time;       /* 自动增益步进控制时间 */
    volatile int8_t agc_ctrl_dbm = ctx->pdata->channel[ch].rf_para.agc_mid_freq_out_level;/* 自动增益目标控制功率db值 */
    volatile uint8_t rf_mode = ctx->pdata->channel[ch].rf_para.rf_mode_code; /* 射频模式 */
    int8_t agc_dbm_val = 0;     /* 读取信号db值 */
    struct agc_ctrl_args *_agc_ctrl = g_agc_ctrl;

    if(_agc_ctrl == NULL)
        _agc_ctrl = agc_init();

    /* 当增益模式变化时 */
    if(_agc_ctrl[ch].method != gain_ctrl_method || _agc_ctrl[ch].rf_mode != rf_mode || _agc_ctrl[ch].dst_val != agc_ctrl_dbm){
        _agc_ctrl[ch].method = gain_ctrl_method;
        _agc_ctrl[ch].dst_val = agc_ctrl_dbm;
        _agc_ctrl[ch].rf_mode = rf_mode;
        printf_debug("ch:%d ctrl_method:%d rf_mode:%d, agc_ctrl_dbm=%d\n",ch,gain_ctrl_method,rf_mode, agc_ctrl_dbm);
        if(gain_ctrl_method == POAL_MGC_MODE){
            /* 手动模式直接退出 */
            return -1;
        }
        else if(gain_ctrl_method == POAL_AGC_MODE){
            /* 设置半增益 */
            agc_set_half_attenuation_value(ch, ctx);
            /* 读取衰减目标值 */
            usleep(100000);
            _agc_ctrl[ch].dst_val = agc_ctrl_dbm;//_spm_read_signal_value(ch);
            _agc_ctrl[ch].slide_down_val = 100; //max_val[ch] - dst_val[ch];
            _agc_ctrl[ch].slide_up_val   = 100; //dst_val[ch] - min_val[ch];
            if(_agc_ctrl[ch].slide_up_val < 0)
                _agc_ctrl[ch].slide_up_val = -_agc_ctrl[ch].slide_up_val;
            printf_debug("up_val=%d, down_val=%d, dst_val=%d\n", 
                        _agc_ctrl[ch].slide_up_val, _agc_ctrl[ch].slide_down_val, _agc_ctrl[ch].dst_val);
            config_read_by_cmd(EX_RF_FREQ_CMD, EX_RF_ATTENUATION, ch, &_agc_ctrl[ch].rf_attenuation);
            config_read_by_cmd(EX_RF_FREQ_CMD, EX_RF_MGC_GAIN, ch, &_agc_ctrl[ch].mgc_attenuation);
            printf_note("rf_attenuation[%d]=%d,mgc_attenuation[%d]=%d\n", 
                ch, _agc_ctrl[ch].rf_attenuation, ch, _agc_ctrl[ch].mgc_attenuation);
        }
    }

    /* 非自动模式退出控制 */
    if(gain_ctrl_method != POAL_AGC_MODE){
        return -1;
    }

    if(agc_get_run_timer(0, ch) < agc_ctrl_time){
        /* 控制步进时间未到 */
        return -1;
    }
    agc_reset_run_timer(0, ch);
    usleep(1000);
    /* 读取信号强度 */
    agc_dbm_val =  agc_spm_read_signal_value(ch);
    usleep(1000);

    printf_debug("ch=%d[%d]dst_val=%d, read signal value: %d, up_val=%d, down_val=%d, rf_attenuation=%d, mgc_attenuation=%d\n", ch,
        agc_ctrl_dbm,_agc_ctrl[ch].dst_val, agc_dbm_val, _agc_ctrl[ch].slide_up_val, _agc_ctrl[ch].slide_down_val, _agc_ctrl[ch].rf_attenuation, _agc_ctrl[ch].mgc_attenuation);

    if(agc_dbm_val < (_agc_ctrl[ch].dst_val - _AGC_CTRL_PRECISION_)){
        if(_agc_ctrl[ch].slide_up_val <= 0){
            printf_note(">>>Arrived TOP<<<\n");
            return -1;
        }
        _agc_ctrl[ch].slide_up_val--;
        _agc_ctrl[ch].slide_down_val++;
        agc_set_down_step_attenuation_value(ch, ctx, &_agc_ctrl[ch].rf_attenuation, &_agc_ctrl[ch].mgc_attenuation);
    }else if(agc_dbm_val > (_agc_ctrl[ch].dst_val + _AGC_CTRL_PRECISION_)){
        if(_agc_ctrl[ch].slide_down_val <= 0){
            printf_note(">>>Arrived DOWN<<<\n");
            return -1;
        }
        _agc_ctrl[ch].slide_down_val--;
        _agc_ctrl[ch].slide_up_val++;
        agc_set_up_step_attenuation_value(ch, ctx, &_agc_ctrl[ch].rf_attenuation, &_agc_ctrl[ch].mgc_attenuation);
    }
    
    return 0;
}



static struct agc_ctrl_args *agc_init(void)
{
    #define DEFAULT_AGC_REF_VAL  0x3000
    static struct agc_ctrl_args _agc_ctrl[MAX_RADIO_CHANNEL_NUM];
    int i, ch;
    
    for(i = 0; i < MAX_RADIO_CHANNEL_NUM; i++){
        _agc_ctrl[i].method = -1;
        _agc_ctrl[i].rf_mode = -1;
        _agc_ctrl[i].dst_val = -1;
        _agc_ctrl[i].rf_attenuation = -1;
        _agc_ctrl[i].mgc_attenuation = -1;
        _agc_ctrl[i].slide_up_val = -1;
        _agc_ctrl[i].slide_down_val = -1;
    }
    g_agc_ctrl = _agc_ctrl;
    agc_init_run_timer(MAX_RADIO_CHANNEL_NUM);
    
    for(ch = 0; ch < MAX_RADIO_CHANNEL_NUM; ch++){
        if( config_get_config()->oal_config.channel[ch].rf_para.agc_ref_val_0dbm != 0)
            agc_ref_val_0dbm[ch] = config_get_config()->oal_config.channel[ch].rf_para.agc_ref_val_0dbm;
        else
            agc_ref_val_0dbm[ch]  = DEFAULT_AGC_REF_VAL;
    }
    printf_note("AGC init...g_agc_ctrl=%p\n", g_agc_ctrl);
    return g_agc_ctrl;
}

