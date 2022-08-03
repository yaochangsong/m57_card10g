source = $(wildcard *.c)  
objs = $(source:%.c=%.o)

target = sroute

top_dir ?= $(abspath $(shell pwd)/../../)
SUB_LIBS = $(top_dir)/dao/json/lib.a

CFLAGS = -Wall -lpthread -Wno-unused-function  -Wno-unused-variable

all:$(target)  

$(target): $(objs)  
	@$(CC) $(CFLAGS) $^ -o $@  $(SUB_LIBS)  -L.$(top_dir)/dao/json/

nr_1800.o: nr_1800.c nr_1800.h nr_config.c nr_config.h
	@$(CC) -c $(CFLAGS) nr_1800.c
nr_config.o:  nr_config.c nr_config.h
	@$(CC) -c $(CFLAGS) nr_config.c

.PHONY:clean all  
clean:  
	@-rm -rf $(objs) $(target)
