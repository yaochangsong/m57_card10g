#ifndef __IO_H__
#define __IO_H__

typedef enum _io_dq_method_code{
    IO_DQ_MODE_AM = 0x00,
    IO_DQ_MODE_FM = 0x01,
    IO_DQ_MODE_WFM = 0x01,
    IO_DQ_MODE_LSB = 0x02,
    IO_DQ_MODE_USB = 0x02,
    IO_DQ_MODE_CW = 0x03,
    IO_DQ_MODE_IQ = 0x07,
}io_dq_method_code;

struct  band_table_t{
    uint32_t extract_factor;
    uint32_t filter_factor;
    uint32_t band;
    float factor;
}__attribute__ ((packed)); 
    
#ifdef SUPPORT_PLATFORM_ARCH_ARM
#ifdef SUPPORT_NET_WZ
#define NETWORK_10G_EHTHERNET_POINT       "eth0"
#define NETWORK_EHTHERNET_POINT           "eth1"
#else
#define NETWORK_EHTHERNET_POINT       "eth0"
#endif
#else
#define NETWORK_10G_EHTHERNET_POINT   "eno2"
#define NETWORK_EHTHERNET_POINT       "eno1"
#endif
#ifdef SUPPORT_AMBIENT_TEMP_HUMIDITY
    #ifdef SUPPORT_TEMP_HUMIDITY_SI7021
    #define io_get_ambient_temperature() si7021_read_temperature()
    #define io_get_ambient_humidity()    si7021_read_humidity()
    #else
    #define io_get_ambient_temperature() 0
    #define io_get_ambient_humidity()    0
    #endif
#else
    #define io_get_ambient_temperature() 0
    #define io_get_ambient_humidity()    0
#endif
extern void io_init(void);
extern void subch_bitmap_init(void);
extern void subch_bitmap_set(uint8_t subch);
extern void subch_bitmap_clear(uint8_t subch);
extern size_t subch_bitmap_weight(void);
extern float io_get_narrowband_iq_factor(uint32_t bindwidth);
extern bool test_audio_on(void);
extern int8_t io_set_enable_command(uint8_t type, int ch, int subch, uint32_t fftsize);
extern int8_t io_set_work_mode_command(void *data);
extern int8_t io_set_para_command(uint8_t type, int ch, void *data);
extern int16_t io_get_adc_temperature(void);
extern int32_t io_get_agc_thresh_val(int ch);
extern void io_set_dq_param(void *pdata);
extern void io_set_smooth_time(uint32_t ch, uint16_t factor);
extern void io_set_fft_size(uint32_t ch, uint32_t fft_size);
extern uint8_t  io_set_network_to_interfaces(void *netinfo);
extern int32_t io_set_extract_ch0(uint32_t ch, uint32_t bandwith);
extern int io_read_more_info_by_name(const char *name, void *info, int32_t (*iofunc)(void *));
extern int32_t io_find_file_info(void *arg);
extern int32_t io_start_backtrace_file(void *arg);
extern int32_t io_stop_backtrace_file(void *arg);
extern bool io_get_adc_status(void *args);
extern bool io_get_clock_status(void *args);
extern int32_t io_set_noise(uint32_t ch, uint32_t noise_en,int8_t noise_level_tmp);
extern int32_t io_set_middle_freq(uint32_t ch, uint64_t middle_freq);
extern void io_get_fpga_status(void *args);
extern void io_get_board_power(void *args);
extern bool io_get_inout_clock_status(void *args);
extern void io_set_rf_calibration_source_level(int level);
extern void io_set_rf_calibration_source_enable(int ch, int enable);
extern bool is_rf_calibration_source_enable(void);
extern void io_set_gain_calibrate_val(uint32_t ch, int32_t  gain_val);
extern void io_set_dc_offset_calibrate_val(uint32_t ch, int32_t  val);
extern int32_t io_set_subch_dec_middle_freq(uint32_t subch, uint64_t dec_middle_freq, uint64_t middle_freq);
extern int32_t io_set_subch_onoff(uint32_t ch, uint32_t subch, uint8_t onoff);
extern int32_t io_set_subch_bandwidth(uint32_t subch, uint32_t bandwidth, uint8_t dec_method);
extern int32_t io_set_subch_dec_method(uint32_t subch, uint8_t dec_method);
#endif