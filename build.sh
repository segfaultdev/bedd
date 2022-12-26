#!/usr/bin/sh

for FILE in $(find . -name "*.c"); do
  if [ $FILE -nt ${FILE}.o ]; then
    echo $FILE
    gcc -c $FILE -o ${FILE}.o -Iinclude -Os -s -Wall -Wextra
  fi
done

gcc $(find . -name "*.o") -s -o ./bedd
