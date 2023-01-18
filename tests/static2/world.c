typedef unsigned long long u64;
const char *WORLD = "World!\n";

int my_syscall(u64 a, ...);

void world() {
    // write, stdout, addr, len
    my_syscall(0x1, 0x1, WORLD, 7);
}
