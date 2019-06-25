SOURCE  := $(wildcard *.c  $(wildcard libubox/*.c))
APP_OBJS    := $(patsubst %.c,%.o,$(SOURCE))
APP  := helloworld
CC      := gcc
CXX      := gcc
LIBS    += -lpthread
LDFLAGS += -Wl,--no-as-needed -lpthread
DEFINES :=
INCLUDE += -I./ -I/usr/local/include/
CFLAGS  += -g -Wall -O2  -pthread -libubox $(DEFINES) $(INCLUDE)
#CFLAGS  += -Wall -O2 --std=c++11 -pthread  $(DEFINES) $(INCLUDE)
CXXFLAGS += $(CFLAGS)


# Add any other object files to this list below

all: build

build: $(APP)

$(APP): $(APP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(APP_OBJS) $(LDLIBS)

clean:
	-rm -f $(APP) *.elf *.gdb *.o ; \
	rm -fr *.so ; \
        rm -fr *.o ; \
        rm -fr $(APP)
