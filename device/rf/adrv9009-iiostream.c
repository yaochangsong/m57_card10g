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
#include "config.h"
#if (RF_ADRV9009_IIO == 1)

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

static bool stop = false;

/* cleanup and exit */
static void iio_shutdown(void)
{
	printf_info("* Destroying buffers\n");
#if (ADRV_9009_IIO_TX_EN == 1)
	if (txbuf) { iio_buffer_destroy(txbuf); }
	printf_info("* Disabling streaming channels\n");
	if (tx0_i) { iio_channel_disable(tx0_i); }
	if (tx0_q) { iio_channel_disable(tx0_q); }
#endif

#if (ADRV_9009_IIO_RX_EN == 1)
	if (rxbuf) { iio_buffer_destroy(rxbuf); }
	printf_info("* Disabling streaming channels\n");
	if (rx0_i) { iio_channel_disable(rx0_i); }
	if (rx0_q) { iio_channel_disable(rx0_q); }
#endif
	printf_info("* Destroying context\n");
	if (ctx) { iio_context_destroy(ctx); }
	exit(0);
}

static void handle_sig(int sig)
{
	printf_info("Waiting for process to finish...\n");
	stop = true;
	sleep(1);
	iio_shutdown();
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
	printf_info("* Acquiring ADRV9009 TRX lo channel\n");
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
	signal(SIGINT, handle_sig);
	signal(SIGKILL, handle_sig);

	// TRX stream config
	trxcfg.lo_hz = GHZ(1);

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

	printf_info("* Initializing ADRV9009 IIO streaming channels\n");
#if (ADRV_9009_IIO_RX_EN == 1)
	ASSERT(get_adrv9009_stream_ch(ctx, RX, rx, 1, 'i', &rx0_i) && "RX chan i not found");
	ASSERT(get_adrv9009_stream_ch(ctx, RX, rx, 1, 'q', &rx0_q) && "RX chan q not found");
#endif
#if (ADRV_9009_IIO_TX_EN == 1)
	ASSERT(get_adrv9009_stream_ch(ctx, TX, tx, 0, 0, &tx0_i) && "TX chan i not found");
	ASSERT(get_adrv9009_stream_ch(ctx, TX, tx, 1, 0, &tx0_q) && "TX chan q not found");
#endif

	printf_info("* Enabling IIO streaming channels\n");
#if (ADRV_9009_IIO_RX_EN == 1)
	iio_channel_enable(rx0_i);
	iio_channel_enable(rx0_q);
#endif
#if (ADRV_9009_IIO_TX_EN == 1)
	iio_channel_enable(tx0_i);
	iio_channel_enable(tx0_q);
#endif

	printf_info("* Creating non-cyclic IIO buffers with 1 MiS\n");
#if (ADRV_9009_IIO_RX_EN == 1)
	rxbuf = iio_device_create_buffer(rx, 1024*1024, false);
	if (!rxbuf) {
		perror("Could not create RX0 buffer");
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

int16_t adrv9009_iio_set_freq(uint64_t freq_hz)
{
	static uint64_t s_freq_hz = 0;
	if(get_spectrum_debug() == true)
	{
		return 0;
	}
	if(s_freq_hz == freq_hz){
		return 0;
	}
	if(stop == true){
		return 0;
	}
	s_freq_hz = freq_hz;
	printf_note("* Setting ADRV9009 RX freq:%llu\n", freq_hz);
	
	struct iio_channel *chn = NULL;
	if (!get_lo_chan(ctx, &chn)) { return -1; }
	wr_ch_lli(chn, "frequency", freq_hz);
	return 0;
}

int16_t *iio_read_rx0_data(ssize_t *rsize)
{
	ssize_t nbytes_rx;
	if(stop == true){
		return NULL;
	}
	printf_info("* iio_read_rx_data\n");
	nbytes_rx = iio_buffer_refill(rxbuf);
	if (nbytes_rx < 0) { printf("Error refilling buf %d\n",(int) nbytes_rx); iio_shutdown(); }
	*rsize = nbytes_rx;
	return (int16_t *)iio_buffer_first(rxbuf, rx0_i);
}

#endif

