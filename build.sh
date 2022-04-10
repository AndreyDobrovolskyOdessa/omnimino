#!/bin/sh

CFLAGS="-Wall -Wextra -Wno-format-truncation $(pkg-config --cflags ncursesw)"
LDFLAGS="$(pkg-config --libs ncursesw)"

gcc $CFLAGS -o omnimino md5hash.c omnimino.c $LDFLAGS

