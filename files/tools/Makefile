SOURCE = $(wildcard *.c)  
TARGETS = $(patsubst %.c, %, $(SOURCE))  
top_dir ?= $(abspath $(shell pwd)/../)
SUB_LIBS = $(top_dir)/dao/json/libjson.a

CFLAGS = -Wall -g 

all:$(TARGETS)  

$(TARGETS):%:%.c  
	$(CC) $< $(CFLAGS) -o $@  $(SUB_LIBS)  -L.$(top_dir)/dao/json/
	
.PHONY:clean all  
clean:  
	-rm -rf $(TARGETS)