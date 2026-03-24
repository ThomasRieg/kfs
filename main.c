#include "io.h"
#include "drivers/pci.h"
#include "drivers/pic.h"
#include "common.h"
#include "tty/tty.h"
#include "vga/vga.h"
#include "libk/libk.h"
#include "dt/dt.h"
#include "mem_page/mem_paging.h"
#include "mem_page/mem_defines.h"
#include "vmalloc/vmalloc.h"
#include "multiboot2.h"
#include "interrupts/interrupts.h"
#include "syscalls/syscalls.h"
#include "ext2.h"
#include "tasks/task.h"
#include "errno.h"

enum e_print_level debug_print_level = PRINT_ERROR;

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

void print_vmas(t_task *task)
{
	t_vma *curr = task->proc_memory.vma_list;
	printk("listing task's VMAs:\n");
	while (curr)
	{
		t_vma *next = curr->next;
		printk("start: %p, end: %p, flags: %u, prots: %u\n", curr->start, curr->end, curr->flags, curr->prots);
		curr = next;
	}
}

__attribute__((noreturn)) void kernel_panic(const char *message, t_interrupt_data *regs)
{
	disable_interrupts();
	vga_set_color(VGA_RED, VGA_BLACK);
	if (regs)
		print_cpu_state(regs);
	if (g_curr_task)
	{
		printk("pid %u\n", g_curr_task->task_id);
		printk("task kernel stack bot %u, top %u\n", g_curr_task->k_stack, g_curr_task->k_stack + TASK_KERNEL_SIZE);
		print_vmas(g_curr_task);
	}
	else
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

struct timespec rtc_get_time(void)
{
	unsigned int century = from_cmos(RTC_USUAL_CENTURY);
	unsigned int year = century * 100 + from_cmos(RTC_YEAR);
	unsigned int month = from_cmos(RTC_MONTH);
	unsigned int day = from_cmos(RTC_DAY);
	unsigned int hour = from_cmos(RTC_HOUR);
	unsigned int minute = from_cmos(RTC_MINUTE);
	unsigned int second = from_cmos(RTC_SECOND);
	// TODO: gregorian calendar calculator
	unsigned int seconds_since_epoch = (year - 1970) * 31536000 + ((month - 1) * 30 + day) * 86400 + hour * 3600 + minute * 60 + second;
	// leap years
	seconds_since_epoch += (year - 1970) * 86400 / 4;
	return (struct timespec){seconds_since_epoch, 0};
}

/* Identifier for system-wide realtime clock.  */
#define CLOCK_REALTIME 0
/* Monotonic system-wide clock.  */
#define CLOCK_MONOTONIC 1
/* High-resolution timer from the CPU.  */
#define CLOCK_PROCESS_CPUTIME_ID 2
/* Thread-specific CPU-time clock.  */
#define CLOCK_THREAD_CPUTIME_ID 3
/* Monotonic system-wide clock, not adjusted for frequency scaling.  */
#define CLOCK_MONOTONIC_RAW 4
/* Identifier for system-wide realtime clock, updated only on ticks.  */
#define CLOCK_REALTIME_COARSE 5
/* Monotonic system-wide clock, updated only on ticks.  */
#define CLOCK_MONOTONIC_COARSE 6
/* Monotonic system-wide clock that includes time spent in suspension.  */
#define CLOCK_BOOTTIME 7
/* Like CLOCK_REALTIME but also wakes suspended system.  */
#define CLOCK_REALTIME_ALARM 8
/* Like CLOCK_BOOTTIME but also wakes suspended system.  */
#define CLOCK_BOOTTIME_ALARM 9
/* Like CLOCK_REALTIME but in International Atomic Time.  */
#define CLOCK_TAI 11

static uint32_t syscall_clock_gettime(t_interrupt_data *regs)
{
	unsigned int clock_id = regs->ebx;

	if (clock_id == CLOCK_REALTIME || clock_id == CLOCK_MONOTONIC)
	{
		struct timespec *dst = (struct timespec *)regs->ecx;
		*dst = rtc_get_time();
		return 0;
	}
	return (-EINVAL);
}

static uint32_t syscall_clock_gettime64(t_interrupt_data *regs)
{
	unsigned int clock_id = regs->ebx;

	if (clock_id == CLOCK_REALTIME || clock_id == CLOCK_MONOTONIC)
	{
		struct timespec64 *dst = (struct timespec64 *)regs->ecx;
		struct timespec time = rtc_get_time();
		*dst = (struct timespec64){.tv_sec = time.tv_sec, .tv_nsec = time.tv_nsec};
		return 0;
	}
	return (-EINVAL);
}

static uint32_t syscall_sendto(t_interrupt_data *regs)
{
	int sockfd = regs->ebx;
	unsigned char *buf = (unsigned char *)regs->ecx;
	unsigned int size = regs->edx;
	int flags = regs->esi;
	void *dest_addr = (void *)regs->edi;
	print_trace("sendto: %d %s %u %d %p\n", sockfd, buf, size, flags, dest_addr);
	return (-ENOSYS);
}

static void print_header(void)
{
	vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
	vga_writes(&g_ttys[g_current_tty], "\xC9\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBB\n");
	writes("\xBA                                                                             \xBA\n"
		   "\xBA                                                         :::      ::::::::   \xBA\n"
		   "\xBA      KFS                                              :+:      :+:    :+:   \xBA\n"
		   "\xBA                                                     +:+ +:+         +:+     \xBA\n"
		   "\xBA      By: alier & thrieg                           +#+  +:+       +#+        \xBA\n"
		   "\xBA                                                 +#+#+#+#+#+   +#+           \xBA\n"
		   "\xBA      Welcome to our little kernel!                   #+#    #+#             \xBA\n"
		   "\xBA      Use command `help` to get started.             ###   ########.fr       \xBA\n"
		   "\xBA                                                                             \xBA\n");
	vga_writes(&g_ttys[g_current_tty], "\xC8\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBC\n");
	vga_set_color(VGA_WHITE, VGA_BLACK);
}

void kernel_main(struct s_mb2_info *multi)
{
	extern void init_active_tty(void);
	init_active_tty();

	writes("CPU vendor: ");
	unsigned int highest_val;
	char vendor_1[4], vendor_2[4], vendor_3[4];
	asm("cpuid" : "=a"(highest_val), "=b"(vendor_1), "=d"(vendor_2), "=c"(vendor_3) : "a"(0));
	write(vendor_1, 4);
	write(vendor_2, 4);
	write(vendor_3, 4);
	writes("\n");
	unsigned int model;
	unsigned int brand_clflush_apic;
	unsigned int feature_1, feature_2;
	asm("cpuid" : "=a"(model), "=b"(brand_clflush_apic), "=d"(feature_1), "=c"(feature_2) : "a"(1));
	if (feature_1 & (1 << 25))
	{
		printk("SSE supported, enabling\n");
		unsigned int cr0, cr4;
		asm("mov %%cr0, %0\n"
			"mov %%cr4, %1" : "=r"(cr0), "=r"(cr4));
		cr0 &= 0xFFFFFFFB; // disable floating-point coprocessor emulation
		cr0 |= 0x2;		   // set coprocessor monitoring
		cr4 |= 3 << 9;	   // enable FXSAVE and FXRSTOR instructions, unmasked SIMD floating-point exceptions
		asm("mov %0, %%cr0\n"
			"mov %1, %%cr4" ::"r"(cr0),
			"r"(cr4));
	}

	writes("Initializing...\n");

	gdt_install_basic();
	paging_init(multi);

	writes("GDT installed.\n");

	setup_interrupts();
	bool pmm_refcount_init();
	if (!pmm_refcount_init())
		kernel_panic("couldn't init pmm_refcount\n", NULL);
	writes("Interrupt Descriptor Table loaded.\n");
	init_syscalls();
	add_syscall(1, syscall_exit);
	add_syscall(2, syscall_fork);
	add_syscall(3, syscall_read);
	add_syscall(4, syscall_write);
	add_syscall(5, syscall_open);
	add_syscall(6, syscall_close);
	add_syscall(10, syscall_unlink);
	add_syscall(11, syscall_execve);
	add_syscall(12, syscall_chdir);
	add_syscall(15, syscall_chmod);
	add_syscall(20, syscall_getpid);
	add_syscall(37, syscall_kill);
	add_syscall(41, syscall_dup);
	add_syscall(42, syscall_pipe);
	add_syscall(45, syscall_brk);
	add_syscall(48, syscall_signal);
	add_syscall(54, syscall_ioctl);
	add_syscall(57, syscall_setpgid);
	add_syscall(63, syscall_dup2);
	add_syscall(64, syscall_getppid);
	add_syscall(85, syscall_readlink);
	add_syscall(91, syscall_munmap);
	add_syscall(102, syscall_socketcall);
	add_syscall(114, syscall_wait4);
	add_syscall(119, syscall_sigreturn);
	add_syscall(122, syscall_uname);
	add_syscall(125, syscall_mprotect);
	add_syscall(132, syscall_getpgid);
	add_syscall(145, syscall_readv);
	add_syscall(146, syscall_writev);
	add_syscall(162, syscall_nanosleep);
	add_syscall(168, syscall_poll);
	add_syscall(173, syscall_rt_sigreturn);
	add_syscall(174, syscall_rt_sigaction);
	add_syscall(175, syscall_rt_sigprocmask);
	add_syscall(183, syscall_getcwd);
	// add_syscall(191, syscall_set_tid_address); // TODO: implement
	add_syscall(192, syscall_mmap2);
	add_syscall(197, syscall_fstat64);
	add_syscall(199, syscall_getuid);
	add_syscall(200, syscall_getgid);
	add_syscall(201, syscall_geteuid32);
	add_syscall(202, syscall_getegid32);
	add_syscall(213, syscall_setuid32);
	add_syscall(214, syscall_setgid32);
	add_syscall(220, syscall_getdents64);
	add_syscall(221, syscall_fcntl64);
	add_syscall(224, syscall_gettid);
	add_syscall(238, syscall_tkill);
	add_syscall(243, syscall_set_thread_area);
	add_syscall(244, syscall_get_thread_area);
	add_syscall(252, syscall_exit);
	add_syscall(258, syscall_set_tid_address);
	add_syscall(265, syscall_clock_gettime);
	add_syscall(268, syscall_statfs64);
	add_syscall(295, syscall_openat);
	add_syscall(300, syscall_fstatat);
	// add_syscall(305, syscall_set_tid_address); // TODO: implement
	// add_syscall(311, syscall_set_tid_address); // TODO: implement
	add_syscall(331, syscall_pipe2);
	add_syscall(359, syscall_socket);
	add_syscall(360, syscall_socketpair);
	add_syscall(361, syscall_bind);
	add_syscall(362, syscall_connect);
	add_syscall(363, syscall_listen);
	add_syscall(364, syscall_accept4);
	// add_syscall(355, syscall_set_tid_address); // TODO: implement
	add_syscall(369, syscall_sendto);
	add_syscall(383, syscall_statx);
	// add_syscall(386, syscall_set_tid_address); // TODO: implement
	add_syscall(384, syscall_archprctl);
	add_syscall(403, syscall_clock_gettime64);

	setup_pics();
	writes("PICs configured.\n");

	enable_interrupts();
	writes("Hardware interrupts enabled.\n");

	mem_test_all();
	pci_init_all();
	pci_enumerate();

	print_header();

	struct VecU8 init_binary = read_full_file("/bin/init");
	t_task *init_task = vcalloc(1, sizeof(t_task));
	if (!init_task || !setup_process(init_task, NULL, 0, &init_binary))
	{
		printk("couldn't launch init\n");
	}
	VecU8_destruct(&init_binary);

	while (1)
		asm("hlt");
}
