#!/usr/bin/sh

CFLAGS="-s -Iinclude -Os -Wall -Wextra"

for FILE in $(find . -name "*.c"); do
  if [ $FILE -nt ./bedd ]; then
    echo $FILE
    gcc $FILE $CFLAGS -c
  fi
done

gcc $(find . -name "*.o") -s -o ./bedd
