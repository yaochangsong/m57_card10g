#ifndef _MISC_H_H
#define _MISC_H_H

#include "config.h"

struct misc_ops {
    void (*freq_ctrl)(void *args);                                       /* 采样频率相关控制 */
    uint64_t (*freq_convert)(uint64_t m_freq);
    fft_t *(*spm_order_after)(fft_t *ptr, size_t len, void *args, size_t *ordlen);
};

extern int misc_init(void);
extern const struct misc_ops *misc_get(void);


#endif
