
TARGETS=bin/arm64/*

ifeq ($(CONFIG_ARCH_ARM32),y)
        TARGETS := bin/arm32/*
else ifeq ($(CONFIG_ARCH_X86),y)
        TARGETS := bin/x86/*
endif


all:

install:
	install -m 0755 $(TARGETS) ../../${D}/usr/bin


.PHONY:clean all
clean:	
