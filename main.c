enum vga_color {
	VGA_WHITE = 15
};

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

#define IDT_PRESENT_AND_GATE_32_INT 0x8e

struct multiboot_header {
	int magic;
	int arch;
	int hdr_len;
	int checksum;
	int flags;
};

struct multiboot_header __attribute__((section(".multiboot"))) multiboot = {
	.magic = 0xe85250d6,
	.hdr_len = 20,
	.checksum = 397258538 - 20
};

void outb(unsigned short port, unsigned char value) {
	asm("mov %0, %%dx\n"
		"mov %1, %%al\n"
		"outb %%al, %%dx\n": : "r" (port), "r" (value));
}

void update_cursor(int pos)
{
	outb(0x3D4, 0x0F);
	outb(0x3D5, (unsigned char) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (unsigned char) ((pos >> 8) & 0xFF));
}

int vga_text_location = 0;

unsigned char * const vga_text_buf = (unsigned char *)0xb8000;

void write(const char *str) {
	while (*str) {
		vga_text_buf[vga_text_location] = *str;
		vga_text_buf[vga_text_location + 1] = VGA_WHITE;
		vga_text_location += 2;
		str++;
	}
	update_cursor(vga_text_location / 2);
}

struct idt_entry_32 {
	short offset_1;
	short selector;
	unsigned char zero;
	unsigned char type_attributes;
	short offset_2;
};

struct idt_entry_32 idt[256];

__attribute__((interrupt)) void double_fault_handler(void *interrupt_frame, unsigned int error_code) {
	write("double fault");

	while (1);
}

__attribute__((interrupt)) void breakpoint_handler(void *interrupt_frame) {
	write("breakpoint");
}

void kernel_main(void) {
	idt[3] = (struct idt_entry_32){
		.selector = 16,
		.type_attributes = IDT_PRESENT_AND_GATE_32_INT,
		.offset_1 = ((unsigned int) &breakpoint_handler) & 0xFFFF,
		.offset_2 = ((unsigned int) &breakpoint_handler) >> 16,
	};
	idt[8] = (struct idt_entry_32){
		.selector = 16,
		.type_attributes = IDT_PRESENT_AND_GATE_32_INT,
		.offset_1 = ((unsigned int) &double_fault_handler) & 0xFFFF,
		.offset_2 = ((unsigned int) &double_fault_handler) >> 16,
	};
	struct descriptor_table_pointer {
		unsigned short limit;
		unsigned int base;
	} __attribute__((packed)) idt_pointer = {
		.limit = sizeof(idt) - 1,
		.base = (unsigned int)&idt[0],
	};
	asm("lidt %0" : : "m" (idt_pointer));
	asm("sti"); // enable interrupts
	write("Hello world! KFS @ 42");
	asm("int3"); // breakpoint
	*((unsigned char *)16000000000)=5; // double fault

	while (1)
		asm("hlt");
}
