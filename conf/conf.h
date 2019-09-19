#ifndef _CONF_H_
#define _CONF_H_
#include "config.h"

/** Defaults configuration values */
#ifdef PLAT_FORM_ARCH_X86
#define DEFAULT_CONFIGFILE "spectrum.xml"
#define CALIBRATION_FILE   "calibration.xml"
#else
#define DEFAULT_CONFIGFILE "spectrum.xml"
#define CALIBRATION_FILE   "calibration.xml"
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

extern int8_t config_save_batch(exec_cmd cmd, uint8_t type,s_config *config);
extern int8_t config_read_by_cmd(exec_cmd cmd, uint8_t type, uint8_t ch, void *data);
extern int8_t config_write_data(exec_cmd cmd, uint8_t type, uint8_t ch, void *data);


#endif


