source = $(wildcard *.c)  
objs = $(source:%.c=%.o)

target = sroute

top_dir ?= $(abspath $(shell pwd)/../../)
SUB_LIBS = $(top_dir)/dao/json/lib.a

CFLAGS = -Wall -g -lpthread

all:$(target)  

$(target): $(objs)  
	$(CC) $(CFLAGS) $^ -o $@ $(KBUILD_CFLAGS) $(SUB_LIBS) -L.$(top_dir)/dao/json/
	
.PHONY:clean all  
clean:  
	@-rm -rf $(objs)