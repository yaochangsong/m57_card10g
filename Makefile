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

#export CC=gcc
#export CC=arm-linux-gnueabihf-gcc

SOURCE_DIR = log net protocol/http protocol/akt protocol/xnrp protocol/oal dao/oal conf executor utils device device/audio device/lcd device/rf device/uart device/gpio
SUB_LIBS := dao/mxml-3.0/libmxml.a dao/json/libjson.a libubox/libubox.a

ALL_C_FILES := $(foreach n,$(SOURCE_DIR),$(n)/*.c)
SUB_LIB_DIRS := $(foreach n,$(SUB_LIBS),$(dir $(n)))

top_dir = $(abspath $(shell pwd)/../../../../../)
prefix=$(top_dir)/tmp/sysroots-components/cortexa9hf-neon
libdir_so=-L${prefix}/libiio/usr/lib -L${prefix}/libxml2/usr/lib -L${prefix}/zlib/usr/lib
includedir_so=-I${prefix}/libiio/usr/include -I${prefix}/libxml2/usr/include/libxml2/libxml -I${prefix}/zlib/usr/include


INCLUDE_DIR = -I. -I./ ${includedir_so} 

LDFLAGS = $(SUB_LIBS)  ${libdir_so} -liio -lpthread  -lm -lxml2 -lz
CFLAGS = -Wall -Wno-unused-function  -Wno-unused-variable -Wno-discarded-qualifiers $(INCLUDE_DIR)

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
