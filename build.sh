#!/usr/bin/bash

CFLAGS="-std=gnu2x -Os -s"
# CFLAGS="-Og -g -fsanitize=undefined,address"

for FILE in $(find . -name "*.c" -not -wholename "*/style/*" -not -wholename "*/bare_x86/*"); do
  if [ $FILE -nt ${FILE}.o ]; then
    echo $FILE
    gcc -c $FILE -o ${FILE}.o -Iinclude -Wall -Wextra $CFLAGS
  fi
done

gcc $(find . -name "*.o") $CFLAGS -o ./bedd
