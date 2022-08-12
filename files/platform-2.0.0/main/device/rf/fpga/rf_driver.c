#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <time.h>
#include <stdint.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <math.h>
#include <linux/spi/spidev.h>
#include <stdbool.h>
#include "rf_driver.h"
#include "config.h"

pthread_mutex_t rf_param_mutex[MAX_RF_NUM];

void set_rf_safe(int rfch, uint32_t *reg, uint32_t val)
{
    pthread_mutex_lock(&rf_param_mutex[rfch]);
    *reg = val;
    usleep(100);
    pthread_mutex_unlock(&rf_param_mutex[rfch]);
}

uint32_t get_rf_safe(int rfch, uint32_t *reg)
{
    pthread_mutex_lock(&rf_param_mutex[rfch]);
    uint32_t val;
    val = *reg;
    usleep(500);
    pthread_mutex_unlock(&rf_param_mutex[rfch]);
    return val;
}

static int rf_get_temperature(uint8_t ch, int16_t *temperatue)
{
    int ret = 0;
    int16_t temp = 0;
    
    if(reg_get()->rf && reg_get()->rf->get_temperature)
        ret = reg_get()->rf->get_temperature(ch, &temp);

    if(ret < 0)
        return -1;
    
    *temperatue = temp;
    return 0;
}

static int rf_set_rf_gain(uint8_t ch, uint8_t gain)
{
    if(reg_get()->rf && reg_get()->rf->set_attenuation)
        reg_get()->rf->set_attenuation(ch, 0, gain);
    return 0;
}

static int _rf_set_bw(uint8_t channel, uint32_t bw_hz)
{
    if(reg_get()->rf && reg_get()->rf->set_bandwidth)
        reg_get()->rf->set_bandwidth(channel, 0, bw_hz);

    return 0;
}


static int _rf_set_mode(uint8_t channel, uint8_t mode)
{
    if(reg_get()->rf && reg_get()->rf->set_mode_code)
        reg_get()->rf->set_mode_code(channel, 0, mode);

    return 0;
}


static int rf_set_mf_gain(uint8_t channel, uint8_t gain)
{
    if(reg_get()->rf && reg_get()->rf->set_mgc_gain)
        reg_get()->rf->set_mgc_gain(channel, 0, gain);

    return 0;
}


static int _rf_set_frequency(uint8_t channel, uint64_t freq_hz)
{
    if(reg_get()->rf && reg_get()->rf->set_frequency)
        reg_get()->rf->set_frequency(channel, 0, freq_hz);

    return 0;
}

static int _rf_set_sample_ctrl(uint8_t channel, int  onoff)
{
    if(reg_get()->rf && reg_get()->rf->set_direct_sample_ctrl)
        reg_get()->rf->set_direct_sample_ctrl(0, 0, onoff);

    return 0;
}

static bool check_rf_status(uint8_t ch)
{
    int ret = 0;
    int16_t temperature;
    if(reg_get()->rf && reg_get()->rf->get_temperature)
        ret = reg_get()->rf->get_temperature(ch, &temperature);

    return (ret < 0 ? false : true);
}

static const struct rf_ops _rf_ops = {
    .set_mid_freq   = _rf_set_frequency,
    .set_bindwidth  = _rf_set_bw,
    .set_work_mode  = _rf_set_mode,
    .set_mgc_gain   = rf_set_mf_gain,
    .set_rf_gain    = rf_set_rf_gain,
    .get_status     = check_rf_status,
    .get_temperature = rf_get_temperature,
};


struct rf_ctx * rf_create_context(void)
{
    struct rf_ctx *ctx = calloc(1, sizeof(*ctx));
     if (!ctx)
        return NULL;
     
    ctx->ops = &_rf_ops;
    for(int i = 0; i < MAX_RF_NUM; i++){
        pthread_mutex_init(&rf_param_mutex[i], NULL);
    }
    return ctx;
}
