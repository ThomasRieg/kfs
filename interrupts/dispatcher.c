/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dispatcher.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/12 00:37:11 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/19 20:08:34 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "interrupts.h"
#include "../libk/libk.h"
#include "../drivers/pic.h"
#include "../tasks/task.h"
#include "../gdt/gdt.h"

static t_isr_handler g_isr_handlers[256];

void isr_dispatch_c(t_interrupt_data *regs)
{
	if (g_curr_task)
	{
		g_curr_task->k_esp = (uintptr_t)regs;
	}
	if (g_isr_handlers[regs->int_no])
	{
		if (regs->int_no >= PIC_OFFSET && regs->int_no < (PIC_OFFSET + 16))
			pic_eoi((unsigned char)(regs->int_no & 0xFF)); // send before handler in case of a context switch in the handler
		g_isr_handlers[regs->int_no](regs);
	}
	else
	{
		printk("received unhandled interrupt %u\n", regs->int_no); // TODO add better handling here, idk how to make something robust for when we implement software interrupts and processes, so WIP
		kernel_panic("unhandled interrupt", regs);
	}
}

void isr_add_handler(uint8_t int_no, t_isr_handler func)
{
	g_isr_handlers[int_no] = func;
}
