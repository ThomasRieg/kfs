/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   process_basics.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/19 23:13:08 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/26 16:31:29 by alier            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../syscalls.h"
#include "../../tasks/task.h"
#include "../../vmalloc/vmalloc.h"
#include "../../libk/libk.h"
#include "../../mmap/mmap.h"
#include "../../mem_page/mem_paging.h"
#include "../../errno.h"
#include "../../tty/tty.h"

// 199
// void
uint32_t syscall_getuid(__attribute__((unused)) t_interrupt_data *regs)
{
	return (g_curr_task->uid);
}

uint32_t syscall_getgid(__attribute__((unused))t_interrupt_data *regs)
{
	return (g_curr_task->gid);
}

uint32_t syscall_geteuid32(__attribute__((unused))t_interrupt_data *regs)
{
	return (g_curr_task->euid);
}

uint32_t syscall_getegid32(__attribute__((unused))t_interrupt_data *regs)
{
	return (g_curr_task->egid);
}

uint32_t syscall_archprctl(t_interrupt_data *regs)
{
	printk("arch prctl %x %x\n", regs->ebx, regs->ecx);
	return (-EINVAL);
}

uint32_t syscall_brk(t_interrupt_data *regs)
{
	printk("brk %x\n", regs->ebx);
	unsigned int old_brk = (unsigned int)g_curr_task->proc_memory.heap_current;
	if (regs->ebx == 0)
		return old_brk;
	unsigned int new_brk = (unsigned int)page_align_up((void *)regs->ebx);
	if (new_brk < old_brk)
		return -EINVAL; // TODO: support deallocation
	unsigned int size = (unsigned int)page_align_up((void *)(new_brk - old_brk));
	if (mmap((void *)old_brk, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED, -1, 0, &g_curr_task->proc_memory) == MAP_FAILED) {
		printk("couldn't allocate brk");
		return -ENOMEM;
	}
	g_curr_task->proc_memory.heap_current = (void *)new_brk;
	return new_brk;
}

struct utsname {
	char sysname[65];
	char nodename[65];
	char release[65];
	char version[65];
	char machine[65];
	char __domainname[65];
};

uint32_t syscall_uname(t_interrupt_data *regs)
{
	// We be doppelgänging as Linux to get glibc to work :D
	struct utsname *buf = (void *)regs->ebx;
	memcpy(buf->sysname, "Linux", 6);
	memcpy(buf->nodename, "kfs", 4);
	memcpy(buf->release, "6.8.0-40-generic", 17);
	memcpy(buf->version, "", 1);
	memcpy(buf->machine, "x86_64", 7);
	memcpy(buf->__domainname, "(none)", 7);
	return 0;
}

uint32_t syscall_mmap2(t_interrupt_data *regs)
{
	return (unsigned int)mmap((void *)regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi, regs->ebp, &g_curr_task->proc_memory);
}

struct user_desc {
	unsigned int entry_number;
	unsigned int base_addr;
	unsigned int limit;
	unsigned int seg_32bit: 1;
	unsigned int contents: 2;
	unsigned int read_exec_only: 1;
	unsigned int limit_in_pages: 1;
	unsigned int seg_not_present: 1;
	unsigned int useable: 1;
};

uint32_t syscall_get_thread_area(t_interrupt_data *regs)
{
	printk("get thread area %p\n", regs->ebx);
	return (0);
}

uint32_t syscall_set_thread_area(t_interrupt_data *regs)
{
	// https://github.com/lattera/glibc/blob/895ef79e04a953cac1493863bcae29ad85657ee1/sysdeps/i386/nptl/tls.h#L217

	struct user_desc *desc = (struct user_desc *)regs->ebx;
	printk("set thread area %p\n", desc);

	desc->entry_number = 8; // entry #8 in GDT
	volatile t_dt_entry_32 *gdt = (volatile t_dt_entry_32 *)(KERNEL_VIRT_BASE+GDT_START);

	t_dt_ptr_32 gp;
	gp.limit = (unsigned short)(GDT_NB_ENTRY * sizeof(t_dt_entry_32) - 1);
	gp.base = (unsigned int)gdt;
	dt_set_entry(gdt, 8, desc->base_addr, desc->limit, ACCESS_PRESENT | ACCESS_READ | ACCESS_RING3 | ACCESS_CODE_OR_DATA, FLAG_PROTECTED_32BITS | FLAG_PAGE_GRANULARITY); // 0x40
	asm volatile(
		"lgdt %0\n"
		 : : "m"(gp));
	return (0);
}

// dummy used by all non-implemented syscalls at the moment
uint32_t syscall_set_tid_address(t_interrupt_data *regs) {
	printk("stub syscall #%u at eip %p\n", regs->eax, regs->eip);
	// TODO: return task ID
	return (-ENOSYS);
}

uint32_t syscall_mprotect(__attribute__((unused))t_interrupt_data *regs) {
	// lie otherwise glibc won't proceed :)
	return (0);
}

struct iovec {
	void *iov_base;
	unsigned int iov_len;
};

uint32_t syscall_writev(t_interrupt_data *regs) {
	printk("writev %u %p %u\n", regs->ebx, regs->ecx, regs->edx);
	struct iovec *iovecs = (struct iovec *)regs->ecx;
	for (unsigned int i = 0; i < regs->edx; i++) {
		write(iovecs[i].iov_base, iovecs[i].iov_len);
		//printk("%u bytes at %p\n", iovecs[i].iov_len, iovecs[i].iov_base);
	}
	return (0);
}

#define AT_EMPTY_PATH 0x1000

uint32_t syscall_statx(t_interrupt_data *regs) {
	unsigned int flags = regs->edx;
	void *buffer = (void*)regs->edi;
	printk("statx at eip %p: %u %s %u %u %p\n", regs->eip, regs->ebx, regs->ecx, flags, regs->esi, buffer);
	if (*(unsigned char *)regs->ecx == 0 && !(flags & AT_EMPTY_PATH))
		return (-ENOENT);
	memset(buffer, 0, 256);
	return (0);
}

uint32_t syscall_fstatat(t_interrupt_data *regs) {
	unsigned int flags = regs->esi;
	printk("fstatat at eip %p: %u %s %p %u\n", regs->eip, regs->ebx, regs->ecx, regs->edx, flags);
	if (*(unsigned char *)regs->ecx == 0 && !(flags & AT_EMPTY_PATH))
		return (-ENOENT);
	memset((void *)regs->edx, 0, 88);
	return (0);
}

static t_task *find_zombie_child(t_task *parent, int pid)
{
	t_task *c = parent->children;
	if (pid == -1)
	{
		while (c)
		{
			if (c->status == STATUS_ZOMBIE)
				return c;
			c = c->next_sibling;
		}
		return NULL;
	}
	if (pid > 0)
	{
		while (c)
		{
			if ((int)c->task_id == pid && c->status == STATUS_ZOMBIE)
				return c;
			c = c->next_sibling;
		}
		return NULL;
	}
	return NULL; // pid==0 or pid<-1: not supported yet
}

static void unlink_child(t_task *parent, t_task *child)
{
	t_task **pp = &parent->children;
	while (*pp)
	{
		if (*pp == child)
		{
			*pp = child->next_sibling;
			child->next_sibling = NULL;
			return;
		}
		pp = &(*pp)->next_sibling;
	}
}

// 114
// pid_t pid | int *stat_addr | int options | struct rusage *ru
uint32_t syscall_wait4(t_interrupt_data *regs)
{
	int pid = (int)regs->ebx;
	uint32_t stat_uaddr = regs->ecx;
	uint32_t options = regs->edx;

	const uint32_t WNOHANG = 1;

	// Reject unsupported pid modes for now
	if (pid == 0 || pid < -1)
	{
		return (uint32_t)(-38); // -ENOSYS
	}

	for (;;)
	{
		disable_interrupts();

		if (!g_curr_task->children)
		{
			enable_interrupts();
			return (uint32_t)(-10); // -ECHILD
		}

		t_task *z = find_zombie_child(g_curr_task, pid);
		if (z)
		{
			uint32_t child_pid = z->task_id;
			uint32_t status = ((uint32_t)(z->exit_code & 0xFF) << 8);

			unlink_child(g_curr_task, z);

			enable_interrupts();

			// write status to user if requested
			if (stat_uaddr)
			{
				if (!user_range_ok((virt_ptr)stat_uaddr, sizeof(uint32_t), true))
					return (uint32_t)(-14); // -EFAULT
				*(uint32_t *)(uintptr_t)stat_uaddr = status;
			}

			task_reap_zombie(z);
			return child_pid;
		}

		// no zombie found
		if (options & WNOHANG)
		{
			enable_interrupts();
			return 0;
		}

		// Sleep until a child changes state
		g_curr_task->wait_reason = WAIT_CHILD;
		g_curr_task->status = STATUS_SLEEP;

		enable_interrupts();

		// yield; when woken, loop and try again
		schedule_next_task();
	}
}

// 48
// int sig | __sighandler_t handler
uint32_t syscall_signal(__attribute__((unused)) t_interrupt_data *regs)
{
	return (g_curr_task->uid); // todo implement
}

// 37
//  	pid_t pid | int sig
// TODO handle pid = 0 (process group) and pid = -1 (all killeable process)
uint32_t syscall_kill(__attribute__((unused)) t_interrupt_data *regs)
{
	return (g_curr_task->uid); // TODO implement (find efficient way to find the task struct of target pid)
}

// 2
// void
uint32_t syscall_fork(__attribute__((unused)) t_interrupt_data *regs)
{
	t_task *task = vmalloc(sizeof(*task));
	setup_process(task, g_curr_task, g_curr_task->uid, &((struct VecU8){}));
	// TODO copy memory pages and stack, add different ret value in child
	return (0);
}

// 1
// int error_code
__attribute__((noreturn)) uint32_t syscall_exit(t_interrupt_data *regs)
{
	disable_interrupts(); // doesn't need to reenable because parent's task will override cpu flags

	if (!g_curr_task->parent_task)
		kernel_panic("parentless process trying to exit, no process left?\n", regs);

	g_curr_task->exit_code = regs->ebx;
	g_curr_task->status = STATUS_ZOMBIE;
	g_curr_task->parent_task->pending_signals |= (1 << SIGCHLD);

	if (g_curr_task->parent_task->status == STATUS_SLEEP &&
		g_curr_task->parent_task->wait_reason == WAIT_CHILD)
	{
		g_curr_task->parent_task->status = STATUS_RUNNABLE;
		g_curr_task->parent_task->wait_reason = WAIT_NONE;
	}
	cleanup_task(g_curr_task);
	context_switch(g_curr_task->parent_task);
	__builtin_unreachable();
}
