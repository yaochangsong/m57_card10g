/*
 * libiio - ADRV9009 IIO streaming example
 *
 * Copyright (C) 2014 IABG mbH
 * Author: Michael Feilen <feilen_at_iabg.de>
 * Copyright (C) 2019 Analog Devices Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 **/
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include "log/log.h"
#include "adrv9009-iiostream.h"

#ifdef __APPLE__
#include <iio/iio.h>
#else
#include <iio.h>
#endif


/* helper macros */
#define MHZ(x) ((long long)(x*1000000.0 + .5))
#define GHZ(x) ((long long)(x*1000000000.0 + .5))

#define ASSERT(expr) { \
	if (!(expr)) { \
		(void) fprintf(stderr, "assertion failed (%s:%d)\n", __FILE__, __LINE__); \
		(void) abort(); \
	} \
}

/* RX is input, TX is output */
enum iodev { RX, TX };

/* common RX and TX streaming params */
struct stream_cfg {
	long long lo_hz; // Local oscillator frequency in Hz
};

/* static scratch mem for strings */
static char tmpstr[64];

/* IIO structs required for streaming */
static struct iio_context *ctx   = NULL;
static struct iio_channel *rx0_i = NULL;
static struct iio_channel *rx0_q = NULL;
static struct iio_channel *tx0_i = NULL;
static struct iio_channel *tx0_q = NULL;
static struct iio_buffer  *rxbuf = NULL;
static struct iio_buffer  *txbuf = NULL;

static bool stop;

/* cleanup and exit */
static void iio_shutdown(void)
{
	printf("* Destroying buffers\n");
#if (ADRV_9009_IIO_TX_EN == 1)
	if (txbuf) { iio_buffer_destroy(txbuf); }
	printf("* Disabling streaming channels\n");
	if (tx0_i) { iio_channel_disable(tx0_i); }
	if (tx0_q) { iio_channel_disable(tx0_q); }
#endif

#if (ADRV_9009_IIO_RX_EN == 1)
	if (rxbuf) { iio_buffer_destroy(rxbuf); }
	printf("* Disabling streaming channels\n");
	if (rx0_i) { iio_channel_disable(rx0_i); }
	if (rx0_q) { iio_channel_disable(rx0_q); }
#endif
	printf("* Destroying context\n");
	if (ctx) { iio_context_destroy(ctx); }
	exit(0);
}

static void handle_sig(int sig)
{
	printf("Waiting for process to finish...\n");
	stop = true;
}

/* check return value of attr_write function */
static void errchk(int v, const char* what) {
	 if (v < 0) { fprintf(stderr, "Error %d writing to channel \"%s\"\nvalue may not be supported.\n", v, what); iio_shutdown(); }
}

/* write attribute: long long int */
static void wr_ch_lli(struct iio_channel *chn, const char* what, long long val)
{
	errchk(iio_channel_attr_write_longlong(chn, what, val), what);
}

/* write attribute: long long int */
static long long rd_ch_lli(struct iio_channel *chn, const char* what)
{
	long long val;

	errchk(iio_channel_attr_read_longlong(chn, what, &val), what);

	printf_info("\t %s: %lld\n", what, val);
	return val;
}

#if 0
/* write attribute: string */
static void wr_ch_str(struct iio_channel *chn, const char* what, const char* str)
{
	errchk(iio_channel_attr_write(chn, what, str), what);
}
#endif

/* helper function generating channel names */
static char* get_ch_name_mod(const char* type, int id, char modify)
{
	snprintf(tmpstr, sizeof(tmpstr), "%s%d_%c", type, id, modify);
	return tmpstr;
}

/* helper function generating channel names */
static char* get_ch_name(const char* type, int id)
{
	snprintf(tmpstr, sizeof(tmpstr), "%s%d", type, id);
	return tmpstr;
}

/* returns adrv9009 phy device */
static struct iio_device* get_adrv9009_phy(struct iio_context *ctx)
{
	struct iio_device *dev =  iio_context_find_device(ctx, "adrv9009-phy");
	ASSERT(dev && "No adrv9009-phy found");
	return dev;
}

/* finds adrv9009 streaming IIO devices */
static bool get_adrv9009_stream_dev(struct iio_context *ctx, enum iodev d, struct iio_device **dev)
{
	switch (d) {
	case TX: *dev = iio_context_find_device(ctx, "axi-adrv9009-tx-hpc"); return *dev != NULL;
	case RX: *dev = iio_context_find_device(ctx, "axi-adrv9009-rx-hpc");  return *dev != NULL;
	default: ASSERT(0); return false;
	}
}

/* finds adrv9009 streaming IIO channels */
static bool get_adrv9009_stream_ch(struct iio_context *ctx, enum iodev d, struct iio_device *dev, int chid, char modify, struct iio_channel **chn)
{
	*chn = iio_device_find_channel(dev, modify ? get_ch_name_mod("voltage", chid, modify) : get_ch_name("voltage", chid), d == TX);
	if (!*chn)
		*chn = iio_device_find_channel(dev, modify ? get_ch_name_mod("voltage", chid, modify) : get_ch_name("voltage", chid), d == TX);
	return *chn != NULL;
}

/* finds adrv9009 phy IIO configuration channel with id chid */
static bool get_phy_chan(struct iio_context *ctx, enum iodev d, int chid, struct iio_channel **chn)
{
	switch (d) {
	case RX: *chn = iio_device_find_channel(get_adrv9009_phy(ctx), get_ch_name("voltage", chid), false); return *chn != NULL;
	case TX: *chn = iio_device_find_channel(get_adrv9009_phy(ctx), get_ch_name("voltage", chid), true);  return *chn != NULL;
	default: ASSERT(0); return false;
	}
}

/* finds adrv9009 local oscillator IIO configuration channels */
static bool get_lo_chan(struct iio_context *ctx, struct iio_channel **chn)
{
	 // LO chan is always output, i.e. true
	*chn = iio_device_find_channel(get_adrv9009_phy(ctx), get_ch_name("altvoltage", 0), true); return *chn != NULL;
}

/* applies streaming configuration through IIO */
bool cfg_adrv9009_streaming_ch(struct iio_context *ctx, struct stream_cfg *cfg, enum iodev type, int chid)
{
	struct iio_channel *chn = NULL;

	// Configure phy and lo channels
	printf_info("* Acquiring ADRV9009 phy channel %d\n", chid);
	if (!get_phy_chan(ctx, type, chid, &chn)) {	return false; }

	rd_ch_lli(chn, "rf_bandwidth");
	rd_ch_lli(chn, "sampling_frequency");

	// Configure LO channel
	printf("* Acquiring ADRV9009 TRX lo channel\n");
	if (!get_lo_chan(ctx, &chn)) { return false; }
	wr_ch_lli(chn, "frequency", cfg->lo_hz);
	return true;
}

void adrv9009_iio_init(void)
{
	// Streaming devices
	struct iio_device *tx;
	struct iio_device *rx;

	// RX and TX sample counters
	size_t nrx = 0;
	size_t ntx = 0;

	// Stream configuration
	struct stream_cfg trxcfg;

	// Listen to ctrl+c and ASSERT
	//signal(SIGINT, handle_sig);

	// TRX stream config
	trxcfg.lo_hz = GHZ(2.5);

	printf_info("* Acquiring IIO context\n");
	ASSERT((ctx = iio_create_default_context()) && "No context");
	ASSERT(iio_context_get_devices_count(ctx) > 0 && "No devices");

	
#if (ADRV_9009_IIO_TX_EN == 1)
	printf_info("* Acquiring ADRV9009 TX streaming devices\n");
	ASSERT(get_adrv9009_stream_dev(ctx, TX, &tx) && "No tx dev found");
	printf_info("* Configuring ADRV9009 for streaming\n");
	ASSERT(cfg_adrv9009_streaming_ch(ctx, &trxcfg,TX, 0) && "TRX device not found");
#endif
#if (ADRV_9009_IIO_RX_EN == 1)
	printf_info("* Acquiring ADRV9009 RX streaming devices\n");
	ASSERT(get_adrv9009_stream_dev(ctx, RX, &rx) && "No rx dev found");
	printf_info("* Configuring ADRV9009 for streaming\n");
	ASSERT(cfg_adrv9009_streaming_ch(ctx, &trxcfg,RX, 0) && "TRX device not found");
#endif

	printf("* Initializing ADRV9009 IIO streaming channels\n");
#if (ADRV_9009_IIO_RX_EN == 1)
	ASSERT(get_adrv9009_stream_ch(ctx, RX, rx, 0, 'i', &rx0_i) && "RX chan i not found");
	ASSERT(get_adrv9009_stream_ch(ctx, RX, rx, 0, 'q', &rx0_q) && "RX chan q not found");
#endif
#if (ADRV_9009_IIO_TX_EN == 1)
	ASSERT(get_adrv9009_stream_ch(ctx, TX, tx, 0, 0, &tx0_i) && "TX chan i not found");
	ASSERT(get_adrv9009_stream_ch(ctx, TX, tx, 1, 0, &tx0_q) && "TX chan q not found");
#endif

	printf("* Enabling IIO streaming channels\n");
#if (ADRV_9009_IIO_RX_EN == 1)
	iio_channel_enable(rx0_i);
	iio_channel_enable(rx0_q);
#endif
#if (ADRV_9009_IIO_TX_EN == 1)
	iio_channel_enable(tx0_i);
	iio_channel_enable(tx0_q);
#endif

	printf("* Creating non-cyclic IIO buffers with 1 MiS\n");
#if (ADRV_9009_IIO_RX_EN == 1)
	rxbuf = iio_device_create_buffer(rx, 1024*1024, false);
	if (!rxbuf) {
		perror("Could not create RX buffer");
		iio_shutdown();
	}
#endif
#if (ADRV_9009_IIO_TX_EN == 1)
	txbuf = iio_device_create_buffer(tx, 1024*1024, false);
	if (!txbuf) {
		perror("Could not create TX buffer");
		iio_shutdown();
	}
#endif

}

int16_t adrv9009_iio_set_freq(long long freq)
{
	struct stream_cfg trxcfg;
	trxcfg.lo_hz = freq;
	printf_info("* Configuring ADRV9009 for streaming\n");
	ASSERT(cfg_adrv9009_streaming_ch(ctx, &trxcfg,RX, 0) && "TRX device not found");
	return 0;
}

int16_t * iio_read_rx_data(ssize_t *rsize)
{
	ssize_t nbytes_rx, nbytes_tx;
	printf_info("* iio_read_rx_data\n");
	nbytes_rx = iio_buffer_refill(rxbuf);
	if (nbytes_rx < 0) { printf("Error refilling buf %d\n",(int) nbytes_rx); iio_shutdown(); }
	*rsize = nbytes_rx;
	return (int16_t *)iio_buffer_first(rxbuf, rx0_i);
}

ssize_t iio_get_rx_buf_size(void)
{
	ssize_t nbytes_rx;
	printf_info("* iio_read_rx_buffer size and refill rxbuf data \n");
	nbytes_rx = iio_buffer_refill(rxbuf);
	if (nbytes_rx < 0) { printf("Error refilling buf %d\n",(int) nbytes_rx); iio_shutdown(); }
	return nbytes_rx;
}




/* simple configuration and streaming */
void adrv_9009_iio_work_thread(void *arg)
{
	int write_file_cnter = 0;
	char strbuf[128];
	// Streaming devices
	struct iio_device *tx;
	struct iio_device *rx;

	// RX and TX sample counters
	size_t nrx = 0;
	size_t ntx = 0;

	// Stream configuration
	struct stream_cfg trxcfg;

	// Listen to ctrl+c and ASSERT
	signal(SIGINT, handle_sig);

	// TRX stream config
	trxcfg.lo_hz = GHZ(2.5);

	printf_info("* Acquiring IIO context\n");
	ASSERT((ctx = iio_create_default_context()) && "No context");
	ASSERT(iio_context_get_devices_count(ctx) > 0 && "No devices");

	
#if (ADRV_9009_IIO_TX_EN == 1)
	printf_info("* Acquiring ADRV9009 TX streaming devices\n");
	ASSERT(get_adrv9009_stream_dev(ctx, TX, &tx) && "No tx dev found");
	printf_info("* Configuring ADRV9009 for streaming\n");
	ASSERT(cfg_adrv9009_streaming_ch(ctx, &trxcfg,TX, 0) && "TRX device not found");
#endif
#if (ADRV_9009_IIO_RX_EN == 1)
	printf_info("* Acquiring ADRV9009 RX streaming devices\n");
	ASSERT(get_adrv9009_stream_dev(ctx, RX, &rx) && "No rx dev found");
	printf_info("* Configuring ADRV9009 for streaming\n");
	ASSERT(cfg_adrv9009_streaming_ch(ctx, &trxcfg,RX, 0) && "TRX device not found");
#endif

	printf("* Initializing ADRV9009 IIO streaming channels\n");
#if (ADRV_9009_IIO_RX_EN == 1)
	ASSERT(get_adrv9009_stream_ch(ctx, RX, rx, 0, 'i', &rx0_i) && "RX chan i not found");
	ASSERT(get_adrv9009_stream_ch(ctx, RX, rx, 0, 'q', &rx0_q) && "RX chan q not found");
#endif
#if (ADRV_9009_IIO_TX_EN == 1)
	ASSERT(get_adrv9009_stream_ch(ctx, TX, tx, 0, 0, &tx0_i) && "TX chan i not found");
	ASSERT(get_adrv9009_stream_ch(ctx, TX, tx, 1, 0, &tx0_q) && "TX chan q not found");
#endif

	printf("* Enabling IIO streaming channels\n");
#if (ADRV_9009_IIO_RX_EN == 1)
	iio_channel_enable(rx0_i);
	iio_channel_enable(rx0_q);
#endif
#if (ADRV_9009_IIO_TX_EN == 1)
	iio_channel_enable(tx0_i);
	iio_channel_enable(tx0_q);
#endif

	printf("* Creating non-cyclic IIO buffers with 1 MiS\n");
#if (ADRV_9009_IIO_RX_EN == 1)
	rxbuf = iio_device_create_buffer(rx, 1024*1024, false);
	if (!rxbuf) {
		perror("Could not create RX buffer");
		iio_shutdown();
	}
#endif
#if (ADRV_9009_IIO_TX_EN == 1)
	txbuf = iio_device_create_buffer(tx, 1024*1024, false);
	if (!txbuf) {
		perror("Could not create TX buffer");
		iio_shutdown();
	}
#endif

	printf("* Starting IO streaming (press CTRL+C to cancel)\n");
	while (!stop)
	{
		ssize_t nbytes_rx, nbytes_tx;
		char *p_dat, *p_end;
		ptrdiff_t p_inc;
		int i;
		int16_t *iqdata;
		

#if (ADRV_9009_IIO_RX_EN == 1)
		// Refill RX buffer
		nbytes_rx = iio_buffer_refill(rxbuf);
		if (nbytes_rx < 0) { printf("Error refilling buf %d\n",(int) nbytes_rx); iio_shutdown(); }

		// READ: Get pointers to RX buf and read IQ from RX buf port 0
		p_inc = iio_buffer_step(rxbuf);
		p_end = iio_buffer_end(rxbuf);
		printf("p_first=%p, p_end=%p\n", iio_buffer_first(rxbuf, rx0_i), p_end);
		for (p_dat = iio_buffer_first(rxbuf, rx0_i); p_dat < p_end; p_dat += p_inc) {
			// Example: swap I and Q
			const int16_t i = ((int16_t*)p_dat)[0]; // Real (I)
			const int16_t q = ((int16_t*)p_dat)[1]; // Imag (Q)
			((int16_t*)p_dat)[0] = q;
			((int16_t*)p_dat)[1] = i;
		}
		// Sample counter increment and status output
		//nrx += nbytes_rx / iio_device_get_sample_size(rx);
		//iio_buffer_get_data(rxbuf);
		iqdata = (int16_t *)iio_buffer_first(rxbuf, rx0_i);
		printf("get_sample_size=%d, nbytes_rx=%d, q=%d, i=%d\n", iio_device_get_sample_size(rx), nbytes_rx, ((int16_t*)p_dat)[0], ((int16_t*)p_dat)[1]);
		printfi("IQ data:");
		for(i = 0; i< 10; i++){
			printfi("%d ",*(int16_t*)(iqdata+i));
		}
		printfi("\nIQ data End\n");
		//printf("\tRX %8.2f MSmp, ((int16_t*)p_dat)[0]=%d\n", nrx/1e6, ((int16_t*)p_dat)[0]);
		if(write_file_cnter++ < 3){
			sprintf(strbuf, "/run/wav%d", write_file_cnter);
			printf("write iq data to:[len=%d]%s\n", nbytes_rx/2, strbuf);
			write_file_in_int16((void*)(iqdata), nbytes_rx/2, strbuf);
		}
        	
#endif
#if (ADRV_9009_IIO_TX_EN == 1)
				// Schedule TX buffer
		nbytes_tx = iio_buffer_push(txbuf);
		if (nbytes_tx < 0) { printf("Error pushing buf %d\n", (int) nbytes_tx); iio_shutdown(); }

		// WRITE: Get pointers to TX buf and write IQ to TX buf port 0
		p_inc = iio_buffer_step(txbuf);
		p_end = iio_buffer_end(txbuf);
		for (p_dat = iio_buffer_first(txbuf, tx0_i); p_dat < p_end; p_dat += p_inc) {
			// Example: fill with zeros
			// 14-bit sample needs to be MSB alligned so shift by 2
			// https://wiki.analog.com/resources/eval/user-guides/ad-fmcomms2-ebz/software/basic_iq_datafiles#binary_format
			((int16_t*)p_dat)[0] = 0 << 2; // Real (I)
			((int16_t*)p_dat)[1] = 0 << 2; // Imag (Q)
		}
		// Sample counter increment and status output
		ntx += nbytes_tx / iio_device_get_sample_size(tx);
		printf("\tTX %8.2f MSmp\n", nrx/1e6, ntx/1e6);
#endif
	}

	iio_shutdown();

	return 0;
}

