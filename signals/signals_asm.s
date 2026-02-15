    .section .signaltrampoline,"ax",@progbits
    .globl __sigreturn_trampoline
__sigreturn_trampoline:
    movl $119, %eax
    int  $0x80
    ud2

