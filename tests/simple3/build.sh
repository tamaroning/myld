cd $(dirname $0)
LD="../../build/myld"

cc simple3.c -c -o simple3.o -m64 -fno-asynchronous-unwind-tables -g0
$LD simple3.o -o myld-a.out
chmod +x myld-a.out
