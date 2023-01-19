cd `dirname $0`
LD=$1

cc static2.c -c -o static2.o -m64 -fno-asynchronous-unwind-tables -g0
cc hello.c -c -o hello.o -m64 -fno-asynchronous-unwind-tables -g0
cc world.c -c -o world.o -m64 -fno-asynchronous-unwind-tables -g0
as -c my_syscall.s -o my_syscall.o --noexecstack
$LD static2.o my_syscall.o hello.o world.o -T static2.ld -nostdlib
