#include "io.h"

enum vga_color {
	VGA_WHITE = 15
};

enum interrupt {
	INT_BREAKPOINT = 3,
	INT_DOUBLE_FAULT = 8,
	INT_TIMER = 32,
	INT_KEYBOARD
};

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_SIZE (VGA_WIDTH * VGA_HEIGHT * 2)

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
	.hdr_len = sizeof(struct multiboot_header),
	.checksum = 397258538 - sizeof(struct multiboot_header)
};

struct multiboot_info {
	unsigned int total_size;
	unsigned int reserved;
};

void update_cursor(int pos)
{
	outb(PORT_VGA_INDEX, 0x0F);
	outb(PORT_VGA_INDEXED, (unsigned char) (pos & 0xFF));
	outb(PORT_VGA_INDEX, 0x0E);
	outb(PORT_VGA_INDEXED, (unsigned char) ((pos >> 8) & 0xFF));
}

unsigned int vga_text_location = 0;

unsigned char * const vga_text_buf = (unsigned char *)0xb8000;

static void clear_last_line(void)
{
    unsigned int start = (VGA_HEIGHT - 1) * VGA_WIDTH * 2;
    for (unsigned int i = 0; i < VGA_WIDTH; i++) {
        vga_text_buf[start + i * 2] = ' ';
        vga_text_buf[start + i * 2 + 1] = VGA_WHITE;
    }
}

void scroll_down()
{
	for (unsigned int i = VGA_WIDTH * 2; i < VGA_SIZE; i++)
	{
		vga_text_buf[i - VGA_WIDTH * 2] = vga_text_buf[i];
	}
	clear_last_line();
	vga_text_location = (VGA_HEIGHT - 1) * VGA_WIDTH * 2;
}

void write(const char *str) {
	if (vga_text_location + 1 >= VGA_SIZE)
		scroll_down();
	while (*str) {
		if (*str == '\n')
			vga_text_location += (VGA_WIDTH * 2) - (vga_text_location % (VGA_WIDTH * 2));
		else
		{
			vga_text_buf[vga_text_location] = *str;
			vga_text_buf[vga_text_location + 1] = VGA_WHITE;
			vga_text_location += 2;
		}
		if (vga_text_location + 1 >= VGA_SIZE)
			scroll_down();
		str++;
	}
	update_cursor(vga_text_location / 2);
}

void int32_str_10(char out[12], int n) {
	unsigned int u = n < 0 ? -n : n;
	unsigned int i = 1;
	do {
		i++;
	} while (u /= 10);
	if (n < 0)
		i++;
	u = n < 0 ? -n : n;
	out[--i] = 0;
	do {
		out[--i] = u % 10 + '0';
		u /= 10;
	} while(u);
	if (n < 0)
		out[--i] = '-';
}

void uint32_str_10(char out[11], unsigned int n) {
	unsigned int i = 1;
	unsigned int u = n;
	do {
		i++;
	} while (u /= 10);
	out[--i] = 0;
	do {
		out[--i] = n % 10 + '0';
		n /= 10;
	} while(n);
}

static void u32_to_hex(char out[9], unsigned int x, int upper)
{
    static const char *lo = "0123456789abcdef";
    static const char *hi = "0123456789ABCDEF";
    const char *digits = upper ? hi : lo;

    for (int i = 7; i >= 0; --i) {
        out[i] = digits[x & 0xF];
        x >>= 4;
    }
    out[8] = 0;
}

struct idt_entry_32 {
	short offset_1;
	short selector;
	unsigned char zero;
	unsigned char type_attributes;
	short offset_2;
};

struct idt_entry_32 idt[256];

// This is wrong
// TODO: fix
struct interrupt_stack_frame {
	void *instruction_pointer;
	short cs_selector;
	char _reserved1[6];
	unsigned int flags;
	void *stack_pointer;
	short ss_selector;
	char _reserved2[6];
} __attribute__((packed));

__attribute__((interrupt)) void double_fault_handler(struct interrupt_stack_frame *interrupt_frame, unsigned int error_code) {
	char buf[11];
	write("interrupt frame at ");
	uint32_str_10(buf, (unsigned int)interrupt_frame);
	write(buf);
	write(" double fault at ");
	uint32_str_10(buf, (unsigned int)interrupt_frame->instruction_pointer);
	write(buf);
	write(" cs sel: ");
	uint32_str_10(buf, (unsigned int)interrupt_frame->cs_selector);
	write(buf);

	while (1);
}

__attribute__((interrupt)) void breakpoint_handler(struct interrupt_stack_frame *interrupt_frame) {
	write("breakpoint");
}

void pic_eoi(unsigned char irq);
void setup_pics(void);

__attribute__((interrupt)) void timer_handler(struct interrupt_stack_frame *interrupt_frame) {
	write(".");
	pic_eoi(INT_TIMER);
}

__attribute__((interrupt)) void keyboard_handler(struct interrupt_stack_frame *interrupt_frame) {
	unsigned char scancode = inb(PORT_PS2_DATA);
	char buf[11];
	uint32_str_10(buf, scancode);
	write("key event: ");
	write(buf);
	pic_eoi(INT_KEYBOARD);
}

#define DEF_INTERRUPT(handler) (struct idt_entry_32){\
		.selector = 16,\
		.type_attributes = IDT_PRESENT_AND_GATE_32_INT,\
		.offset_1 = ((unsigned int) &handler) & 0xFFFF,\
		.offset_2 = ((unsigned int) &handler) >> 16,\
	}

void kernel_main(struct multiboot_info *multi) {
	char buf[11];
	write("multiboot size: ");
	uint32_str_10(buf, multi->total_size);
	write(buf);
	write(" ");
	unsigned int *flag = (unsigned int *)(multi + 1);
	while (*flag) {
		write("type: ");
		uint32_str_10(buf, *flag);
		write(buf);
		write(" ");
		flag++;
		flag = (unsigned int *)((unsigned char *)flag + *flag + 4);
	}

	idt[INT_BREAKPOINT] = DEF_INTERRUPT(breakpoint_handler);
	idt[INT_DOUBLE_FAULT] = DEF_INTERRUPT(double_fault_handler);
	idt[INT_TIMER] = DEF_INTERRUPT(timer_handler);
	idt[INT_KEYBOARD] = DEF_INTERRUPT(keyboard_handler);
	struct descriptor_table_pointer {
		unsigned short limit;
		unsigned int base;
	} __attribute__((packed)) idt_pointer = {
		.limit = sizeof(idt) - 1,
		.base = (unsigned int)&idt[0],
	};
	asm volatile("lidt %0" : : "m" (idt_pointer));
	setup_pics();
	asm volatile("sti"); // enable interrupts
	write("Hello world! KFS @ 42");
	asm volatile("int3"); // breakpoint
	*((unsigned char *)3115098112)=5; // no double fault, hmm

	while (1)
		asm volatile("hlt");
}
