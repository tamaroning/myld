cd $(dirname $0)
LD="ld"

cc a.c -c -o a.o -m64 -fno-asynchronous-unwind-tables -g0
cc b.c -c -o b.o -m64 -fno-asynchronous-unwind-tables -g0
$LD a.o b.o -o ld-a.out -T static1.ld -nostdlib
chmod +x ld-a.out 2>/dev/null
