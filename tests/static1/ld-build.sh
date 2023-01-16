cd `dirname $0`

LD="ld"

cc a.c -S -o a.s -m64 -fno-asynchronous-unwind-tables -g0
cc b.c -S -o b.s -m64 -fno-asynchronous-unwind-tables -g0

as -c a.s -o a.o --noexecstack
as -c b.s -o b.o --noexecstack

$LD a.o b.o -o ld-a.out -T static1.ld -nostdlib

chmod +x ld-a.out 2> /dev/null
