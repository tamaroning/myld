cd `dirname $0`
LD="ld"

cc simple2.c -c -o simple2.o -m64 -fno-asynchronous-unwind-tables -g0
$LD simple2.o -o ld-a.out -T simple2.ld -nostdlib
chmod +x ld-a.out
