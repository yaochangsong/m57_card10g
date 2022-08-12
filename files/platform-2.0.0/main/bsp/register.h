#ifndef _REGISTER_H_
#define _REGISTER_H_

struct rf_reg_ops {
    /* RF */
    int32_t (*get_temperature)(int ch, int16_t *);                              /* 获取射频温度 */
    bool (*get_lock_clk)(int ch, int index);                                    /* 获取射频时钟锁定状态 */
    bool (*get_ext_clk)(int ch, int index);                                     /* 获取射频是否外时钟 */
    int32_t (*get_magnification)(int ch, int index,void *args, uint64_t freq);  /* 获取射频放大倍数 */
    void (*set_attenuation)(int ch, int index, uint8_t atten);                  /* 设置射频衰减 */
    void (*set_bandwidth)(int ch, int index, uint32_t bw_hz);                   /* 设置射频带宽 */
    void (*set_mode_code)(int ch, int index, uint8_t code);                     /* 设置射频工作模式 */
    void (*set_mgc_gain)(int ch, int index, uint8_t gain);                      /* 设置射频MGC增益 */
    void (*set_frequency)(int ch, int index, uint64_t freq_hz);                 /* 设置射频中心频率 */
    void (*set_cali_source_attenuation)(int ch, int index, uint8_t level);      /* 设置射频校准源衰减 */
    void (*set_direct_sample_ctrl)(int ch, int index, uint8_t val);             /* 设置射频直采控制 */
    void (*set_cali_source_choise)(int ch, int index, uint8_t val);             /* 设置射频校准源选择 */
};

struct if_reg_ops {
    /* IF */
    bool (*get_adc_status)(void);                                                /* 获取中频AD工作状态 */
    void (*set_narrow_channel)(int ch, int subch, int enable);                   /* 设置中频窄带通道 */
    void (*set_narrow_audio_sample_rate)(int ch, int subch, int rate);           /* 设置中频窄带音频采样率 */
    void (*set_narrow_audio_gain)(int ch, int subch, float gain);                /* 设置中频窄带音频增益 */
    void (*set_narrow_audio_gain_mode)(int ch, int subch, int mode);             /* 设置中频窄带音频增益模式 */
    void (*set_narrow_agc_time)(int ch, int subch, int mode);                    /* 设置中频窄带AGC时间 */
    void (*set_niq_channel)(int ch, int subch, void *args, int enable);          /* 设置中频窄带IQ通道 */
    void (*set_fft_channel)(int ch, int enable, void *args);                     /* 设置中频FFT通道 */
    void (*set_biq_channel)(int ch, void *args);                                 /* 设置中频宽带IQ通道 */
    void (*set_channel)(int ch, int enable);
};

struct misc_reg_ops {
    /* misc */
    void (*set_ssd_mode)(int ch,int back);                                       /* 设置磁盘回溯/正常模式 */
	void (*set_audio_volume)(int ch, int vol);									 /* 设置音频音量 */
    void (*set_iq_time_mark)(int ch, bool is_mark);                              /* 设置IQ存储时标 */
    int  (*get_channel_by_mfreq)(uint64_t rang_freq, void *args);                /* 通过解调中心频率获取通道号 */
    uint64_t (*get_channel_middle_freq)(int ch, uint64_t rang_freq, void *args); /* 通过解调中心频率获取主通道中心频率 */
    uint64_t (*get_middle_freq_by_channel)(int ch, void *args);                  /* 通过通道号获取工作中心频率 */
    uint64_t (*get_rel_middle_freq_by_channel)(int ch, void *args);              /* 通过通道号获取实际中心频率 */
};


struct register_ops {
    const struct rf_reg_ops *rf;
    const struct if_reg_ops *iif;
    const struct misc_reg_ops *misc;
};

extern int reg_init(void);
extern struct register_ops *reg_get(void);
extern struct register_ops * reg_create(void);

#endif
