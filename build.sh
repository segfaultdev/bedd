#!/usr/bin/sh

CFLAGS="-Os -s"
# CFLAGS="-fsanitize=address -Og -g"

for FILE in $(find . -name "*.c"); do
  if [ $FILE -nt ${FILE}.o ]; then
    echo $FILE
    gcc -c $FILE -o ${FILE}.o -Iinclude -Wall -Wextra $CFLAGS
  fi
done

gcc $(find . -name "*.o") $CFLAGS -o ./bedd
