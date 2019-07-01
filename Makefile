CC=gcc
CFLAGS = -Wall

SUBDIRS := json  http  libubox log  net
LIBS := json/libjson.a http/libuhttpd.a libubox/libubox.a log/liblog.a net/libnet.a
LDFLAGS = $(LIBS)

RM = -rm -rf
__OBJS = main.o
__SRCS = $(subst .o,.c,$(__OBJS))

INCLUDE = ./http/
target = spectrum
MAKE = make

all: $(target)

$(__OBJS): $(__SRCS)
	$(CC) $(CFLAGS) -c $^ -I ./http/ -I ./libubox/ 

$(target): $(__OBJS)
	for dir in $(SUBDIRS); \
	do $(MAKE) -C $$dir all || exit 1; \
	done
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	@for dir in $(SUBDIRS); do make -C $$dir clean|| exit 1; done
	$(RM) $(__OBJS) $(target) *.bak *~