CC=gcc
#arm-linux-gnueabihf-gcc

SOURCE_DIR = log net protocol/http dao/oal
SUB_LIBS := dao/mxml-3.0/libmxml.a dao/json/libjson.a libubox/libubox.a

ALL_C_FILES := $(foreach n,$(SOURCE_DIR),$(n)/*.c)
SUB_LIB_DIRS := $(foreach n,$(SUB_LIBS),$(dir $(n)))

#ALL_INCLUDE_DIR := $(foreach n,$(SOURCE_DIR),-I./$(n))
#ALL_INCLUDE_DIR += $(foreach n,$(SUB_LIBS),-I./$(dir $(n)))

INCLUDE_DIR = -I.

LDFLAGS = $(SUB_LIBS)
CFLAGS = -Wall $(INCLUDE_DIR)

MAKE := make

source = $(wildcard *.c $(ALL_C_FILES))
objs = $(source:%.c=%.o)

#VPATH = foo:fun

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
