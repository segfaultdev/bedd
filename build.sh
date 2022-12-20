#!/usr/bin/sh

gcc $(find . -name "*.c") -Iinclude -o bedd -Os -s -Wall -Wextra
# gcc $(find . -name "*.c") -Iinclude -o bedd -Og -g -fsanitize=address -Wall -Wextra
