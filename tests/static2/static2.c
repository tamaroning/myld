typedef unsigned long long u64;

void hello();
void world();

__attribute__((force_align_arg_pointer)) void _start() {
    hello();
    world();

    // exit(0)
    asm("mov $60, %rax");
    asm("mov $0, %rdi");
    asm("syscall");

    __builtin_unreachable();
}
