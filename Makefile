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

CC=gcc
#arm-linux-gnueabihf-gcc

SOURCE_DIR = log net protocol/http protocol/akt protocol/xnrp protocol/oal dao/oal conf executor utils device/audio device/lcd device/rf device/uart
SUB_LIBS := dao/mxml-3.0/libmxml.a dao/json/libjson.a libubox/libubox.a

ALL_C_FILES := $(foreach n,$(SOURCE_DIR),$(n)/*.c)
SUB_LIB_DIRS := $(foreach n,$(SUB_LIBS),$(dir $(n)))


INCLUDE_DIR = -I.

LDFLAGS = $(SUB_LIBS)
CFLAGS = -Wall $(INCLUDE_DIR)

MAKE := make

source = $(wildcard *.c $(ALL_C_FILES))
objs = $(source:%.c=%.o)

target = spectrum

all: $(target)

$(target): $(objs)
	for dir in $(SUB_LIB_DIRS); \
	do $(MAKE) -C $$dir all || exit 1; \
	done
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)  -lpthread
	
%.o:%c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	@for dir in $(SUB_LIB_DIRS); do make -C $$dir clean|| exit 1; done
	$(RM) $(objs) $(target) *.bak *~
