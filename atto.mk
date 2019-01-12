.SUFFIXES:
.DEFAULT:
MAKEFLAGS += -r --no-print-directory

BUILDDIR ?= build
CC ?= cc
CFLAGS += -Wall -Wextra -Werror -I$(ATTO_BASEDIR)/include -I$(ATTO_BASEDIR)/src

ifeq ($(DEBUG), 1)
	CONFIG = dbg
	CFLAGS += -O0 -g
else
	CONFIG = rel
	CFLAGS += -O3
endif

ifeq ($(RASPBERRY), 1)
	PLATFORM = pi

	ifeq ($(CROSS), 1)
		RPI_ROOT ?= $(HOME)/opt/raspberry-pi
		RPI_TOOLS ?= $(RPI_ROOT)/tools
		RPI_TOOLCHAIN ?= gcc-linaro-arm-linux-gnueabihf-raspbian-x64
		RPI_TOOLCHAINDIR ?= $(RPI_TOOLS)/arm-bcm2708/$(RPI_TOOLCHAIN)
		RPI_FIRMWARE ?= $(RPI_ROOT)/firmware
		RPI_VCDIR ?= $(RPI_FIRMWARE)/hardfp/opt/vc
		CC = $(RPI_TOOLCHAINDIR)/bin/arm-linux-gnueabihf-gcc
		COMPILER = gcc
	else
		RPI_VCDIR ?= /opt/vc
		CC ?= cc
	endif

	CFLAGS += -I$(RPI_VCDIR)/include -I$(RPI_VCDIR)/include/interface/vcos/pthreads
	CFLAGS += -I$(RPI_VCDIR)/include/interface/vmcs_host/linux -DATTO_PLATFORM_RPI
	LIBS += -lbrcmGLESv2 -lbrcmEGL -lbcm_host -lvchiq_arm -lvcos -L$(RPI_VCDIR)/lib -lrt -lm

	ATTO_SOURCES += \
		src/app_linux.c \
		src/app_rpi.c

else
	PLATFORM = linux-x11
	COMPILER ?= $(CC)
	CC ?= cc
	CFLAGS += -pedantic
	LIBS += -lX11 -lXfixes -lGL -lm -pthread
	ATTO_SOURCES += \
		src/app_linux.c \
		src/app_x11.c
endif

DEPFLAGS = -MMD -MP
COMPILE.c = $(CC) -std=gnu99 $(CFLAGS) $(DEPFLAGS) -MT $@ -MF $@.d

OBJDIR ?= $(BUILDDIR)/$(PLATFORM)-$(CONFIG)

$(OBJDIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(COMPILE.c) -c $< -o $@

# TODO how to handle ../
ATTO_OBJS = $(ATTO_SOURCES:%=$(OBJDIR)/%.o)
ATTO_DEPS = $(ATTO_OBJS:%=%.d)
-include $(ATTO_DEPS)

all:

clean:
	rm -rf build

.PHONY: all clean
