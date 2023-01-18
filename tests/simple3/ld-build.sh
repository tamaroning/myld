cd $(dirname $0)
LD="ld"

cc simple3.c -c -o simple3.o -m64 -fno-asynchronous-unwind-tables -g0
$LD simple3.o -o ld-a.out -T simple3.ld -nostdlib
chmod +x ld-a.out
