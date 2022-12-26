#!/usr/bin/sh

CFLAGS="-Iinclude -Os -s -Wall -Wextra"
LDFLAGS="-s"

FINDFLAGS=
if [ -f ./bedd ]; then FINDFLAGS="-newer ./bedd"; fi

find -name "*.c" $FINDFLAGS | xargs -I{} sh -c "echo {}; gcc {} $CFLAGS -c"
gcc $(find . -name "*.o") $LDFLAGS -o bedd
