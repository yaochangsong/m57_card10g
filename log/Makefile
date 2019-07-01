AR = ar
ARFLAGS = cr

LIB = liblog.a

RM = -rm -rf

SOURCE  := $(wildcard *.c)
OBJS    := $(patsubst %.c,%.o,$(SOURCE))

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

clean:
	$(RM) $(OBJS) $(LIB) *.bak *~