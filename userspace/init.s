.global _start
.section .text

_start:
	mov $4, %eax
	mov $hi, %ebx
	mov $6, %ecx
	int $0x80
	jmp _start
	mov $1, %eax
	int $0x80

.section .rodata

hi:
.asciz "hello\n"
