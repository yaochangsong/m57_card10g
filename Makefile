SOURCE  := $(wildcard *.c)
APP_OBJS    := $(patsubst %.c,%.o,$(SOURCE))
HEADERS = $(wildcard *.h)

STA_DIR = libubox
SOURCES_STA = $(wildcard $(STA_DIR)/*.c)
OBJS_STA = $(patsubst %.c, %.o, $(SOURCES_STA))

APP  := helloworld
CC      := gcc
CXX      := gcc
LIBS    += -lpthread
LDFLAGS += -Wl,--no-as-needed -lpthread
DEFINES :=
INCLUDE += -I./ -I./$(STA_DIR)
CFLAGS  += -g -Wall -O2  -pthread  $(DEFINES) $(INCLUDE)
#CFLAGS  += -Wall -O2 --std=c++11 -pthread  $(DEFINES) $(INCLUDE)
CXXFLAGS += $(CFLAGS)


# Add any other object files to this list below

all: build

build: $(APP)
@echo "sources_sta:" $(SOURCES_STA)
$(APP): $(APP_OBJS) $(OBJS_STA)
	$(CC) $(LDFLAGS) -o $@ $(APP_OBJS) $(OBJS_STA) $(LDLIBS) $(CXXFLAGS)

clean:
	-rm -f $(APP) *.elf *.gdb *.o ; \
	rm -fr *.so ; \
        rm -fr *.o ; \
        rm -fr $(APP)
