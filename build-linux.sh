#!/bin/sh

WORKDIR=${WORKDIR:="./.workdir"}

CC=${CC:="cc"}
AR=${AR:="ar"}
APP=${APP:="app"}
LDFLAGS="$LDFLAGS -Wl,--gc-sections -lX11 -lXfixes -lGL -latto -lm -L."
CFLAGS="$CFLAGS -I. -std=c99 -O3 -fdata-sections -ffunction-sections"
#-g -O3"

mkdir -p "$WORKDIR"

$CC -c $CFLAGS -o $WORKDIR/app_linux.o src/app_linux.c
$CC -c $CFLAGS -o $WORKDIR/app_x11.o src/app_x11.c

$AR rcs libatto.a $WORKDIR/app_linux.o $WORKDIR/app_x11.o

if [ $# -ne 0 ]
then
	$CC -o $APP $CFLAGS $LDFLAGS $@
fi
