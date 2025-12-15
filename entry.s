.section .bss
.align 16

stack_bottom:
.skip 16384
stack_top:

.global _start
.section .text
_start:
	mov $stack_top, %esp
	call kernel_main
