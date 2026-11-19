#
# Makefile
#

#ENABLE_RECORDER = 1

CIRCLEHOME = ../..

OBJS	= main.o kernel.o audio/audio_io.o effects/reverb.o

LIBS	+= $(CIRCLEHOME)/lib/sched/libsched.a \
	   $(CIRCLEHOME)/lib/sound/libsound.a \
	   $(CIRCLEHOME)/lib/usb/libusb.a \
	   $(CIRCLEHOME)/lib/input/libinput.a \
	   $(CIRCLEHOME)/lib/fs/libfs.a \
	   $(CIRCLEHOME)/lib/libcircle.a

include ../Rules.mk

-include $(DEPS)