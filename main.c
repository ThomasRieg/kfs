#include "io.h"
#include "scancode.h"
#include "common.h"
#include "tty/tty.h"
#include "vga/vga.h"
#include "libk/libk.h"
#include "gdt/gdt.h"
#include "mem_page/mem_paging.h"
#include "mem_page/mem_defines.h"
#include "vmalloc/vmalloc.h"
#include "multiboot2.h"

enum interrupt
{
	INT_BREAKPOINT = 3,
	INT_DOUBLE_FAULT = 8,
	INT_PAGE_FAULT = 14,
	INT_TIMER = PIC_OFFSET,
	INT_KEYBOARD,
	INT_SERIAL1 = PIC_OFFSET + 4,
};

#define IDT_PRESENT_AND_GATE_32_INT 0x8e

struct multiboot2_header __attribute__((section(".multiboot"))) multiboot = {
	.magic = 0xe85250d6,
	.hdr_len = sizeof(struct multiboot2_header),
	.checksum = 397258538 - sizeof(struct multiboot2_header)};

struct idt_entry_32
{
	short offset_1;
	short selector;
	unsigned char zero;
	unsigned char type_attributes;
	short offset_2;
};

__attribute__((noreturn)) void kernel_panic(const char *message)
{
	disable_interrupts();
	vga_set_color(VGA_RED, VGA_BLACK);
	writes("/!\\Kernel Panic/!\\\n");
	writes(message);
	while (1)
		asm volatile("hlt");
}

struct idt_entry_32 idt[256];

struct interrupt_stack_frame
{
	void *instruction_pointer;
	unsigned short cs_selector;
	unsigned short _padding;
	unsigned int flags;
} __attribute__((packed));

void print_interrupt_frame(struct interrupt_stack_frame *interrupt_frame)
{
	printk("interrupt frame at %p\n", interrupt_frame);
	printk("eip %p\n", interrupt_frame->instruction_pointer);
	printk("flags ");
	unsigned int f = interrupt_frame->flags;
	bool first = true;
	for (unsigned int i = 0; f && i < 31; i++, f >>= 1)
	{
		if (f & 1)
		{
			if (!first)
				printk(", ");
			switch (i)
			{
			case 0:
				printk("CARRY");
				break;
			case 1:
				printk("RESERVED_1");
				break;
			case 2:
				printk("PARITY");
				break;
			case 4:
				printk("AUX_CARRY");
				break;
			case 6:
				printk("ZERO");
				break;
			case 7:
				printk("SIGN");
				break;
			case 8:
				printk("TRAP");
				break;
			case 9:
				printk("INTERRUPT_ENABLE");
				break;
			case 10:
				printk("DIRECTION");
				break;
			case 11:
				printk("OVERFLOW");
				break;
			default:
				printk("%u", i);
			}
			first = false;
		}
	}
	printk("\n");
	printk("code segment selector: %u\n", (unsigned int)interrupt_frame->cs_selector);
}

__attribute__((interrupt)) void double_fault_handler(struct interrupt_stack_frame *interrupt_frame, __attribute__((unused)) unsigned int error_code)
{
	writes("double fault :(\n");
	print_interrupt_frame(interrupt_frame);
	while (1)
		asm volatile("hlt");
}

__attribute__((interrupt)) void page_fault_handler(struct interrupt_stack_frame *interrupt_frame, unsigned int error_code)
{
	writes("page fault :(\n");
	printk("error code: %u\n", error_code);
	unsigned int virtual_address;
	asm volatile("mov %%cr2, %0" : "=r"(virtual_address));
	printk("while %s %s page at virtual address: %p\n", error_code & 2 ? "writing" : "reading", error_code & 1 ? "present" : "non-present", virtual_address);
	print_interrupt_frame(interrupt_frame);
	while (1)
		asm volatile("hlt");
}

__attribute__((interrupt)) void breakpoint_handler(struct interrupt_stack_frame *interrupt_frame)
{
	writes("breakpoint\n");
	print_interrupt_frame(interrupt_frame);
}

void pic_eoi(unsigned char irq);
void setup_pics(void);

__attribute__((interrupt)) void timer_handler(__attribute__((unused)) struct interrupt_stack_frame *interrupt_frame)
{
	// writes(".");
	pic_eoi(INT_TIMER);
}

bool shift_held = false;
bool caps_lock = false;
bool lctrl_held = false;

__attribute__((interrupt)) void serial1_handler(__attribute__((unused)) struct interrupt_stack_frame *interrupt_frame)
{
	char c = inb(PORT_COM1);
	tty_add_input(c);
	pic_eoi(INT_SERIAL1);
}

__attribute__((interrupt)) void keyboard_handler(__attribute__((unused)) struct interrupt_stack_frame *interrupt_frame)
{
	unsigned char scancode = inb(PORT_PS2_DATA);
	if (!(scancode >> 7))
	{ // if not a release event
		char c;
		if (shift_held)
			c = scan_code_set_1_qwerty_shifted[scancode];
		else
			c = scan_code_set_1_qwerty[scancode];
		if (caps_lock)
			c = invert_caps(c);
		if (lctrl_held && c == '+')
		{
			save_tty();
			next_tty();
		}
		else if (lctrl_held && c == '-')
		{
			save_tty();
			prev_tty();
		}
		else if (c)
		{
			tty_add_input(c);
		}
		else if (scancode == SET1_QW_CAPLOCK)
		{
			caps_lock = !caps_lock;
		}
		else if (scancode == SET1_QW_SHIFT)
		{
			shift_held = true;
		}
		else if (scancode == SET1_QW_LCTRL)
		{
			lctrl_held = true;
		}
	}
	else
	{ // if release
		scancode &= 0x7F;
		if (scancode == SET1_QW_SHIFT)
		{
			shift_held = false;
		}
		else if (scancode == SET1_QW_LCTRL)
		{
			lctrl_held = false;
		}
	}
	pic_eoi(INT_KEYBOARD);
}

// selector 16 = kernel code in GDT
#define DEF_INTERRUPT(handler)                          \
	(struct idt_entry_32)                               \
	{                                                   \
		.selector = 16,                                 \
		.type_attributes = IDT_PRESENT_AND_GATE_32_INT, \
		.offset_1 = ((unsigned int)&handler) & 0xFFFF,  \
		.offset_2 = ((unsigned int)&handler) >> 16,     \
	}

static inline unsigned int bcd(unsigned char a)
{
	return ((a & 0xF0) >> 1) + ((a & 0xF0) >> 3) + (a & 0xf);
}

enum cmos_register
{
	RTC_SECOND = 0x00,
	RTC_MINUTE = 0x02,
	RTC_HOUR = 0x04,
	RTC_DAY = 0x07,
	RTC_MONTH = 0x08,
	RTC_YEAR = 0x09,
	RTC_USUAL_CENTURY = 0x32 // not the same on every machine?
};

unsigned char from_cmos(enum cmos_register reg)
{
	// does not handle binary format
	// is it a good idea to disable NMI?
	outb(PORT_CMOS_INDEX, reg);
	return bcd(inb(PORT_CMOS_DATA));
}

// print clock from CMOS
void print_clock(void)
{
	unsigned int century = from_cmos(RTC_USUAL_CENTURY);
	unsigned int year = from_cmos(RTC_YEAR);
	unsigned int month = from_cmos(RTC_MONTH);
	unsigned int day = from_cmos(RTC_DAY);
	unsigned int hour = from_cmos(RTC_HOUR);
	unsigned int minute = from_cmos(RTC_MINUTE);
	unsigned int second = from_cmos(RTC_SECOND);

	uint8_t orig_background;
	uint8_t orig_foreground;
	vga_get_color(&orig_foreground, &orig_background);
	vga_set_color(VGA_CYAN, VGA_YELLOW);
	printk("date is %u:%u:%u %u/%u/%u\n", hour, minute, second, day, month, century * 100 + year);
	vga_set_color(orig_foreground, orig_background);
}

void kernel_main(struct s_mb2_info *multi)
{
	writes("Initializing...\n");
	gdt_install_basic();
	writes("GDT installed.\n");
	idt[INT_BREAKPOINT] = DEF_INTERRUPT(breakpoint_handler);
	idt[INT_DOUBLE_FAULT] = DEF_INTERRUPT(double_fault_handler);
	idt[INT_TIMER] = DEF_INTERRUPT(timer_handler);
	idt[INT_KEYBOARD] = DEF_INTERRUPT(keyboard_handler);
	idt[INT_PAGE_FAULT] = DEF_INTERRUPT(page_fault_handler);
	idt[INT_SERIAL1] = DEF_INTERRUPT(serial1_handler);
	struct descriptor_table_pointer
	{
		unsigned short limit;
		unsigned int base;
	} __attribute__((packed)) idt_pointer = {
		.limit = sizeof(idt) - 1,
		.base = (unsigned int)&idt[0],
	};
	asm volatile("lidt %0" : : "m"(idt_pointer));
	writes("Interrupt Descriptor Table loaded.\n");
	paging_init(multi);

	setup_pics();
	writes("PICs configured.\n");
	enable_interrupts();
	mem_test_all();
	writes("Hardware interrupts enabled.\n");
	writes("Hello world! KFS @ 42\n");
	print_clock();
	writes("\n");
	writes("> ");
	while (1)
		asm volatile("hlt");
}
