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
	.checksum = 0
};

void kernel_main(void) {
	unsigned char *vga_text_buf = (unsigned char *)0xb8000;
	vga_text_buf[0] = '4';
	vga_text_buf[1] = VGA_WHITE;
	vga_text_buf[2] = '2';
	vga_text_buf[3] = VGA_WHITE;
}
