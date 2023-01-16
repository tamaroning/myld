void f() {
    asm("mov $60, %rax");
    asm("mov $0, %rdi");
    asm("syscall");

    __builtin_unreachable();
}

// linked executable should also contain this function
void f2() {
    // do nothing
}
