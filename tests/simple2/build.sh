cd `dirname $0`

LD="../../build/myld"

cc simple2.c -S -o simple2.s -m64 -fno-asynchronous-unwind-tables -g0

as -c simple2.s -o simple2.o --noexecstack

$LD simple2.o -o myld-a.out > bulid.log

chmod +x myld-a.out 2> /dev/null

