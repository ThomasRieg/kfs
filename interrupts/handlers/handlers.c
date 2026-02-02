/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handlers.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/12 01:06:53 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/02 15:17:03 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../interrupts.h"
#include "../../tty/tty.h"
#include "../../libk/libk.h"

void print_interrupt_frame(t_interrupt_data *regs)
{
	printk("interrupt frame at %p\n", regs);
	printk("eip %p\n", regs->eip);
	printk("flags ");
	unsigned int f = regs->eflags;
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
	printk("code segment selector: %u\n", (unsigned int)regs->cs);
}

void double_fault_handler(t_interrupt_data *regs)
{
	writes("double fault :(\n");
	print_interrupt_frame(regs);
	while (1)
		asm volatile("hlt");
}

#include "../../tasks/task.h"
#include "../../pmm/pmm.h"
#include "../../mem_page/mem_paging.h"
#include "../../mmap/mmap.h"

static t_vma *find_vma(t_mm *mm, uintptr_t va)
{
	if (!mm)
		return 0;

	// list sorted by increasing address
	for (t_vma *v = mm->vma_list; v; v = v->next)
	{
		if (va < (uintptr_t)v->start)
			return 0;
		if (va >= (uintptr_t)v->start && va < (uintptr_t)v->end)
			return v;
	}
	return 0;
}

/* If you want the VMA in the PF handler to get prot/flags */
t_vma *vma_for_address(t_mm *proc_memory, uintptr_t va)
{
	if (va >= (uintptr_t)KERNEL_VIRT_BASE)
		return 0;
	return find_vma(proc_memory, (uintptr_t)page_align_down((virt_ptr)va));
}

static inline uint32_t get_vma_flags(t_vma *vma)
{
	uint32_t flags = PTE_P;
	if (!(vma->prots & PROT_NONE))
		flags |= PTE_US;
	if (vma->prots & PROT_WRITE)
		flags |= PTE_RW;
	// READ and EXEC are implied if PROT_NONE is not specified
	return (flags);
}

/*static inline bool is_user_mode_fault(const t_interrupt_data *r)
{
	return ((r->err_code & 0x4u) != 0); // U/S bit
}*/

static inline bool is_present_fault(const t_interrupt_data *r)
{
	return ((r->err_code & 0x1u) != 0); // P bit (page-protection violation)
}

// faulting_va has to be page_aligned down
static uint32_t find_frame_from_shm_anon(t_shm_anon *backing, virt_ptr faulting_va)
{
	struct shm_page *curr = backing->pages;
	while (curr)
	{
		if (curr->virt_start == faulting_va)
			return (curr->frame);
		curr = curr->next;
	}
	return (0);
}

// returns false on vmalloc fail
static bool add_page_to_shm_anon(t_shm_anon *backing, virt_ptr virtual_address_page_start, phys_ptr frame)
{
	struct shm_page *page = vmalloc(sizeof(*page));
	if (!page)
		return (false);
	page->frame = frame;
	page->virt_start = virtual_address_page_start;
	if (!backing->pages)
	{
		backing->pages = page;
		page->next = NULL;
		return (true);
	}
	else
	{
		struct shm_page *curr = backing->pages;
		if (curr->virt_start > page->virt_start)
		{
			page->next = curr;
			backing->pages = page;
			return (true);
		}
		while (curr && curr->next)
		{
			if (curr->next && curr->next->virt_start > page->virt_start)
			{
				page->next = curr->next;
				curr->next = page;
				return (true);
			}
			curr = curr->next;
		}
		curr->next = page;
		page->next = NULL;
	}
	return (true);
}

void page_fault_handler(t_interrupt_data *regs)
{
	disable_interrupts();
	unsigned int virtual_address;
	asm volatile("mov %%cr2, %0" : "=r"(virtual_address));
	if (!g_curr_task)
	{
		// page fault inside kernel code
		writes("page fault :(\n");
		printk("error code: %u\n", regs->err_code);
		printk("while %s %s page at virtual address: %p\n", regs->err_code & 2 ? "writing" : "reading", regs->err_code & 1 ? "present" : "non-present", virtual_address);
		print_interrupt_frame(regs);
		while (1)
			asm volatile("hlt");
	}
	t_vma *vma = vma_for_address(&g_curr_task->proc_memory, virtual_address);
	uint32_t *pte = get_pte((virt_ptr)virtual_address);
	uint32_t *pde = get_pde((virt_ptr)virtual_address);
	if (vma)
	{
		if (!(*pde & PTE_P))
		{
			// allocate the page table itself
			phys_ptr pde_frame = pmm_alloc_frame();
			if (!pde_frame)
			{
				kernel_panic("out of physical memory in page_fault_handler lazy allocator\n", regs); // TODO not panic here, liberate memory of a process (when oom killer implemented)
			}
			map_page(pde_frame, pde, PTE_US | PTE_RW | PTE_P); // set permissive flags
			uint32_t *page_table = (uint32_t *)(uintptr_t)(PT_BASE_VA + PDE_INDEX(virtual_address) * PAGE_SIZE);
			invalidate_cache(page_table);
			memset(page_table, 0, PAGE_SIZE);
			pte = get_pte((virt_ptr)virtual_address); // pte was null, because no pde, now we need to set it before the rest of the code
			g_curr_task->proc_memory.physical_pages++;
		}
	}
	if (vma && vma->flags == MAP_SHARED)
	{
		if (vma->backing == VMA_ANON)
		{
			t_shm_anon *backing = (t_shm_anon *)vma->backing_obj;
			virt_ptr virtual_address_page_start = page_align_down((virt_ptr)virtual_address);
			phys_ptr frame = find_frame_from_shm_anon(backing, virtual_address_page_start);
			if (!frame)
			{
				frame = pmm_alloc_frame();
				if (!frame)
					kernel_panic("out of physical memory in page_fault_handler lazy allocator\n", regs); // TODO not panic here, liberate memory of a process (when oom killer implemented)
				map_page(frame, pte, get_vma_flags(vma));
				invalidate_cache(virtual_address_page_start);
				memset(virtual_address_page_start, 0, PAGE_SIZE);
				if (!add_page_to_shm_anon(backing, virtual_address_page_start, frame))
				{
					kernel_panic("out of physical memory in page_fault_handler lazy allocator\n", regs); // TODO not panic here, liberate memory of a process (when oom killer implemented)
				}
			}
			else
			{
				map_page(frame, pte, get_vma_flags(vma));
				invalidate_cache(virtual_address_page_start);
			}
			g_curr_task->proc_memory.physical_pages++;
		}
		else
		{
			kernel_panic("unhandled backing for shared memory\n", regs);
		}
	}
	else if (vma && !is_present_fault(regs) && (!pte || !(*pte & PTE_P)))
	{
		// printk("debug, allocated new zone at %x\n", virtual_address);
		phys_ptr frame = pmm_alloc_frame();
		if (!frame)
			kernel_panic("out of physical memory in page_fault_handler lazy allocator\n", regs); // TODO not panic here, liberate memory of a process (when oom killer implemented)
		map_page(frame, pte, get_vma_flags(vma));
		virt_ptr virtual_address_page_start = page_align_down((virt_ptr)virtual_address);
		invalidate_cache(virtual_address_page_start);
		memset(virtual_address_page_start, 0, PAGE_SIZE);
		g_curr_task->proc_memory.physical_pages++;
	}
	else if ((pte && (*pte & PTE_P) && (*pte & PTE_COW)) && (regs->err_code & 2) && (vma->prots & PROT_WRITE))
	{
		// COW 🐂
		phys_ptr frame = *pte & 0xFFFFF000;
		printk("COW at frame %x, at va %x, from pid %u\n", frame, virtual_address, g_curr_task->task_id);
		if (pmm_get_refs(frame) > 1)
		{
			// copy frame
			phys_ptr new_frame = pmm_alloc_frame();
			if (!new_frame)
				kernel_panic("out of physical memory in page_fault_handler lazy allocator\n", regs); // TODO not panic here, liberate memory of a process (when oom killer implemented)
			virt_ptr virtual_address_page_start = page_align_down((virt_ptr)virtual_address);
			char buff[PAGE_SIZE];
			memcpy(buff, virtual_address_page_start, PAGE_SIZE);
			map_page(new_frame, pte, get_vma_flags(vma));
			invalidate_cache(virtual_address_page_start);
			memcpy(virtual_address_page_start, buff, PAGE_SIZE);
			g_curr_task->proc_memory.physical_pages++;
		}
		else
		{
			// last referencce, just make it rw
			pmm_free_frame(frame); // will just decrease the refcount here
			*pte |= PTE_RW;
			*pte &= ~PTE_COW;
			virt_ptr virtual_address_page_start = page_align_down((virt_ptr)virtual_address);
			invalidate_cache(virtual_address_page_start);
		}
	}
	else
	{
		// TODO handle just killing the process, schedule another one and return
		writes("page fault :(\n");
		printk("error code: %u\n", regs->err_code);
		if (regs->err_code & 4)
			printk("while in userspace\n");
		printk("while %s %s page at virtual address: %p\n", regs->err_code & 2 ? "writing" : "reading", regs->err_code & 1 ? "present" : "non-present", virtual_address);
		printk("by pid %u\n", g_curr_task->task_id);
		print_interrupt_frame(regs);
		while (1)
			asm volatile("hlt");
	}
	enable_interrupts();
}

void breakpoint_handler(t_interrupt_data *regs)
{
	writes("breakpoint\n");
	print_interrupt_frame(regs);
}
