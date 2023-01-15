cd `dirname $0`

LD="ld"

cc simple3.c -S -o simple3.s -m64 -fno-asynchronous-unwind-tables -g0

as -c simple3.s -o simple3.o --noexecstack

$LD simple3.o -o ld-a.out -T simple3.ld -nostdlib

chmod +x ld-a.out 2> /dev/null
