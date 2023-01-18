cd $(dirname $0)

LD="../../build/myld"

cc a.c -S -o a.s -m64 -fno-asynchronous-unwind-tables -g0
cc b.c -S -o b.s -m64 -fno-asynchronous-unwind-tables -g0

as -c a.s -o a.o --noexecstack
as -c b.s -o b.o --noexecstack

$LD a.o b.o -o myld-a.out >bulid.log

chmod +x myld-a.out 2>/dev/null
