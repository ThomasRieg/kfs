/*OUTPUT_FORMAT(binary)*/
ENTRY(_start)
OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
	. = 2M;

	__kernel_start = .;
	.text BLOCK(4K) : ALIGN(4K) {
		*(.multiboot)
		*(.text)
	}
	.rodata BLOCK(4K) : ALIGN(4K) {
		*(.rodata)
	}
	.data BLOCK(4K) : ALIGN(4K) {
		*(.data)
	}
	.bss BLOCK(4K) : ALIGN(4K) {
		*(COMMON)
		*(.bss)

		. = ALIGN(4K);
		__kstack_bottom = .;
		. = . + 16384;
		__kstack_top = .;
	}
	.kmap BLOCK(4K) : ALIGN(4K) {
		*(.kmap)
	}
	__kernel_end = .;
}
