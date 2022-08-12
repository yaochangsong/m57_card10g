#include "config.h"
//#include "adrv9009/adrv9009-iiostream.h"
//#include "adrv9361/adrv9361-iiostream.h"
//#include "condition/condition.h"


static int adrv_set_rx_gain(uint8_t channel, uint8_t gain)
{
#ifdef CONFIG_DEVICE_RF_ADRV_9009
    return adrv9009_iio_set_gain(gain);
#elif defined(CONFIG_DEVICE_RF_ADRV_9361)
    return 0;
#endif

}

static int adrv_set_rx_rf_gain(uint8_t channel, uint8_t gain)
{
#ifdef CONFIG_DEVICE_RF_ADRV_CONDITION
    condition_attenuation(gain);
#endif
    return 0;
}




static int adrv_set_rx_freq(uint8_t channel, uint64_t freq_hz)
{
//#ifdef CONFIG_BSP_SSA_MONITOR
    if(misc_get() && misc_get()->freq_convert){
        freq_hz = misc_get()->freq_convert(freq_hz);
    }
    //freq_hz = freq_convert(freq_hz);
//#endif
#ifdef CONFIG_DEVICE_RF_ADRV_CONDITION
    condition_freq(freq_hz);
#endif
#ifdef CONFIG_DEVICE_RF_ADRV_9009
#ifdef CONFIG_DEVICE_RF_ADRV_HOP_FREQ
	return adrv9009_iio_set_hop_freq(freq_hz);
#else
    return adrv9009_iio_set_freq(freq_hz);
#endif
#elif defined(CONFIG_DEVICE_RF_ADRV_9361)
    iio_config_rx_freq(freq_hz);
    return 0;
#endif
}

static int adrv_set_rx_bw(uint8_t channel, uint32_t bw_hz)
{
#ifdef CONFIG_DEVICE_RF_ADRV_9009
    return adrv9009_iio_set_bw(bw_hz);
#elif defined(CONFIG_DEVICE_RF_ADRV_9361)
    iio_config_rx_bw(bw_hz);
    return 0;
#endif
}


int8_t adrv_set_rx_dc_offset(uint8_t mshift)
{
#ifdef CONFIG_DEVICE_RF_ADRV_9009
    return adrv9009_iio_set_dc_offset_mshift(mshift);
#elif defined(CONFIG_DEVICE_RF_ADRV_9361)

#endif
}


int16_t *adrv_read_rx_data(ssize_t *rsize)
{
#ifdef CONFIG_DEVICE_RF_ADRV_9009
    return adrv9009_iio_read_rx0_data(rsize);
#elif defined(CONFIG_DEVICE_RF_ADRV_9361)
    return adrv9361_iio_read_rx0_data(rsize);
#endif
}


size_t adrv_get_rx_samples_count(void)
{
#ifdef CONFIG_DEVICE_RF_ADRV_9009
    return RF_ADRV9009_IQ_SIZE;
#elif defined(CONFIG_DEVICE_RF_ADRV_9361)
    return RF_ADRV9361_IQ_SIZE;
#endif
}

static bool check_adrv_status(uint8_t ch)
{
    ch = ch;
    return true;
}


void adrv_init(void)
{
#ifdef CONFIG_DEVICE_RF_ADRV_9009
    adrv9009_iio_init();
#elif defined(CONFIG_DEVICE_RF_ADRV_9361)
    ad9361_init();
#else
    #error "Not Defined ADRV CHIP...."
    exit(0);
#endif
}


static int _rf_init(void)
{
    adrv_init();
    return 0;
}

static const struct rf_ops _rf_ops = {
    .init = _rf_init,
    .set_mid_freq   = adrv_set_rx_freq,
    .set_bindwidth  = adrv_set_rx_bw,
    .set_mgc_gain   = adrv_set_rx_gain,
    .get_rf_gain    = adrv_set_rx_rf_gain,
    .get_status     = check_adrv_status,
};


struct rf_ctx * rf_create_context(void)
{
    struct rf_ctx *ctx = calloc(1, sizeof(*ctx));
     if (!ctx)
        return NULL;
     
    ctx->ops = &_rf_ops;
    adrv_init();

    return ctx;
}


