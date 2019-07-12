#ifndef _CONF_H_
#define _CONF_H_
#include "config.h"

/** Defaults configuration values */
#ifdef PLAT_FORM_ARCH_X86
#define DEFAULT_CONFIGFILE "spectrum.xml"
#else
#define DEFAULT_CONFIGFILE "/etc/spectrum.xml"
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
    int daemon;             /**< @brief if daemon > 0, use daemon mode */
    struct poal_config oal_config;
} s_config;



/** @brief Initialise the conf system */
void config_init(void);


#define LOCK_CONFIG() do { \
	printf_debug("Locking config"); \
	pthread_mutex_lock(&config_mutex); \
	printf_debug("Config locked"); \
} while (0)

#define UNLOCK_CONFIG() do { \
	printf_debug("Unlocking config"); \
	pthread_mutex_unlock(&config_mutex); \
	printf_debug("Config unlocked"); \
} while (0)

extern s_config *config_get_config(void);
#endif

