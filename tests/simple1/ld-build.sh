cd `dirname $0`
LD="ld"

cc simple1.c -c -o simple1.o -m64 -fno-asynchronous-unwind-tables -g0
$LD simple1.o -o ld-a.out -T simple1.ld -nostdlib
chmod +x ld-a.out
