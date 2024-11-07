#!/bin/sh

CFLAGS="-O2 -Wall -Wextra\
	-Wno-format-truncation\
	-fno-asynchronous-unwind-tables\
	$(pkg-config --cflags ncursesw)"

LDFLAGS="$(pkg-config --libs ncursesw)"

SOURCES="md5hash.c omnigame.c omnifunc.c omniload.c omnilua.c omnimem.c\
	omninew.c omnidraw/omnidraw.c omnisave.c omnimino.c"

gcc $CFLAGS -o omnimino $SOURCES $LDFLAGS

test -e omnifill || ln -s omnimino omnifill

