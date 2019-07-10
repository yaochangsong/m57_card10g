#ifndef _PROTOCOL_OAL_H_
#define _PROTOCOL_OAL_H_

#include "config.h"

#if PROTOCAL_XNRP != 0
    #define oal_handle_request  xnrp_handle_request
    #define assamble_response_data  xnrp_assamble_response_data
#elif PROTOCAL_ATE != 0
    #define oal_handle_request  akt_handle_request
    #define assamble_response_data  akt_assamble_response_data
#else
    #error "NOT DEFINE PROTOCAL"
#endif

struct protocal_oal_handle {
    uint8_t  class_code;
    uint8_t  bussiness_code;
    
    //bool (*poal_parse_header)(const uint8_t *data, int len, uint8_t **payload);
   // bool (*poal_execute_method)(void);
    bool (*poal_execute_get_command)(void);
    bool (*poal_execute_set_command)(void);
    bool (*dao_save_config)(void);
    bool (*executor_set_command)(void);
};

/* Third party protocol comparison table define */
struct third_party_comparison_table {
    uint8_t  xnrp_class_code;
    uint8_t  xnrp_bussiness_code;
    uint8_t  akt_class_code;
    uint8_t  akt_bussiness_code;
};

int poal_handle_request(struct net_tcp_client *cl, char *data, int len);
#endif