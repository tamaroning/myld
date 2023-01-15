cd `dirname $0`

LD="../../build/myld"

cc simple1.c -S -o simple1.s -m64 -fno-asynchronous-unwind-tables -g0

as -c simple1.s -o simple1.o --noexecstack

$LD simple1.o -o myld-a.out > bulid.log

chmod +x myld-a.out 2> /dev/null
