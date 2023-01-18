cd `dirname $0`
LD="ld"

cc static3.c -c -o static3.o -m64 -fno-asynchronous-unwind-tables -g0
as -c my_syscall.s -o my_syscall.o --noexecstack
$LD static3.o my_syscall.o -o ld-a.out
chmod +x ld-a.out
