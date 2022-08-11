include ../../.config

SOURCE = $(wildcard *.c)  
TARGETS = $(patsubst %.c, %, $(SOURCE))  
TOPDIR = $(abspath $(shell pwd)/../../)
LIBDIR = ../../main/dao/json/lib.a 
LDLIBS += -lpthread

CFLAGS = -Wall -g 


tool_sub_dir = 

ifeq ($(CONFIG_NRS1800),y)
	tool_sub_dir += nrs1800
endif

ifeq ($(CONFIG_SSDP),y)
	ssdp_exist=$(shell if [ -d ssdp ]; then echo "y"; else echo "n"; fi)
ifeq ($(ssdp_exist), y)
	tool_sub_dir += ssdp
endif
endif

ifeq ($(CONFIG_JQ),y)
        ssdp_exist=$(shell if [ -d jq ]; then echo "y"; else echo "n"; fi)
ifeq ($(ssdp_exist), y)
        tool_sub_dir += jq
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
				if [ "$(CC)" != "gcc" ]; then      \
					if [ "$(CC)" = "aarch64-linux-gnu-gcc" ]; then      \
						./configure --host='aarch64-linux-gnu';	\
					elif [ "$(CC)" = "arm-linux-gnueabihf-gcc" ]; then \
						 ./configure --host='arm-linux-gnueabihf';\
					fi;	\
				else	\
				./configure; \
				fi;	\
			fi;	\
			make;\
			cd -; \
		elif [ "$$dir" = "jq" ]; then \
			echo "build jq............" ;    \
			cd jq/jq-1.6/build; make clean;  \
			chmod +x ../configure;	\
			if [ "$(CC)" != "gcc" ]; then      \
				echo "configure CC =$(CC)"; \
				if [ "$(CC)" = "aarch64-linux-gnu-gcc" ]; then      \
                        		../configure --without-oniguruma --disable-maintainer-mode CFLAGS='-std=c99' --host='aarch64-linux-gnu';   \
				elif [ "$(CC)" = "arm-linux-gnueabihf-gcc" ]; then \
					../configure --without-oniguruma --disable-maintainer-mode CFLAGS='-std=c99' --host='arm-linux-gnueabihf'; \
				fi;\
				make;\
			fi;     \
			cd -; \
		else \
			echo "build $$dir"; \
			$(MAKE) -C $$dir all || exit 1; \
		fi; \
	done

install:
	@for dir in $(tool_sub_dir);	\
	do	\
		make -C $$dir $@;	\
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
