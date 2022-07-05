#ifndef _CONF_H_
#define _CONF_H_


/** Defaults configuration values */
#if defined (CONFIG_DAO_XML)
    #define DEFAULT_CONFIGFILE "/etc/spectrum.xml"
#elif defined(CONFIG_DAO_JSON)
    #define DEFAULT_CONFIGFILE "/etc/config.json"
#endif

/**
 * Mutex for the configuration file, used by the auth_servers related
 * functions. */
extern pthread_mutex_t config_mutex;


#include "config.h"
/**
 * Configuration structure
 */
typedef struct {
    char *configfile;       /**< @brief name of the config file */
    char *version;
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
extern int8_t config_write_data(int cmd, uint8_t type, uint8_t ch, void *data, ...);
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
extern bool config_get_is_internal_clock(void);
extern int config_set_network(char *ifname, uint32_t ipaddr, uint32_t netmask, uint32_t gateway);
extern int config_set_ip(char *ifname, uint32_t ipaddr);
extern int config_set_netmask(char *ifname, uint32_t netmask);
extern int config_set_gateway(char *ifname, uint32_t gateway);
extern int config_get_if_nametoindex(char *ifname);
extern char *config_get_if_indextoname(int index);
extern int config_get_fft_resolution(int mode, int ch, int index, uint32_t bw_hz,uint32_t *fft_size, uint32_t *resolution);
extern bool config_is_temperature_warning(int16_t temperature);
extern uint16_t *config_get_fft_window_data(int type, size_t *fsize);
extern size_t config_get_niq_send_size(void);
extern char *config_get_devicecode(void);
extern bool config_set_file_auto_save_mode(bool is_auto);
extern bool config_get_file_auto_save_mode(void);
extern bool config_match_gateway_addr(const char *ifname, uint32_t gateway);
extern bool config_match_ipaddr_addr(const char *ifname, uint32_t ipaddr);
extern bool config_match_netmask_addr(const char *ifname, uint32_t netmask);
extern char *config_get_ifname_by_addr(struct sockaddr_in *addr);
extern int config_get_if_cmd_port(const char *ifname, uint16_t *port);
extern int config_set_if_cmd_port(const char *ifname, uint16_t port);
extern uint32_t config_get_noise_level(void);
#endif


