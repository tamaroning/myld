    .globl my_syscall
my_syscall:
    mov     %rdi, %rax
    mov     %rsi, %rdi
    mov     %rdx, %rsi
    mov     %rcx, %rdx
    syscall
    ret
