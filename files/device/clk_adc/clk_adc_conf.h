
#ifndef _CLOCK_ADC_CONF_
#define _CLOCK_ADC_CONF_
extern    uint8_t clock_7044_config[][3];
extern    uint8_t clock_7044_config_internal[][3];
extern    uint8_t ad_9690_config[][3];

extern    uint32_t get_internal_clock_cfg_array_size(void);
extern    uint32_t get_external_clock_cfg_array_size(void);
extern    uint32_t get_adc_cfg_array_size(void);

#endif

