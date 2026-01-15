/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handlers.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/12 01:06:53 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/15 15:10:55 by alier            ###   ########.fr       */
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

void page_fault_handler(t_interrupt_data *regs)
{
	writes("page fault :(\n");
	printk("error code: %u\n", regs->err_code);
	unsigned int virtual_address;
	asm volatile("mov %%cr2, %0" : "=r"(virtual_address));
	printk("while %s %s page at virtual address: %p\n", regs->err_code & 2 ? "writing" : "reading", regs->err_code & 1 ? "present" : "non-present", virtual_address);
	print_interrupt_frame(regs);
	while (1)
		asm volatile("hlt");
}

void breakpoint_handler(t_interrupt_data *regs)
{
	writes("breakpoint\n");
	print_interrupt_frame(regs);
}

void timer_handler(__attribute__((unused)) t_interrupt_data *regs)
{
	// writes(".");
	// pic_eoi(INT_TIMER);
}

