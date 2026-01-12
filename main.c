#include "io.h"
#include "pci.h"
#include "pic.h"
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
#include "interupts/interupts.h"

struct multiboot2_header __attribute__((section(".multiboot"))) multiboot = {
	.magic = 0xe85250d6,
	.hdr_len = sizeof(struct multiboot2_header),
	.checksum = 397258538 - sizeof(struct multiboot2_header)};

__attribute__((noreturn)) void kernel_panic(const char *message)
{
	disable_interrupts();
	vga_set_color(VGA_RED, VGA_BLACK);
	writes("/!\\Kernel Panic/!\\\n");
	writes(message);
	while (1)
		asm volatile("hlt");
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
	setup_interupts();
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
