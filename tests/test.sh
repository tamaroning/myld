test_name=$1
src="./$test_name/$test_name.c"
asm="./$test_name/$test_name.s"
obj="./$test_name/$test_name.o"
exe="./$test_name/myld-a.out"
ldscr="./$test_name/$test_name.ld"

LD="../build/myld"

echo "test name : $test_name"
echo "removing files"
rm $asm $obj $exe

echo "compiling $src to assembly"
cc -S $src -o $asm -m64 -fno-asynchronous-unwind-tables -g0

echo "assembling $asm to object file"
as -c $asm -o $obj --noexecstack

echo "linking $obj"
#$LD $obj -o $exe -T $ldscr -nostdlib
$LD $obj -o $exe

chmod +x $exe

echo "running $exe"
./$exe

actual=$?
expected=0

if [ $actual -eq $expected ]; then
    echo "."
else
    echo "expected $expected, but got $actual"
fi
