cd $(dirname $0)
LD=$1

cc simple3.c -c -o simple3.o -m64 -fno-asynchronous-unwind-tables -g0
$LD simple3.o -T simple3.ld -nostdlib
chmod +x myld-a.out
