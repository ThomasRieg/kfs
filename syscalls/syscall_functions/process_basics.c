/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   process_basics.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/19 23:13:08 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/29 17:18:59 by thrieg           ###   ########.fr       */
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

uint32_t syscall_getpid(__attribute__((unused)) t_interrupt_data *regs)
{
	return (g_curr_task->task_id);
}

// 199
// void
uint32_t syscall_getuid(__attribute__((unused)) t_interrupt_data *regs)
{
	return (g_curr_task->uid);
}

uint32_t syscall_getgid(__attribute__((unused)) t_interrupt_data *regs)
{
	return (g_curr_task->gid);
}

uint32_t syscall_geteuid32(__attribute__((unused)) t_interrupt_data *regs)
{
	return (g_curr_task->euid);
}

uint32_t syscall_getegid32(__attribute__((unused)) t_interrupt_data *regs)
{
	return (g_curr_task->egid);
}

uint32_t syscall_archprctl(t_interrupt_data *regs)
{
	printk("arch prctl %x %x\n", regs->ebx, regs->ecx);
	return (-EINVAL);
}

struct pollfd
{
	int fd;		   /* file descriptor */
	short events;  /* requested events */
	short revents; /* returned events */
};

uint32_t syscall_poll(t_interrupt_data *regs)
{
	struct pollfd *fds = (struct pollfd *)regs->ebx;
	int nfds = regs->ecx;
	void *tmo_p = (void *)regs->edx;
	void *sigmask = (void *)regs->esi;
	printk("poll %p %u %p %p\n", fds, nfds, tmo_p, sigmask);
	return (0);
}

uint32_t syscall_tkill(t_interrupt_data *regs)
{
	int tid = regs->ebx;
	int sig = regs->ecx;
	printk("tkill %d %x\n", tid, sig);
	return (0);
}

uint32_t syscall_rt_sigprocmask(t_interrupt_data *regs)
{
	int how = regs->ebx;
	void *set = (void *)regs->ecx;
	void *oldset = (void *)regs->edx;
	printk("rt_sigprocmask %x %x %x\n", how, set, oldset);
	return (0);
}

uint32_t syscall_rt_sigaction(t_interrupt_data *regs)
{
	int signum = regs->ebx;
	void *act = (void *)regs->ecx;
	void *oldact = (void *)regs->edx;
	printk("rt_sigaction %u %x %x\n", signum, act, oldact);
	return (0);
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
	if (mmap((void *)old_brk, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED, -1, 0, &g_curr_task->proc_memory) == MAP_FAILED)
	{
		printk("couldn't allocate brk\n");
		return -ENOMEM;
	}
	g_curr_task->proc_memory.heap_current = (void *)new_brk;
	return new_brk;
}

struct utsname
{
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

struct user_desc
{
	unsigned int entry_number;
	unsigned int base_addr;
	unsigned int limit;
	unsigned int seg_32bit : 1;
	unsigned int contents : 2;
	unsigned int read_exec_only : 1;
	unsigned int limit_in_pages : 1;
	unsigned int seg_not_present : 1;
	unsigned int useable : 1;
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
	volatile t_dt_entry_32 *gdt = (volatile t_dt_entry_32 *)(KERNEL_VIRT_BASE + GDT_START);

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
uint32_t syscall_set_tid_address(t_interrupt_data *regs)
{
	printk("stub syscall #%u at eip %p\n", regs->eax, regs->eip);
	// TODO: return task ID
	return (-ENOSYS);
}

uint32_t syscall_mprotect(__attribute__((unused)) t_interrupt_data *regs)
{
	// lie otherwise glibc won't proceed :)
	return (0);
}

struct iovec
{
	void *iov_base;
	unsigned int iov_len;
};

uint32_t syscall_writev(t_interrupt_data *regs)
{
	printk("writev %u %p %u\n", regs->ebx, regs->ecx, regs->edx);
	struct iovec *iovecs = (struct iovec *)regs->ecx;
	for (unsigned int i = 0; i < regs->edx; i++)
	{
		write(iovecs[i].iov_base, iovecs[i].iov_len);
		// printk("%u bytes at %p\n", iovecs[i].iov_len, iovecs[i].iov_base);
	}
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

static t_vma *dup_vma(t_vma *src)
{
	t_vma *ret = vmalloc(sizeof(*ret));
	if (!ret)
		return (NULL);
	memcpy(ret, src, sizeof(*src));
	ret->next = NULL;
	return (ret);
}

static void free_vmas(t_mm *mm)
{
	t_vma *curr = mm->vma_list;
	while (curr)
	{
		t_vma *next = curr->next;
		vfree(curr);
		curr = next;
	}
}

// returns 0 on success, or negative error code on error
// sets new_mm->physical_pages = 0;
static uint32_t copy_mm(t_mm *new_mm, t_mm *to_copy)
{
	if (!to_copy->vma_list)
		return (0);
	new_mm->vma_list = dup_vma(to_copy->vma_list);
	if (!new_mm->vma_list)
		return (-ENOMEM);
	t_vma *curr = to_copy->vma_list->next;
	t_vma *last_node_new = new_mm->vma_list;
	while (curr)
	{
		last_node_new->next = dup_vma(curr);
		if (!last_node_new->next)
		{
			free_vmas(new_mm);
			return (-ENOMEM);
		}
		last_node_new = last_node_new->next;
		curr = curr->next;
	}
	new_mm->reserved_pages = to_copy->reserved_pages;
	new_mm->user_stack_bot = to_copy->user_stack_bot;
	new_mm->user_stack_top = to_copy->user_stack_top;
	new_mm->heap_current = to_copy->heap_current;
	new_mm->physical_pages = 0;
	return (0);
}

/*static inline uint32_t get_vma_flags(t_vma *vma)
{
	uint32_t flags = PTE_P;
	if (!(vma->prots & PROT_NONE))
		flags |= PTE_US;
	if (vma->prots & PROT_WRITE)
		flags |= PTE_RW;
	// READ and EXEC are implied if PROT_NONE is not specified
	return (flags);
}

// assumes interrupts are disabled
// returns 0 if this va is not mapped in pd
static uint32_t get_phys_frame(virt_ptr ptr, uint32_t pd)
{
	uint32_t *pd_virt = kmap(pd);
	uint32_t pde = pd_virt[PDE_INDEX(ptr)];
	kunmap();
	if (!(pde & PTE_P))
		return (0);
	uint32_t *pde_virt = kmap(pde & 0xFFFFF000);
	uint32_t pte = pde_virt[PTE_INDEX(ptr)];
	kunmap();
	if (!(pte & PTE_P))
		return (0);
	return ((pte & 0xFFFFF000) + (((uintptr_t)ptr) & 0x00000FFF));
}*/

#include "../../pmm/pmm.h"

// returns 0 on success, or negative error code on error
// assumes we have the new process' cr3 loaded, also assumes the copied process has sane and safe mm
// assumes interrupts are disabled IMPORTANT
// TODO implement COW and reference count for free later, for now this doesn't free or separate memory
uint32_t copy_current_user_memory(__attribute__((unused)) uint32_t pd_to_copy, t_mm *mm)
{
	t_vma *curr = mm->vma_list;
	while (curr)
	{
		if (curr->flags & MAP_SHARED)
		{
			if (curr->backing == VMA_ANON)
			{
				t_shm_anon *backing = (t_shm_anon *)curr->backing_obj;
				backing->refcnt++;
			}
			else
			{
				return (-ENOSYS); // file and dev backing aren't supported yet
			}
		}
		else if (curr->flags & MAP_PRIVATE)
		{
			// TODO handle the case where frame isn't allocated yet
			/*virt_ptr start = page_align_down(curr->start);
			for (virt_ptr i = start; i < curr->end; i += PAGE_SIZE)
			{
				uint32_t frame = get_phys_frame(i, pd_to_copy);
				if (!frame)
					continue;
				uint32_t *pte = get_pte(i);
				if (!pte)
				{
					// allocate PDE, PDEs are never shared anyway
					phys_ptr pde_frame = pmm_alloc_frame();
					if (!pde_frame)
					{
						return (ENOMEM); // TODO cleanup here
					}
					map_page(pde_frame, get_pde(i), PTE_US | PTE_RW | PTE_P); // set permissive flags
					uint32_t *page_table = (uint32_t *)(uintptr_t)(PT_BASE_VA + PDE_INDEX(i) * PAGE_SIZE);
					invalidate_cache(page_table);
					memset(page_table, 0, PAGE_SIZE);
					pte = get_pte(i); // pte was null, because no pde, now we need to set it before the rest of the code
					g_curr_task->proc_memory.physical_pages++;
				}

				map_page(frame, pte, get_vma_flags(curr));
				virt_ptr virtual_address_page_start = page_align_down(i);
				invalidate_cache(virtual_address_page_start);
				g_curr_task->proc_memory.physical_pages++;
			}*/
			return (-ENOSYS); // TODO handle
		}
		curr = curr->next;
	}
	return (0);
}

// 2
// void
uint32_t syscall_fork(__attribute__((unused)) t_interrupt_data *regs)
{
	t_task *task = vcalloc(1, sizeof(*task));
	if (!task)
		return (-ENOMEM);

	task->pending_signals = 0;
	task->task_id = g_next_pid++;
	task->parent_task = g_curr_task;
	task->uid = g_curr_task->uid;
	task->euid = g_curr_task->euid;
	task->suid = g_curr_task->suid;
	task->gid = g_curr_task->gid;
	task->egid = g_curr_task->egid;
	memcpy(task->open_files, g_curr_task->open_files, sizeof(task->open_files));
	for (unsigned short i = 0; i < sizeof(task->open_files)/sizeof(task->open_files[0]); i++)
		if (task->open_files[i])
			task->open_files[i]->ref_count += 1;
	disable_interrupts();
	extern phys_ptr copy_current_pd();
	task->pd = copy_current_pd(); // shallow copy of the kernel address space
	if (!task->pd)
		return (-1);
	uint32_t ret = copy_mm(&task->proc_memory, &g_curr_task->proc_memory); // copy the reserved zones allocated with mmap
	if (ret)
		return (enable_interrupts(), ret);
	uint32_t saved_pd = g_curr_task->pd;
	write_cr3(task->pd);
	ret = copy_current_user_memory(saved_pd, &task->proc_memory); // deep copy of the process's address space
	write_cr3(saved_pd);
	if (ret)
		return (enable_interrupts(), ret);
	enable_interrupts();
	task->proc_memory.heap_current = g_curr_task->proc_memory.heap_current; // temporary
	task->proc_memory.user_stack_bot = g_curr_task->proc_memory.user_stack_bot;
	task->proc_memory.user_stack_top = g_curr_task->proc_memory.user_stack_top;
	memcpy(task->k_stack, g_curr_task->k_stack, sizeof(g_curr_task->k_stack)); // TODO copy only until k_esp to save instruction?
	task->k_esp = (uint32_t)(((uintptr_t)&task->k_stack[0]) + (((uintptr_t)g_curr_task->k_esp) - ((uintptr_t)&g_curr_task->k_stack[0])));
	((t_interrupt_data *)task->k_esp)->eax = 0;
	task->status = STATUS_RUNNABLE;
	task->next = g_curr_task->next;
	g_curr_task->next = task;

	return (task->task_id);
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

#include "../../tasks/elf.h"

__attribute__((noreturn)) static inline void iret_from_frame(t_interrupt_data *frame)
{
	__asm__ volatile(
		"movl %0, %%esp \n"
		"jmp isr_common_epilogue \n"
		:
		: "r"(frame)
		: "memory");
	__builtin_unreachable();
}

// 11
// const char *filename 	const char *const *argv 	const char *const *envp
uint32_t syscall_execve(t_interrupt_data *regs)
{
	const char *filename = (const char *)regs->ebx;
	// const char *const *argv = (const char *const *)regs->ecx;
	// const char *const *envp = (const char *const *)regs->edx;
	//  if ((user_range_ok(filename, strlen(filename), false)) //TODO do this
	disable_interrupts();

	extern struct VecU8 read_full_file(const char *path);
	struct VecU8 binary = read_full_file(filename);
	if (binary.length < sizeof(struct elf_header))
		return (-ENOEXEC);

	struct elf_header *header = (struct elf_header *)binary.data;
	if (memcmp(header->signature, "\x7F"
								  "ELF",
			   4) != 0)
		return (-ENOEXEC);
	if (header->bits != 0x01 || header->endianness != 0x01 || header->target != 0x03 || header->header_version != 0x01 || (header->abi != 0x00 && header->abi != 0x03) || header->abi_version != 0x00 || header->type != 0x02)
		return (-ENOEXEC);
	if (header->program_hdrs_offset + header->program_header_count * header->program_header_size > binary.length)
		return (-ENOEXEC);

	t_task *task = vcalloc(sizeof(*task), 1);
	if (!task)
		return (-ENOMEM);
	task->pending_signals = 0;
	task->task_id = g_curr_task->task_id;
	task->parent_task = g_curr_task->parent_task;
	task->uid = g_curr_task->uid;
	task->euid = g_curr_task->euid;
	task->suid = g_curr_task->suid;
	task->gid = g_curr_task->gid;
	task->egid = g_curr_task->egid;
	extern phys_ptr copy_current_pd();
	task->pd = copy_current_pd();
	if (!task->pd)
		return (vfree(task), -ENOMEM);
	task->proc_memory.heap_current = 0; // temporary
	task->proc_memory.user_stack_bot = mmap((void *)(TASK_STACK_TOP - TASK_STACK_SIZE), TASK_STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED, -1, 0, &task->proc_memory);
	if (task->proc_memory.user_stack_bot == MAP_FAILED)
	{
		vfree(task);
		pmm_free_frame(task->pd);
		return (-ENOMEM);
	}
	task->next = g_curr_task->next;
	write_cr3(task->pd);
	task->proc_memory.user_stack_top = (virt_ptr)((uintptr_t)task->proc_memory.user_stack_bot + TASK_STACK_SIZE);
	{
		struct elf_program_header *base = (struct elf_program_header *)(binary.data + header->program_hdrs_offset);
		for (unsigned short i = 0; i < header->program_header_count; i++)
		{
			if (base[i].type == 0x01 && base[i].file_offset + base[i].file_size < binary.length)
			{
				void *virt = page_align_down((void *)base[i].virt_addr);
				unsigned int to_add = base[i].virt_addr - (unsigned int)virt;

				void *end = page_align_up((char *)virt + base[i].mem_size + to_add);
				if (mmap(virt, base[i].mem_size + to_add, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED, -1, 0, &task->proc_memory) == MAP_FAILED)
				{
					cleanup_task(task);
					vfree(task);
					pmm_free_frame(task->pd);
					return (-ENOMEM);
				}
				memcpy((void *)base[i].virt_addr, binary.data + base[i].file_offset, base[i].file_size);
				if ((unsigned int)task->proc_memory.heap_current < (unsigned int)end)
					task->proc_memory.heap_current = end;
			}
		}
	}
	extern void build_initial_user_frame(t_task * t, uint32_t entry, uint32_t user_stack_top);
	build_initial_user_frame(task, header->entrypoint, (uintptr_t)(task->proc_memory.user_stack_top)); // put argc = 0, argv[0] = NULL, envp[0] = NULL TODO handle these
	task->status = STATUS_RUNNABLE;
	task->cwd_inode_nr = 2;
	tss_set_kernel_stack((uintptr_t)&(task->k_stack[sizeof(task->k_stack)]));
	cleanup_task(g_curr_task);
	memcpy(g_curr_task, task, sizeof(*task));
	vfree(task);
	/*printk("kstack top=%p bot=%p\n",
	   &task->k_stack[sizeof(task->k_stack)],
	   &task->k_stack[0]);*/
	volatile t_dt_entry_32 *gdt = (volatile t_dt_entry_32 *)(KERNEL_VIRT_BASE + GDT_START);

	t_dt_ptr_32 gp;
	gp.limit = (unsigned short)(GDT_NB_ENTRY * sizeof(t_dt_entry_32) - 1);
	// clears entry possibly set for previous process
	// TODO: save and switch on context switch
	gp.base = (unsigned int)gdt;
	dt_set_entry(gdt, 8, 0, 0, 0, 0);
	asm volatile(
		"lgdt %0\n"
		: : "m"(gp));
	enable_interrupts();
	iret_from_frame((t_interrupt_data *)task->k_esp);
	__builtin_unreachable();
}
