typedef unsigned long long u64;
const char *HELLO = "Hello World!\n";

int my_syscall(u64 a, ...);

__attribute__((force_align_arg_pointer)) void _start() {
    // write, stdout, addr, len
    my_syscall(0x1, 0x1, HELLO, 13);
    
    // exit(0)
    asm("mov $60, %rax");
    asm("mov $0, %rdi");
    asm("syscall");

    __builtin_unreachable();
}
