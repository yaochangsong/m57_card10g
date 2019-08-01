#ifndef __ADRV_9009_IIO_H__
#define __ADRV_9009_IIO_H__

#define ADRV_9009_IIO_TX_EN  0
#define ADRV_9009_IIO_RX_EN  1

extern void adrv_9009_iio_work_thread(void *arg);
extern void adrv9009_iio_init(void);

#endif
