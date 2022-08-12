#ifndef _AGC_H_H
#define _AGC_H_H

extern int agc_ctrl(int ch, struct spm_context *ctx);
extern int agc_init(void);
extern void agc_ctrl_thread_notify(int ch);


#endif

