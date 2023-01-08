// gcc -S simple.c -m64 -fno-asynchronous-unwind-tables -g0
// gcc simple.s -o simple -T simple.ld -nostdlib

// Tell the compiler incoming stack alignment is not RSP%16==8 or ESP%16==12
__attribute__((force_align_arg_pointer))
void _start() {
    /* main body of program: call main(), etc */

    /* exit system call */
    asm("mov $60, %rax");
    asm("mov $42, %rdi");
    asm("syscall");

    __builtin_unreachable();
}
