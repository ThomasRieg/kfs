#include "io.h"
#include "scancode.h"
#include "common.h"
#include "tty/tty.h"
#include "vga/vga.h"

enum interrupt {
	INT_BREAKPOINT = 3,
	INT_DOUBLE_FAULT = 8,
	INT_TIMER = 32,
	INT_KEYBOARD
};

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
	writes("interrupt frame at ");
	uint32_str_10(buf, (unsigned int)interrupt_frame);
	writes(buf);
	writes(" double fault at ");
	uint32_str_10(buf, (unsigned int)interrupt_frame->instruction_pointer);
	writes(buf);
	writes(" cs sel: ");
	uint32_str_10(buf, (unsigned int)interrupt_frame->cs_selector);
	writes(buf);

	while (1);
}

__attribute__((interrupt)) void breakpoint_handler(struct interrupt_stack_frame *interrupt_frame) {
	writes("breakpoint");
}

void pic_eoi(unsigned char irq);
void setup_pics(void);

__attribute__((interrupt)) void timer_handler(struct interrupt_stack_frame *interrupt_frame) {
	writes(".");
	pic_eoi(INT_TIMER);
}

char toupper(char c) {
	if (c >= 'a' && c <= 'z')
		c -= ('a' - 'A');
	else if (c >= 'A' && c <= 'Z')
		c += ('a' - 'A');
	return c;
}

bool shift_held = false;
bool caps_lock = false;
bool lctrl_held = false;

__attribute__((interrupt)) void keyboard_handler(struct interrupt_stack_frame *interrupt_frame) {
	unsigned char scancode = inb(PORT_PS2_DATA);
	char buf[11];
	if (!(scancode >> 7)) { // if not a release event
		char c = scan_code_set_1_qwerty[scancode];
		if (lctrl_held && c == '+') {
			save_tty();
			next_tty();
		} else if (lctrl_held && c == '-') {
			save_tty();
			prev_tty();
		} else if (c) {
			if (shift_held || caps_lock)
				c = toupper(c);
			buf[0] = c;
			buf[1] = 0;
			writes(buf);
		} else if (scancode == SET1_QW_CAPLOCK) {
			caps_lock = !caps_lock;
		} else if (scancode == SET1_QW_SHIFT) {
			shift_held = true;
		} else if (scancode == SET1_QW_LCTRL){
			lctrl_held = true;
		} else {
			uint32_str_10(buf, scancode);
			writes("key event: ");
			writes(buf);
		}
	} else { // if release
		scancode &= 0x7F;
		if (scancode == SET1_QW_SHIFT) {
			shift_held = false;
		}
		else if (scancode == SET1_QW_LCTRL){
			lctrl_held = false;
		}
	}
	pic_eoi(INT_KEYBOARD);
}

#define DEF_INTERRUPT(handler) (struct idt_entry_32){\
		.selector = 16,\
		.type_attributes = IDT_PRESENT_AND_GATE_32_INT,\
		.offset_1 = ((unsigned int) &handler) & 0xFFFF,\
		.offset_2 = ((unsigned int) &handler) >> 16,\
	}

static inline unsigned int bcd(unsigned char a) {
	return ( (a & 0xF0) >> 1) + ( (a & 0xF0) >> 3) + (a & 0xf);
}

enum cmos_register {
	RTC_SECOND = 0x00,
	RTC_MINUTE = 0x02,
	RTC_HOUR = 0x04,
	RTC_DAY = 0x07,
	RTC_MONTH = 0x08,
	RTC_YEAR = 0x09,
	RTC_USUAL_CENTURY = 0x32 // not the same on every machine?
};

unsigned char from_cmos(enum cmos_register reg) {
	// does not handle binary format
	// is it a good idea to disable NMI?
	outb(PORT_CMOS_INDEX, reg);
	return bcd(inb(PORT_CMOS_DATA));
}

// print clock from CMOS
void print_clock(void) {
	char buf[11];

	unsigned int century = from_cmos(RTC_USUAL_CENTURY);
	unsigned int year = from_cmos(RTC_YEAR);
	unsigned int month = from_cmos(RTC_MONTH);
	unsigned int day = from_cmos(RTC_DAY);
	unsigned int hour = from_cmos(RTC_HOUR);
	unsigned int minute = from_cmos(RTC_MINUTE);
	unsigned int second = from_cmos(RTC_SECOND);

	writes("date is ");
	uint32_str_10(buf, hour);
	writes(buf);
	writes(":");
	uint32_str_10(buf, minute);
	writes(buf);
	writes(":");
	uint32_str_10(buf, second);
	writes(buf);
	writes(" ");
	uint32_str_10(buf, day);
	writes(buf);
	writes("/");
	uint32_str_10(buf, month);
	writes(buf);
	writes("/");
	uint32_str_10(buf, century * 100 + year);
	writes(buf);
	writes("\n");
}

void kernel_main(struct multiboot_info *multi) {
	char buf[11];
	writes("multiboot size: ");
	uint32_str_10(buf, multi->total_size);
	writes(buf);
	writes(" ");
	unsigned int *flag = (unsigned int *)(multi + 1);
	while (*flag) {
		writes("type: ");
		uint32_str_10(buf, *flag);
		writes(buf);
		writes(" ");
		flag++;
		flag = (unsigned int *)((unsigned char *)flag + *flag + 4);
	}
	writes("\n");
	print_clock();

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
	writes("Hello world! KFS @ 42");
	asm volatile("int3"); // breakpoint
	*((unsigned char *)3115098112)=5; // no double fault, hmm

	while (1)
		asm volatile("hlt");
}
