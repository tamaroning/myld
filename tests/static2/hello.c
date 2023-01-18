typedef unsigned long long u64;
const char *HELLO = "Hello ";

int my_syscall(u64 a, ...);

void hello() {
    // write, stdout, addr, len
    my_syscall(0x1, 0x1, HELLO, 6);
}
