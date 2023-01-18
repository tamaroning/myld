cd `dirname $0`
LD=$1

cc simple1.c -c -o simple1.o -m64 -fno-asynchronous-unwind-tables -g0
$LD simple1.o -T simple1.ld -nostdlib
chmod +x myld-a.out
