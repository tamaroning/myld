cd `dirname $0`
LD="../../build/myld"

cc simple1.c -c -o simple1.o -m64 -fno-asynchronous-unwind-tables -g0
$LD simple1.o -o myld-a.out
chmod +x myld-a.out
