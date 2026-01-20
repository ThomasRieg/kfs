/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handlers.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/12 01:06:53 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/20 17:03:23 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../interrupts.h"
#include "../../scancode.h"
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

static t_vma *find_vma(t_mm *mm, uintptr_t va)
{
	if (!mm)
		return 0;

	/* Your list is sorted by start, so you can early-exit */
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
	return find_vma(proc_memory, page_align_down(va));
}

void page_fault_handler(t_interrupt_data *regs)
{
	disable_interrupts();
	unsigned int virtual_address;
	asm volatile("mov %%cr2, %0" : "=r"(virtual_address));
	if (!g_curr_task || !(regs->err_code & 0x4))
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
	if (vma)
	{
		if ((uintptr_t)get_pte(virtual_address) & PTE_P)
			goto page_fault_handler_error; // already mapped, another error
		phys_ptr frame = pmm_alloc_frame();
		if (!frame)
			kernel_panic("out of physical memory in page_fault_handler lazy allocator\n", regs); // TODO not panic here, liberate memory of a process (when oom killer implemented)
		if (!((uintptr_t)get_pte(virtual_address) & PTE_P))
		{
			// allocate the page table itself
			phys_ptr pde_frame = pmm_alloc_frame();
			if (!pde_frame)
			{
				pmm_free_frame(frame);
				kernel_panic("out of physical memory in page_fault_handler lazy allocator\n", regs); // TODO not panic here, liberate memory of a process (when oom killer implemented)
			}
			map_page(pde_frame, get_pde(virtual_address), PTE_P | PTE_RW | PTE_US); // set permissive flags
		}
		map_page(frame, get_pte(virtual_address), PTE_P | PTE_RW | PTE_US); // TODO get the flags from the t_mm
	}
	else
	{
	page_fault_handler_error:
		// TODO handle just killing the process, schedule another one and return
		writes("page fault :(\n");
		printk("error code: %u\n", regs->err_code);
		printk("while %s %s page at virtual address: %p\n", regs->err_code & 2 ? "writing" : "reading", regs->err_code & 1 ? "present" : "non-present", virtual_address);
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
