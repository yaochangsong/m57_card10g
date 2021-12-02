#ifndef RF_H
#define RF_H
#include <stdbool.h>
#include <stdarg.h>


struct rf_ops {
    int (*init)(void);
    /* SET */
    int (*set_mid_freq)(uint8_t, uint64_t);     /* 设置中心频率 */
    int (*set_bindwidth)(uint8_t, uint32_t);    /* 设置中频带宽 */
    int (*set_work_mode)(uint8_t, uint8_t);     /* 设置工作模式 */
    int (*set_gain_mode)(uint8_t, uint8_t);     /* 设置增益模式:MGC/AGC */
    int (*set_mgc_gain)(uint8_t, uint8_t);      /* 设置中频增益/衰减 */
    int (*set_rf_gain)(uint8_t, uint8_t);       /* 设置射频频增益/衰减 */
    int (*set_rf_cali)(uint8_t, uint8_t);       /* 设置射频校准 */
    int (*set_direct_sample_onoff)(uint8_t, int); /* 设置直采开关 */
    int (*set_rf_mid_freq)(uint8_t, uint64_t);   /*设置射频中频频率*/
    /* GET */
    int (*get_mid_freq)(uint8_t, uint64_t*);    /* 获取中心频率 */
    int (*get_bindwidth)(uint8_t, uint32_t*);   /* 获取中频带宽 */
    int (*get_work_mode)(uint8_t, uint8_t*);    /* 获取工作模式 */
    int (*get_gain_mode)(uint8_t, uint8_t*);    /* 获取增益模式:MGC/AGC */
    int (*get_mgc_gain)(uint8_t, uint8_t*);     /* 获取中频增益/衰减 */
    int (*get_rf_gain)(uint8_t, uint8_t*);      /* 获取射频频增益/衰减 */
    int (*get_temperature)(uint8_t, int16_t*);  /* 获取射频温度 */
    bool(*get_status)(uint8_t);                 /* 获取射频工作状态 */
    int (*get_rf_mid_freq)(uint8_t, uint64_t*);   /*获取射频中频频率*/
    int (*get_rf_identify_info)(uint8_t, uint8_t, void *);     /*获取射频身份信息*/
};

struct rf_ctx {
    const struct rf_ops *ops;
};

extern    int rf_init(void);
extern    uint8_t  rf_set_interface(uint8_t cmd,uint8_t ch,void *data);                 //射频设置接口
extern    int8_t  rf_read_interface(uint8_t cmd,uint8_t ch,void *data, va_list ap);                //射频查询接口
extern    struct rf_ctx *get_rfctx(void);

#endif

