#ifndef _PLATFORM_H
#define _PLATFORM_H

#ifndef MHZ
#define MHZ(x) ((long long)((x)*1000000.0 + .5))
#endif
#ifndef GHZ
#define GHZ(x) ((long long)((x)*1000000000.0 + .5))
#endif

#define PLATFORM_VERSION  "1.1.0" /* application version */

extern bool is_disk_format(void);
extern uint32_t get_power_level_threshold(void);
extern bool is_spectrum_aditool_debug(void);

#endif
