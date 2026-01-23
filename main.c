#include "io.h"
#include "drivers/pci.h"
#include "drivers/pic.h"
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
#include "interrupts/interrupts.h"
#include "syscalls/syscalls.h"
#include "ext2.h"
#include "tasks/task.h"

struct multiboot2_header __attribute__((section(".multiboot"))) multiboot = {
	.magic = 0xe85250d6,
	.hdr_len = sizeof(struct multiboot2_header),
	.checksum = 397258538 - sizeof(struct multiboot2_header)};

void print_cpu_state(t_interrupt_data *regs)
{
	printk("cpu state: \n");
	printk("ds %u; es %u; fs %u; gs %u\n", regs->ds, regs->es, regs->fs, regs->gs);
	printk("edi %u; esi %u; ebp %u; orig_esp %u; ebx %u; edx %u; ecx %u; eax %u\n", regs->edi, regs->esi, regs->ebp, regs->orig_esp, regs->ebx, regs->edx, regs->ecx, regs->eax);
	printk("int_number %u; err_code %u\n", regs->int_no, regs->err_code);
	printk("eip %p; cs %u; eflags %u\n", (void *)regs->eip, regs->cs, regs->eflags);
	if ((regs->cs & 3) == 3)
		printk("useresp %u; ss %u\n", regs->useresp, regs->ss);
}

__attribute__((noreturn)) void kernel_panic(const char *message, t_interrupt_data *regs)
{
	disable_interrupts();
	vga_set_color(VGA_RED, VGA_BLACK);
	if (regs)
		print_cpu_state(regs);
	stack_dump_words(0);
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
	vga_set_color(VGA_WHITE, VGA_LIGHT_RED);
	printk("datetime is %u:%u:%u %u/%u/%u\n", hour, minute, second, day, month, century * 100 + year);
	vga_set_color(orig_foreground, orig_background);
}

// showcase function for kfs-4 bonuses, not the actual write syscall
uint32_t syscall_write(t_interrupt_data *regs)
{
	write("hello from userspace\n", 22);
	write((void *)regs->ebx, regs->ecx);
	return (0);
}

// also removes the line from the tty.cmd
char *read_line()
{
	while (1)
	{
		extern void handle_ps2(void);
		extern void handle_serial(void);
		asm volatile("hlt");
		handle_ps2();
		if (g_ttys[g_current_tty].cmd.index && g_ttys[g_current_tty].cmd.buffer[g_ttys[g_current_tty].cmd.index - 1] == '\r')
		{
			g_ttys[g_current_tty].cmd.index--;
			char *ret = ft_vtoc(&(g_ttys[g_current_tty].cmd));
			g_ttys[g_current_tty].cmd.index = 0;
			return (ret);
		}
		handle_serial();
		if (g_ttys[g_current_tty].cmd.index && g_ttys[g_current_tty].cmd.buffer[g_ttys[g_current_tty].cmd.index - 1] == '\r')
		{
			g_ttys[g_current_tty].cmd.index--;
			char *ret = ft_vtoc(&(g_ttys[g_current_tty].cmd));
			g_ttys[g_current_tty].cmd.index = 0;
			return (ret);
		}
	}
}

static void print_header(void)
{
	vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
	vga_writes("\xC9\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBB\n");
	writes("\xBA                                                                             \xBA\n"
		   "\xBA                                                         :::      ::::::::   \xBA\n"
		   "\xBA      KFS                                              :+:      :+:    :+:   \xBA\n"
		   "\xBA                                                     +:+ +:+         +:+     \xBA\n"
		   "\xBA      By: alier & thrieg                           +#+  +:+       +#+        \xBA\n"
		   "\xBA                                                 +#+#+#+#+#+   +#+           \xBA\n"
		   "\xBA      Welcome to our little kernel!                   #+#    #+#             \xBA\n"
		   "\xBA      Use command `help` to get started.             ###   ########.fr       \xBA\n"
		   "\xBA                                                                             \xBA\n");
	vga_writes("\xC8\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBC\n");
	vga_set_color(VGA_WHITE, VGA_BLACK);
}

void kernel_main(struct s_mb2_info *multi)
{
	writes("Initializing...\n");

	gdt_install_basic();
	paging_init(multi);

	writes("GDT installed.\n");

	setup_interrupts();
	writes("Interrupt Descriptor Table loaded.\n");
	init_syscalls();
	add_syscall(4, syscall_write);
	add_syscall(1, syscall_exit);
	add_syscall(2, syscall_fork);
	add_syscall(114, syscall_wait4);
	add_syscall(199, syscall_getuid);
	add_syscall(48, syscall_signal);
	add_syscall(37, syscall_kill);

	setup_pics();
	writes("PICs configured.\n");

	extern void init_active_tty(void);
	init_active_tty();

	enable_interrupts();
	writes("Hardware interrupts enabled.\n");

	mem_test_all();
	pci_init_all();

	uint32_t syscall_ret = syscall_call(4, "Hello world! KFS @ 42\n", sizeof("Hello world! KFS @ 42\n") - 1);
	printk("syscall returned %u\n", syscall_ret);
	print_clock();
	writes("\n");

	struct VecU8 init_binary = read_full_file("/bin/init");
	t_task *init_task = vcalloc(1, sizeof(t_task));
	if (!init_task || !setup_process(init_task, NULL, 0, &init_binary)) {
		printk("couldn't launch init");
	}
	VecU8_destruct(&init_binary);

	print_header();
	writes("> ");
	while (1)
	{
		extern void handle_command(unsigned char len, const char *cmd);
		char *line = read_line();
		if (!line)
			kernel_panic("readline couldn't allocate memory in main loop\n", NULL);
		handle_command(strlen(line), line);
		vfree(line);
	}
}
