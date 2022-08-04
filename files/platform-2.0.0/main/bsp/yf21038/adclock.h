#ifndef _CLOCK_ADC_FPGA_H_
#define _CLOCK_ADC_FPGA_H_

struct clock_adc_t{
        int fd_mem_dev;
        volatile void *vir_addr;
        int in_clock;
        int adc_status;
};

extern struct clock_adc_ctx * clock_adc_fpga_cxt(void);
extern void volume_set(intptr_t base,uint8_t dat, uint8_t ch);
#endif
