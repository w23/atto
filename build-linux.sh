#!/bin/sh

CC=${CC:="cc"}
AR=${AR:="ar"}
WORKDIR=${WORKDIR:="./.workdir"}
APP=${APP:="app"}
LDFLAGS="$LDFLAGS -lX11 -lGL -latto -L."
CFLAGS="$CFLAGS -I."

mkdir -p "$WORKDIR"

$CC -c $CFLAGS -o $WORKDIR/app_linux.o src/app_linux.c
$CC -c $CFLAGS -o $WORKDIR/app_x11.o src/app_x11.c

$AR rcs libatto.a $WORKDIR/app_linux.o $WORKDIR/app_x11.o

if [ $# -ne 0 ]
then
	$CC -o $APP $CFLAGS $LDFLAGS $@
fi
