source = $(wildcard *.c)  
objs = $(source:%.c=%.o)

target = nrs1800_desc
top_dir ?= $(abspath $(shell pwd)/../../)
SUB_LIBS = $(top_dir)/dao/json/libjson.a
LDLIBS += -lpthread
INCLUDE_DIR = -I.
CFLAGS = -Wall -g  $(INCLUDE_DIR)

all:$(target)  

$(target): $(objs) 
	$(CC) $(CFLAGS) $^ -o $@ $(SUB_LIBS)  -L.$(top_dir)/dao/json/ $(LDLIBS)  
	
.PHONY:clean all  
clean:  
	-rm -rf $(objs) $(target)