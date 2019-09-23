#ifndef _XNRP_XML_H_
#define _XNRP_XML_H_
int8_t xnrp_xml_parse_data(uint8_t classcode, uint8_t methodcode, void *data, uint32_t len);

int xnrp_xml_execute_get_command(uint8_t classcode, uint8_t methodcode,uint8_t  *resp_header ,uint8_t len);

#endif