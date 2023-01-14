void g() {
    asm("mov $60, %rax");
    asm("mov $0, %rdi");
    asm("syscall");

    __builtin_unreachable();
}

void f() {
    g();

    asm("mov $60, %rax");
    asm("mov $1, %rdi");
    asm("syscall");

    __builtin_unreachable();
}

// Tell the compiler incoming stack alignment is not RSP%16==8 or ESP%16==12
__attribute__((force_align_arg_pointer)) void _start() {
    /* main body of program: call main(), etc */

    f();

    /* exit system call */
    asm("mov $60, %rax");
    asm("mov $1, %rdi");
    asm("syscall");

    __builtin_unreachable();
}
