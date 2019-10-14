#ifndef __ADRV_9009_IIO_H__
#define __ADRV_9009_IIO_H__

#define ADRV_9009_IIO_TX_EN  0
#define ADRV_9009_IIO_RX_EN  1

#define  ADRV_9009_BAND_WITH_100M (100000000ULL)
#define  ADRV_9009_BAND_WITH_200M (200000000ULL)

#define RF_ADRV9009_BANDWITH ADRV_9009_BAND_WITH_200M
#define RF_ADRV9009_IQ_SIZE  (1024*1024)

extern void adrv_9009_iio_work_thread(void *arg);
extern void adrv9009_iio_init(void);
extern int16_t *adrv9009_iio_read_rx0_data(ssize_t *rsize);
#endif
