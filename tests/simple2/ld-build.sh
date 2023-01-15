cd `dirname $0`

LD="ld"

cc simple2.c -S -o simple2.s -m64 -fno-asynchronous-unwind-tables -g0

as -c simple2.s -o simple2.o --noexecstack

$LD simple2.o -o myld-a.out -T simple2.ld -nostdlib

chmod +x myld-a.out 2> /dev/null

