TARGET = deskclock
OBJS = main.o \
       core/pomodoro.o \
       core/input_logic.o \
       core/app_state.o \
       ui/font.o \
       ui/graphics.o \
       ui/screens.o \
       platform/psp_audio.o \
       platform/psp_battery.o

CFLAGS = -O2 -G0 -Wall -Wextra -I.
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBS = -lpspgu -lpspaudio -lpsppower -lpsprtc -lm

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Desk Clock & Pomodoro
PSP_EBOOT_SFO = PARAM.SFO

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
