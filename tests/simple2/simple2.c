void f() {
    asm("mov $60, %rax");
    asm("mov $0, %rdi");
    asm("syscall");

    __builtin_unreachable();
}

__attribute__((force_align_arg_pointer)) void _start() {
    f();

    // exit(42)
    asm("mov $60, %rax");
    asm("mov $42, %rdi");
    asm("syscall");

    __builtin_unreachable();
}
