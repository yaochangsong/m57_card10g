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

int poal_handle_request(struct net_tcp_client *cl, char *data, int len);
#endif