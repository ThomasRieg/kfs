/*OUTPUT_FORMAT(binary)*/
OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
	.multiboot : {
		*(.multiboot)
	}
	.text : {
		*(.text)
	}
	.data : {
		*(.data)
	}
	.bss : {
		*(.bss)
	}
}
