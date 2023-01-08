test_name=$1
src="./$test_name/$test_name.c"
asm="./$test_name/$test_name.s"
exe="./$test_name/$test_name"
ldscr="./$test_name/$test_name.ld"

echo "test name : $test_name"
echo "removing files"
rm $asm $exe

echo "compiling $src"
gcc -S $src -o $asm -m64 -fno-asynchronous-unwind-tables -g0

echo "linking $asm"
gcc $asm -o $exe -T $ldscr -nostdlib

echo "running $exe"
./$test_name/$test_name

echo "return value is $?"
