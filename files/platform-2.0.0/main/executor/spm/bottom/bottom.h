#ifndef _BOTTOM_H
#define _BOTTOM_H

typedef int16_t fft_t;
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))
#endif

extern void bottom_init(void);
extern int bottom_calibration(int ch, void *data, size_t data_len, int fftsize, long long mfreq, uint32_t bw);
extern void bottom_deal(int ch, fft_t *data, size_t data_len, size_t fft_size, long long mfreq, uint32_t bw);


#endif
