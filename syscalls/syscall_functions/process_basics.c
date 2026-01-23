/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   process_basics.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/19 23:13:08 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/23 15:31:58 by alier            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../syscalls.h"
#include "../../tasks/task.h"
#include "../../vmalloc/vmalloc.h"
#include "../../libk/libk.h"
#include "../../mmap/mmap.h"
#include "../../mem_page/mem_paging.h"
#include "../../errno.h"

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

uint32_t syscall_archprctl(__attribute__((unused))t_interrupt_data *regs)
{
	printk("arch prctl %x %x\n", regs->ebx, regs->ecx);
	return (-EINVAL);
}

uint32_t syscall_brk(__attribute__((unused))t_interrupt_data *regs)
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

uint32_t syscall_get_thread_area(__attribute__((unused))t_interrupt_data *regs)
{
	printk("get thread area %p\n", regs->ebx);
	return (0);
}

uint32_t syscall_set_thread_area(__attribute__((unused))t_interrupt_data *regs)
{
	// TODO: set up selector in the GDT for process (must be choosable by user-space)
	printk("set thread area %p\n", regs->ebx);
	return (-EINVAL);
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
