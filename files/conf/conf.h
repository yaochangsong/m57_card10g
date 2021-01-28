#ifndef _CONF_H_
#define _CONF_H_
#include "config.h"

/** Defaults configuration values */
#if defined (SUPPORT_DAO_XML)
    #ifdef SUPPORT_PLATFORM_ARCH_ARM
    #define DEFAULT_CONFIGFILE "/spectrum.xml"
    #define CALIBRATION_FILE   "/calibration.xml"
    #else
    #define DEFAULT_CONFIGFILE "conf/spectrum.xml"
    #define CALIBRATION_FILE   "conf/calibration.xml"
    #endif
#elif defined(SUPPORT_DAO_JSON)
    #ifdef SUPPORT_PLATFORM_ARCH_ARM
    #define DEFAULT_CONFIGFILE "/etc/config.json"
    #else
    #define DEFAULT_CONFIGFILE "conf/config.json"
    #endif
#endif

/**
 * Mutex for the configuration file, used by the auth_servers related
 * functions. */
extern pthread_mutex_t config_mutex;


/**
 * Configuration structure
 */
typedef struct {
    char *configfile;       /**< @brief name of the config file */
    char *calibrationfile;
    int daemon;             /**< @brief if daemon > 0, use daemon mode */
    struct poal_config oal_config;
} s_config;



/** @brief Initialise the conf system */
void config_init(void);


#define LOCK_CONFIG() do { \
	pthread_mutex_lock(&config_mutex); \
} while (0)

#define UNLOCK_CONFIG() do { \
	pthread_mutex_unlock(&config_mutex); \
} while (0)

extern s_config *config_get_config(void);
extern int8_t config_save_all(void);
extern int8_t config_save_batch(exec_cmd cmd, uint8_t type,s_config *config);
extern int8_t config_read_by_cmd(exec_cmd cmd, uint8_t type, uint8_t ch, void *data, ...);
extern int8_t config_write_data(int cmd, uint8_t type, uint8_t ch, void *data);
extern int8_t config_write_save_data(exec_cmd cmd, uint8_t type, uint8_t ch, void *data);
extern void config_save_cache(int cmd, uint8_t type, int8_t ch, void *data);
extern int32_t config_get_gain_calibration_value(uint8_t ch, uint32_t fft_size, uint64_t m_freq);
extern int32_t config_get_fft_calibration_value(uint8_t ch, uint32_t fft_size, uint64_t m_freq);
extern int32_t config_get_analysis_calibration_value(uint64_t m_freq_hz);
extern int32_t config_get_dc_offset_nshift_calibration_value(uint8_t ch, uint32_t fft_size, uint64_t m_freq);
extern uint64_t config_get_disk_alert_threshold(void);
extern void config_set_disk_alert_threshold(uint64_t val);
extern uint64_t config_get_split_file_threshold(void);
extern void config_set_split_file_threshold(uint64_t val);
extern uint32_t config_get_disk_file_notifier_timeout(void);
extern int config_get_control_mode(void);

#endif


