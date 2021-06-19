#!/bin/sh

CFLAGS="-Wall -Wextra $(pkg-config --cflags ncursesw)"
LDFLAGS="$(pkg-config --libs ncursesw)"

gcc $CFLAGS -o omnimino omnimino.c $LDFLAGS

