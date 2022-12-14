#******************************************************************************
#  Copyright 2019, Showay Technology Dev Co.,Ltd.
#  ---------------------------------------------------------------------------
#  Statement:
#  ----------
#  This software is protected by Copyright and the information contained
#  herein is confidential. The software may not be copied and the information
#  contained herein may not be used or disclosed except with the written
#  permission of Showay Technology Dev Co.,Ltd. (C) 2019
#*****************************************************************************
#*****************************************************************************     
#  Rev 1.0   06 July 2019   yaochangsong
#  Initial revision.
#*****************************************************************************
include make.settings

#debug
CFLAGS:=
ifneq ($(findstring y,$(SUPPORT_GDB_DEBUG)),)
	top_dir = $(abspath $(shell pwd)/../../../../../)
	export CC=$(PETALINUX)/tools/linux-i386/gcc-arm-linux-gnueabi/bin/arm-linux-gnueabihf-gcc
	C_VAR += -DSUPPORT_GDB_DEBUG
	CFLAGS = -g
endif
	top_dir ?= $(abspath $(shell pwd)/../../../../../../)
#arch
ifneq ($(findstring x86,$(SUPPORT_PLAT_FORM_ARCH)),)
	CC?=gcc
	C_VAR += -DSUPPORT_PLATFORM_ARCH_X86
else ifneq ($(findstring arm,$(SUPPORT_PLAT_FORM_ARCH)),)
	#make CC=aarch64-linux-gnu-gcc
	CC?=arm-linux-gnueabihf-gcc
	C_VAR += -DSUPPORT_PLATFORM_ARCH_ARM
	C_VAR += -DPETALINUX_VER_$(subst .,_,$(PETALINUX_VER))
endif

#protocal
ifneq ($(findstring http,$(SUPPORT_PROTOCAL)),)
	SOURCE_DIR := protocol/http
	SOURCE_DIR += protocol/resetful
	C_VAR += -DSUPPORT_PROTOCAL_HTTP
endif

ifneq ($(findstring xw,$(SUPPORT_PROTOCAL)),)
	SOURCE_DIR := protocol/http
	SOURCE_DIR += protocol/resetful
	C_VAR += -DSUPPORT_PROTOCAL_XW

endif

ifneq ($(findstring m57,$(SUPPORT_PROTOCAL)),)
	SOURCE_DIR += protocol/m57
	C_VAR += -DSUPPORT_PROTOCAL_M57
endif

ifneq ($(findstring akt,$(SUPPORT_PROTOCAL)),)
	SOURCE_DIR += protocol/akt
	C_VAR += -DSUPPORT_PROTOCAL_AKT
endif

ifneq ($(findstring akt,$(SUPPORT_DATA_PROTOCAL)),)
	C_VAR += -DSUPPORT_DATA_PROTOCAL_AKT
else ifneq ($(findstring xw,$(SUPPORT_DATA_PROTOCAL)),)
	C_VAR += -DSUPPORT_DATA_PROTOCAL_XW
else ifneq ($(findstring m57,$(SUPPORT_DATA_PROTOCAL)),)
	C_VAR += -DSUPPORT_DATA_PROTOCAL_M57
endif


#??????
ifneq ($(findstring y,$(SUPPORT_NET_WZ)),)
	C_VAR += -DSUPPORT_NET_WZ
endif

#xwfs
ifneq ($(findstring y,$(SUPPORT_XWFS)),)
	C_VAR += -DSUPPORT_XWFS
	SOURCE_DIR += executor/xwfs
endif

#fs2.0
ifneq ($(findstring y,$(SUPPORT_FS)),)
	C_VAR += -DSUPPORT_FS
	SOURCE_DIR += fs
endif

#lcd
ifneq ($(findstring y,$(SUPPORT_LCD)),)
	SOURCE_DIR += device/lcd
	C_VAR += -DSUPPORT_LCD
endif

#rs485
ifneq ($(findstring y,$(SUPPORT_RS485)),)
	SOURCE_DIR += device/rs485
	C_VAR += -DSUPPORT_RS485
endif
ifneq ($(findstring y,$(SUPPORT_RS485_AMPLIFIER)),)
	C_VAR += -DSUPPORT_RS485_AMPLIFIER
endif
ifneq ($(findstring y,$(SUPPORT_RS485_EC)),)
	C_VAR += -DSUPPORT_RS485_EC
endif


#uart
ifneq ($(findstring y,$(SUPPORT_UART)),)
	SOURCE_DIR += device/uart
	C_VAR += -DSUPPORT_UART
endif

#ambient temperature&humidity
ifneq ($(findstring y,$(SUPPORT_AMBIENT_TEMP_HUMIDITY)),)
	SOURCE_DIR += device/humidity
	C_VAR += -DSUPPORT_AMBIENT_TEMP_HUMIDITY
endif
ifneq ($(findstring y,$(SUPPORT_TEMP_HUMIDITY_SI7021)),)
	C_VAR += -DSUPPORT_TEMP_HUMIDITY_SI7021
endif

#audio
ifneq ($(findstring y,$(SUPPORT_AUDIO)),)
	SOURCE_DIR += device/audio
	C_VAR += -DSUPPORT_AUDIO
endif

#NRS1800
ifneq ($(findstring y,$(SUPPORT_NRS1800)),)
	SOURCE_DIR += device/nrs1800
	C_VAR += -DSUPPORT_NRS1800
endif

#gps
ifneq ($(findstring y,$(SUPPORT_GPS)),)
	SOURCE_DIR += device/gps
	C_VAR += -DSUPPORT_GPS
endif

#rf
ifneq ($(findstring y,$(SUPPORT_RF)),)
	SOURCE_DIR += device/rf
	C_VAR += -DSUPPORT_RF
endif
#rf spi
ifneq ($(findstring y,$(SUPPORT_RF_SPI)),)
	SOURCE_DIR += device/rf/spi
	C_VAR += -DSUPPORT_RF_SPI
endif

ifneq ($(findstring y,$(SUPPORT_RF_SPI_M57)),)
	SOURCE_DIR += device/rf/spi/m57
	C_VAR += -DSUPPORT_RF_SPI_M57
endif
#rf fpga
ifneq ($(findstring y,$(SUPPORT_RF_FPGA)),)
	C_VAR += -DSUPPORT_RF_FPGA
endif

ifneq ($(findstring y,$(SUPPORT_DIRECT_SAMPLE)),)	
	C_VAR += -DSUPPORT_DIRECT_SAMPLE
endif

#clock adc
##clock adc spis
ifneq ($(findstring y,$(SUPPORT_CLOCK_ADC)),)
	SOURCE_DIR += device/clk_adc
	C_VAR += -DSUPPORT_CLOCK_ADC
endif
ifneq ($(findstring y,$(SUPPORT_CLOCK_ADC_SPI)),)
	C_VAR += -DSUPPORT_CLOCK_ADC_SPI
endif
##clock adc fpga
ifneq ($(findstring y,$(SUPPORT_CLOCK_ADC_FPGA)),)
	C_VAR += -DSUPPORT_CLOCK_ADC_FPGA
endif

ifneq ($(findstring y,$(SUPPORT_RF_ADRV)),)
	SOURCE_DIR += device/rf/adrv
	C_VAR += -DSUPPORT_RF_ADRV
endif
ifneq ($(findstring y,$(SUPPORT_RF_ADRV9009)),)
	SOURCE_DIR += device/rf/adrv/adrv9009
	C_VAR += -DSUPPORT_RF_ADRV9009
endif
ifneq ($(findstring y,$(SUPPORT_RF_ADRV9361)),)
	SOURCE_DIR += device/rf/adrv/adrv9361
	C_VAR += -DSUPPORT_RF_ADRV9361
endif

#specturm
ifneq ($(findstring y,$(SUPPORT_SPECTRUM_V2)),)
	SOURCE_DIR += executor/spm
	C_VAR += -DSUPPORT_SPECTRUM_V2
	ifneq ($(findstring y,$(SUPPORT_SPECTRUM_FPGA)),)
		C_VAR += -DSUPPORT_SPECTRUM_FPGA
	endif
	ifneq ($(findstring y,$(SUPPORT_SPECTRUM_CHIP)),)
		C_VAR += -DSUPPORT_SPECTRUM_CHIP
	endif
endif

ifneq ($(findstring y,$(SUPPORT_SPECTRUM_SERIAL)),)
	C_VAR += -DSUPPORT_SPECTRUM_SERIAL
endif
#specturm user
ifneq ($(findstring y,$(SUPPORT_SPECTRUM_FFT)),)
	SOURCE_DIR += executor/fft
	C_VAR += -DSUPPORT_SPECTRUM_FFT
endif

#specturm analysis
ifneq ($(findstring y,$(SUPPORT_SPECTRUM_ANALYSIS)),)
	SOURCE_DIR += executor/analysis
	C_VAR += -DSUPPORT_SPECTRUM_ANALYSIS
endif

ifneq ($(findstring y,$(SUPPORT_SPECTRUM_SCAN_SEGMENT)),)
	C_VAR += -DSUPPORT_SPECTRUM_SCAN_SEGMENT
endif



#
ifneq ($(findstring y,$(SUPPORT_RESIDENCY_STRATEGY)),)
	C_VAR += -DSUPPORT_RESIDENCY_STRATEGY
endif

ifneq ($(findstring y,$(SUPPORT_CALIBRATION_GAIN)),)
	C_VAR += -DSUPPORT_CALIBRATION_GAIN
endif

SOURCE_DIR += bsp log net protocol/oal dao/oal conf executor  utils device  device/gpio  

#control enable
ifneq ($(findstring y,$(SUPPORT_REMOTE_LOCAL_CTRL_EN)),)
	C_VAR += -DSUPPORT_REMOTE_LOCAL_CTRL_EN
endif

#project name
ifneq ($(findstring wd_xcr,$(SUPPORT_PROJECT_NAME)),)
	C_VAR += -DSUPPORT_PROJECT_WD_XCR
	SOURCE_DIR += bsp/wd_xcr
else ifneq ($(findstring 160m_v2,$(SUPPORT_PROJECT_NAME)),)
	C_VAR += -DSUPPORT_PROJECT_160M_V2
	SOURCE_DIR += bsp/160m_v2
else ifneq ($(findstring 40g,$(SUPPORT_PROJECT_NAME)),)
	C_VAR += -DSUPPORT_PROJECT_WD_XCR_40G
else ifneq ($(findstring ssa,$(SUPPORT_PROJECT_NAME)),)
	C_VAR += -DSUPPORT_PROJECT_SSA
else ifneq ($(findstring ssa_monitor,$(SUPPORT_PROJECT_NAME)),)
	C_VAR += -DSUPPORT_PROJECT_SSA_MONITOR
else ifneq ($(findstring ssa_9361,$(SUPPORT_PROJECT_NAME)),)
	C_VAR += -DSUPPORT_PROJECT_SSA_9361
else ifneq ($(findstring tf713_2ch,$(SUPPORT_PROJECT_NAME)),)
	SOURCE_DIR += bsp/tf713_2ch
	C_VAR += -DSUPPORT_PROJECT_TF713_2CH
else ifneq ($(findstring tf713_4ch,$(SUPPORT_PROJECT_NAME)),)
	SOURCE_DIR += bsp/tf713_4ch
	C_VAR += -DSUPPORT_PROJECT_TF713_4CH
else ifneq ($(findstring akt_4ch,$(SUPPORT_PROJECT_NAME)),)
	SOURCE_DIR += bsp/akt_4ch
	C_VAR += -DSUPPORT_PROJECT_AKT_4CH
else ifneq ($(findstring card10g_57,$(SUPPORT_PROJECT_NAME)),)
    SOURCE_DIR += bsp/card10g_57
    C_VAR += -DSUPPORT_PROJECT_CARD10G_57
endif

ifneq ($(findstring y,$(SUPPORT_WAV)),)
	C_VAR += -DSUPPORT_WAV
endif
#export CC=arm-linux-gnueabihf-gcc

#dao lib
SUB_LIBS :=  lib/libubox/libubox.a
ifneq ($(findstring json,$(SUPPORT_DAO)),)
	SUB_LIBS += dao/json/libjson.a
	SOURCE_DIR += dao/oal/json
	C_VAR += -DSUPPORT_DAO_JSON
else ifneq ($(findstring xml,$(SUPPORT_DAO)),)
	SUB_LIBS += dao/mxml-3.0/libmxml.a
	SOURCE_DIR += dao/oal/xml
	C_VAR += -DSUPPORT_DAO_XML
endif

#HASH
ifneq ($(findstring y,$(SUPPORT_HASH)),)
	C_VAR += -DSUPPORT_HASH
	SOURCE_DIR += lib/hash
endif


ALL_C_FILES := $(foreach n,$(SOURCE_DIR),$(n)/*.c)
SUB_LIB_DIRS := $(foreach n,$(SUB_LIBS),$(dir $(n)))

prefix=$(top_dir)/build/tmp/sysroots-components/cortexa9hf-neon
libdir_so=-L${prefix}/libiio/usr/lib -L${prefix}/libxml2/usr/lib -L${prefix}/zlib/usr/lib
includedir_so=-I${prefix}/libiio/usr/include -I${prefix}/libxml2/usr/include/libxml2/libxml -I${prefix}/zlib/usr/include

ifeq ($(CC), gcc)
libdir_so += -L./lib/libfftw3/x86/lib
includedir_so += -I./lib/libfftw3/x86/include
libdir_so += -L./lib/libiio/x86/lib
includedir_so += -I./lib/libiio/arm/include
else
	ifneq ($(findstring y,$(SUPPORT_SPECTRUM_FFT)),)
		libdir_so += -L./lib/libfftw3/arm/lib
		includedir_so += -I./lib/libfftw3/arm/include
	endif
	ifneq ($(findstring y,$(SUPPORT_LIB_IIO)),)
		libdir_so += -L./lib/libiio/arm/lib
		includedir_so += -I./lib/libiio/arm/include
	endif
endif

HASHDIR = lib/uthash/src
INCLUDE_DIR = -I. -I./ ${includedir_so} -I$(HASHDIR)

LDFLAGS = $(SUB_LIBS)  ${libdir_so}  -lpthread  -lm -lrt

#fft
ifneq ($(findstring y,$(SUPPORT_SPECTRUM_FFT)),)
	LDFLAGS += -lz -lfftw3f -lxml2
endif

ifeq ($(CC), gcc)
else
#libiio
ifneq ($(findstring y,$(SUPPORT_LIB_IIO)),)
	C_VAR += -DSUPPORT_LIB_IIO
	LDFLAGS += -liio
endif
endif



CFLAGS += -Wall -Wno-unused-function  -Wno-unused-variable -Wno-discarded-qualifiers $(INCLUDE_DIR)  $(C_VAR) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILE

MAKE := make

source = $(wildcard *.c $(ALL_C_FILES))
objs = $(source:%.c=%.o)

target = platform 
ifneq ($(findstring y,$(SUPPORT_TOOLS)),)
tools_m = tools
endif

commit_tag=$(shell git rev-list --max-count=1 HEAD) 

all: version $(target) $(tools_m) 
	
$(target): $(objs)
	for dir in $(SUB_LIB_DIRS); \
	do $(MAKE) -C $$dir all || exit 1; \
	done

	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)  
	
%.o:%c
	$(CC) $(CFLAGS) -c $< $(LDFLAGS)  

$(tools_m): libjson.a dao/json/cJSON.o
	for dir in tools; \
	do $(MAKE) -C $$dir all || exit 1; \
	done

version:
	@if ! grep "$(commit_tag)" version.h >/dev/null ; then \
		echo "update version.h $(commit_tag)"       ;    \
		sed 's/COMMIT_TAG/"$(commit_tag)"/g' version.tpl.h > version.h    ;  \
	fi
    
libjson.a:
	echo Creating $@...
	$(RM) $@
	for dir in dao/json/; \
	do $(MAKE) -C $$dir all || exit 1; \
	done
	
.PHONY: clean
clean:
	@for dir in $(SUB_LIB_DIRS); do make -C $$dir clean|| exit 1; done
	for dir in tools; do make -C $$dir clean|| exit 1; done
	$(RM) $(objs) $(target) *.bak *~
