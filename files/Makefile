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

#export CC=arm-linux-gnueabihf-gcc
#compiler
ifneq ($(findstring x86,$(SUPPORT_PLAT_FORM_ARCH)),)
	CC=gcc
	C_VAR += -DPLAT_FORM_ARCH_X86
else ifneq ($(findstring arm,$(SUPPORT_PLAT_FORM_ARCH)),)
#	CC=arm-linux-gnueabihf-gcc
	C_VAR += -DPLAT_FORM_ARCH_ARM
endif

#protocal
ifneq ($(findstring http,$(SUPPORT_PROTOCAL)),)
	SOURCE_DIR := protocol/http
	C_VAR += -DSUPPORT_PROTOCAL_HTTP
endif
ifneq ($(findstring akt,$(SUPPORT_PROTOCAL)),)
	SOURCE_DIR += protocol/akt
	C_VAR += -DSUPPORT_PROTOCAL_AKT
else ifneq ($(findstring akt,$(SUPPORT_PROTOCAL)),)
	SOURCE_DIR += protocol/xnrp
	C_VAR += -DSUPPORT_PROTOCAL_XNRP
endif


#lcd
ifneq ($(findstring y,$(SUPPORT_LCD)),)
	SOURCE_DIR += device/lcd
	C_VAR += -DSUPPORT_LCD
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

#rf
ifneq ($(findstring y,$(SUPPORT_RF)),)
	SOURCE_DIR += device/rf
	C_VAR += -DSUPPORT_RF
endif
ifneq ($(findstring y,$(SUPPORT_RF_ADRV9009)),)
	SOURCE_DIR += device/rf/adrv9009
	C_VAR += -DSUPPORT_RF_ADRV9009
endif

SOURCE_DIR += log net protocol/oal dao/oal conf executor executor/fft utils device  device/gpio 

#control enable
ifneq ($(findstring y,$(SUPPORT_REMOTE_LOCAL_CTRL_EN)),)
	C_VAR += -DSUPPORT_REMOTE_LOCAL_CTRL_EN
endif

#project
ifneq ($(findstring y,$(SUPPORT_PROJECT_SSA)),)
	C_VAR += -DSUPPORT_PROJECT_SSA
endif
#export CC=arm-linux-gnueabihf-gcc

#dao lib
SUB_LIBS :=  libubox/libubox.a
ifneq ($(findstring json,$(SUPPORT_DAO)),)
	SUB_LIBS += dao/json/libjson.a
	C_VAR += -DSUPPORT_DAO_JSON
else ifneq ($(findstring xml,$(SUPPORT_DAO)),)
	SUB_LIBS += dao/mxml-3.0/libmxml.a
	C_VAR += -DSUPPORT_DAO_XML
endif


ALL_C_FILES := $(foreach n,$(SOURCE_DIR),$(n)/*.c)
SUB_LIB_DIRS := $(foreach n,$(SUB_LIBS),$(dir $(n)))

top_dir = $(abspath $(shell pwd)/../../../../../)
prefix=$(top_dir)/tmp/sysroots-components/cortexa9hf-neon
libdir_so=-L${prefix}/libiio/usr/lib -L${prefix}/libxml2/usr/lib -L${prefix}/zlib/usr/lib
includedir_so=-I${prefix}/libiio/usr/include -I${prefix}/libxml2/usr/include/libxml2/libxml -I${prefix}/zlib/usr/include

ifeq ($(CC), gcc)
libdir_so += -L./executor/fft/libfftw3/x86/lib
includedir_so += -I./executor/fft/libfftw3/x86/include
else
libdir_so += -L./executor/fft/libfftw3/arm/lib
includedir_so += -I./executor/fft/libfftw3/arm/include
endif

INCLUDE_DIR = -I. -I./ ${includedir_so} 

LDFLAGS = $(SUB_LIBS)  ${libdir_so}  -lpthread  -lm -lz -lfftw3f

ifeq ($(CC), gcc)
else
LDFLAGS += -liio -lxml2
endif

CFLAGS = -Wall -Wno-unused-function  -Wno-unused-variable -Wno-discarded-qualifiers $(INCLUDE_DIR)  $(C_VAR)

MAKE := make

source = $(wildcard *.c $(ALL_C_FILES))
objs = $(source:%.c=%.o)

target = platform

all: $(target)

$(target): $(objs)
	for dir in $(SUB_LIB_DIRS); \
	do $(MAKE) -C $$dir all || exit 1; \
	done

	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)  
	
%.o:%c
	$(CC) $(CFLAGS) -c $< $(LDFLAGS)  

.PHONY: clean
clean:
	@for dir in $(SUB_LIB_DIRS); do make -C $$dir clean|| exit 1; done
	$(RM) $(objs) $(target) *.bak *~
