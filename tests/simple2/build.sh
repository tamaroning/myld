cd `dirname $0`
LD=$1

cc simple2.c -c -o simple2.o -m64 -fno-asynchronous-unwind-tables -g0
$LD simple2.o -T simple2.ld -nostdlib
chmod +x myld-a.out