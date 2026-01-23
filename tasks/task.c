/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   task.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 17:52:50 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/23 15:07:55 by alier            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../common.h"
#include "task.h"
#include "elf.h"
#include "../pmm/pmm.h"
#include "../mem_page/mem_paging.h"
#include "../mem_page/mem_defines.h"
#include "../libk/libk.h"
#include "../gdt/gdt.h"
#include "../vmalloc/vmalloc.h"
#include "../mmap/mmap.h"

t_task *g_curr_task = 0;
static uint32_t g_next_pid;

static void build_initial_user_frame(t_task *t, uint32_t entry, uint32_t user_stack_top)
{
	// Put the frame at the top of kernel stack and grow downward
	uint32_t ktop = (uint32_t)&t->k_stack[sizeof(t->k_stack)];
	ktop &= 0xFFFFFFF0u; // nice alignment

	ktop -= sizeof(t_interrupt_data);
	t_interrupt_data *f = (t_interrupt_data *)ktop;

	memset(f, 42, sizeof(*f));

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

	f->useresp = user_stack_top - 16; // TODO: put argc, envp etc in stack
	f->ss = udata;

	t->k_esp = (uint32_t)f;
	/*printk("IRET frame: eip=%p cs=%x eflags=%x useresp=%p ss=%x\n",
           f->eip, f->cs, f->eflags, f->useresp, f->ss);
	printk("task kesp %X\n", t->k_esp);
	printk("task eip %X\n", f->eip);*/
}

// will copy the ring0 pages cr3 currently loaded on the cpu
phys_ptr copy_current_pd()
{
	phys_ptr pd = pmm_alloc_frame();
	if (!pd)
		return (0);
	disable_interrupts();
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
	enable_interrupts();
	return (pd);
}

__attribute__((noreturn)) static inline void iret_from_frame(t_interrupt_data *frame)
{
    __asm__ volatile(
        "movl %0, %%esp \n"
        "jmp isr_common_epilogue \n"
        :
        : "r"(frame)
        : "memory"
    );
    __builtin_unreachable();
}

// task has to be allocated by vmalloc
bool setup_process(t_task *task, t_task *parent, uint32_t user_id, struct VecU8 *binary)
{
	if (binary->length < sizeof(struct elf_header)) return false;

	struct elf_header *header = (struct elf_header *)binary->data;
	if (memcmp(header->signature, "\x7F""ELF", 4) != 0) return false;
	if (header->bits != 0x01 || header->endianness != 0x01 || header->target != 0x03
			|| header->header_version != 0x01
			|| (header->abi != 0x00 && header->abi != 0x03) || header->abi_version != 0x00 || header->type != 0x02) return false;
	if (header->program_hdrs_offset + header->program_header_count * header->program_header_size > binary->length) return false;

	task->pending_signals = 0;
	task->task_id = g_next_pid++;
	task->parent_task = parent;
	task->uid = task->euid = task->suid = user_id;
	task->gid = task->egid = user_id;
	task->pd = copy_current_pd();
	if (!task->pd)
		return (false);
	task->proc_memory.heap_current = (void *)0x08100000; // temporary
	task->proc_memory.user_stack_bot = mmap((void *)(TASK_STACK_TOP - TASK_STACK_SIZE), TASK_STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED, -1, 0, &task->proc_memory);
	if (task->proc_memory.user_stack_bot == MAP_FAILED)
	{
		pmm_free_frame(task->pd);
		return (false);
	}
	g_curr_task = task; //temporary
	task->next = task;  // temporary
	write_cr3(task->pd); //temporary
	task->proc_memory.user_stack_top = (virt_ptr)((uintptr_t)task->proc_memory.user_stack_bot + TASK_STACK_SIZE);
	{
		struct elf_program_header *base = (struct elf_program_header *)(binary->data + header->program_hdrs_offset);
		for (unsigned short i = 0; i < header->program_header_count; i++) {
			if (base[i].type == 0x01 && base[i].file_offset + base[i].file_size < binary->length) {
				void *virt = page_align_down((void*)base[i].virt_addr);
				unsigned int to_add = base[i].virt_addr - (unsigned int)virt;
				if (mmap(virt, base[i].mem_size + to_add, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED, -1, 0, &task->proc_memory) == MAP_FAILED) {
					kernel_panic("couldn't map for process", 0);
				}
				memcpy((void*)base[i].virt_addr, binary->data + base[i].file_offset, base[i].file_size);
			}
		}
	}
	build_initial_user_frame(task, header->entrypoint, (uintptr_t)(task->proc_memory.user_stack_top));
	task->status = STATUS_RUNNABLE;
	tss_set_kernel_stack((uintptr_t)&(task->k_stack[sizeof(task->k_stack)]));
	/*printk("kstack top=%p bot=%p\n",
       &task->k_stack[sizeof(task->k_stack)],
       &task->k_stack[0]);*/
	iret_from_frame((t_interrupt_data *)task->k_esp);
	
	return (true);
}

__attribute__((noreturn)) static inline void switch_esp_to(uint32_t new_esp)
{
	__asm__ volatile(
		"movl %0, %%esp \n"
		"ret            \n"
		:
		: "r"(new_esp)
		: "memory");
	__builtin_unreachable();
}

// called from interrupt handler
void context_switch(t_task *next)
{
	g_curr_task = next;
	tss_set_kernel_stack((uintptr_t)&(next->k_stack[sizeof(next->k_stack)]));
	write_cr3(next->pd);
	switch_esp_to(next->k_esp); // then iret path restores regs
}

void schedule_next_task()
{
	if (!g_curr_task)
		return;
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

void timer_handler(__attribute__((unused)) t_interrupt_data *regs)
{
	g_tick++;
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


//Also frees now-empty PT frames
static void free_vma_range_pages(virt_ptr start, virt_ptr end)
{
    start = page_align_down(start);
    end   = page_align_up(end);

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

        //if (task && task->proc_memory.resident_pages)
        //    task->proc_memory.resident_pages--;
    }

    volatile uint32_t *pd = (volatile uint32_t *)PD_VA;

    for (uint32_t pdi = 0; pdi < ((KERNEL_VIRT_BASE >> 22) & 0x3FFu); pdi++)
    {
        if (!touched_pde[pdi])
            continue;

        uint32_t pde = pd[pdi];
        if (!(pde & PTE_P))
            continue;

		//should be ensured by loop condition already
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

void free_vmas(t_task *task)
{
	t_vma *curr = task->proc_memory.vma_list;
	while (curr)
	{
		t_vma *next = curr->next;
		free_vma_range_pages(curr->start, curr->end);
		vfree(curr);
		curr = next;
	}
	task->proc_memory.vma_list = NULL;
}
// does not free the task struct itself
void cleanup_task(t_task *task)
{
	disable_interrupts();
	free_vmas(task);
	enable_interrupts();
	// TODO implement (clean up memory, fd and everything
}

// call whean reaping a zombie task that already has been called in cleanup_task
void task_reap_zombie(t_task *t)
{
	// TODO link prev task to next task in scheduler linked list
	vfree(t);
}
