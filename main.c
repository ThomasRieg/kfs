#define VGA_WHITE 15

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

struct multiboot_header {
	int magic;
	int arch;
	int hdr_len;
	int checksum;
	int flags;
};

struct idt_entry_32 {
	short offset_1;
	short selector;
	unsigned char zero;
	unsigned char type_attributes;
	short offset_2;
};

struct idt_entry_32 idt[256];

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

void double_fault_handler(void) {
	write("double fault");
}

void breakpoint_handler(void) {
	write("breakpoint");
}

void kernel_main(void) {
	idt[8] = (struct idt_entry_32){
		.offset_1 = ((unsigned int) &double_fault_handler) & 0xFFFF,
		.offset_2 = ((unsigned int) &double_fault_handler) >> 2,
	};
	idt[3] = (struct idt_entry_32){
		.offset_1 = ((unsigned int) &breakpoint_handler) & 0xFFFF,
		.offset_2 = ((unsigned int) &breakpoint_handler) >> 2,
	};
	asm("lidt %0" : : "m" (idt));
	write("Hello world! KFS @ 42");
	//asm("int3");
	*((unsigned char *)6)=5;

	while (1) {}
}
