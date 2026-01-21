/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   task.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 17:52:50 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/21 23:04:24 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../common.h"
#include "task.h"
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

	memset(f, 0, sizeof(*f));

	uint32_t udata = (GDT_SEL_UDATA | 3);
	f->ds = udata;
	f->es = udata;
	f->fs = udata;
	f->gs = udata;

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

// task has to be allocated by vmalloc
bool setup_process(t_task *task, t_task *parent, uint32_t user_id)
{
	task->pending_signals = 0;
	task->task_id = g_next_pid++;
	task->parent_task = parent;
	task->uid = user_id;
	task->pd = copy_current_pd();
	if (!task->pd)
		return (false);
	task->proc_memory.user_stack_bot = mmap((void *)(TASK_STACK_TOP - TASK_STACK_SIZE), TASK_STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED, -1, 0);
	if (task->proc_memory.user_stack_bot == MAP_FAILED)
	{
		pmm_free_frame(task->pd);
		return (false);
	}
	task->proc_memory.user_stack_top = (virt_ptr)((uintptr_t)task->proc_memory.user_stack_bot + TASK_STACK_SIZE);
	// TODO ELF parsing goes here
	build_initial_user_frame(task, 0x00200000, (uintptr_t)(task->proc_memory.user_stack_top)); // TODO entrypoint = ELF entrypoint
	task->status = STATUS_RUNNABLE;
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
