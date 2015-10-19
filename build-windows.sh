#!/bin/sh

WORKDIR=${WORKDIR:="./.workdir"}

CC=${CC:="mingw32-gcc"}
AR=${AR:="mingw32-ar"}
APP=${APP:="app"}
LDFLAGS="$LDFLAGS -latto -lopengl32 -lgdi32 -L."
CFLAGS="--std=c89 -Wall -Werror -pedantic $CFLAGS -I."

mkdir -p "$WORKDIR"

$CC -c $CFLAGS -o $WORKDIR/app_windows.o src/app_windows.c

$AR rcs libatto.a $WORKDIR/app_windows.o

if [ $# -ne 0 ]
then
	$CC -o $APP.exe $CFLAGS $@ $LDFLAGS
fi
