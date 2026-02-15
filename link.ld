/*OUTPUT_FORMAT(binary)*/
ENTRY(_start)
OUTPUT_FORMAT(elf32-i386)

KERNEL_PHYS_BASE = 0x00200000;
KERNEL_VIRT_BASE = 0xC0000000;

SECTIONS
{
	/* ------------------------------------------------------------ *
     * Low boot/trampoline region (paging OFF / identity)
     * ------------------------------------------------------------ */
    . = KERNEL_PHYS_BASE;

    .boot ALIGN(4K) : AT(ADDR(.boot))
	{
		*(.multiboot)
		*(.boot)
		*(.boot.*)
		*(.text.boot)
		*(.text.boot.*)
	}
	_boot_end_phys = .;

    /* ------------------------------------------------------------ *
     * High-half kernel proper (paging ON)
     * VMA: KERNEL_VIRT_BASE + PHYS_BASE
     * LMA: PHYS_BASE
     * ------------------------------------------------------------ */

	. += KERNEL_VIRT_BASE;

	__kernel_start = .;
	__kernel_start_phys = __kernel_start - KERNEL_VIRT_BASE;
	.text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VIRT_BASE) {
		*(.text)
	}
	.rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_VIRT_BASE) {
		*(.rodata)
	}
	.data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VIRT_BASE) {
		*(.data)
	}
	.bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VIRT_BASE) {
		*(COMMON)
		*(.bss)

		. = ALIGN(4K);
		__kstack_bottom = .;
		__kstack_bottom_phys = __kstack_bottom - KERNEL_VIRT_BASE;
		. = . + 64K;
		__kstack_top = .;
		__kstack_top_phys = __kstack_top - KERNEL_VIRT_BASE;
	}
	.kmap ALIGN(4K) : AT(ADDR(.kmap) - KERNEL_VIRT_BASE) {
		*(.kmap)
	}
	.signaltrampoline ALIGN(4K) : AT(ADDR(.signaltrampoline) - KERNEL_VIRT_BASE) {
		*(.signaltrampoline)
	}
	__kernel_end = .;
	__kernel_end_phys = __kernel_end - KERNEL_VIRT_BASE;
}
