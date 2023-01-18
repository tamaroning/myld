cd `dirname $0`
LD="../../build/myld"

cc simple2.c -c -o simple2.o -m64 -fno-asynchronous-unwind-tables -g0
$LD simple2.o -o myld-a.out
chmod +x myld-a.out