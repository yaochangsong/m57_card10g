include ../../.config

SOURCE = $(wildcard *.c)  
TARGETS = $(patsubst %.c, %, $(SOURCE))  
TOPDIR = $(abspath $(shell pwd)/../../)
LIBDIR = ../../main/dao/json/lib.a 
LDLIBS += -lpthread

CFLAGS = -Wall -g 


tool_sub_dir =
ifeq ($(CONFIG_SSDP),y)
	ssdp_exist=$(shell if [ -d ssdp ]; then echo "y"; else echo "n"; fi)
ifeq ($(ssdp_exist), y)
	tool_sub_dir += ssdp 
endif
endif

all:$(TARGETS)  tool_sub

$(TARGETS):%:%.c  
	@$(CC) $< $(CFLAGS) -o $@  -Wl,--start-group $(LIBDIR) -Wl,--end-group $(LDLIBS)
	
tool_sub:
	@for dir in $(tool_sub_dir);  \
	do \
		if [ "$$dir" = "ssdp" ]; then \
			echo "build ssdp" ;    \
			LDFLAGS=	; LIBS=;\
			cd ssdp;	\
			if [ ! -f Makefile ]; then	\
				./autogen.sh; \
				./configure;	\
			fi;	\
			make;\
			cd -; \
		elif [ "$$dir" = "jq" ]; then \
			echo "build jq............" ;    \
			cd jq/jq-1.6/build; rm -rf ./*; autoreconf -fi ..;  \
			if [ "$(CC)" != "cc" ]; then      \
				echo "configure CC =$(CC)"; \
                        	../configure --without-oniguruma --disable-maintainer-mode CFLAGS='-std=c99' --host='aarch64-linux-gnu';   \
			else \
				echo "configure CC =$(CC)";\
                        	../configure --without-oniguruma --disable-maintainer-mode CFLAGS='-std=c99'; \
		fi;     \
			make;\
			cd -; \
		else \
			echo "build $$dir"; \
			$(MAKE) -C $$dir all || exit 1; \
		fi; \
	done
	
.PHONY:clean all  
clean:
	@for dir in $(tool_sub_dir);\
 	do \
		if [ "$$dir" = "jq" ]; then \
			cd jq/jq-1.6/build; \
                        if [  -f Makefile ]; then      \
                                 cd jq/jq-1.6/build; make clean;   \
                        fi;   \
			cd -; \
                else \
			make -C $$dir clean|| exit 1;\
		fi;\
	 done
	@-rm -rf $(TARGETS)
