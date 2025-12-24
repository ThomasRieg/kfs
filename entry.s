.global _start
.section .text
_start:
	mov $__kstack_top, %esp
	push %ebx
	call kernel_main
