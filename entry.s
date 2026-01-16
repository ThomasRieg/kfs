.global _start
.global boot_jump_high
.extern __kstack_top_phys
.extern boot_main_low
.extern kernel_main

.section .boot, "ax"
_start:
	movl $0xB8000, %edi
    movb $'A', (%edi)
    movb $0x0F, 1(%edi)
    movl $__kstack_top_phys, %esp
	movl $0xB8000, %edi
    movb $'B', (%edi)
    movb $0x0F, 1(%edi)
    pushl %ebx
    call boot_main_low

.section .boot.text, "ax"
boot_jump_high:
    movl 4(%esp), %eax        /* read mbi_va from low stack */
    movl $__kstack_top, %esp  /* switch to high stack */
    pushl %eax                /* push mbi_va onto high stack */
    call kernel_main
