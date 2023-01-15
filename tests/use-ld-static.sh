test_name=$1
src1="./$test_name/a.c"
asm1="./$test_name/a.s"
obj1="./$test_name/a.o"

src2="./$test_name/b.c"
asm2="./$test_name/b.s"
obj2="./$test_name/b.o"

exe="./$test_name/ld-a.out"
ldscr="./$test_name/$test_name.ld"

LD="ld"

echo "test name : $test_name"
rm $asm1 $obj1 $asm2 $obj2 $exe

cc -S $src1 -o $asm1 -m64 -fno-asynchronous-unwind-tables -g0
cc -S $src2 -o $asm2 -m64 -fno-asynchronous-unwind-tables -g0

as -c $asm1 -o $obj1 --noexecstack
as -c $asm2 -o $obj2 --noexecstack

echo "linking $obj"
$LD $obj1 $obj2 -o $exe -T $ldscr -nostdlib
#$LD $obj1 $obj2 -o $exe

chmod +x $exe

echo "running $exe"
./$exe

actual=$?
expected=0

if [ $actual -eq $expected ]; then
    echo "ok"
else
    echo "expected $expected, but got $actual"
fi
