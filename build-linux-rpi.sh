#!/bin/sh

WORKDIR=${WORKDIR:="./.workdir"}
APP=${APP:="app"}

: ${RPI_ROOT:?"set RPI_ROOT to the path where raspberry-tools and -firmware are cloned"}

RPI_TOOLCHAIN=${RPI_TOOLCHAIN:="gcc-linaro-arm-linux-gnueabihf-raspbian-x64"}
RPI_TOOLCHAINDIR=${RPI_TOOLCHAINDIR:="$RPI_ROOT/raspberry-tools/arm-bcm2708/$RPI_TOOLCHAIN"}
RPI_VCDIR=${RPI_VCDIR:="$RPI_ROOT/raspberry-firmware/hardfp/opt/vc"}

CC=${CC:="$RPI_TOOLCHAINDIR/bin/arm-linux-gnueabihf-gcc"}
AR=${AR:="$RPI_TOOLCHAINDIR/bin/arm-linux-gnueabihf-ar"}
CFLAGS="$CFLAGS -Wall -Werror"
CFLAGS="$CFLAGS -I. -I$RPI_VCDIR/include -I$RPI_VCDIR/include/interface/vcos/pthreads"
CFLAGS="$CFLAGS -I$RPI_VCDIR/include/interface/vmcs_host/linux -DATTO_PLATFORM_RPI"
LDFLAGS="$LDFLAGS -latto -L. -lGLESv2 -lEGL -lbcm_host -lvcos -lvchiq_arm -L$RPI_VCDIR/lib -lrt"

mkdir -p "$WORKDIR"

$CC -c $CFLAGS -o $WORKDIR/app_linux.o src/app_linux.c || exit
$CC -c $CFLAGS -Wno-error -std=gnu89 -o $WORKDIR/app_rpi.o src/app_rpi.c || exit

$AR rcs libatto.a $WORKDIR/app_linux.o $WORKDIR/app_rpi.o

if [ $# -ne 0 ]
then
	$CC -o $APP $CFLAGS $@ $LDFLAGS
fi
