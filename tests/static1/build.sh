cd $(dirname $0)
LD=$1

cc a.c -c -o a.o -m64 -fno-asynchronous-unwind-tables -g0
cc b.c -c -o b.o -m64 -fno-asynchronous-unwind-tables -g0
$LD a.o b.o -T static1.ld -nostdlib
