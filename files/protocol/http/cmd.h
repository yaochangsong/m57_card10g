
#ifndef _CMD_HTTP_RESTFUL_
#define _CMD_HTTP_RESTFUL_
extern int cmd_muti_point(struct uh_client *cl, void **arg);
extern int cmd_multi_band(struct uh_client *cl, void **arg);
extern int cmd_demodulation(struct uh_client *cl, void **arg);
extern int cmd_if_single_value_set(struct uh_client *cl, void **arg);
extern int cmd_rf_single_value_set(struct uh_client *cl, void **arg);
extern int cmd_if_multi_value_set(struct uh_client *cl, void **arg);
extern int cmd_rf_multi_value_set(struct uh_client *cl, void **arg);
extern int cmd_disk_cmd(struct uh_client *cl, void **arg);
extern int cmd_ch_enable_set(struct uh_client *cl, void **arg);
extern int cmd_subch_enable_set(struct uh_client *cl, void **arg);

#endif
