CC=gcc
CFLAGS = -Wall

SUBDIRS := dao  protocol  libubox log  net
LIBS := dao/mxml-3.0/libmxml.a dao/json/libjson.a dao/oal/liboal.a protocol/http/libuhttpd.a libubox/libubox.a log/liblog.a net/libnet.a
LDFLAGS = $(LIBS)

RM = -rm -rf
__OBJS = main.o
__SRCS = $(subst .o,.c,$(__OBJS))


target = spectrum
MAKE = make

all: $(target)

$(__OBJS): $(__SRCS)
	$(CC) $(CFLAGS) -c $^ 

$(target): $(__OBJS)
	for dir in $(SUBDIRS); \
	do $(MAKE) -C $$dir all || exit 1; \
	done
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)  -lpthread

clean:
	@for dir in $(SUBDIRS); do make -C $$dir clean|| exit 1; done
	$(RM) $(__OBJS) $(target) *.bak *~
