SOURCE = $(wildcard *.c)  
TARGETS = $(patsubst %.c, %, $(SOURCE))  
TOPDIR = $(abspath $(shell pwd)/../../)
LIBDIR = $(TOPDIR)/$(LIBS)
LDLIBS += -lpthread

CFLAGS = -Wall -g 

tool_sub_dir = 

all:$(TARGETS)  tool_sub

$(TARGETS):%:%.c  
	@$(CC) $< $(CFLAGS) -o $@  -Wl,--start-group $(LIBDIR) -Wl,--end-group $(LDLIBS)
	
tool_sub:
	for dir in $(tool_sub_dir); \
	do $(MAKE) -C $$dir all || exit 1; \
	done
	
.PHONY:clean all  
clean:
	@for dir in $(tool_sub_dir); do make -C $$dir clean|| exit 1; done
	@-rm -rf $(TARGETS)
