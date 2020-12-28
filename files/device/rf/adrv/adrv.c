#include "adrv.h"
#include "adrv9009/adrv9009-iiostream.h"
#include "adrv9361/adrv9361-iiostream.h"

int8_t adrv_set_rx_gain(uint8_t gain)
{
#ifdef SUPPORT_RF_ADRV9009
    return adrv9009_iio_set_gain(gain);
#elif defined(SUPPORT_RF_ADRV9361)
    return 0;
#endif

}


int16_t adrv_set_rx_freq(uint64_t freq_hz)
{
#ifdef SUPPORT_RF_ADRV9009
    return adrv9009_iio_set_freq(freq_hz);
#elif defined(SUPPORT_RF_ADRV9361)
    iio_config_rx_freq(freq_hz);
    return 0;
#endif
}


int8_t adrv_set_rx_bw(uint32_t bw_hz)
{
#ifdef SUPPORT_RF_ADRV9009
    return adrv9009_iio_set_bw(bw_hz);
#elif defined(SUPPORT_RF_ADRV9361)
    iio_config_rx_bw(bw_hz);
    return 0;
#endif
}


int8_t adrv_set_rx_dc_offset(uint8_t mshift)
{
#ifdef SUPPORT_RF_ADRV9009
    return adrv9009_iio_set_dc_offset_mshift(mshift);
#elif defined(SUPPORT_RF_ADRV9361)

#endif
}


int16_t *adrv_read_rx_data(ssize_t *rsize)
{
#ifdef SUPPORT_RF_ADRV9009
    return adrv9009_iio_read_rx0_data(rsize);
#elif defined(SUPPORT_RF_ADRV9361)
    return adrv9361_iio_read_rx0_data(rsize);
#endif
}


size_t adrv_get_rx_samples_count(void)
{
#ifdef SUPPORT_RF_ADRV9009
    return RF_ADRV9009_IQ_SIZE;
#elif defined(SUPPORT_RF_ADRV9361)
    return RF_ADRV9361_IQ_SIZE;
#endif
}


void adrv_init(void)
{
#ifdef SUPPORT_RF_ADRV9009
    adrv9009_iio_init();
#elif defined(SUPPORT_RF_ADRV9361)
    ad9361_init();
#else
    #error "Not Defined ADRV CHIP...."
    exit(0);
#endif
}



