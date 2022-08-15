include ../../.config

SOURCE = $(wildcard *.c)
TARGETS = $(patsubst %.c, %, $(SOURCE))
TOPDIR = $(abspath $(shell pwd)/../../)
LIBDIR = ../../main/dao/json/lib.a
LDLIBS += -lpthread

CFLAGS = -Wall


tool_sub_dir = 

ifeq ($(CONFIG_NRS1800),y)
	tool_sub_dir += nrs1800
endif

ifeq ($(CONFIG_SSDP),y)
	tool_sub_dir += ssdp
endif

ifeq ($(CONFIG_JQ),y)
        tool_sub_dir += jq
endif

all:$(TARGETS)  tool_sub

$(TARGETS):%:%.c
	@$(CC) $< $(CFLAGS) -o $@  -Wl,--start-group $(LIBDIR) -Wl,--end-group $(LDLIBS)

tool_sub:
	@for dir in $(tool_sub_dir);  	\
	do				\
	make -C $$dir $@;      		\
        done
install:
	install -m 0755 $(TARGETS) ../${D}/usr/bin

	@for dir in $(tool_sub_dir);	\
	do				\
		make -C $$dir $@;	\
	done
	
	
.PHONY:clean all
clean:
	@for dir in $(tool_sub_dir);	\
	do				\
	make -C $$dir clean|| exit 1;	\
	done
	@-rm -rf $(TARGETS)
