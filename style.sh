#!/usr/bin/sh

gcc -D__STYLE_C__ style/style.c -Ofast -s -o style/style

astyle $(find . -name "*.c") $(find . -name "*.h") -A14 -s2 -xk -xV -j -xb -W3 -k3 -U -H -xg -p -f -m0 -Y -xW -xp -c -xf -xh -z2 -n
style/style $(find . -name "*.c") $(find . -name "*.h")
