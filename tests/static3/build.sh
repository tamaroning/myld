cd `dirname $0`
LD="../../build/myld"

cc static3.c -c -o static3.o -m64 -fno-asynchronous-unwind-tables -g0
as -c my_syscall.s -o my_syscall.o --noexecstack
$LD static3.o my_syscall.o -o myld-a.out
chmod +x myld-a.out
