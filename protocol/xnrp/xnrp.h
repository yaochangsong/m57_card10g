#ifndef _XNRP_H_H
#define _XNRP_H_H

#define XNRP_HEADER_START  "XNRP"

struct xnrp_header {
    char start_flag[4];
    uint8_t version;
    uint8_t method_code;
    uint8_t error_code;
    uint8_t class_code;
    uint8_t business_code;
    uint8_t client_id[6];
    uint8_t device_id[6];
    long time_stamp;
    uint16_t msg_id;
    uint16_t check_sum;
    long payload_len;
    uint8_t *payload;
}__attribute__ ((packed));

struct xnrp_multi_frequency_point {
}__attribute__ ((packed));



bool xnrp_handle_request(uint8_t *data, int len, int *code);

#endif

