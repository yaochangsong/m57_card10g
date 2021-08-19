SOURCE = $(wildcard *.c)  
TARGETS = $(patsubst %.c, %, $(SOURCE))  
top_dir ?= $(abspath $(shell pwd)/../)
SUB_LIBS = $(LIBS)
LDLIBS += -lpthread

CFLAGS = -Wall -g 

tool_sub_dir = ftp-client

all:$(TARGETS)  tool_sub

$(TARGETS):%:%.c  
	@$(CC) $< $(CFLAGS) -o $@  $(SUB_LIBS)  -L.$(top_dir)/dao/json/ $(LDLIBS)
	
tool_sub:
	for dir in $(tool_sub_dir); \
	do $(MAKE) -C $$dir all || exit 1; \
	done
	
.PHONY:clean all  
clean:
	@for dir in $(tool_sub_dir); do make -C $$dir clean|| exit 1; done
	@-rm -rf $(TARGETS)
