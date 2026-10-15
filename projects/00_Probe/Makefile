TARGET = probe
OBJS = main.o

CFLAGS = -O2 -G0 -Wall
ASFLAGS = $(CFLAGS)

LIBS = -lpsppower -lpsprtc

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = 00 Slide Probe

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
