#define VGA_WHITE 15

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

int vga_text_location = 0;

unsigned char * const vga_text_buf = (unsigned char *)0xb8000;

void write(const char *str) {
	while (*str) {
		vga_text_buf[vga_text_location] = *str;
		vga_text_buf[vga_text_location + 1] = VGA_WHITE;
		vga_text_location += 2;
		str++;
	}
}

void kernel_main(void) {
	write("Hello world! KFS @ 42");

	while (1) {}
}
