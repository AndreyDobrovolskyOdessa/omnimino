#!/bin/sh

CFLAGS="-Wall -Wextra -Wno-format-truncation $(pkg-config --cflags ncursesw)"
LDFLAGS="$(pkg-config --libs ncursesw)"

SOURCES="md5hash.c omnimino.c omnifunc.c omniload.c omnilua.c omninew.c omniplay.c omnisave.c main.c"

gcc $CFLAGS -o omnimino $SOURCES $LDFLAGS

