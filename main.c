#include "io.h"
#include "scancode.h"
#include "common.h"
#include "tty/tty.h"
#include "vga/vga.h"
#include "libk/libk.h"

enum interrupt {
	INT_BREAKPOINT = 3,
	INT_DOUBLE_FAULT = 8,
	INT_TIMER = PIC_OFFSET,
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
	printk("interrupt frame at %p\n", interrupt_frame);
	printk("double fault at %p\n", interrupt_frame->instruction_pointer);
	printk("cs sel: %u\n", (unsigned int)(unsigned short)interrupt_frame->cs_selector);
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
	return c;
}

bool shift_held = false;
bool caps_lock = false;
bool lctrl_held = false;

__attribute__((interrupt)) void keyboard_handler(struct interrupt_stack_frame *interrupt_frame) {
	unsigned char scancode = inb(PORT_PS2_DATA);
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
			tty_add_input(c);
		} else if (scancode == SET1_QW_CAPLOCK) {
			caps_lock = !caps_lock;
		} else if (scancode == SET1_QW_SHIFT) {
			shift_held = true;
		} else if (scancode == SET1_QW_LCTRL){
			lctrl_held = true;
		} else {
			printk("key event: %u\n", scancode);
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
	unsigned int century = from_cmos(RTC_USUAL_CENTURY);
	unsigned int year = from_cmos(RTC_YEAR);
	unsigned int month = from_cmos(RTC_MONTH);
	unsigned int day = from_cmos(RTC_DAY);
	unsigned int hour = from_cmos(RTC_HOUR);
	unsigned int minute = from_cmos(RTC_MINUTE);
	unsigned int second = from_cmos(RTC_SECOND);

	printk("date is %u:%u:%u %u/%u/%u\n", hour, minute, second, day, month, century * 100 + year);
}

void kernel_main(struct multiboot_info *multi) {
	printk("multiboot size: %u", multi->total_size);
	writes(" ");
	unsigned int *flag = (unsigned int *)(multi + 1);
	while (*flag) {
		printk("type: %u", *flag);
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
