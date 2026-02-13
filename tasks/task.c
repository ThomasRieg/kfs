/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   task.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 17:52:50 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/13 07:17:02 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../common.h"
#include "task.h"
#include "elf.h"
#include "../pmm/pmm.h"
#include "../mem_page/mem_paging.h"
#include "../mem_page/mem_defines.h"
#include "../libk/libk.h"
#include "../dt/dt.h"
#include "../vmalloc/vmalloc.h"
#include "../mmap/mmap.h"
#include "../interrupts/interrupts.h"

t_task *g_curr_task = 0;
t_task *g_init_task = 0;
uint32_t g_next_pid = 1;

static void copy_strings(unsigned char *dst_s, struct process_strings strings, unsigned char **dst_p)
{
	memcpy(dst_s, strings.string_data.data, strings.string_data.length);
	VecU8_destruct(&strings.string_data);
	unsigned int str = 0, i = 0;
	if (strings.string_count)
	{
		do
		{
			dst_p[str] = dst_s + i;
			while (dst_s[i])
				i++;
			i++; // i now points to next string
			str++;
		} while (str < strings.string_count);
	}
	dst_p[str] = 0;
}

unsigned int build_user_stack(uint32_t user_stack_top, struct process_strings argv, struct process_strings envp)
{
	// stack layout
	// BOTTOM (high addresses)
	//
	// - null-terminated string data pointed to by argv and envp
	// - padding for 4-byte alignment
	// - 16 "random" bytes pointed to by auxv
	// - ELF auxv NONE (to mark end of auxiliary vectors)
	// - ELF auxv RANDOM (required by glibc)
	// - envp[n - 1] = NULL
	// - envp[...]
	// - envp[0]
	// - argv[argc] = NULL
	// - argv[...]
	// - argv[0]
	// - argc
	//
	// TOP (low addresses)

	union
	{
		unsigned int n;
		unsigned char *p;
	} stck = {.n = user_stack_top};

	unsigned char **envp0 = (unsigned char **)(stck.n - 16 - argv.string_data.length - envp.string_data.length - 2 * sizeof(struct elf_auxiliary_vector) - (envp.string_count + 1) * sizeof(char *));
	stck.n -= envp.string_data.length;
	copy_strings(stck.p, envp, envp0);

	unsigned char **argv0 = (unsigned char **)((unsigned int)envp0 - (argv.string_count + 1) * sizeof(char *));
	stck.n -= argv.string_data.length;
	copy_strings(stck.p, argv, argv0);
	// TODO: properly align and calculate pointer locations correctly in copy_strings
	//stck.n &= ~(3);

	// ELF auxiliary vectors
	stck.n -= 16;
	unsigned int random_auxv_addr = stck.n; // yes, the "random" bytes are all zero
	// memory is initialized to zero by mmap = NULL auxv
	stck.n -= sizeof(struct elf_auxiliary_vector);
	stck.n -= sizeof(struct elf_auxiliary_vector);
	struct elf_auxiliary_vector *random_auxv = (struct elf_auxiliary_vector *)stck.p;
	random_auxv->type = ELF_AT_RANDOM;
	random_auxv->value = random_auxv_addr;

	stck.n = (unsigned int)argv0;

	// put argc at top of stack
	stck.n -= sizeof(unsigned int);
	*(uint32_t *)stck.p = argv.string_count;
	return stck.n;
}

void build_initial_user_frame(t_task *t, uint32_t entry, uint32_t user_stack_top)
{
	// Put the frame at the top of kernel stack and grow downward
	uint32_t ktop = (uint32_t)&t->k_stack[sizeof(t->k_stack)];
	ktop &= 0xFFFFFFF0u; // nice alignment

	ktop -= sizeof(t_interrupt_data);
	t_interrupt_data *f = (t_interrupt_data *)ktop;

	memset(f, 0, sizeof(*f));

	uint32_t udata = (GDT_SEL_UDATA | 3);
	f->gs = udata;
	f->fs = udata;
	f->es = udata;
	f->ds = udata;

	// Discarded before iret anyway
	f->int_no = 0;
	f->err_code = 0;

	// IRET frame
	f->eip = entry;
	f->cs = (GDT_SEL_UCODE | 3);
	f->eflags = 0x202u; // IF=1 + reserved bit
	f->useresp = user_stack_top;
	f->ss = udata;

	t->k_esp = (uint32_t)f;
	/*print_trace("IRET frame: eip=%p cs=%x eflags=%x useresp=%p ss=%x\n",
		   f->eip, f->cs, f->eflags, f->useresp, f->ss);
	print_trace("task kesp %X\n", t->k_esp);
	print_trace("task eip %X\n", f->eip);*/
}

// will copy the ring0 pages cr3 currently loaded on the cpu
phys_ptr copy_current_pd()
{
	phys_ptr pd = pmm_alloc_frame();
	if (!pd)
		return (0);
	// disable_interrupts();
	virt_ptr tmp = kmap(pd);
	memset(tmp, 0, PAGE_SIZE);
	uint32_t *pde = (uint32_t *)tmp;
	for (uint32_t i = ((KERNEL_VIRT_BASE >> 22) & 0x3FFu); i < 1023; i++)
	{
		uint32_t *kernel_pde = (uint32_t *)PD_VA;
		if ((kernel_pde[i] & PTE_P) && !(kernel_pde[i] & PTE_US))
			pde[i] = kernel_pde[i]; // copy only kernel pages to avoid inter-process shared memory
	}
	pde[1023] = (pd & 0xFFFFF000u) | PTE_P | PTE_RW; // recursive mapping
	kunmap();
	// enable_interrupts();
	return (pd);
}

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

// task has to be allocated by vmalloc
// only called to create init
bool setup_process(t_task *task, t_task *parent, uint32_t user_id, struct VecU8 *binary)
{
	if (binary->length < sizeof(struct elf_header))
		return false;

	struct elf_header *header = (struct elf_header *)binary->data;
	if (memcmp(header->signature, "\x7F"
								  "ELF",
			   4) != 0)
		return false;
	if (header->bits != 0x01 || header->endianness != 0x01 || header->target != 0x03 || header->header_version != 0x01 || (header->abi != 0x00 && header->abi != 0x03) || header->abi_version != 0x00 || header->type != 0x02)
		return false;
	if (header->program_hdrs_offset + header->program_header_count * header->program_header_size > binary->length)
		return false;

	task->pending_signals = 0;
	task->task_id = g_next_pid++;
	task->parent_task = parent;
	task->uid = task->euid = task->suid = user_id;
	task->gid = task->egid = user_id;
	task->pd = copy_current_pd();
	waitq_init(&task->wait_child);
	if (!task->pd)
		return (false);
	task->proc_memory.heap_current = 0; // temporary
	task->proc_memory.user_stack_bot = mmap((void *)(TASK_STACK_TOP - TASK_STACK_SIZE), TASK_STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED, -1, 0, &task->proc_memory);
	if (task->proc_memory.user_stack_bot == MAP_FAILED)
	{
		pmm_free_frame(task->pd);
		return (false);
	}
	g_curr_task = task;	 // temporary
	g_init_task = task;
	task->next = task;	 // temporary
	write_cr3(task->pd); // temporary
	task->proc_memory.user_stack_top = (virt_ptr)((uintptr_t)task->proc_memory.user_stack_bot + TASK_STACK_SIZE);
	{
		struct elf_program_header *base = (struct elf_program_header *)(binary->data + header->program_hdrs_offset);
		for (unsigned short i = 0; i < header->program_header_count; i++)
		{
			if (base[i].type == 0x01 && base[i].file_offset + base[i].file_size < binary->length)
			{
				void *virt = page_align_down((void *)base[i].virt_addr);
				unsigned int to_add = base[i].virt_addr - (unsigned int)virt;

				void *end = page_align_up((char *)virt + base[i].mem_size + to_add);
				if (mmap(virt, base[i].mem_size + to_add, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED, -1, 0, &task->proc_memory) == MAP_FAILED)
				{
					kernel_panic("couldn't map for process", 0);
				}
				memcpy((void *)base[i].virt_addr, binary->data + base[i].file_offset, base[i].file_size);
				if ((unsigned int)task->proc_memory.heap_current < (unsigned int)end)
					task->proc_memory.heap_current = end;
			}
		}
	}
	struct process_strings empty_strs = {.string_count = 0};
	unsigned int stacktop = build_user_stack((uintptr_t)(task->proc_memory.user_stack_top), empty_strs, empty_strs);
	build_initial_user_frame(task, header->entrypoint, stacktop);
	task->status = STATUS_RUNNABLE;
	task->cwd_inode_nr = 2;
	tss_set_kernel_stack((uintptr_t)&(task->k_stack[sizeof(task->k_stack)]));
	print_trace("kstack top=%p bot=%p\n",
	   &task->k_stack[sizeof(task->k_stack)],
	   &task->k_stack[0]);
	gdt_set_user_segment(&task->user_gdt_segment);
	extern void timer_handler(__attribute__((unused)) t_interrupt_data *regs);
	isr_add_handler(INT_TIMER, timer_handler); //switch to scheduler in timer handler
	iret_from_frame((t_interrupt_data *)task->k_esp);

	return (true);
}

void add_child(t_task *parent, t_task *child)
{
	t_task *children_head = parent->children;
	parent->children = child;
	child->next_sibling = children_head;
}

// called from interrupt handler
__attribute__((noreturn)) void context_switch(t_task *next)
{
	print_trace("context_switch to pid %u\n", next->task_id);
	g_curr_task = next;
	tss_set_kernel_stack((uintptr_t)&(next->k_stack[sizeof(next->k_stack)]));
	gdt_set_user_segment(&next->user_gdt_segment);
	write_cr3(next->pd);

	iret_from_frame((t_interrupt_data *)next->k_esp);
	__builtin_unreachable();
}

t_task *g_to_schedule = NULL;
static bool g_sleeping;

void schedule_next_task()
{
	//have to loop because it's not safe to exit kernel space when !g_curr_task
	while (!g_curr_task && !g_to_schedule)
	{
		g_sleeping = true; //prevent reentrency that would create a stack overflow of timer interrupt_data
		enable_interrupts();
		print_trace("scheduler: no running program, sleeping\n");
		asm volatile("hlt");
		disable_interrupts();
	}
	if (g_sleeping)
		g_sleeping = false;
	if (g_to_schedule)
	{
		t_task *to_schedule = g_to_schedule;
		g_to_schedule = NULL;
		context_switch(to_schedule);
		return;
	}
	t_task *next = g_curr_task->next;
	while (next != g_curr_task)
	{
		if (next->status == STATUS_RUNNABLE)
		{
			context_switch(next);
			return;
		}
		next = next->next;
	}
	return; // didn't find any other runnable task, don't context switch (continue current)
}

static uint32_t g_tick;

void timer_handler_before_scheduler(__attribute__((unused)) t_interrupt_data *regs)
{
	g_tick++;
	return;
}

void timer_handler(__attribute__((unused)) t_interrupt_data *regs)
{
	g_tick++;
	if (g_sleeping)
		return;
	if (g_tick >= NB_TICKS_PER_TASK)
	{
		g_tick = 0;
		schedule_next_task();
	}
}

// Returns true if the PT for pdi has no present PTEs
static bool pt_is_empty(uint32_t pdi)
{
	volatile uint32_t *pt = (volatile uint32_t *)(uintptr_t)(PT_BASE_VA + pdi * PAGE_SIZE);
	for (uint32_t i = 0; i < 1024; i++)
	{
		if (pt[i] & PTE_P)
			return false;
	}
	return true;
}

// Also frees now-empty PT frames
static void free_vma_range_pages(virt_ptr start, virt_ptr end)
{
	start = page_align_down(start);
	end = page_align_up(end);

	// Track which PDE indices we touched
	uint8_t touched_pde[1024] = {0};

	for (virt_ptr va = start; va < end; va += PAGE_SIZE)
	{
		if (va >= (virt_ptr)KERNEL_VIRT_BASE)
			break;

		uint32_t pdi = PDE_INDEX(va);

		uint32_t *pte = get_pte(va);
		if (!pte)
			continue;

		uint32_t entry = *pte;
		if (!(entry & PTE_P))
			continue;

		phys_ptr pa = (phys_ptr)(entry & 0xFFFFF000u);

		*pte = 0;
		invalidate_cache(va);

		pmm_free_frame(pa);

		touched_pde[pdi] = 1;

		// if (task && task->proc_memory.resident_pages)
		//     task->proc_memory.resident_pages--;
	}

	volatile uint32_t *pd = (volatile uint32_t *)PD_VA;

	for (uint32_t pdi = 0; pdi < ((KERNEL_VIRT_BASE >> 22) & 0x3FFu); pdi++)
	{
		if (!touched_pde[pdi])
			continue;

		uint32_t pde = pd[pdi];
		if (!(pde & PTE_P))
			continue;

		// should be ensured by loop condition already
		if (pdi == 1023)
			continue;

		if (!pt_is_empty(pdi))
			continue;

		phys_ptr pt_pa = (phys_ptr)(pde & 0xFFFFF000u);

		pd[pdi] = 0;
		// invalidate the PT window page itself
		invalidate_cache((void *)(uintptr_t)(PT_BASE_VA + pdi * PAGE_SIZE));
		pmm_free_frame(pt_pa);
	}
}

void free_backing_obj(t_vma *vma)
{
	if (vma->backing == VMA_ANON && vma->flags == MAP_SHARED)
	{
		t_shm_anon *backing = vma->backing_obj;
		backing->refcnt--;
		if (!backing->refcnt)
		{
			struct shm_page *curr = backing->pages;
			uint32_t *pte = get_pte(curr->virt_start);
			if (pte && !(*pte & PTE_P))
			{
				*pte = 0;
				invalidate_cache(curr->virt_start);
			}
			while (curr)
			{
				struct shm_page *next = curr->next;
				pmm_free_frame(curr->frame);
				curr = next;
			}
			print_trace("freed shared backing obj\n");
			vfree(backing);
		}
	}
}

//close a vma and sets next = NULL, increment refcnt if n has a backing object
t_vma *vma_clone(const t_vma *v)
{
    t_vma *n = vcalloc(1, sizeof(*n));
    if (!n) return (NULL);
    *n = *v;
    n->next = NULL;
	if (n->backing_obj)
	{
		if (n->backing == VMA_ANON && n->flags == MAP_SHARED)
		{
			t_shm_anon *backing = n->backing_obj;
			backing->refcnt++;
		}
	}
    return (n);
}

void free_vma(t_vma *curr)
{
	if (curr->backing_obj)
		free_backing_obj(curr);
	else
	 	free_vma_range_pages(curr->start, curr->end);
	vfree(curr);
}

void free_vmas(t_mm *mm)
{
	t_vma *curr = mm->vma_list;
	while (curr)
	{
		t_vma *next = curr->next;
		free_vma(curr);
		curr = next;
	}
}
// does not free the task struct itself
void cleanup_task(t_task *task)
{
	// disable_interrupts();
	free_vmas(&task->proc_memory);
	task->proc_memory.vma_list = NULL;
	for (uint32_t i = 0; i < MAX_OPEN_FILES; i++)
	{
		if (g_curr_task->open_files[i])
		{
			extern uint32_t syscall_close(t_interrupt_data *regs);
			t_interrupt_data dummy;
			dummy.ebx = i; //fd
			syscall_close(&dummy);
		}
	}
	// enable_interrupts();
	//  TODO implement (clean up memory, fd and everything
}

// call whean reaping a zombie task that already has been called in cleanup_task
void task_reap_zombie(t_task *t)
{
	// Remove from circular run list: find predecessor
	pmm_free_frame(t->pd);
	t_task *prev = t;
	while (prev->next != t)
	{
		prev = prev->next;
	}
	if (prev == t) // should never happen unless list corrupt
		kernel_panic("reaped last task in task_reap_zombie", NULL);

	if (g_curr_task == t)
		kernel_panic("task wants to reap itself, wtf", NULL);//g_curr_task = t->next;

	prev->next = t->next;

	vfree(t);
}

void unlink_task_from_runq(t_task *task)
{
	if (task->next == task)
	{
		print_debug("run queue empty, last task unlinked pid %u\n", task->task_id);
		g_curr_task = NULL;
	}
	t_task *next = task->next;
	t_task *prev = task->next;
	while (prev->next != task)
		prev = prev->next;
	prev->next = next;
	task->next = NULL; //make sure we explicitly mark this task as not in the queue
	if (task == g_curr_task)
		g_to_schedule = next;
}

void add_task_to_runq(t_task *task)
{
	if (g_curr_task)
	{
		task->next = g_curr_task->next;
		g_curr_task->next = task;
	}
	else
	{
		task->next = task;
		g_curr_task = task;
	}
}

void yield()
{
	asm volatile("int $0x81" ::: "memory");
}

void yield_to(t_task *next_exec)
{
	g_to_schedule = next_exec;
	asm volatile("int $0x81" ::: "memory");
}

void yield_handler(__attribute__((unused)) t_interrupt_data *regs)
{
	g_tick = 0;
	schedule_next_task();
}
