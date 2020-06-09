#ifndef _CLOCK_ADC_FPGA_H_
#define _CLOCK_ADC_FPGA_H_

static struct clock_adc_t{
        int fd_mem_dev;
        uint32_t *vir_addr;
        int in_clock;
        int adc_status;
};

#endif
