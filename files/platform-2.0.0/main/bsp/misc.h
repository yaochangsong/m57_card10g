#ifndef _MISC_H_H
#define _MISC_H_H


enum misc_rw_type{
    MISC_WRITE,
    MISC_READ
};

typedef int16_t fft_t;
typedef int16_t iq_t;

struct misc_ops {
    void (*freq_ctrl)(void *args);                                       /* 采样频率相关控制 */
    uint64_t (*freq_convert)(uint64_t m_freq);
    fft_t *(*spm_order_after)(fft_t *ptr, size_t len, void *args, size_t *ordlen);
    int (*pre_handle)(int, void *);
    int (*post_handle)(int, void *);
    ssize_t (*write_handle)(int, const void *, size_t);
    ssize_t (*read_handle)(int, void *);
    int (*usr1_handle)(int, void *);
};

extern int misc_init(void);
extern const struct misc_ops *misc_get(void);


#endif
