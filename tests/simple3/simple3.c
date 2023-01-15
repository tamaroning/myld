int h() { return 1; }

void g() {
    if (h() == 1) {
        asm("mov $60, %rax");
        asm("mov $0, %rdi");
        asm("syscall");
    }
}

void f() {
    g();
}

__attribute__((force_align_arg_pointer)) void _start() {
    f();

    asm("mov $60, %rax");
    asm("mov $1, %rdi");
    asm("syscall");

    __builtin_unreachable();
}
