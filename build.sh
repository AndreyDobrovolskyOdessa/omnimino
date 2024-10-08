#!/bin/sh

CFLAGS="-Wall -Wextra -Wno-format-truncation $(pkg-config --cflags ncursesw)"
LDFLAGS="$(pkg-config --libs ncursesw)"

SOURCES="md5hash.c omnigame.c omnifunc.c omniload.c omnilua.c omninew.c omniplay/omniplay.c omnisave.c omnimino.c"

gcc $CFLAGS -o omnimino $SOURCES $LDFLAGS

test -e omnifill || ln -s omnimino omnifill

