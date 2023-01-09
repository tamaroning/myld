test_name=$1
src="./$test_name/$test_name.c"
asm="./$test_name/$test_name.s"
obj="./$test_name/$test_name.o"
exe="./$test_name/$test_name"
ldscr="./$test_name/$test_name.ld"

echo "test name : $test_name"
echo "removing files"
rm $asm $obj $exe

echo "compiling $src to assembly"
cc -S $src -o $asm -m64 -fno-asynchronous-unwind-tables -g0

echo "assembling $asm to object file"
as -c $asm -o $obj --noexecstack

echo "linking $obj"
ld $obj -o $exe -T $ldscr -nostdlib

echo "running $exe"
./$test_name/$test_name

echo "return value is $?"
